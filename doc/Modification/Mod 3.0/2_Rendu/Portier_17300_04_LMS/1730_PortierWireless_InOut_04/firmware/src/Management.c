//--------------------------------------------------------
// DoorBellMgmt.c
//--------------------------------------------------------
// Header pour gestion Door et Bell du systeme PortierWireless
//
// Auteurs     : Projet ETML / adaptation LMS
// Date        : 2025
// Version     : V3.0
// Compilateur : XC32
//--------------------------------------------------------

/* =========================
 * Section des inclusions
 * ========================= */
#include "Management.h" // Declarations des fonctions de gestion (porte, sonnerie)
#include "app.h"        // Structures et variables globales de l'application
#include "led.h"        // Fonctions de gestion des LEDs
#include "sound.h"      // Fonctions de gestion des sons
#include "Pairing.h"    // Fonctions de gestion du jumelage (pairing)
#include "RF.h"         // Fonctions de gestion de la communication RF
#include "peripheral/reset/plib_reset.h" // Pour effectuer un reset logiciel

/**
 * @brief Gestion du bouton-poussoir de la porte avec filtre de pairing integre.
 *
 * Cette fonction gere la logique associee au bouton-poussoir situe a la porte,
 * incluant un filtre pour le processus de jumelage (pairing).
 * 
 * @details
 * - Surveille l'etat du bouton-poussoir.
 * - Applique un filtre logiciel pour eviter les rebonds et les appuis involontaires.
 * - Integre la gestion du pairing pour l'association des dispositifs sans fil.
 *
 * @note
 * Cette fonction fait partie du module de gestion de la porte (Door_Mgmt).
 */
void Door_Mgmt(void)
{
    #define DOOR_TIMEOUT 3100 //en 1/100e de sec.
    
    static uint8_t firstCntr = 0; // Compteur pour ignorer les premiers passages (anti-rebond)
    static uint16_t timeoutCntr = 0; // Compteur de timeout pour la gestion du delai d'attente
    static uint16_t cycleCntr = 0; // Compteur de cycles pour la gestion periodique
    static DOOR_STATES doorState = DOOR_STATE_INIT; // Etat courant de la porte
    uint8_t msg[9]; // Buffer pour stocker les messages RF recus
    static uint8_t answer = '-'; // Variable pour stocker la reponse recue ('E', 'W', 'B', 'N' ou '-')
    PairingReset_DoorTask(); // Reinitialise le filtre de pairing pour la tache porte

    // Ignore le bouton pendant les 10 premiers passages pour eviter les faux positifs dus au rebond
    if (firstCntr<9)
    {   
        if (firstCntr==0)   // Au premier passage, declenche la sonnerie
            doorState = DOOR_STATE_PRESSED; 
        firstCntr++; // Incremente le compteur d'initialisation
        DebounceClearPressed(&swRing);  // Efface l'etat "presse" du bouton (anti-rebond)
    }
    
    // Verifie si le bouton est appuye
    if(DebounceIsPressed(&swRing))
    {        
        DebounceClearPressed(&swRing); // Efface l'etat "presse" du bouton

        doorState = DOOR_STATE_PRESSED; // Passe a l'etat "presse"
    }
    
    switch(doorState)
    {
        case DOOR_STATE_INIT:
            // Rien a faire a l'initialisation
            break;
            
        case DOOR_STATE_PRESSED:
            timeoutCntr = 0; // Reinitialise le compteur de timeout
            // Si aucune reponse n'a encore ete recue
            if (answer == '-')  
            {
                // Clignotement different selon l'etat de la batterie
                if (appData.isLowBat)
                    Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_2X_FAST); // Clignote rapidement si batterie faible
                else
                    Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_NORMAL);  // Clignote normalement sinon
            }
            
            doorState = DOOR_STATE_WAITING; // Passe a l'etat d'attente de reponse
            // Pas de break pour enchaîner sur l'etat suivant
            
        case DOOR_STATE_WAITING:
            // Verifie si un message RF a ete recu
            if (RF_GetMessage(msg)) {
                if (!Serial_CheckIfFromPaired(msg)) {
                    break; // Ignore le message s'il ne vient pas d'un appareil jumele
                }
                if (msg[1]=='B') // Verifie si le message vient de la sonnerie
                {
                    timeoutCntr = 0; // Reinitialise le timeout
                    cycleCntr = 0; // Reinitialise le compteur de cycles
                    
                    switch(msg[4])
                    {
                        case 'E': // Reponse "entrez"
                            answer = 'E';                           
                            doorState = DOOR_STATE_ANSWER; // Passe a l'etat de traitement de la reponse
                            break;
                        case 'W': // Reponse "attendez"
                            answer = 'W';                           
                            doorState = DOOR_STATE_ANSWER;
                            break;
                        case 'B': // Reponse "occupe"
                            answer = 'B';                            
                            doorState = DOOR_STATE_ANSWER;
                            break;
                        case 'N': // Reponse "personne"
                            answer = 'N';
                            doorState = DOOR_STATE_NOANSWER; // Passe a l'etat "pas de reponse"
                            break;
                    }                    
                }
            }
            else // Si aucun message recu, gestion du timeout
            {
                // Si le timeout n'est pas encore atteint
                if (timeoutCntr < DOOR_TIMEOUT)
                {                
                    if (answer == '-')  // Si aucune reponse n'a encore ete recue
                    {                       
                        // Renvoie periodiquement la trame (toutes les 100 ms)
                        if (cycleCntr%10==0)
                        {
                            static uint8_t bellTriggerMsg[8] = "[D00OK]";
                            RF_SendWithSerial(bellTriggerMsg); // Envoie la demande a la sonnerie
                        }                        
                    }                    

                    timeoutCntr++; // Incremente le compteur de timeout
                    if (cycleCntr < 99)
                        cycleCntr++; // Incremente le compteur de cycles
                    else
                        cycleCntr = 0; // Reinitialise le compteur de cycles
                }  
                else    // Si le timeout est atteint
                {
                    if (answer == '-')  // Si aucune reponse n'a ete recue
                        doorState = DOOR_STATE_NOANSWER; // Passe a l'etat "pas de reponse"
                    else    // Si une reponse a ete recue
                        doorState = DOOR_STATE_SHUTDOWN; // Passe a l'etat d'arret
                }
            }
            break;
        
        case DOOR_STATE_ANSWER:
            // Si le module sonore est inactif (aucun son en cours)
            if(Sound_IsIdle())
            {
                switch (answer)
                {
                    case 'E': // Reponse "entrez"
                        Led_SetMode(LED_ID_ENTER, LED_MODE_ON); // Allume la LED "entrez"
                        Led_SetMode(LED_ID_WAIT, LED_MODE_OFF); // Eteint la LED "attendez"
                        Led_SetMode(LED_ID_BUSY, LED_MODE_OFF); // Eteint la LED "occupe"
                        break;
                    case 'W': // Reponse "attendez"
                        Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                        break;
                    case 'B': // Reponse "occupe"
                        Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                        Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                        Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                        break;
                }        
                Sound_Start(SUCCESS); // Joue le son de succes
                doorState = DOOR_STATE_WAITING; // Retourne a l'etat d'attente
            }
            break;
            
        case DOOR_STATE_NOANSWER:
            // Si le module sonore est inactif
            if (Sound_IsIdle())
            {
                Sound_Start(ERROR); // Joue le son d'erreur
                doorState = DOOR_STATE_SHUTDOWN; // Passe a l'etat d'arret
            }        
            break;
            
        case DOOR_STATE_SHUTDOWN:
            // Si le module sonore est inactif
            if (Sound_IsIdle())
            {   
                DRV_TMR0_Stop(); // Arrete le timer principal
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF); // Eteint la LED "attendez"
                LATBbits.LATB0 = 0;    // Coupe l'alimentation (mise en veille)
                while(1);              // Boucle infinie (arret du systeme)
            }  
            break; 
            
        default: /* Le cas par defaut ne devrait jamais etre execute. */
            PLIB_RESET_SoftwareResetEnable(RESET_ID_0); // Effectue un reset logiciel
            break;
    }
}

/**
 * @brief Gestion de la carte de sonnerie (Bell_Mgmt).
 *
 * Cette fonction gere la logique de la carte de sonnerie, y compris la reception des messages,
 * la gestion des boutons de reponse, l'envoi des reponses, et la gestion des etats associes.
 *
 * @details
 * - Surveille les messages RF entrants pour declencher la sonnerie ou repondre a une demande.
 * - Gere les boutons physiques pour envoyer les reponses appropriees (entrez, attendez, occupe).
 * - Gere les delais et le clignotement des LEDs selon l'etat de la batterie et les reponses.
 * - Assure la synchronisation avec d'autres cartes de sonnerie si necessaire.
 *
 * @note
 * Cette fonction fait partie du module de gestion de la sonnerie (Bell_Mgmt).
 */
void Bell_Mgmt(void)
{
    #define BELL_TIMEOUT 3000 //en 1/100e de sec.
    #define ANSWER_NR_MAX 50 //nb de reponses a envoyer * 10 (50 = 5 reponses)

    static BELL_STATES bellState = BELL_STATE_INIT; // Etat courant de la sonnerie
    uint8_t msg[9]; // Buffer pour stocker les messages RF recus
    static uint16_t timeoutCntr = 0; // Compteur de timeout pour la gestion du delai d'attente
    static uint16_t cycleCntr = 0; // Compteur de cycles pour la gestion periodique
    static int8_t answerNr = -1; // Compteur pour le nombre de reponses envoyees
    static uint8_t answerFrame[8] = "[B00OK]"; // Trame de reponse initialisee
    PairingReset_BellTask(); // Reinitialise le filtre de pairing pour la tache sonnerie

    switch(bellState)
    {
        case BELL_STATE_INIT:
            bellState = BELL_STATE_IDLE; // Passe a l'etat inactif
            break;
            
        case BELL_STATE_IDLE:
            DebounceClearPressed(&swEnter); // Efface l'etat "presse" du bouton "entrez"
            DebounceClearPressed(&swWait); // Efface l'etat "presse" du bouton "attendez"
            DebounceClearPressed(&swBusy); // Efface l'etat "presse" du bouton "occupe"
            // Verifie si un message RF a ete recu
            if (RF_GetMessage(msg)) {
                if (!Serial_CheckIfFromPaired(msg)) {
                    break; // Ignore le message s'il ne vient pas d'un appareil jumele
                }
                if (msg[1] == 'D') // Message provenant du bouton de porte
                {
                    timeoutCntr = 0; // Reinitialise le timeout
                    cycleCntr = 0; // Reinitialise le compteur de cycles
                    answerNr = -1;  // Aucune reponse envoyee pour l'instant
                    Sound_Start(RING);      // Lance la sonnerie 
                    
                    // Traitement de la valeur de la tension de la pile
                    appData.batVoltage = 1000*(msg[4]-48) + 100*(msg[5]-48) + 10*(msg[6]-48);
                    appData.isLowBat = appData.batVoltage < LOWBAT_THRESHOLD;
                    
                    // Clignotement des LEDs selon l'etat de la batterie
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
                    bellState = BELL_STATE_RING; // Passe a l'etat de sonnerie
                } else if (msg[1]=='B') // Message provenant d'une autre sonnerie
                {
                    bellState = BELL_STATE_RING; // Passe a l'etat de sonnerie (synchronisation)
                }                 
            }
            break;
            
        case BELL_STATE_RING:
            if (timeoutCntr<BELL_TIMEOUT)
                timeoutCntr++; // Incremente le compteur de timeout
            if (cycleCntr < 99)
                cycleCntr++; // Incremente le compteur de cycles
            else
                cycleCntr = 0; // Reinitialise le compteur de cycles
            
            // Verifie si un message RF a ete recu (vider le buffer UART)
            if (RF_GetMessage(msg)) {
                if (!Serial_CheckIfFromPaired(msg)) {
                    break; // Ignore le message s'il ne vient pas d'un appareil jumele
                }
                if (msg[1]=='D') // Message provenant du bouton de porte
                {
                    timeoutCntr = 0;    // Reinitialise le timeout (nouvel appui sur le bouton)
                } else if (msg[1]=='B') // Message provenant d'une autre sonnerie
                {                
                    answerNr = ANSWER_NR_MAX; // Ne pas envoyer de messages RF (l'autre sonnerie s'en charge)
                    switch(msg[4])
                    {
                        case 'E':   // Reponse "entrez"
                            Led_SetMode(LED_ID_ENTER, LED_MODE_ON);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                            timeoutCntr = 0;                            
                            break;
                        case 'W':   // Reponse "attendez"
                            Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                            timeoutCntr = 0;                            
                            break;
                        case 'B':   // Reponse "occupe"
                            Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                            Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                            Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                            timeoutCntr = 0;
                            break;
                    }
                }    
            }           
            
            // Gestion des boutons de reponse
            if(DebounceIsPressed(&swEnter)) // Bouton "entrez" appuye
            {        
                DebounceClearPressed(&swEnter);
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_ON);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                answerNr =0;
                memcpy (answerFrame, "[B00E  ]", 8); // Prepare la trame de reponse "entrez"
            }    
            if(DebounceIsPressed(&swWait)) // Bouton "attendez" appuye
            {        
                DebounceClearPressed(&swWait); 
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_ON);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                answerNr =0;
                memcpy (answerFrame, "[B00W  ]", 8); // Prepare la trame de reponse "attendez"
            }  
            if(DebounceIsPressed(&swBusy)) // Bouton "occupe" appuye
            {        
                DebounceClearPressed(&swBusy); 
                timeoutCntr = 0;
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_ON);
                answerNr =0;
                memcpy (answerFrame, "[B00B  ]", 8); // Prepare la trame de reponse "occupe"
            }  
           
            // Gestion du timeout : si aucune reponse et timeout atteint et sonnerie terminee
            if (answerNr==-1 && timeoutCntr>=BELL_TIMEOUT && Sound_IsIdle())
            {                   
                answerNr = 0;
                memcpy (answerFrame, "[B00N  ]", 8); // Prepare la trame "il n'y a personne"
            }

            // Envoie 5 trames de reponse (une toutes les 100ms)
            if (answerNr >= 0 && answerNr < ANSWER_NR_MAX) {
                if ((answerNr % 10) == 0) // Toutes les 100 ms
                {
                    answerFrame[3] = (answerNr / 10) + 48; // Met a jour le numero de la trame
                    RF_SendWithSerial(answerFrame); // Envoie la trame de reponse    
                }
                answerNr++; // Incremente le compteur de reponses envoyees
            }
            else if (answerNr>=ANSWER_NR_MAX && timeoutCntr>=BELL_TIMEOUT)   // Retour a l'etat idle si toutes les reponses ont ete envoyees et timeout atteint
            {
                Led_SetMode(LED_ID_ENTER, LED_MODE_OFF);
                Led_SetMode(LED_ID_WAIT, LED_MODE_OFF);
                Led_SetMode(LED_ID_BUSY, LED_MODE_OFF);
                bellState = BELL_STATE_IDLE; // Retour a l'etat inactif
            }  

            break;
                        
        default: /* Le cas par defaut ne devrait jamais etre execute. */
            PLIB_RESET_SoftwareResetEnable(RESET_ID_0); // Effectue un reset logiciel
            break;
    }
    
}
