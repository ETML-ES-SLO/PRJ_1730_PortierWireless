//--------------------------------------------------------
// sound.c
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

//#define NO_SOUND    //pour désactiver le son

#include "sound.h"
#include "peripheral/oc/plib_oc.h"
#include "peripheral/tmr/plib_tmr.h"

/* ************************************************************************** */
typedef enum
{
	/* Application's state machine's initial state. */
	SOUND_STATE_INIT=0,
    SOUND_STATE_IDLE,
	SOUND_STATE_PLAYING,            
} SOUND_STATES;

//notes en Hz
#define DO3 262
#define RE3 294
#define MI3 330
#define FA3 349
#define SOL3 392
#define LA3 440
#define SI3 494
#define DO4 523
#define RE4 587
#define MI4 659
#define FA4 699
#define SOL4 784
#define LA4 880
#define SI4 988
#define DO5 1047

/* ************************************************************************** */
//déclarations des sons
//la fin d'une mélodie est indiquée par freq et durée à 0
const NOTE_t RING[10] = 
//   {{.duration=150,.freq=LA3},
//    {.duration=150,.freq=SI3},
//    {.duration=150,.freq=DO4},
//    {.duration=150,.freq=FA3},
    {{.duration=200,.freq=DO4},
     {.duration=200,.freq=RE4},
     {.duration=200,.freq=MI4},
     {.duration=200,.freq=FA4},
     {.duration=200,.freq=SOL4},
     {.duration=200,.freq=LA4},
     {.duration=200,.freq=SI4},
     {.duration=1000,.freq=DO5},
     {.duration=0,.freq=0},                         
     {.duration=0,.freq=0}};

const NOTE_t SUCCESS[10] = 
   {{.duration=150,.freq=FA3},
    {.duration=150,.freq=SOL3},
    {.duration=150,.freq=LA3},
    {.duration=150,.freq=SI3},
    {.duration=0,.freq=0},
    {.duration=0,.freq=0},                         
    {.duration=0,.freq=0},                         
    {.duration=0,.freq=0},
    {.duration=0,.freq=0},
    {.duration=0,.freq=0}};

const NOTE_t ERROR[10] = 
   {{.duration=150,.freq=SI3},
    {.duration=150,.freq=LA3},
    {.duration=150,.freq=SOL3},
    {.duration=150,.freq=FA3},
    {.duration=0,.freq=0},
    {.duration=0,.freq=0},                         
    {.duration=0,.freq=0},                         
    {.duration=0,.freq=0},
    {.duration=0,.freq=0},
    {.duration=0,.freq=0}};

/* ************************************************************************** */
//variables
SOUND_STATES soundState;

NOTE_t* currentNotes;
uint16_t currentNoteIndex;
uint16_t currentNoteDuration;

/* ************************************************************************** */
//règle OC1+timer3 pour fréquence de sortie donnée
void SetNote(uint16_t freq)
{
#define F_TMR_CLK 10000000
    uint16_t period;
    // 1/2 période PWM = period_timer * 1/F_TMR_CLK
    period = F_TMR_CLK/(2*freq);
    
    PLIB_TMR_Period16BitSet(TMR_ID_3,period);   
}

/* ************************************************************************** */
void Sound_Init(void)
{
    soundState = SOUND_STATE_INIT;   
}

/* ************************************************************************** */
void Sound_Start(const NOTE_t* sound)
{
    currentNotes = (NOTE_t*)sound; //quelle mélodie jouer  
    currentNoteIndex=0;
    currentNoteDuration=0;
    
    //règle PWM
    SetNote(sound[0].freq);
    
    //starte PWM
    PLIB_TMR_Start(TMR_ID_3);
    #ifndef NO_SOUND
    PLIB_OC_Enable(OC_ID_1);
    #endif

    soundState = SOUND_STATE_PLAYING;   
}

/* ************************************************************************** */
bool Sound_IsIdle(void)
{
    return(soundState == SOUND_STATE_IDLE);   
}

/* ************************************************************************** */
//machine d'etat de gestion des sons.
//A appeler périodiquement (10ms)
void Sound_Mgmt(void)
{

    switch(soundState)
    {
        case SOUND_STATE_INIT:
            soundState = SOUND_STATE_IDLE;
            break;
            
        case SOUND_STATE_IDLE:
            //rien à faire
            break;
            
        case SOUND_STATE_PLAYING:                        
            if (currentNoteDuration<currentNotes[currentNoteIndex].duration)
                currentNoteDuration += 10; //toutes les 10ms et la durée est donnée en ms   
            else //note terminée
            {
                currentNoteIndex++;
                if (currentNotes[currentNoteIndex].duration > 0 
                        && currentNotes[currentNoteIndex].freq > 0) //mélodie pas terminée ?
                {
                    SetNote(currentNotes[currentNoteIndex].freq);
                    currentNoteDuration = 0;
                    
                } else //mélodie terminée
                {
                        //stoppe PWM
                    PLIB_OC_Disable(OC_ID_1);
                    PLIB_TMR_Stop(TMR_ID_3);

                    soundState = SOUND_STATE_IDLE;    
                }
            }    
                   
            break;
    }
}

/* *****************************************************************************
 End of File
 */
