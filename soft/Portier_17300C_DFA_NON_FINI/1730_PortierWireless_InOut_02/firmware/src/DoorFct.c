//--------------------------------------------------------
// BoorFct.c
//--------------------------------------------------------
// Gestion communication	
//
//	Auteur 		: 	DFA
//  Date        :   5.12.2024
//	Version		:	V1.0
//	Compilateur	:	XC32 V2.15
//  Modifications :
//   -
/*--------------------------------------------------------*/
#include "DoorFct.h"
#include <xc.h> //pour les définitions des registres
#include "Mc32Delays.h"
#include "peripheral/usart/plib_usart.h"
#include <string.h> //pour memcpy()
#include "led.h"

void CheckDebounce (void)
{
    static uint8_t firstCntr = 0;
    //ignore bouton pendant les 10 premiers passages car sinon l'anti-rebond
    // signale bouton appuyé après quelques passages
    if (firstCntr<9)
    {   
        //if (firstCntr==0)   //au 1er passage => envoi sonnerie
        //    doorState = DOOR_STATE_PRESSED; 
        firstCntr++;
        DebounceClearPressed(&swRing);  
    }
}

void CheckStartRing(DOOR_STATES *doorState)
{
    if(DebounceIsPressed(&swRing) /*|| DebounceIsReleased(&swRing)*//* || first*/)
    {        
        DebounceClearPressed(&swRing); 

        *doorState = DOOR_STATE_PRESSED;            
    }
}
#define DOOR_TIMEOUT 10000 //en 1/100e de sec.
#define TIME_WAIT_TIMEOUT 100    //attente de 500ms
void SendRequestEnter(uint32_t adress)
{
    //static uint16_t timeoutCntr = 0;
                         
//    //renvoi périodique de la trame (ttes les 100 ms)
//    if (waitStartTimeout == TIME_WAIT_TIMEOUT)
//    {
//        //LATAbits.LATA4 = 1; //debug
//        sprintf((char*)msg, "[D%dT%02d%03d]",getSerialRF(), timeoutCntr/10, appData.batVoltage/10);
//        RF_SendMessage(msg, 0);
//        waitStartTimeout = 0;
//        //LATAbits.LATA4 = 0; //debug   
//    }
//    else
//        waitStartTimeout++;
//
//    if (answer == '-')  //pas de réponse
//    {
//        //clignotement (différent si low bat ou pas)
//        if (appData.isLowBat)
//            Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_2X_FAST);
//        else
//            Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_NORMAL);    
//    }
//    RF_GetMessage(msg);
//    if(Checkcorrespondance(msg))
//    {
//        waitStartTimeout = TIME_WAIT_TIMEOUT;
//        doorState = DOOR_STATE_WAITING;
//    }
}

/* *****************************************************************************
 End of File
 */
