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

#include "app.h"
//#include "Mc32Delays.h"
#include "peripheral/adc/plib_adc.h"
#include "sound.h"
#include "RF.h"
#include "led.h"
#include <stdio.h>  //pour sprintf
#include "peripheral/reset/plib_reset.h" //pour reset soft


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
    
S_SwitchDescriptor swRing;
S_SwitchDescriptor swEnter;
S_SwitchDescriptor swWait;
S_SwitchDescriptor swBusy;


// *****************************************************************************
// Prototypes



// *****************************************************************************
// Section: Application Local Functions

//gestion board bouton-poussoir à la porte
void Door_Mgmt(void)
{
    #define DOOR_TIMEOUT 3100 //en 1/100e de sec.
    
    static uint8_t firstCntr = 0;
    static uint16_t timeoutCntr = 0;
    static uint16_t cycleCntr = 0;
    static DOOR_STATES doorState = DOOR_STATE_INIT;
    uint8_t msg[9];
    static uint8_t answer = '-';
    
    //ignore bouton pendant les 10 premiers passages car sinon l'anti-rebond
    // signale bouton appuyé après quelques passages
    if (firstCntr<9)
    {   
        if (firstCntr==0)   //au 1er passage => envoi sonnerie
            doorState = DOOR_STATE_PRESSED; 
        firstCntr++;
        DebounceClearPressed(&swRing);  
    }
    
    //bouton appuyé ?
    if(DebounceIsPressed(&swRing) /*|| DebounceIsReleased(&swRing)*//* || first*/)
    {        
        DebounceClearPressed(&swRing); 

        doorState = DOOR_STATE_PRESSED;            
    }
    
    switch(doorState)
    {
        case DOOR_STATE_INIT:
            //rien à faire
            break;
            
        case DOOR_STATE_PRESSED:
            timeoutCntr = 0;
            //LATAbits.LATA4 = 1; //debug
            //sprintf((char*)msg, "[D%02d%03d]", timeoutCntr/10, appData.batVoltage/10);
            //RF_SendMessage(msg, 0);
            //LATAbits.LATA4 = 0; //debug
            if (answer == '-')  //pas de réponse
            {
                //clignotement (différent si low bat ou pas)
                if (appData.isLowBat)
                    Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_2X_FAST);
                else
                    Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_NORMAL);    
            }
            
            doorState = DOOR_STATE_WAITING;
            //break;
            
        case DOOR_STATE_WAITING:
            if (RF_GetMessage(msg)) //nouveau msg reçu ?
            {
                if (msg[1]=='B') //message bien en provenance de la sonnerie ?
                {
                    //LATAbits.LATA4 = !LATAbits.LATA4;//1; //debug
                    //LATAbits.LATA4 = 0; //debug
                    
                    timeoutCntr = 0;
                    cycleCntr = 0;
                    
                    switch(msg[4])
                    {
                        case 'E': //entrez
                            answer = 'E';                           
                            doorState = DOOR_STATE_ANSWER;
                            break;
                        case 'W': //attendez
                            answer = 'W';                           
                            doorState = DOOR_STATE_ANSWER;
                            break;
                        case 'B': //occupé
                            answer = 'B';                            
                            doorState = DOOR_STATE_ANSWER;
                            break;
                        case 'N': //personne
                            answer = 'N';
                            doorState = DOOR_STATE_NOANSWER  ;                            
                            break;
                    }                    
                }
            }
            else //attente timeout
            {
                //gestion timeout
                if (timeoutCntr < DOOR_TIMEOUT) //1000=10[s] / 200=2[s]]
                {                
                    if (answer == '-')  //pas de réponse
                    {                       
                        //renvoi périodique de la trame (ttes les 100 ms)
                        if (cycleCntr%10==0)
                        {
                            //LATBbits.LATB4 = 1; //debug
                            sprintf((char*)msg, "[D%02d%03d]", timeoutCntr/10, appData.batVoltage/10);
                            RF_SendMessage(msg, 0);
                            //LATBbits.LATB4 = 0; //debug
                        }                        
                    }                    

                    timeoutCntr++;
                    if (cycleCntr < 99)
                        cycleCntr++;
                    else
                        cycleCntr = 0;    
                }  
                else    // timeout
                {
//                    LATAbits.LATA4 = 1; //debug
//                    LATAbits.LATA4 = 0; //debug
                    //cycleCntr = 0; 
                    if (answer == '-')  //pas eu de réponse
                        doorState = DOOR_STATE_NOANSWER;
                    else    //il y a eu une réponse
                        doorState = DOOR_STATE_SHUTDOWN;
                }
            }
            break;
        
        case DOOR_STATE_ANSWER:
//            if (cycleCntr==0) //1er passage ?
//            {                
            if(Sound_IsIdle())
            {
                switch (answer)
                {
                    case 'E': //entrez                        
                        Led_SetMode(LED_ID_ENTER, LED_MODE_ON);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);                          
                        break;
                    case 'W': //attendez
                        Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);                        
                        break;
                    case 'B': //occupé
                        Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                        break;
                }        
                //cycleCntr++;
                Sound_Start(SUCCESS);
                doorState = DOOR_STATE_WAITING; 
            }
            break;
            
        case DOOR_STATE_NOANSWER:
            if (Sound_IsIdle())
            {
                Sound_Start(ERROR);
                doorState = DOOR_STATE_SHUTDOWN;
            }        
            break;
            
        case DOOR_STATE_SHUTDOWN:
            if (Sound_IsIdle())
            {   
                //extinction si son terminé
                DRV_TMR0_Stop();
                //LATBbits.LATB5 = 0;    //éteint LED "attendez"
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                LATBbits.LATB0 = 0;    //coupure alim.
                while(1);                 
            }  
            break; 
            
        default: /* The default state should never be executed. */
            PLIB_RESET_SoftwareResetEnable(RESET_ID_0); //reset soft
            break;
    }
}

// *****************************************************************************
//gestion board sonnerie
void Bell_Mgmt(void)
{
    #define BELL_TIMEOUT 3000 //en 1/100e de sec.
    #define ANSWER_NR_MAX 50 //nb de réponses à envoyer * 10 (50 = 5 réponses))

    static BELL_STATES bellState = BELL_STATE_INIT;
    uint8_t msg[9];
    static uint16_t timeoutCntr = 0;
    static uint16_t cycleCntr = 0;
    static int8_t answerNr = -1;
    static uint8_t answerFrame[8];  
    
    switch(bellState)
    {
        case BELL_STATE_INIT:
            bellState = BELL_STATE_IDLE;
            break;
            
        case BELL_STATE_IDLE:
            DebounceClearPressed(&swEnter); 
            DebounceClearPressed(&swWait); 
            DebounceClearPressed(&swBusy);
            if (RF_GetMessage(msg)) //msg reçu ?
            {
                if (msg[1]=='D') //message bien en provenance du bouton de porte ?
                {
                    timeoutCntr = 0;
                    cycleCntr = 0;
                    answerNr = -1;  //pas encore de réponse
                    Sound_Start(RING);      //lance sonnerie 
                    
                    //traitement valeur tension pile
                    appData.batVoltage = 1000*(msg[4]-48) + 100*(msg[5]-48) + 10*(msg[6]-48);
                    appData.isLowBat = appData.batVoltage < LOWBAT_THRESHOLD;
                    
                    //clignotement (différent si low bat ou pas)
                    if (appData.isLowBat)
                    {
                        Led_SetMode(LED_ID_ENTER, LED_MODE_BLINK_2X_FAST);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_2X_FAST);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_BLINK_2X_FAST);
                    } else
                    {
                        Led_SetMode(LED_ID_ENTER, LED_MODE_BLINK_NORMAL);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_NORMAL);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_BLINK_NORMAL);
                    }
                    bellState = BELL_STATE_RING;
                } else if (msg[1]=='B') //message en provenance d'une autre Bell ?
                {
                    bellState = BELL_STATE_RING;       
                }                 
            }
            break;
            
        case BELL_STATE_RING:
            if (timeoutCntr<BELL_TIMEOUT)
                timeoutCntr++;
            if (cycleCntr < 99)
                cycleCntr++;
            else
                cycleCntr = 0; 
            
            //msg reçu ? Il faut vider le buffer uart car les trames arrivent toutes les 100ms
            if (RF_GetMessage(msg)) 
            {
                if (msg[1]=='D') //message bien en provenance du bouton de porte ?
                {
                    timeoutCntr = 0;    //remise compteur timeout à 0 (la personne à la porte à réappuyé sur bouton)
                } else if (msg[1]=='B') //message en provenance d'une autre Bell ?
                {                
                    answerNr = ANSWER_NR_MAX; //pour ne pas envoyer de messages RF (l'autre bell le fait déjà)
                    switch(msg[4])
                    {
                        case 'E':   //entrez !
                            Led_SetMode(LED_ID_ENTER, LED_MODE_ON);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                            timeoutCntr = 0;                            
                            break;
                        case 'W':   //Attendez (wait) !
                            Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                            timeoutCntr = 0;                            
                            break;
                        case 'B':   //occupé (busy) !
                            Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                            timeoutCntr = 0;
                            break;
                    }
                }    
            }           
            
            //gestion boutons réponse
            if(DebounceIsPressed(&swEnter)) //entrez !
            {        
                DebounceClearPressed(&swEnter);
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_ON);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                answerNr =0;
                memcpy (answerFrame, "[B00E  ]", 8); //réponse : entrez !
            }    
            if(DebounceIsPressed(&swWait)) //attendez !
            {        
                DebounceClearPressed(&swWait); 
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                answerNr =0;
                memcpy (answerFrame, "[B00W  ]", 8); //réponse : attendez !
            }  
            if(DebounceIsPressed(&swBusy)) //occupé !
            {        
                DebounceClearPressed(&swBusy); 
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                answerNr =0;
                memcpy (answerFrame, "[B00B  ]", 8); //réponse : occupé !
            }  
           
            //gestion timeout
            if (answerNr==-1 && timeoutCntr>=BELL_TIMEOUT && Sound_IsIdle()) //pas eu de réponse ET timeout ET sonnerie terminée ?
            {                   
                answerNr = 0;
                memcpy (answerFrame, "[B00N  ]", 8); //réponse : il n'y a personne !  
            }            
                       
            //envoie 5 trames de réponses (une ttes les 100ms)
            if (answerNr>=0 && answerNr<ANSWER_NR_MAX)
            {
                if ((answerNr%10) == 0) //multiple 100 ms ?
                {
                    answerFrame[3] = (answerNr/10) + 48;
                    RF_SendMessage(answerFrame, 8); //envoi réponse    
                }
                answerNr++;
            }
            else if (answerNr>=ANSWER_NR_MAX && timeoutCntr>=BELL_TIMEOUT)   //retour état idle si réponse envoyée et timeout
            {
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                bellState = BELL_STATE_IDLE;
            }  

            break;
                        
        default: /* The default state should never be executed. */
            PLIB_RESET_SoftwareResetEnable(RESET_ID_0); //reset soft
            break;
    }
    
}
            
// *****************************************************************************
//renvoie la tension sur AN1
//dans le cas d'un montage bouton à la porte : on lira la tension de la pile
//  (attention diviseur résistif 2/3)
//dans le cas de la sonnerie distante : on lira 0 (uniquement pull-down montée)
uint16_t ReadBatVoltage(void)
{
    uint16_t result;
    
    //PLIB_ADC_Enable(DRV_ADC_ID_1);
    
    // Sampling    
//    PLIB_ADC_SamplingStart(DRV_ADC_ID_1);
//    while(PLIB_ADC_SamplingIsActive(DRV_ADC_ID_1)); //attente fin
    
//    PLIB_ADC_SamplingStop(DRV_ADC_ID_1);
    //conversion
    PLIB_ADC_ConversionStart(DRV_ADC_ID_1); 
    while(!PLIB_ADC_ConversionHasCompleted(DRV_ADC_ID_1)); //attente fin
    
    //Lire résultat
    result = PLIB_ADC_ResultGetByIndex(DRV_ADC_ID_1, 0);
    
    //PLIB_ADC_Disable(DRV_ADC_ID_1);
    
    return (result);
}

// *****************************************************************************
//cette fonction détermine si le CPU est à la porte (Door)
// ou est une sonnrerie à l'intérieur de la pièce (Bell)
//modifie appData.isDoor en conséquence
//si isDoor => lecture tension pile et set appData.batVolrtage (tension pile)
// et appData.isLowBat (indication tension pile faible)
void SetDoor(void)
{
    //détermination si le CPU est sur board bouton à la porte (Door) ou sonnerie
    PLIB_ADC_Enable(DRV_ADC_ID_1);
    PLIB_ADC_SamplingStart(DRV_ADC_ID_1);            
    appData.isDoor = ReadBatVoltage() > 600;                  
    if (appData.isDoor)   //si on est sur le board bouton => lecture tension pile
    {
        //commute sur la ref de tension
        PLIB_ADC_Disable(DRV_ADC_ID_1);
        PLIB_ADC_VoltageReferenceSelect(DRV_ADC_ID_1, ADC_REFERENCE_VREFPLUS_TO_AVSS);
        PLIB_ADC_Enable(DRV_ADC_ID_1);
        PLIB_ADC_SamplingStart(DRV_ADC_ID_1);                 
        appData.batVoltage = ReadBatVoltage();
        appData.batVoltage = 2 * appData.batVoltage; //conversion en mV (1024 pour 2.048 V)
        appData.batVoltage = (3 * appData.batVoltage) / 2; //calcul tension batterie (diviseur résistif externe 2/3)
        appData.isLowBat = appData.batVoltage < LOWBAT_THRESHOLD;
    }
    //on peut éteindre l'ADC car on n'en a plus besoin (économise piles)
    PLIB_ADC_Disable(DRV_ADC_ID_1);
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    LATBbits.LATB0 = 1;    //maintien power

    LATAbits.LATA4 = 0;   //debug
    
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
//    appData.timeoutCntr = 0;
//    appData.first = true;

    DebounceInit(&swRing);
    DebounceInit(&swEnter);
    DebounceInit(&swWait);
    DebounceInit(&swBusy);
    
    Led_Init();
    Sound_Init();
    RF_Init(); 
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {   
            SetDoor();
            
            DRV_TMR0_Start();           
            
            appData.state = APP_STATE_WAIT;
            break;
        }
        
        case APP_STATE_WAIT:
        {
            //on rentre en idle mode. Fait gagner 5 mA de conso @ 10 MHz / 100 Hz interrupt Timer 1
            SYS_DEVCON_PowerModeEnter(SYS_POWER_MODE_IDLE); // ou SYS_POWER_MODE_SLEEP
            break;
        }
        
        case APP_STATE_SERVICE_TASKS:
        {        
            //LATAbits.LATA4 = ~LATAbits.LATA4;   //debug : inverse RA4
 
            if (appData.isDoor)
                Door_Mgmt();    //le montage est situé à la porte
            else
                Bell_Mgmt();    //le montage est la sonnerie à l'intérieur du bureau            
            
            Led_Mgmt();
            Sound_Mgmt();
            
            appData.state = APP_STATE_WAIT;
            
            break;
        }     
                
        default: /* The default state should never be executed. */
            PLIB_RESET_SoftwareResetEnable(RESET_ID_0); //reset soft
            break;
    }
}

void APP_UpdateState(APP_STATES newState)
{
    appData.state = newState;    
}

bool APP_GetIsDoor(void)
{
    return(appData.isDoor);
}


/*******************************************************************************
 End of File
 */
