/*
 * Pairing.c
 * Gestion de l'appairage Door / Bell
 * - Envoi automatique des trames P0 / P1 / A0 / A1 (non bloquant)
 * - Reset d'appairage par appui long (Door : swRing, Bell : trois boutons)
 * - Sauvegarde / lecture du numero appaire en Flash (Harmony NVM)
 *
 * MCU      : PIC32MX130F064B
 * Outils   : XC32 v2.50, Harmony v2_06
 * Auteurs  : Projet PortierWireless LMS
 * ---------------------------------------------------------------------------
 * Historique
 * V3.1  2025-05-31  Nettoyage : variables inutilisees supprimees,
 *                    prototypes internes ajoutes, en-tete normalise.
 */

// *****************************************************************************
// *****************************************************************************
// Section: Fichiers inclus 
// *****************************************************************************
// *****************************************************************************
// Inclusion du fichier d'en-tete principal de l'application
#include "app.h" // Inclusion du header principal de l'application

// Inclusion des en-tetes de la bibliotheque standard
#include <stdio.h> // Inclusion pour les fonctions d'entree/sortie standard
#include <string.h>                     // Pour la manipulation des chaines de caracteres

// Inclusion des en-tetes specifiques au projet
#include "driver/nvm/drv_nvm.h"         // Pour la gestion de la memoire non volatile (NVM)
#include "led.h"                        // Pour la gestion des LEDs
#include "Mc32Debounce.h"               // Pour la gestion de l'anti-rebond des boutons
#include "Mc32Delays.h"                 // Pour les fonctions de temporisation
#include "Pairing.h"                    // Pour la gestion de l'appairage
#include "RF.h"                         // Pour la gestion de la radiofrequence
#include "sound.h"                      // Pour la gestion du son


// *****************************************************************************
// *****************************************************************************
// Donnees globales de l'application
// *****************************************************************************
// *****************************************************************************
static DRV_HANDLE nvmHandle = DRV_HANDLE_INVALID; // Handle du driver NVM
static uint32_t pairedSerial = 0; // Numero de serie appaire
static uint32_t mySerial = 0; // Mon numero de serie
static uint16_t pairingSerialHigh = 0; // Partie haute du numero de serie en cours d'appairage
static uint16_t pairingSerialLow = 0; // Partie basse du numero de serie en cours d'appairage
static bool pairingInProgress = false; // Indique si un appairage est en cours
static bool p0Sent = false; // Indique si la trame P0 a ete envoyee
static bool p1Sent = false; // Indique si la trame P1 a ete envoyee
static bool a0Sent = false; // Indique si la trame A0 a ete envoyee
static bool a1Sent = false; // Indique si la trame A1 a ete envoyee
static uint16_t incomingSerialHigh = 0; // Partie haute du numero de serie recu
static uint16_t incomingSerialLow = 0; // Partie basse du numero de serie recu
static uint32_t lastReceivedSerial = 0; // Dernier numero de serie recu
static bool waitingForFullSerial = true; // Attente de la reception complete du numero de serie

// *****************************************************************************
// *****************************************************************************
// Machines d'etat appairage
// *****************************************************************************
// *****************************************************************************
typedef enum {
    BELL_IDLE, BELL_SEND_P0, BELL_SEND_P1, BELL_WAIT_ACK, BELL_DONE
} BellState; // Etats de la machine d'etat cote Bell

typedef enum {
    DOOR_IDLE, DOOR_WAIT_P, DOOR_SEND_A0, DOOR_SEND_A1, DOOR_DONE
} DoorState; // Etats de la machine d'etat cote Door

static BellState bellState = BELL_IDLE; // Etat courant cote Bell
static DoorState doorState = DOOR_IDLE; // Etat courant cote Door

// *****************************************************************************
// *****************************************************************************
// Fonctions utilitaires generales
// *****************************************************************************
// *****************************************************************************

/**
 * @name Serial_IsPairingRunning
 * @brief Indique si une procedure d'appairage est en cours.
 * @return true si l'appairage est en cours, false sinon.
 */
bool Serial_IsPairingRunning(void) {
    return pairingInProgress; // Retourne l'etat d'appairage
}

/**
 * @name NVM_OpenDriver
 * @brief Ouvre le driver NVM si ce n'est pas deja fait.
 */
void NVM_OpenDriver(void) {
    if (nvmHandle == DRV_HANDLE_INVALID) { // Si le driver n'est pas ouvert
        nvmHandle = DRV_NVM_Open(DRV_NVM_INDEX_0, DRV_IO_INTENT_READWRITE); // Ouvre le driver NVM
        if (nvmHandle == DRV_HANDLE_INVALID) { // Si l'ouverture a echoue
            while (1); // Boucle infinie en cas d'erreur
        }
    }
}

/**
 * @name SaveSerialHarmony
 * @brief Sauvegarde le numero de serie appaire en memoire non volatile (NVM).
 * @param serial Numero de serie a sauvegarder.
 */
void SaveSerialHarmony(uint32_t serial) {
    DRV_NVM_COMMAND_HANDLE commandHandle; // Handle de commande NVM
    NVM_OpenDriver(); // Ouvre le driver NVM si besoin
    DRV_NVM_Write(nvmHandle, &commandHandle,
            (uint8_t*) &serial, NVM_SERIAL_BLOCK, sizeof(uint32_t)); // Ecrit le numero de serie en NVM
    while (DRV_NVM_CommandStatus(nvmHandle, commandHandle)
            == DRV_NVM_COMMAND_IN_PROGRESS); // Attend la fin de l'ecriture
}

/**
 * @name LoadSerialHarmony
 * @brief Charge le numero de serie appaire depuis la memoire non volatile (NVM).
 * @return Le numero de serie charge, ou 0 si non valide.
 */
uint32_t LoadSerialHarmony(void) {
    DRV_NVM_COMMAND_HANDLE commandHandle; // Handle de commande NVM
    uint32_t val = 0; // Variable de lecture
    NVM_OpenDriver(); // Ouvre le driver NVM si besoin
    DRV_NVM_Read(nvmHandle, &commandHandle,
            (uint8_t*) &val, NVM_SERIAL_BLOCK, sizeof(uint32_t)); // Lit le numero de serie en NVM
    while (DRV_NVM_CommandStatus(nvmHandle, commandHandle)
            == DRV_NVM_COMMAND_IN_PROGRESS); // Attend la fin de la lecture

    if (val == 0xFFFFFFFF || val == 0x00000000) { // Si la valeur lue est invalide
        return 0; // Retourne 0
    }
    return val; // Retourne la valeur lue
}

/**
 * @name SavePairedSerialToFlash
 * @brief Sauvegarde le numero de serie appaire courant en memoire flash.
 */
void SavePairedSerialToFlash(void) {
    SaveSerialHarmony(pairedSerial); // Sauvegarde le numero de serie appaire
}

/**
 * @name LoadPairedSerialAndApply
 * @brief Charge le numero de serie appaire depuis la flash et l'applique a l'application.
 */
void LoadPairedSerialAndApply(void) {
    uint32_t tempSerial = LoadSerialHarmony(); // Charge le numero de serie
    if (tempSerial != 0) { // Si la valeur est valide
        pairedSerial = tempSerial; // Applique le numero de serie appaire
        appPairedSerial = tempSerial; // Applique a l'application
    } else {
        pairedSerial = 0; // Reinitialise
        appPairedSerial = 0; // Reinitialise
    }
}

/**
 * @name GetActifSerialNbr
 * @brief Retourne le numero de serie actuellement actif (appaire).
 * @return Le numero de serie appaire.
 */
uint32_t GetActifSerialNbr(void) {
    return pairedSerial; // Retourne le numero de serie appaire
}

/**
 * @name ResetSerialList
 * @brief Reinitialise la liste des numeros de serie et l'etat d'appairage.
 */
void ResetSerialList(void) {
    pairedSerial = 0; // Reinitialise le numero de serie appaire
    pairingSerialHigh = 0; // Reinitialise la partie haute
    pairingSerialLow = 0; // Reinitialise la partie basse
    bellState = BELL_IDLE; // Reinitialise l'etat Bell
    doorState = DOOR_IDLE; // Reinitialise l'etat Door
    p0Sent = p1Sent = a0Sent = a1Sent = false; // Reinitialise les flags d'envoi
}

/**
 * @name ShowPairingSuccess
 * @brief Indique visuellement et auditivement le succes de l'appairage.
 */
void ShowPairingSuccess(void) {
    Sound_Start(SUCCESS); // Joue le son de succes
    Led_SetMode(LED_ID_ENTER, LED_MODE_ON); // Allume la LED ENTER
    Led_SetMode(LED_ID_WAIT, LED_MODE_OFF); // Eteint la LED WAIT
    Led_SetMode(LED_ID_BUSY, LED_MODE_OFF); // Eteint la LED BUSY
}

// *****************************************************************************
// *****************************************************************************
// Reset par appui long
// *****************************************************************************
// *****************************************************************************

/**
 * @name PairingReset_DoReset
 * @brief Effectue la reinitialisation de l'appairage.
 *
 * Cette fonction reinitialise la liste des numeros de serie appaires,
 * efface la memoire flash correspondante, remet l'etat d'appairage a zero,
 * et effectue une indication visuelle et sonore de la reinitialisation.
 * @return void
 */
void PairingReset_DoReset(void) {
    uint8_t i = 0; // Compteur de boucle
    ResetSerialList(); // Reinitialise la liste des numeros de serie
    SaveSerialHarmony(0); // Efface la memoire flash
    appPairedSerial = 0; // Reinitialise la variable d'application
    Sound_Start(SUCCESS); // Joue le son de succes
    Led_SetMode(LED_ID_ENTER, LED_MODE_BLINK_2X_FAST); // Fait clignoter la LED ENTER
    Led_SetMode(LED_ID_WAIT, LED_MODE_BLINK_2X_FAST); // Fait clignoter la LED WAIT
    Led_SetMode(LED_ID_BUSY, LED_MODE_BLINK_2X_FAST); // Fait clignoter la LED BUSY
    delay_msCt(400); // Attente

    Led_SetMode(LED_ID_ENTER, LED_MODE_OFF); // Eteint la LED ENTER
    Led_SetMode(LED_ID_WAIT, LED_MODE_OFF); // Eteint la LED WAIT
    Led_SetMode(LED_ID_BUSY, LED_MODE_OFF); // Eteint la LED BUSY

    for (i = 0; i < 3; i++) { // Boucle pour clignoter les LEDs
        Led_SetMode(LED_ID_ENTER, LED_MODE_ON); // Allume la LED ENTER
        Led_SetMode(LED_ID_WAIT, LED_MODE_ON); // Allume la LED WAIT
        Led_SetMode(LED_ID_BUSY, LED_MODE_ON); // Allume la LED BUSY
        delay_msCt(100); // Attente

        Led_SetMode(LED_ID_ENTER, LED_MODE_OFF); // Eteint la LED ENTER
        Led_SetMode(LED_ID_WAIT, LED_MODE_OFF); // Eteint la LED WAIT
        Led_SetMode(LED_ID_BUSY, LED_MODE_OFF); // Eteint la LED BUSY
        delay_msCt(100); // Attente
    }
}

/**
 * @name PairingReset_DoorTask
 * @brief Tache de surveillance du bouton d'appairage pour la porte (Door).
 *
 * Declenche une reinitialisation de l'appairage si le bouton swRing est maintenu appuye
 * suffisamment longtemps (appui long).
 * @return void
 */
void PairingReset_DoorTask(void) {
    static uint16_t cnt = 0; // Compteur d'appui long
    if (pairingInProgress || appPairedSerial == 0 || DebounceGetInput(&swRing)) { // Si appairage en cours ou pas appaire ou bouton relache
        cnt = 0; // Reinitialise le compteur
        return; // Sort de la fonction
    }
    if (++cnt >= LONG_PRESS_TICKS) { // Si appui long detecte
        PairingReset_DoReset(); // Effectue le reset
    }
}

/**
 * @name PairingReset_BellTask
 * @brief Tache de surveillance des boutons d'appairage pour la sonnette (Bell).
 *
 * Declenche une reinitialisation de l'appairage si les trois boutons (swEnter, swWait, swBusy)
 * sont maintenus appuyes simultanement suffisamment longtemps (appui long).
 * @return void
 */
void PairingReset_BellTask(void) {
    static uint16_t cnt = 0; // Compteur d'appui long
    if (!pairingInProgress &&
        (appPairedSerial != 0) &&
        !DebounceGetInput(&swEnter) &&
        !DebounceGetInput(&swWait) &&
        !DebounceGetInput(&swBusy)) { // Si les trois boutons sont maintenus
        if (cnt < LONG_PRESS_TICKS) cnt++; // Incremente le compteur
        if (cnt == LONG_PRESS_TICKS) { // Si appui long detecte
            PairingReset_DoReset(); // Effectue le reset
            cnt = 0; // Reinitialise le compteur
        }
    } else {
        cnt = 0; // Reinitialise le compteur
    }
}

// *****************************************************************************
// *****************************************************************************
// Fonctions RF d'envoi de trames
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Envoie une trame RF avec le numero de serie, en utilisant P0 et P1.
 * @param payload Pointeur vers la charge utile a envoyer.
 */
void RF_SendWithSerial(uint8_t* payload) {
    RF_SendP0(); // Envoie la trame P0
    delay_msCt(10); // Attente
    RF_SendP1(); // Envoie la trame P1
    delay_msCt(10); // Attente
    RF_SendMessage(payload, 0); // Envoie le message principal
}

/**
 * @brief Envoie la trame P0 contenant la partie haute du numero de serie.
 */
void RF_SendP0(void) {
    char msg[9]; // Buffer du message
    snprintf(msg, 9, "[P0%04X]", (mySerial >> 16) & 0xFFFF); // Formate la trame P0
    RF_SendMessage((uint8_t*) msg, 8); // Envoie la trame
    p0Sent = true; // Marque P0 comme envoyee
}

/**
 * @brief Envoie la trame P1 contenant la partie basse du numero de serie.
 */
void RF_SendP1(void) {
    char msg[9]; // Buffer du message
    snprintf(msg, 9, "[P1%04X]", mySerial & 0xFFFF); // Formate la trame P1
    RF_SendMessage((uint8_t*) msg, 8); // Envoie la trame
    p1Sent = true; // Marque P1 comme envoyee
}

/**
 * @brief Envoie la trame A0 contenant la partie haute du numero de serie specifie.
 * @param serial Numero de serie a inclure dans la trame.
 */
void RF_SendA0(uint32_t serial) {
    char msg[9]; // Buffer du message
    snprintf(msg, 9, "[A0%04X]", (serial >> 16) & 0xFFFF); // Formate la trame A0
    RF_SendMessage((uint8_t*) msg, 8); // Envoie la trame
    a0Sent = true; // Marque A0 comme envoyee
}

/**
 * @brief Envoie la trame A1 contenant la partie basse du numero de serie specifie.
 * @param serial Numero de serie a inclure dans la trame.
 */
void RF_SendA1(uint32_t serial) {
    char msg[9]; // Buffer du message
    snprintf(msg, 9, "[A1%04X]", serial & 0xFFFF); // Formate la trame A1
    RF_SendMessage((uint8_t*) msg, 8); // Envoie la trame
    a1Sent = true; // Marque A1 comme envoyee
}

/**
 * @brief Verifie si le message RF recu provient du peripherique appaire.
 * @param msg Pointeur vers le message RF recu.
 * @return true si le message provient du peripherique appaire, false sinon.
 */
bool Serial_CheckIfFromPaired(uint8_t *msg) {
    if (msg[0] != '[' || msg[7] != ']') return false; // Verifie le format du message

    uint16_t val = 0; // Valeur extraite
    sscanf((char*) &msg[3], "%04hX", &val); // Extrait la valeur

    if (msg[1] == 'P') { // Si trame P
        if (msg[2] == '0') { // Si P0
            if (val == ((mySerial >> 16) & 0xFFFF)) return false; // Ignore si c'est moi
            incomingSerialHigh = val; // Stocke la partie haute
            waitingForFullSerial = true; // Attend la suite
            return false; // Pas encore complet
        }
        if (msg[2] == '1') { // Si P1
            uint32_t candidate = ((uint32_t) incomingSerialHigh << 16) | val; // Recompose le numero
            if (candidate == mySerial) {
                waitingForFullSerial = false; // Ignore si c'est moi
                return false;
            }
            incomingSerialLow = val; // Stocke la partie basse
            lastReceivedSerial = candidate; // Stocke le numero complet
            waitingForFullSerial = false; // Numero complet recu
            return false;
        }
    }
    if ((msg[1] == 'D' || msg[1] == 'B') && appPairedSerial != 0)
        return true; // Message d'un peripherique appaire
    return false; // Sinon
}

/**
 * @brief Traite un message RF recu lors de la procedure d'appairage.
 * @param msg Pointeur vers le message RF recu.
 */
void HandlePairingMessage(uint8_t* msg) {
    if (msg[0] != '[' || msg[7] != ']') return; // Verifie le format du message

    uint16_t value = 0; // Valeur extraite
    sscanf((char*) &msg[3], "%04hX", &value); // Extrait la valeur

    if (msg[1] == 'P' || msg[1] == 'A') { // Si trame P ou A
        if (msg[2] == '0') pairingSerialHigh = value; // Stocke la partie haute
        if (msg[2] == '1') pairingSerialLow = value; // Stocke la partie basse
    }
}

// *****************************************************************************
// *****************************************************************************
// Gestion complete de l'appairage
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Gere la procedure complete d'appairage cote Door ou Bell.
 *
 * Cette fonction implemente la machine d'etat d'appairage pour les deux roles (Door/Bell).
 * Elle pilote l'envoi/reception des trames, la sauvegarde du numero de serie appaire,
 * l'affichage du succes, et la sortie de la procedure.
 *
 * @return Le numero de serie appaire si l'appairage est termine, 0 sinon.
 */
uint32_t PairingManagement(void) {
    uint8_t msg[8]; // Buffer de message
    mySerial = getSerialRF(); // Recupere mon numero de serie

    if (pairedSerial != 0) return pairedSerial; // Si deja appaire, retourne

    pairingInProgress = true; // Indique que l'appairage est en cours

    if (APP_GetIsDoor()) { // Si cote Door
        switch (doorState) {
            case DOOR_IDLE:
                if (DebounceIsPressed(&swRing)) { // Si bouton appuye
                    DebounceClearPressed(&swRing); // Efface l'etat appuye
                    pairingSerialHigh = pairingSerialLow = 0; // Reinitialise
                    Led_SetMode(LED_ID_ENTER, LED_MODE_ON); // Allume la LED
                    doorState = DOOR_WAIT_P; // Passe a l'etat suivant
                }
                break;
            case DOOR_WAIT_P:
                if (RF_GetMessage(msg)) HandlePairingMessage(msg); // Traite les messages RF
                if (pairingSerialHigh && pairingSerialLow) doorState = DOOR_SEND_A0; // Si recu, passe a l'envoi
                break;
            case DOOR_SEND_A0:
                if (!a0Sent) {
                    RF_SendA0(mySerial); // Envoie la trame A0
                    delay_msCt(10); // Attente
                }
                doorState = DOOR_SEND_A1; // Passe a l'envoi A1
                break;
            case DOOR_SEND_A1:
                if (!a1Sent) {
                    RF_SendA1(mySerial); // Envoie la trame A1
                    pairedSerial = ((uint32_t) pairingSerialHigh << 16) | pairingSerialLow; // Compose le numero appaire
                    SavePairedSerialToFlash(); // Sauvegarde en flash
                    appPairedSerial = pairedSerial; // Applique a l'application
                    ShowPairingSuccess(); // Indique le succes
                }
                doorState = DOOR_DONE; // Passe a l'etat final
                break;
            case DOOR_DONE:
                pairingInProgress = false; // Fin de l'appairage
                APP_UpdateState(APP_STATE_SERVICE_TASKS); // Retour a l'etat normal
                break;
        }
    } else { // Cote Bell
        switch (bellState) {
            case BELL_IDLE:
                if (DebounceIsPressed(&swEnter) &&
                    DebounceIsPressed(&swWait) &&
                    DebounceIsPressed(&swBusy)) { // Si les trois boutons sont appuyes
                    DebounceClearPressed(&swEnter); // Efface l'etat appuye
                    DebounceClearPressed(&swWait); // Efface l'etat appuye
                    DebounceClearPressed(&swBusy); // Efface l'etat appuye
                    pairingSerialHigh = pairingSerialLow = 0; // Reinitialise
                    Led_SetMode(LED_ID_ENTER, LED_MODE_ON); // Allume la LED
                    bellState = BELL_SEND_P0; // Passe a l'envoi P0
                }
                break;
            case BELL_SEND_P0:
                if (!p0Sent) {
                    RF_SendP0(); // Envoie la trame P0
                    delay_msCt(10); // Attente
                }
                bellState = BELL_SEND_P1; // Passe a l'envoi P1
                break;
            case BELL_SEND_P1:
                if (!p1Sent) RF_SendP1(); // Envoie la trame P1
                bellState = BELL_WAIT_ACK; // Passe a l'attente de l'ACK
                break;
            case BELL_WAIT_ACK:
                if (RF_GetMessage(msg)) HandlePairingMessage(msg); // Traite les messages RF
                if (pairingSerialHigh && pairingSerialLow) {
                    pairedSerial = ((uint32_t) pairingSerialHigh << 16) | pairingSerialLow; // Compose le numero appaire
                    SavePairedSerialToFlash(); // Sauvegarde en flash
                    appPairedSerial = pairedSerial; // Applique a l'application
                    ShowPairingSuccess(); // Indique le succes
                    bellState = BELL_DONE; // Passe a l'etat final
                }
                break;
            case BELL_DONE:
                pairingInProgress = false; // Fin de l'appairage
                APP_UpdateState(APP_STATE_SERVICE_TASKS); // Retour a l'etat normal
                break;
        }
    }
    return pairedSerial; // Retourne le numero de serie appaire
}
