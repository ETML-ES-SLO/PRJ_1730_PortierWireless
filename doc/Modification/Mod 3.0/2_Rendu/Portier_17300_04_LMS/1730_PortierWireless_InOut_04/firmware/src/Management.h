//--------------------------------------------------------
// DoorBellMgmt.h
//--------------------------------------------------------
// Header pour gestion Door et Bell du systeme PortierWireless
//
// Auteurs     : Projet ETML / adaptation LMS
// Date        : 2025
// Version     : V3.0
// Compilateur : XC32
//--------------------------------------------------------

#ifndef DOORBELLMGMT_H
#define DOORBELLMGMT_H

//----------------------
// Inclusions
//----------------------
#include <stdint.h>         // Types entiers standard
#include <stdbool.h>        // Types booleens standard
#include "Mc32Debounce.h"   // Gestion anti-rebond des boutons
#include "app.h"            // Definition de APP_DATA

//----------------------
// Definitions d'etats
//----------------------

// Etats internes pour la gestion de la porte (Door)
typedef enum
{
    DOOR_STATE_INIT = 0,      // Initialisation
    DOOR_STATE_PRESSED,       // Bouton presse
    DOOR_STATE_WAITING,       // En attente de reponse
    DOOR_STATE_ANSWER,        // Reponse recue
    DOOR_STATE_NOANSWER,      // Pas de reponse
    DOOR_STATE_SHUTDOWN       // Arret du systeme
} DOOR_STATES;

// Etats internes pour la gestion de la sonnette (Bell)
typedef enum
{
    BELL_STATE_INIT = 0,      // Initialisation
    BELL_STATE_IDLE,          // Attente (repos)
    BELL_STATE_RING           // Sonnerie active
} BELL_STATES;

//----------------------
// Prototypes de fonctions
//----------------------

// Gestion de la porte
void Door_Mgmt(void);

// Gestion de la sonnette
void Bell_Mgmt(void);

//----------------------
// Variables externes
//----------------------

// Descripteurs pour l'anti-rebond des boutons
extern S_SwitchDescriptor swRing;   // Bouton de sonnerie
extern S_SwitchDescriptor swEnter;  // Bouton d'entree
extern S_SwitchDescriptor swWait;   // Bouton d'attente
extern S_SwitchDescriptor swBusy;   // Bouton d'occupation

// Donnees globales de l'application
extern APP_DATA appData;

#endif // DOORBELLMGMT_H
