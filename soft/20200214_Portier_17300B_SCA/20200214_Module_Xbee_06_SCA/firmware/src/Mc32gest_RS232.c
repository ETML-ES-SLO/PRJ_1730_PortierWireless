// Mc32Gest_RS232.C
// Canevas manipulation TP2 RS232 SLO2 2016-2017
// Fonctions d'�mission et de r�ception des message
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

// Declaration des FIFO pour r�ception et �mission
#define FIFO_RX_SIZE ( (4*MAX_RF_PAYLOAD_SIZE) + 1)  // 32 octets 
#define FIFO_TX_SIZE ( (4*MAX_RF_PAYLOAD_SIZE) + 1)  // 32 octets

int8_t fifoRX[FIFO_RX_SIZE];
S_fifo descrFifoRX; // Declaration du descripteur du FIFO de r�ception


int8_t fifoTX[FIFO_TX_SIZE];
S_fifo descrFifoTX; // Declaration du descripteur du FIFO d'�mission


// Initialisation de la communication s�riel
// -----------------------------------------

void InitFifoComm(void)
{
     
   // Initialisation du fifo de r�ception
   InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
   // Initialisation du fifo d'�mission
   InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
   
} // InitComm

 
// Lit size bytes re�us via UART et les stocke dans dstArray
uint8_t Uart_GetMessage(uint8_t* dstArray, uint8_t size)
{
    uint8_t i, carLu;
    
    SYS_INT_SourceDisable(INT_SOURCE_USART_1_RECEIVE); //d�sactive int uart rx
    
    for ( i=0 ; i<size ; i++ )
    {
       GetCharFromFifo (&descrFifoRX, (int8_t*)&carLu);
       dstArray[i] = carLu;    
    }
   
    SYS_INT_SourceEnable(INT_SOURCE_USART_1_RECEIVE); //r�active int uart rx
    
    return (size);
}


//envoyer un msg via uart
//param nbBytesToSend :
// - si > 0 : le nb de bytes � transmettre
// - si == 0 : transmission d'une chaine jusqu'� trouver un car 0 (fin de chaine)
void Uart_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)
{
    uint8_t i = 0;
    
    if (nbBytesToSend != 0) //nb de car. � envoyer sp�cifi�
    {
        while (GetWriteSpace(&descrFifoTX) >= 1 && nbBytesToSend >= 1)
        {
            PutCharInFifo (&descrFifoTX, dataToSend[i]);
            i++;
            nbBytesToSend--;
        }   
    } else //chaine � envoyer (se termine par car. 0)
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
        // Autorise int �mission
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);            
   }
}


// !!!!!!!!
// Attention ne pas oublier de supprimer la r�ponse g�n�r�e dans system_interrupt
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
         // Traitement de l'erreur � la r�ception.
    }
   
    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) {
        
        // Teste si erreur parit�, framing ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);
        
        //pas d'erreur ?
        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) {
                                
             // transfert dans le fifo de tous les char re�u
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
            
        // Clear the TX interrupt Flag (seulement apr�s chargement buffer TX)
       PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
       // disable TX interrupt (pour �viter une int inutile)
       PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
    } // end if TX
}


