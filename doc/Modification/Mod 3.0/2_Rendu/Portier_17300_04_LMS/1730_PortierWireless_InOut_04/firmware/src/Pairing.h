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
*/
#ifndef PAIRING_H // Protection contre l'inclusion multiple du fichier d'en-tete
#define PAIRING_H

#include <stdint.h> // Inclusion des definitions de types entiers standard
#include <stdbool.h> // Inclusion du type booleen standard
/* === Constantes  === */
#define NVM_SERIAL_BLOCK     0 // Bloc memoire NVM utilise pour le numero de serie
#define LONG_PRESS_TICKS     300 // Nombre de ticks pour un appui long

/* === Declarations externes === */
extern S_SwitchDescriptor swRing;         // Descripteur du bouton Ring
extern S_SwitchDescriptor swEnter;        // Descripteur du bouton Enter
extern S_SwitchDescriptor swWait;         // Descripteur du bouton Wait
extern S_SwitchDescriptor swBusy;         // Descripteur du bouton Busy
extern uint32_t appPairedSerial;          // Numero de serie appaire courant

/* === Fonctions publiques === */

/* Acces au numero de serie appaire */
void     LoadPairedSerialAndApply(void);   // Charge le numero appaire depuis la Flash et l'applique en RAM
void     SavePairedSerialToFlash(void);    // Sauvegarde le numero appaire courant en memoire non-volatile
uint32_t GetActifSerialNbr(void);          // Retourne le numero de serie appaire actif
void     ResetSerialList(void);            // Reinitialise la liste des numeros appaires

/* Automate d'appairage complet */
uint32_t PairingManagement(void);          // Gere l'automate d'appairage, retourne le numero appaire (0 si non appaire)

/* Reset d'appairage (appui long) */
void PairingReset_DoorTask(void);          // Gestion du reset d'appairage cote Door
void PairingReset_BellTask(void);          // Gestion du reset d'appairage cote Bell

/* Statut global du pairing */
bool Serial_IsPairingRunning(void);        // Retourne true si l'appairage est en cours

/* Filtrage & envoi de trames RF */
bool Serial_CheckIfFromPaired(uint8_t *msg); // Verifie si la trame recue provient du numero appaire
void RF_SendWithSerial(uint8_t *payload);    // Envoie une trame RF avec le numero de serie appaire

/* Visuel / sonore */
void ShowPairingSuccess(void);             // Indique le succes de l'appairage (bip + LED verte)

/* Tests Flash (optionnel) */
void TestFlashWriteRead(void);             // Teste l'ecriture/lecture du numero appaire en Flash

/* === Fonctions internes (statics, a usage prive) === */
void SaveSerialHarmony(uint32_t serial); // Sauvegarde un numero de serie en Flash (interne)
uint32_t LoadSerialHarmony(void); // Charge un numero de serie depuis la Flash (interne)
void PairingReset_DoReset(void); // Effectue le reset d'appairage (interne)
void RF_SendP0(void); // Envoie une trame P0 (interne)
void RF_SendP1(void); // Envoie une trame P1 (interne)
void RF_SendA0(uint32_t serial); // Envoie une trame A0 avec numero de serie (interne)
void RF_SendA1(uint32_t serial); // Envoie une trame A1 avec numero de serie (interne)
void HandlePairingMessage(uint8_t* msg); // Traite un message d'appairage (interne)

#endif /* PAIRING_H */ // Fin de la protection contre l'inclusion multiple
