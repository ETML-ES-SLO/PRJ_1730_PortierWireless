//--------------------------------------------------------
// led.h
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

#ifndef _LED_H    /* Guard against multiple inclusion */
#define _LED_H

/* ************************************************************************** */

typedef enum
{
    LED_ID_ENTER=0,
	LED_ID_WAIT,
    LED_ID_BUSY,      
} LED_IDS;

typedef enum
{
    LED_MODE_OFF=0,
	LED_MODE_ON,
    LED_MODE_BLINK_NORMAL,
    LED_MODE_BLINK_2X_FAST,       
} LED_MODES; 

/* ************************************************************************** */

void Led_Init(void);
void Led_SetMode(LED_IDS led_Id, LED_MODES led_Mode);
void Led_Mgmt(void);

#endif /* _LED_H */

/* *****************************************************************************
 End of File
 */
