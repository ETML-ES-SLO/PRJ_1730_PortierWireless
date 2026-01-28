//--------------------------------------------------------
// sound.h
//--------------------------------------------------------
// Gestion mélodies sur haut-parleur portier électronique (1730)
//	Description : HP branché sur RB15 (OC1)	
//
//	Auteur 		: 	SCA
//  Date        :   26.11.2019
//	Version		:	V1.0
//	Compilateur	:	XC32 V2.15
//  Modifications :
//   -
/*--------------------------------------------------------*/

#ifndef _SOUND_H    /* Guard against multiple inclusion */
#define _SOUND_H

#include <stdint.h>
#include <stdbool.h>

/* ************************************************************************** */
typedef struct{
    uint16_t duration; //duree de la note en ms
    uint16_t freq;     //freq de la note en Hz
} NOTE_t;

/* ************************************************************************** */

extern const NOTE_t RING[];
extern const NOTE_t SUCCESS[];
extern const NOTE_t ERROR[];

/* ************************************************************************** */

void Sound_Init(void);
void Sound_Start(const NOTE_t* sound);
bool Sound_IsIdle(void);
void Sound_Mgmt(void);

#endif /* _SOUND_H */

/* *****************************************************************************
 End of File
 */
