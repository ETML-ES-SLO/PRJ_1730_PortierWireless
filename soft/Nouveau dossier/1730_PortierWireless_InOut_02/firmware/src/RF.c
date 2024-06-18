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
#include <xc.h> //pour les d�finitions des registres
#include "Mc32Delays.h"
#include "peripheral/usart/plib_usart.h"
#include <string.h> //pour memcpy()
#include "system_definitions.h"

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

/* ************************************************************************** */
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
        // traitement car. re�us
        while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            getCharUART = PLIB_USART_ReceiverByteReceive(USART_ID_1);
            if((rcdvCharCntr > 4) && (rcdvCharCntr < 14))
                msg[rcdvCharCntr-5] = getCharUART; //stockage car. re�u dans chaine
            if(msg[rcdvCharCntr-5] == 0x0D)
            {
                rcdvCharCntr = 0;
                flagGetAdress = 1;
            }
            else
            {
                if (rcdvCharCntr < MAXMSG)
                    rcdvCharCntr++;
                if(rcdvCharCntr == MAXMSG) //trame compl�te re�ue ?
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

uint32_t getSerialRF()
{
    return nbrSerialRF;
}
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
// soit un nb de bytes d�fini par nbBytesToSend
// sinon si nbBytesToSend==0, envoi jusqu'� trouver une fin de chaine (car. 0))
void RF_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)   
//void Uart_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend)
{
    uint8_t i = 0;
    if (nbBytesToSend != 0) //nb de car. � envoyer sp�cifi�
    {
        for (i=0 ; i<nbBytesToSend ; i++)
        {
            while(PLIB_USART_TransmitterBufferIsFull(USART_ID_1));
            PLIB_USART_TransmitterByteSend(USART_ID_1, dataToSend[i]);
        }   
    } else //chaine � envoyer (se termine par car. 0)
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
//teste si une nouvelle trame n'est pas juste une r�p�tidion d'une autre.
// 2 trames sont consid�r�es identiques si il y a juste ne no de trame qui a 
// �t� incr�ment�
bool isNewFrame(uint8_t* rcvdMsg)
{
    bool retValue;
    bool sameFrame;
    uint8_t newFrameNr;
    static uint8_t lastFrameNr = 0;
    static uint8_t lastRcvdMsg[8] = "        ";
       
    //r�cup�re les nos de trames
    newFrameNr =  (rcvdMsg[2]-48)*10 + (rcvdMsg[3]-48);
    //lastFrameNr =  (lastRcvdMsg[2]-48)*10 + (lastRcvdMsg[3]-48);
    
    //teste si le reste des trames (hors nos de trames) est identique
    sameFrame = rcvdMsg[0]==lastRcvdMsg[0]
            &&  rcvdMsg[1]==lastRcvdMsg[1]
            &&  rcvdMsg[4]==lastRcvdMsg[4]
            &&  rcvdMsg[5]==lastRcvdMsg[5]
            &&  rcvdMsg[6]==lastRcvdMsg[6]
            &&  rcvdMsg[7]==lastRcvdMsg[7];
            
    retValue = !((newFrameNr>lastFrameNr) && sameFrame);        
    
    //se souvient du message pour prochain passage
    lastFrameNr = newFrameNr;
    memcpy(lastRcvdMsg, rcvdMsg, 8);
    
    return(retValue);
}

/* ************************************************************************** */
//R�ception et d�codage trame RF re�ue via UART 1
// Renvoie true avec message dans msg si nouvelle trame re�ue
// Le format de la trame est :
// -8 caract�res ASCII
// - trame porte (door) -> sonnerie (bell) : "[Dnnbbb]"
//   D pour l'identit� de l'�metteur (Door)  
//   nn : num�ro de trame (commence � 00, s'incr�mente � chaque envoi)
//        La trame de sonnerie est renvoy�e toutes les 100ms
//   bbb : Tension de piles en 1/100 de [V]
// - trame sonnerie (bell) -> porte (door) : "[Bnnx  ]"
//   B pour l'identit� de l'�metteur (Bell)  
//   nn : num�ro de trame (commence � 00, s'incr�mente � chaque envoi)
//        La trame de r�ponse est renvoy�e toutes les 100ms
//   x : r�ponse 'B' pour Busy / 'W' pour Wait / 'E' pour Enter / 'N' pour Nobody
//   + 2 espaces de padding pour faire 8 caract�res total
bool RF_GetMessage(uint8_t* msg)   
{
    static uint8_t rcvdMsg[MAXMSG];
//    static uint8_t lastRcvdMsg[8] = "        ";
    static uint8_t rcdvCharCntr = 0;
    bool retValue = false;
    uint8_t c;
    
    //clear l'erreur d'overflow (trop de bytes sont arriv�s trop rapidement)
    // cela vide le buffer de r�ception, mais permet de recommencer � fonctionner
    if (PLIB_USART_ReceiverOverrunHasOccurred(USART_ID_1))
        PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1); 
    
    // traitement car. re�us
    while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1) && retValue==false)
    {
        c = PLIB_USART_ReceiverByteReceive(USART_ID_1);
        
        if (c == '[') //d�but de trame ?
            rcdvCharCntr = 1;
        else //incr�mentation compteur de caract�res re�us
            if (rcdvCharCntr < MAXMSG)
                rcdvCharCntr++;
        
        rcvdMsg[rcdvCharCntr-1] = c; //stockage car. re�u dans chaine
        
        if (c==']' && rcdvCharCntr==MAXMSG) //trame compl�te re�ue ?
        {
            memcpy(msg, rcvdMsg, 8);
            rcdvCharCntr = 0;
            retValue = true;
        }
    }
    
    if (retValue)   //trame re�ue ?
    {
        //teste si ce n'est pas une nouvelle trame, mais juste une trame r�p�t�e
        //le no de trame permet de rep�rer cela
        if (!isNewFrame(rcvdMsg))//, lastRcvdMsg)) //ce n'est pas une nouvelle trame
        {           
            retValue = false; 
        }   
    }           
            
    return false;
}
void UART_GetMessage(uint8_t c)
{
    
    if (c == '[') //d�but de trame ?
        rcdvCharCntr = 1;
    //incr�mentation compteur de caract�res re�us
    else if (rcdvCharCntr < MAXMSG)
            rcdvCharCntr++;
    rcvdMsg[rcdvCharCntr-1] = c; //stockage car. re�u dans chaine
    //if((c==']') && (rcdvCharCntr < 5))
    if (c==']') //trame compl�te re�ue ?
    {
        rcdvCharCntr = 0;
        flagGetMsgUart = true;
    }
    
}

/* *****************************************************************************
 End of File
 */
