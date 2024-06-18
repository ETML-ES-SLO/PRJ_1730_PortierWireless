//--------------------------------------------------------
// led.c
//--------------------------------------------------------
// Gestion des leds sur portier électronique (1730)
//
//	Auteur 		: 	SCA
//  Date        :   15.01.2020
//	Version		:	V1.0
//	Compilateur	:	XC32 V2.15
//  Modifications :
//   -
/*--------------------------------------------------------*/

#include "led.h"
#include <stdint.h>
#include <xc.h>
#include <stdbool.h>

/* ************************************************************************** */

#define NUMBER_OF_LEDS 3

typedef enum
{
	/* Application's state machine's initial state. */
	LED_STATE_INIT=0,
    LED_STATE_IDLE,
	LED_STATE_ACTIVE,            
} LED_STATES;

/* ************************************************************************** */
//macros
#define LED_ON(led_Id)  {LATBSET = 1<<(led_Id+4);}
#define LED_OFF(led_Id) {LATBCLR = 1<<(led_Id+4);}

/* ************************************************************************** */
//variables
LED_STATES ledState;
LED_MODES ledModes[NUMBER_OF_LEDS];

/* ************************************************************************** */
void Led_Init(void)
{
    uint8_t i;
    
    //tout éteint
    for (i=0 ; i<NUMBER_OF_LEDS ; i++)
    {
        ledModes[i] = LED_MODE_OFF;
        LED_OFF(i);
    }       
    
    ledState = LED_STATE_INIT;   
}

/* ************************************************************************** */
//renvoie true si au moins une led est en mode clignotement
bool Led_CheckBlink()
{
    uint8_t i;
    for (i=0 ; i<NUMBER_OF_LEDS ; i++)
    {
        if (ledModes[i] != LED_MODE_OFF && ledModes[i] != LED_MODE_ON)
            return (true);
    }
    return (false);
}

/* ************************************************************************** */
void Led_SetMode(LED_IDS led_Id, LED_MODES led_Mode)
{
    ledModes[led_Id] = led_Mode;
    
    if(led_Mode == LED_MODE_OFF)    //éteint directement
    {    
        LED_OFF(led_Id);
    } else if(led_Mode == LED_MODE_ON)     //allume directement    
    {
        LED_ON(led_Id);
    }
    
    if(Led_CheckBlink())    //au moins une led clignote
    {
        ledState = LED_STATE_ACTIVE;  
    }
    else    //sinon aucun clignotement        
    {
        if (ledState == LED_STATE_ACTIVE)          
            ledState = LED_STATE_INIT;     
    }    
}

/* ************************************************************************** */
//machine d'etat de gestion des leds.
//A appeler périodiquement (10ms)
void Led_Mgmt(void)
{
    static uint16_t cycleCntr;
    uint8_t i;

    switch(ledState)
    {
        case LED_STATE_INIT:
            cycleCntr = 0;
            ledState = LED_STATE_IDLE;
        //    break;
            
        case LED_STATE_IDLE:
            //rien à faire
            break;
            
        case LED_STATE_ACTIVE:                        
            //gère clignotements
            for (i=0 ; i<NUMBER_OF_LEDS ; i++)
            {
                if (ledModes[i] != LED_MODE_OFF && ledModes[i] != LED_MODE_ON)
                {
                    switch(ledModes[i])
                    {
                        case LED_MODE_BLINK_NORMAL: //clignotement sur 1 sec
                            if (cycleCntr==0)
                                LED_ON(i)
                            else if(cycleCntr==500)
                                LED_OFF(i)                            
                            break;
                            
                        case LED_MODE_BLINK_2X_FAST: //clignotement 2x en 200ms 
                            if (cycleCntr==0)
                                LED_ON(i)
                            else if(cycleCntr==100)
                                LED_OFF(i)
                            else if(cycleCntr==200)
                                LED_ON(i)
                            else if(cycleCntr==300)
                                LED_OFF(i);                              
                            break;
                            
                        default:
                            break;                            
                    }
                }
            }    
            
            //gestion compteur pour cycles de clignotement
            if (cycleCntr<999)
                cycleCntr++;
            else
                cycleCntr = 0;
            
            break;
    }
}

/* *****************************************************************************
 End of File
 */
