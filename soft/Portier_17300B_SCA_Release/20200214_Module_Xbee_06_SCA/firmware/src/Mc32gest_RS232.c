// Mc32Gest_RS232.C
// Canevas manipulation TP2 RS232 SLO2 2016-2017
// Fonctions d'émission et de réception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart

#include <xc.h>
#include <sys/attribs.h>
#include "system_definitions.h"
// Ajout CHR
#include <GenericTypeDefs.h>
#include "apprf.h"
#include "GesFifoTh32.h"
#include "Mc32gest_RS232.h"



// Definitions pour les FIFO
#define MAX_RF_PAYLOAD_SIZE 20

// Declaration des FIFO pour réception et émission
#define FIFO_RX_SIZE ( (4*MAX_RF_PAYLOAD_SIZE) + 1)  // 32 octets 
#define FIFO_TX_SIZE ( (4*MAX_RF_PAYLOAD_SIZE) + 1)  // 32 octets

int8_t fifoRX[FIFO_RX_SIZE];
S_fifo descrFifoRX; // Declaration du descripteur du FIFO de réception


int8_t fifoTX[FIFO_TX_SIZE];
S_fifo descrFifoTX; // Declaration du descripteur du FIFO d'émission


// Initialisation de la communication sériel
// -----------------------------------------

void InitFifoComm(void)
{
     
   // Initialisation du fifo de réception
   InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
   // Initialisation du fifo d'émission
   InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
   
} // InitComm

 
// Lit size bytes reçus via UART et les stocke dans dstArray
uint8_t Uart_GetMessage(uint8_t* dstArray, uint8_t size)
{
    uint8_t i, carLu;
    
    SYS_INT_SourceDisable(INT_SOURCE_USART_1_RECEIVE); //désactive int uart rx
    
    for ( i=0 ; i<size ; i++ )
    {
       GetCharFromFifo (&descrFifoRX, (int8_t*)&carLu);
       dstArray[i] = carLu;    
    }
   
    SYS_INT_SourceEnable(INT_SOURCE_USART_1_RECEIVE); //réactive int uart rx
    
    return (size);
}


//envoyer un msg via uart
//param nbBytesToSend :
// - si > 0 : le nb de bytes à transmettre
// - si == 0 : transmission d'une chaine jusqu'à trouver un car 0 (fin de chaine)
void Uart_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)
{
    uint8_t i = 0;
    
    if (nbBytesToSend != 0) //nb de car. à envoyer spécifié
    {
        while (GetWriteSpace(&descrFifoTX) >= 1 && nbBytesToSend >= 1)
        {
            PutCharInFifo (&descrFifoTX, dataToSend[i]);
            i++;
            nbBytesToSend--;
        }   
    } else //chaine à envoyer (se termine par car. 0)
    {
        while (GetWriteSpace(&descrFifoTX) >= 1 && dataToSend[i] != 0)
        {
            PutCharInFifo (&descrFifoTX, dataToSend[i]);
            i++;
        }           
    }
    
    Uart_CheckForTxInt();
}


void Uart_CheckForTxInt(void)
{
   PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT); 
   
   if (GetReadSize(&descrFifoTX) > 0 && PLIB_USART_TransmitterIsEmpty(USART_ID_1))
   {
        // Autorise int émission
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);            
   }
}


// !!!!!!!!
// Attention ne pas oublier de supprimer la réponse générée dans system_interrupt
// !!!!!!!!
void __ISR(_UART_1_VECTOR, ipl3AUTO) _IntHandlerDrvUsartInstance0(void)
{
    int8_t c;  
    USART_ERROR  UsartStatus;
    
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) ) {
         /* Clear pending interrupt */
         PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
         // Traitement de l'erreur à la réception.
    }
   
    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) {
        
        // Teste si erreur parité, framing ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);
        
        //pas d'erreur ?
        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) {
                                
             // transfert dans le fifo de tous les char reçu
             while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
             {
                 c = PLIB_USART_ReceiverByteReceive(USART_ID_1);
                 PutCharInFifo ( &descrFifoRX, c);
             }
             // buffer is empty, clear interrupt flag
             PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_RECEIVE);
        } else {    //erreur
             // Suppression des erreurs
             // La lecture des erreurs les efface sauf pour overrun
             if ( (UsartStatus & USART_ERROR_RECEIVER_OVERRUN) == USART_ERROR_RECEIVER_OVERRUN) {
                    PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1);
             }
        }                                                                 
    } // end if RX                                                                                  

    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) ) {            
        
        while (GetReadSize(&descrFifoTX) > 0 && !PLIB_USART_TransmitterBufferIsFull(USART_ID_1))
        {
            GetCharFromFifo(&descrFifoTX, &c);
            PLIB_USART_TransmitterByteSend(USART_ID_1, c);            
        }
            
        // Clear the TX interrupt Flag (seulement après chargement buffer TX)
       PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
       // disable TX interrupt (pour éviter une int inutile)
       PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
    } // end if TX
}


