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
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS LMS BE LIABLE OR OBLIGATED UNDER
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
// Section: Fichiers inclus 
// *****************************************************************************
// *****************************************************************************
// Inclusion du fichier d'en-tête principal de l'application
#include "app.h"

// Inclusion des en-têtes de la bibliothèque standard
#include <stdio.h>  

// Inclusion des en-têtes spécifiques au projet
#include "peripheral/adc/plib_adc.h"         // Pour la gestion de l'ADC (convertisseur analogique/numérique)
#include "peripheral/reset/plib_reset.h"     // Pour effectuer un reset logiciel
#include "sound.h"                           // Pour la gestion du son
#include "RF.h"                              // Pour la gestion de la radiofréquence
#include "led.h"                             // Pour la gestion des LEDs
#include "Pairing.h"                         // Pour la gestion de l'appairage
#include "management.h"                      // Pour les fonctions de gestions des modules

// *****************************************************************************
// *****************************************************************************
// Données globales de l'application
// *****************************************************************************
// *****************************************************************************

// Structure contenant les données de l'application
APP_DATA appData; // Structure principale des données de l'application

// Descripteur pour le bouton "Entrée"
S_SwitchDescriptor swEnter; // Descripteur du bouton d'entrée

// Descripteur pour le bouton "Occupé"
S_SwitchDescriptor swBusy; // Descripteur du bouton occupé

// Descripteur pour le bouton "Sonne"
S_SwitchDescriptor swRing; // Descripteur du bouton de sonnette

// Descripteur pour le bouton "Attente"
S_SwitchDescriptor swWait; // Descripteur du bouton d'attente

// Variable globale pour stocker l'identifiant appairé (numéro de série du module appairé)
uint32_t appPairedSerial = 0; // Numéro de série appairé, initialisé à 0

// Déclaration externe du numéro de série propre à ce module
extern uint32_t mySerial; // Numéro de série du module courant (défini ailleurs)

// *****************************************************************************
// *****************************************************************************
// Section: Fonctions spécifiques à l'application
// *****************************************************************************
// *****************************************************************************
/**
 * @brief Lit la tension mesurée sur l'entrée analogique AN1.
 *
 * Cette fonction lit la valeur convertie par l'ADC sur AN1.
 * - Si le montage est un bouton à la porte, elle retourne la tension de la pile (diviseur résistif 2/3).
 * - Si c'est une sonnerie distante, elle retourne 0 (pull-down).
 *
 * @return uint16_t Valeur brute ADC mesurée sur AN1.
 */
uint16_t ReadBatVoltage(void) {
  uint16_t result;

  // Démarre la conversion ADC
  PLIB_ADC_ConversionStart(DRV_ADC_ID_1);
  while (!PLIB_ADC_ConversionHasCompleted(DRV_ADC_ID_1)); // Attente de la fin de conversion

  // Récupère le résultat de la conversion
  result = PLIB_ADC_ResultGetByIndex(DRV_ADC_ID_1, 0);

  return result;
}

/**
 * @brief Détermine si le CPU est à la porte (Door) ou à la sonnette intérieure (Bell).
 * 
 * Cette fonction met à jour le champ appData.isDoor selon la position du CPU.
 * Si le CPU est à la porte (isDoor), elle lit la tension de la pile,
 * met à jour appData.batVoltage (tension pile en mV) et appData.isLowBat
 * (indication de batterie faible).
 * 
 * @note Utilise l'ADC pour effectuer les mesures nécessaires.
 * @note Désactive l'ADC à la fin pour économiser la batterie.
 */
void SetDoor(void) {
  // Détermination si le CPU est sur le board bouton à la porte (Door) ou sur la sonnerie
  PLIB_ADC_Enable(DRV_ADC_ID_1);                  // Active le module ADC
  PLIB_ADC_SamplingStart(DRV_ADC_ID_1);           // Démarre l'échantillonnage ADC
  appData.isDoor = ReadBatVoltage() > 600;        // Si la tension lue est > 600, on considère que c'est la porte
  if (appData.isDoor) // Si on est sur le board bouton => lecture tension pile
  {
    // Commute sur la référence de tension interne
    PLIB_ADC_Disable(DRV_ADC_ID_1);                                         // Désactive l'ADC pour changer la référence
    PLIB_ADC_VoltageReferenceSelect(DRV_ADC_ID_1, ADC_REFERENCE_VREFPLUS_TO_AVSS); // Sélectionne la référence de tension VREF+ à AVSS
    PLIB_ADC_Enable(DRV_ADC_ID_1);                                          // Réactive l'ADC
    PLIB_ADC_SamplingStart(DRV_ADC_ID_1);                                   // Démarre un nouvel échantillonnage
    appData.batVoltage = ReadBatVoltage();                                  // Lit la tension de la batterie (valeur brute ADC)
    appData.batVoltage = 2 * appData.batVoltage;                            // Conversion en mV (1024 pour 2.048 V)
    appData.batVoltage = (3 * appData.batVoltage) / 2;                      // Calcule la tension batterie réelle (diviseur résistif externe 2/3)
    appData.isLowBat = appData.batVoltage < LOWBAT_THRESHOLD;               // Indique si la batterie est faible
  }
  // On peut éteindre l'ADC car on n'en a plus besoin (économie de piles)
  PLIB_ADC_Disable(DRV_ADC_ID_1);                  // Désactive le module ADC
}

/**
 * @brief Met à jour l'état de l'application.
 * @param newState Le nouvel état à appliquer à l'application.
 * 
 * Cette fonction assigne la valeur de newState à la variable d'état de l'application.
 */
void APP_UpdateState(APP_STATES newState) {
  appData.state = newState;
}

/**
 * @brief Vérifie si l'entité courante est une porte.
 * @return true si l'entité est une porte, false sinon.
 * 
 * Cette fonction retourne la valeur du champ isDoor de la structure appData.
 */
bool APP_GetIsDoor(void) {
  return (appData.isDoor);
}

// *****************************************************************************
// *****************************************************************************
// Section: Initialisation de l'application et fonctions de la machine à états
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Initialise l'application.
 * 
 * Cette fonction configure l'état initial de l'application, initialise
 * les broches matérielles, la logique d'antirebond pour les boutons,
 * ainsi que les périphériques tels que les LEDs, le module sonore et la RF.
 * 
 * @return void
 */
void APP_Initialize(void) {
  LATBbits.LATB0 = 1; // Maintient l'alimentation (power hold)

#ifdef DEBUG_PIN
  LATAbits.LATA4 = 0; // DEBUG: met la broche RA4 à 0 (utilisée pour le debug)
#endif

  /* Place la machine d'état de l'application dans son état initial */
  appData.state = APP_STATE_INIT;

  // Initialise la logique d'antirebond pour chaque bouton
  DebounceInit(&swRing);  // Bouton sonnette
  DebounceInit(&swEnter); // Bouton entrée
  DebounceInit(&swWait);  // Bouton attente
  DebounceInit(&swBusy);  // Bouton occupé

  // Initialise les périphériques
  Led_Init();    // LEDs
  Sound_Init();  // Module sonore
  RF_Init();     // Module radiofréquence
}

/**
 * @brief Tâche principale de l'application.
 *
 * Cette fonction gère la machine à états principale de l'application.
 * Elle est appelée de manière répétée dans la boucle principale du programme.
 *
 * @remarks
 * Voir le prototype dans app.h.
 */
void APP_Tasks(void) {
  /* Vérifie l'état courant de l'application. */
  switch (appData.state) {
    /* État initial de l'application. */
    case APP_STATE_INIT:
    {
      SetDoor(); // Détermine si le module est à la porte ou à la sonnette

      DRV_TMR0_Start(); // Démarre le timer 0
      //TestFlashWriteRead(); // (optionnel) Test lecture/écriture de la flash
      LoadPairedSerialAndApply(); // Charge le numéro de série appairé s'il existe
      appData.state = APP_STATE_WAIT; // Passe à l'état d'attente
      break;
    }

    case APP_STATE_WAIT:
    {
      SYS_DEVCON_PowerModeEnter(SYS_POWER_MODE_IDLE); // Met le CPU en mode idle pour économiser l'énergie
      // On rentre en mode idle. Fait gagner 5 mA de consommation à 10 MHz / 100 Hz interruption Timer 1
      break;
    }

    case APP_STATE_SERVICE_TASKS:
    {
      //LATAbits.LATA4 = ~LATAbits.LATA4;   //debug : inverse l'état de la broche RA4
      // Gestion de l'appairage si non établi
      if (appPairedSerial == 0) {
        appPairedSerial = PairingManagement(); // Lance la gestion de l'appairage
      } else {
        if (appData.isDoor) {
          Door_Mgmt(); // Gestion spécifique si le module est à la porte
        } else {
          Bell_Mgmt(); // Gestion spécifique si le module est à la sonnette
        }
      }
      Led_Mgmt();   // Gestion des LEDs
      Sound_Mgmt(); // Gestion du module sonore

      appData.state = APP_STATE_WAIT; // Retour à l'état d'attente

      break;
    }

    default: /* L'état par défaut ne devrait jamais être exécuté. */
      PLIB_RESET_SoftwareResetEnable(RESET_ID_0); // Effectue un reset logiciel du module
      break;
  }
}



/*******************************************************************************
 Fin du fichier
 */
