//--------------------------------------------------------
// RF.c
//--------------------------------------------------------
// Gestion communication	
//
//	Auteur 		: 	SCA
//  Date        :   5.12.2019
//	Version		:	V1.0
//	Compilateur	:	XC32 V2.15
//  Modifications :
//   -
/*--------------------------------------------------------*/

#include "RF.h"
#include <xc.h> //pour les définitions des registres
#include "Mc32Delays.h"
#include "peripheral/usart/plib_usart.h"
#include <string.h> //pour memcpy()
//#include "peripheral/oc/plib_oc.h"
//#include "peripheral/tmr/plib_tmr.h"
#include "system_definitions.h"
#include <stdio.h> // pour sscanf


/* ************************************************************************** */
//variables
#define MAXMSG  64
uint8_t flagGetAdress = 0;
uint8_t flagErrorAdress = 0;
uint32_t nbrSerialRF = 0;

uint8_t rcvdMsg[64];
uint8_t rcdvCharCntr = 0;

bool flagGetMsgUart = false;
DRV_HANDLE handleUSART;

static uint32_t rfLastSenderSerial = 0;
// Ajout fonction :
uint32_t RF_GetLastSenderSerial(void) {
    return rfLastSenderSerial;
}

void RF_RecordSenderSerial(uint8_t* msg)
{
    if (msg[0] != '[' || msg[7] != ']') return;      // format KO

    /* Seules les trames P0/P1 ou A0/A1 portent le n° série complet */
    if (msg[1] == 'P' || msg[1] == 'A') {
        uint16_t val = 0;
        sscanf((char*)&msg[3], "%04hX", &val);

        if (msg[2] == '0')        /* mot haut */
            rfLastSenderSerial  = ((uint32_t)val << 16);
        else if (msg[2] == '1')   /* mot bas  */
            rfLastSenderSerial |= val;
    }
}


uint8_t ReadSerialNbr(uint32_t *serialInput)
{
    uint8_t msg[MAXMSG];
    uint8_t getCharUART = 0;
    uint8_t rcdvCharCntr = 0;
    uint8_t Puissance = 0;
    flagGetAdress = 0;
    RF_SendMessage((uint8_t*)"AT+GADD\n", 0);
    while(flagGetAdress == 0)
    {
        // traitement car. reçus
        while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            getCharUART = PLIB_USART_ReceiverByteReceive(USART_ID_1);
            if((rcdvCharCntr > 4) && (rcdvCharCntr < 14))
                msg[rcdvCharCntr-5] = getCharUART; //stockage car. reçu dans chaine
            if(msg[rcdvCharCntr-5] == 0x0D)
            {
                rcdvCharCntr = 0;
                flagGetAdress = 1;
            }
            else
            {
                if (rcdvCharCntr < MAXMSG)
                    rcdvCharCntr++;
                if(rcdvCharCntr == MAXMSG) //trame complète reçue ?
                    return 1;
            }
            
        }
    }
    while(Puissance < 8)
    {
        *serialInput = *serialInput << 4;
        if((msg[Puissance] >= '0') && (msg[Puissance] <= '9'))
            *serialInput += msg[Puissance]-'0';
        else
            *serialInput += 0x55-msg[Puissance];
        Puissance++;
    }
    return 0;
}
/* ************************************************************************** */
void RF_Init(void)
{
    handleUSART = DRV_USART_Open(0, DRV_IO_INTENT_READWRITE | DRV_IO_INTENT_NONBLOCKING);
    
    //reset module RF 1 ms
    LATBbits.LATB12 = 0;
    delay_msCt(1);
    LATBbits.LATB12 = 1;

    delay_msCt(10); //attendre fin init module rf
    flagErrorAdress = ReadSerialNbr(&nbrSerialRF);
    // puis envoyer trame pour sortie mode config
    RF_SendMessage((uint8_t*)"AT+EXIT\n", 0);
    
    delay_msCt(10); //attendre traitement cmde EXIT    
}

/* ************************************************************************** */
//envoi trame au module RF via UART 1
// soit un nb de bytes défini par nbBytesToSend
// sinon si nbBytesToSend==0, envoi jusqu'à trouver une fin de chaine (car. 0))
void RF_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)   
//void Uart_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)
{
    uint8_t i = 0;
    
    if (nbBytesToSend != 0) //nb de car. à envoyer spécifié
    {
        for (i=0 ; i<nbBytesToSend ; i++)
        {
            while(PLIB_USART_TransmitterBufferIsFull(USART_ID_1));
            PLIB_USART_TransmitterByteSend(USART_ID_1, dataToSend[i]);
        }   
    } else //chaine à envoyer (se termine par car. 0)
    {
        while (dataToSend[i] != 0)
        {
            while(PLIB_USART_TransmitterBufferIsFull(USART_ID_1));
            PLIB_USART_TransmitterByteSend(USART_ID_1, dataToSend[i]);
            i++;
        }           
    }
}   

/* ************************************************************************** */
//teste si une nouvelle trame n'est pas juste une répétidion d'une autre.
// 2 trames sont considérées identiques si il y a juste ne no de trame qui a 
// été incrémenté
bool isNewFrame(uint8_t* rcvdMsg)
{
    static uint8_t lastRcvdMsg[8] = "        ";
    bool samePayload = (memcmp(rcvdMsg, lastRcvdMsg, 8) == 0);   // compare tout le payload
    memcpy(lastRcvdMsg, rcvdMsg, 8);                            // mémorise pour la suite
    return !samePayload;                                        // nouvelle ? payload différent
}


/* ************************************************************************** */
//Réception et décodage trame RF reçue via UART 1
// Renvoie true avec message dans msg si nouvelle trame reçue
// Le format de la trame est :
// -8 caractères ASCII
// - trame porte (door) -> sonnerie (bell) : "[Dnnbbb]"
//   D pour l'identité de l'émetteur (Door)  
//   nn : numéro de trame (commence à 00, s'incrémente à chaque envoi)
//        La trame de sonnerie est renvoyée toutes les 100ms
//   bbb : Tension de piles en 1/100 de [V]
// - trame sonnerie (bell) -> porte (door) : "[Bnnx  ]"
//   B pour l'identité de l'émetteur (Bell)  
//   nn : numéro de trame (commence à 00, s'incrémente à chaque envoi)
//        La trame de réponse est renvoyée toutes les 100ms
//   x : réponse 'B' pour Busy / 'W' pour Wait / 'E' pour Enter / 'N' pour Nobody
//   + 2 espaces de padding pour faire 8 caractères total
bool RF_GetMessage(uint8_t* msg)   
{
    static uint8_t rcvdMsg[8];
//    static uint8_t lastRcvdMsg[8] = "        ";
    static uint8_t rcdvCharCntr = 0;
    uint8_t c;
    bool retValue = false;
    
    //clear l'erreur d'overflow (trop de bytes sont arrivés trop rapidement)
    // cela vide le buffer de réception, mais permet de recommencer à fonctionner
    if (PLIB_USART_ReceiverOverrunHasOccurred(USART_ID_1))
        PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1); 
    
    // traitement car. reçus
    while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1) && retValue==false)
    {
        c = PLIB_USART_ReceiverByteReceive(USART_ID_1);
        
        if (c == '[') //début de trame ?
            rcdvCharCntr = 1;
        else //incrémentation compteur de caractères reçus
            if (rcdvCharCntr < 8)
                rcdvCharCntr++;
        
        rcvdMsg[rcdvCharCntr-1] = c; //stockage car. reçu dans chaine
        
        if (c==']' && rcdvCharCntr==8) //trame complète reçue ?
        {
            memcpy(msg, rcvdMsg, 8);
            rcdvCharCntr = 0;
            retValue = true;
        }
    }
    
    if (retValue)   //trame reçue ?
    {
        //teste si ce n'est pas une nouvelle trame, mais juste une trame répétée
        //le no de trame permet de repérer cela
        if (!isNewFrame(rcvdMsg))//, lastRcvdMsg)) //ce n'est pas une nouvelle trame
        {           
            retValue = false; 
        }   
    }           
            
    return(retValue);
}

uint32_t getSerialRF()
{
    return nbrSerialRF;
}

/* *****************************************************************************
 End of File
 */
