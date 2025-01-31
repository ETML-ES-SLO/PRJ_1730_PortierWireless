/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "apprf.h"
#include "Mc32Delays.h"
#include <ctype.h>  //pour toupper()
#include <string.h> //pour strpy
#include <stdio.h> //pour sprintf


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

#ifndef SYS_FREQ
    #define SYS_FREQ (10000000L)    //10 MHz
#endif

#define SW_VERSION "1623x_ModulesRxTx868MHz v1.0"


//structure d'un paquet RF (taille fixe 32 bytes = le max pour nrf905):
// 4 bytes adresse src
// 4 bytes adresse dst
// 2 bytes  no de paquet (pour les ack par ex)
// 1 byte type de paquet (data, ack, etc.)
// 1 byte payloadSize
// 20 bytes payload (pas forcément utilisé au complet)
typedef union 
{
    uint8_t bytes[32];    
    struct
    {
        uint32_t srcAddr;
        uint32_t dstAddr;
        uint16_t seqNr;       
        uint8_t  msgType;
        uint8_t  payloadSize;
        uint8_t  payload[20];
    }  
    rfPckt_s;    
} 
rfPckt_t;

#define MSG_TYPE_DATA 1
// pas encore implémenté #define MSG_TYPE_ACK  2
// pas encore implémenté #define MSG_TYPE_KEEPALIVE 3
// pas encore implémenté #define MSG_TYPE_ADVERTISE 4

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APPRF_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/
//bool Timeout = false;
//uint32_t Count_Timeout = 0;
APPRF_DATA appData;

//bool test = false;
//U_32 Received_ADD;
//U_32 Target_ADD;
//U_32 Target_ADD_Same;
//U_32 DataTX;
//U_32 DataRX;

//int8_t rxPckt[8];
//int8_t txPckt[8];
rfPckt_t rxPckt;
rfPckt_t txPckt;

uint16_t txSeqNr = 0;


#define MAX_SRC_ADD 3
#define MAX_DST_ADD 3
//uint32_t scrAddList[MAX_SRC_ADD] = {0};    //on peut spécifier max 3 sources de messages accceptés
//uint32_t dstAddList[MAX_DST_ADD] = {0};    //on peut spécifier max 3 destinataires des messages



// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APPRF_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APPRF_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APPRF_STATE_INIT;
    //scrAddList[0] = 0xFFFFFFFF; //par défaut on accepte tous les messages entrants
    //dstAddList[0] = 0xFFFFFFFF; //par défaut on envoie en broadcast
}


/******************************************************************************
  Function:
    void APPRF_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APPRF_Tasks ( void )
{
#define STR_LENGTH 64
    bool charReceived, cmdReceived;
    static char cmdStr[STR_LENGTH];
    static uint32_t cmdWritePos;
    int8_t carLu;   
    char answerStr[STR_LENGTH];
//    uint32_t i;
    
    //bool GetAM, GetDR;
    static uint32_t uartRxSize[2] = {0, 0};//,Tx_size;
    //static uint32_t cptRxTimeout = 0;
    static uint32_t lastRx = 0;
    bool rxTimeout;

//    uint32_t OK = 0x496c5ee3;
//    uint32_t ERROR = 0x44b284de ;
//    static uint32_t nbrOfTimeout = 0;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APPRF_STATE_INIT:
        {
            //init variables
            cmdWritePos = 0;
            
            //init uart 1
            InitFifoComm();
            while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
                PLIB_USART_ReceiverByteReceive(USART_ID_1);
            SYS_INT_SourceStatusClear(INT_SOURCE_USART_1_RECEIVE);
            SYS_INT_SourceEnable(INT_SOURCE_USART_1_RECEIVE);         
           
            //pr attraper le 1er car. "fantome" (arrive à l'init de l'uart)
            delay_msCt(1);
            while((GetReadSize(&descrFifoRX)) > 0)
                GetCharFromFifo (&descrFifoRX, &carLu);
            
            //debug GPIO1 sur RB2
//            LATBbits.LATB2 = 1;
//            delay_msCt(1);  //delay ms légèrement trop court (999.4 us au lieu de 1 ms)
//            LATBbits.LATB2 = 0;
//            
//            LATBbits.LATB2 = 1;
//            delay_msCt(100);    //delay ms légèrement trop court (99.14 ms au lieu de 100 ms)
//            LATBbits.LATB2 = 0;
//            
//            LATBbits.LATB2 = 1;
//            delay_usCt(1);
//            LATBbits.LATB2 = 0;
//            
//            LATBbits.LATB2 = 1;
//            delay_usCt(100);
//            LATBbits.LATB2 = 0;
            
            Nrf905_Init();            
            //delay_msCt(100);
            
            appData.cycleCntr = 0;
            DRV_TMR0_Start();
            appData.state = APPRF_STATE_CONFIG;

            break;
        }
        case APPRF_STATE_CONFIG:    //pendant n [s], le module xbee se met en reception de commandes
        {
            charReceived = false;
            cmdReceived = false;    
            
            //lecture car. reçus
            while(GetReadSize(&descrFifoRX) > 0)
            {
                GetCharFromFifo (&descrFifoRX, &carLu);
                if (cmdWritePos < STR_LENGTH)
                {                               
                    cmdStr[cmdWritePos] = toupper(carLu);
                    cmdWritePos++;
                }
                charReceived = true;
                cmdReceived = (carLu==0x0A || carLu == 0x0D); //reçu fin de chaine (CR ou LF) ?
            }
            
            //analyse commande reçue
            if (cmdReceived) //reçu commande complète (terminée par CR et/ou LF) ?
            {
                if(!memcmp(cmdStr,"AT+EXIT",7)) //sortir mode config AT+EXIT
                {
                    charReceived = false;
                    cmdReceived = false;                   
                    appData.cycleCntr = 200;
                    strcpy(answerStr, "OK\r\n");  //réponse discutable car on ne devrait rien envoyer dès lors qu'on n'est plus en mode cmd
                } else if (!memcmp(cmdStr,"AT+GVER",7)) //lire no version SW AT+GVER
                {   
                    sprintf(answerStr, "OK,%s\r\n", SW_VERSION);                   
                } else if (!memcmp(cmdStr,"AT+GADD",7)) //lire propre adresse AT+GADD
                {                    
                    sprintf(answerStr, "OK,0x%08X\r\n", Nrf905_GetOwnAddr());                                   
                }/* else if (!memcmp(cmdStr,"AT+SSRCADD ",11)) //programmer émetteur(s) des msg (de qui on les accepte) Set Source Address AT+SSRCADD
                {
                   //analyser arguments et remplir tableau
                    
                } else if (!memcmp(cmdStr,"AT+SDSTADD ",11)) //programmer destinataire(s) des msg Set Dest. Address AT+SDSTADD                 
                {
                   //analyser arguments et remplir tableau
                    
                    
                } else if (!memcmp(cmdStr,"AT+ADV",7)) //se mettre en mode advertise (pour avoir les adresses des autres et appairer) AT+ADV
                {
                    
                    
                } */else  //error, commande non reconnue 
                {
                    strcpy(answerStr, "ERROR\r\n");
                }
                //sauver les param en EEPROM ?
                //les reseter ?
                
                cmdWritePos = 0; //se préparer pour prochaine commande
                
                Uart_SendMessage((uint8_t*)answerStr, 0); //envoie réponse
            }    
                       
            //gestion du timeout pour sortir du mode config
            //TODO : utiliser Core timer pour gestion timeout à la place de Timer 1
            PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_TIMER_1);
            if (charReceived)
            {
                appData.cycleCntr = 0;  //on remet le timeout à 0 si on a reçu qqchose
            }
            else if (appData.cycleCntr >= 200)   //valeur timeout dépassée (200 pour 2s)?
            {
                DRV_TMR0_Stop();
                appData.state = APPRF_STATE_SERVICE_TASKS;
                break;  //pour sortie du case sans réactiver le timer
            }   
            PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_TIMER_1);
            break;
        }
        case APPRF_STATE_SERVICE_TASKS:
        {  
            //TODO : implémenter ack des messages et renvoi
            
            //Active l'écoute du coté RF
            Nrf905_Activer_Ecoute_RF ();                   
            
            //si on recoit du coté RF (Address Match et Data Ready))
            if(Nrf905_GetAm() &&  Nrf905_GetDr())
            {
                //Recupération des données recues
                Nrf905_ReadRxBuf(rxPckt.bytes);
                //le paquet nous est-il destiné ?
                if (rxPckt.rfPckt_s.dstAddr==0xFFFFFFFF || rxPckt.rfPckt_s.dstAddr==Nrf905_GetOwnAddr())                   
                    appData.state = APPRF_STATE_SEND_DATAS_TO_UART;
                else
                    appData.state = APPRF_STATE_SERVICE_TASKS;    
            }            
            else                 
            {        
                // Gestion réception UART
                rxTimeout = false;
                uartRxSize[1] = uartRxSize[0];
                uartRxSize[0] = GetReadSize(&descrFifoRX);                
                if (uartRxSize[0] > uartRxSize[1]) // des nouvelles infos sont arrivées du coté UART ?
                {
                    lastRx = _CP0_GET_COUNT();                    
                }
                else if (uartRxSize[0] > 0) //pas de nouveau byte arrivé, mais déjà qqchose à transmettre
                {
                    //rien reçu pendant l'équivalent de 2 caractères uart ?
                    if ((_CP0_GET_COUNT() - lastRx) > ((SYS_FREQ/2) * 2*10/115200)) //attention adapter si SYS CLK ou baudrate changent 
                        rxTimeout = true;
                }
                            
                // assez d'octets à envoyer ou plus rien qui arrive ?
                if(uartRxSize[0] >= 20 || (uartRxSize[0]>0 && rxTimeout))//8)
                {
                    //txPckt[0] = Uart_GetMessage((char*)&txPckt[1]);
                    //prépare paquet à envoyer
                    txPckt.rfPckt_s.srcAddr = Nrf905_GetOwnAddr();
                    txPckt.rfPckt_s.dstAddr = 0xFFFFFFFF; //broadcast
                    txPckt.rfPckt_s.seqNr = txSeqNr++;
                    txPckt.rfPckt_s.msgType = MSG_TYPE_DATA;                   
                    txPckt.rfPckt_s.payloadSize = uartRxSize[0];
                    Uart_GetMessage(txPckt.rfPckt_s.payload, uartRxSize[0]);                    
                    
                    appData.state = APPRF_STATE_SEND_RF_MESSAGE;       
                }
            }
            
//            //met le CPU en idle pour économie énergie si rien à faire
//            if (appData.state == APPRF_STATE_SERVICE_TASKS)
//            {
//                //on calcule le valeur de comp. du core timer pour se faire réveiller dans 300 us
//                // ~= 4 car. arrivés dans l'UART @ 115200 (la moitié du FIFO HW)
//                _CP0_SET_COMPARE(_CP0_GET_COUNT() + (((SYS_FREQ/2)/1000000) * 300));
//     
//                PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_TIMER_CORE);
//                PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_CT, INT_PRIORITY_LEVEL5);
//                PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_CT, INT_SUBPRIORITY_LEVEL0);
//                //OpenCoreTimer(40000);
//
//                //on rentre en idle mode. Fait gagner 4 mA de conso @ 10 MHz
//                SYS_DEVCON_PowerModeEnter(SYS_POWER_MODE_IDLE); // ou SYS_POWER_MODE_SLEEP    
//            }
            break;
        }
//        case APPRF_SEND_RF_ANSWER:
//        {
//            //on attend que la bande soit disponible
//            while(Nrf905_GetCd());
//            
//            //On écrit les données a envoyer
//            Nrf905_WriteTxBuf(DataTX.val32); 
//            appData.state = APPRF_SEND_DATAS_TO_UART;
//            break;
//        }
        case APPRF_STATE_SEND_DATAS_TO_UART:
        {
            //On envoie le payload reçu en RF sur UART
            Uart_SendMessage (rxPckt.rfPckt_s.payload, rxPckt.rfPckt_s.payloadSize);
            
            appData.state = APPRF_STATE_SERVICE_TASKS;              
            break;
        }
        case APPRF_STATE_SEND_RF_MESSAGE:
        {
            //la bande est-elle disponible ?
            if (!Nrf905_GetCd())                
            {
                Nrf905_WriteTxBufAndTx(txPckt.bytes);   //envoi RF
                appData.state = APPRF_STATE_SERVICE_TASKS;    
            }
            break;
        }
        
//        case APPRF_WAIT_FOR_ANSWER:
//        {
//            //on attend que on nous parle
//            GetAM = Nrf905_GetAm();
//            if(GetAM)
//            {
//                //reset des variables de timeout
//                Count_Timeout = 0;   
//                nbrOfTimeout = 0;
//                //Arret du timer qui gère les timeouts
//                DRV_TMR0_Stop();            
//                appData.state = APPRF_MESSAGE_CHECK_SUCCES;
//            }
//            //si il y a eu timeout
//            else if(Timeout)
//            {
//                //reset des variable de timeout
//                Count_Timeout = 0;    
//                Timeout = false;
//                //incrémentation du nombres de timeouts
//                nbrOfTimeout ++;
//                //si il y a eu 3 timeouts
//                if (nbrOfTimeout >= 4)
//                {
//                    //on pars dans l'erreur
//                    appData.state = APPRF_SEND_ERROR;
//                }
//                else
//                {   
//                    //on renvoie le message
//                    appData.state = APPRF_SEND_RF_MESSAGE;
//                }
//            } 
//            break;               
//        }
        
//        case APPRF_MESSAGE_CHECK_SUCCES:
//        {
//            //start du timer 
//            DRV_TMR0_Start();
//            //Check si le CRC est juste
//            GetDR = Nrf905_GetDr();
//            if(GetDR)
//            {
//                //Reception des info recues en RF
//                DataRX.val32 = Nrf905_ReadRxBuf(&Target_ADD_Same.val32);
//                //rest de la variable de timeout
//                nbrOfTimeout = 0;
//                //on regarde si c'est le mÃªme module qui nous parle
//                if(Target_ADD.val32 == Target_ADD_Same.val32)
//                {
//                    //On regarde si on a bien recu une validation de reception
//                    if(DataRX.val32 == OK)
//                    {
//                        appData.state = APPRF_SEND_SUCCEDED;
//                    }
//                    else
//                    {
//                        appData.state = APPRF_SEND_ERROR;                      
//                    }
//                }
//            }
//            //si il y a eu timeout
//            else if(Timeout)
//            {
//                //reset des variables de timeout
//                Count_Timeout = 0;    
//                Timeout = false;
//                //incrémentation du nombres de timeouts
//                nbrOfTimeout ++;
//                //si il y a eu 3 timeouts
//                if (nbrOfTimeout >= 4)
//                {
//                    appData.state = APPRF_SEND_ERROR;
//                }
//            }
//            break;
//        }
        
//        case APPRF_SEND_SUCCEDED:
//        {
//            //on envoie une confirmation par UART de l'envoi du 
//            //message RF
//            appData.state = APPRF_STATE_SERVICE_TASKS;
//            break;
//        }
//        
//        case APPRF_SEND_ERROR:
//        {
//            //si une erreur a été détectée
//            //on envoie par UART l'info comme quoi il y a eu une erreur
////            Tx_size = GetReadSize(&descrFifoTX);
////            DataTX.val32 = ERROR;
////            if(Tx_size >= 9)
////            {
////                SendMessage (&Target_ADD, &DataTX);
////                appData.state = APPRF_STATE_SERVICE_TASKS;
////            }
//            //modif SCA:
//                //baisser signal LINK
//            appData.state = APPRF_STATE_SERVICE_TASKS;
//            break;
//        }
//        
//        case APP_RF_WAIT:
//        {
//            break;
//        }      

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
    Uart_CheckForTxInt();
}


void APPRF_UpdateCycleCntr (void)
{
    appData.cycleCntr++;
}

/*******************************************************************************
 End of File
 */
