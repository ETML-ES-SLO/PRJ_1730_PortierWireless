//--------------------------------------------------------
// RF.h
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

#ifndef _RF_H    /* Guard against multiple inclusion */
#define _RF_H

#include <stdint.h>
#include <stdbool.h>

/* ************************************************************************** */


/* ************************************************************************** */



/* ************************************************************************** */

void RF_Init(void);
void RF_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend);
bool RF_GetMessage(uint8_t* msg);

#endif /* _RF_H */

/* *****************************************************************************
 End of File
 */
