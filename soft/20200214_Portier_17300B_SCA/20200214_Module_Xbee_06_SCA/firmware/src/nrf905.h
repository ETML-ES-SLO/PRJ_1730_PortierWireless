//--------------------------------------------------------
// nrf905.h
//--------------------------------------------------------
// Gestion chip nrf905 sur modules 868 ES
//	Description :	
//
//	Auteur 		: 	Y. SIMONET
//  Date        :   22.03.2017
//	Version		:	V1.0
//	Compilateur	:	XC32 V1.31
//  Modifications :
//   D. Martins V1.1 reprise, fonctionnement sur projet ticketing
//   SCA 5.11.2019 V1.2 reprise, nettoyage du code
//    pour faire fonctionner avec portier électronique
/*--------------------------------------------------------*/

#ifndef _NRF905_H    /* Guard against multiple inclusion */
#define _NRF905_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
//#include "apprf.h"
#include <stdbool.h>
#include "Mc32gestI2c24AA02.h"
#include "peripheral/ports/plib_ports.h"

/* ************************************************************************** */

//typedef union 
//{
//    uint32_t val32;    
////    struct
////    {
////        uint8_t lsb;
////        uint8_t byte1;
////        uint8_t byte2;
////        uint8_t msb;
////    }  
////    U_32_Bytes;    
//    uint8_t bytes[4];
//} 
//U_32;

/* ************************************************************************** */

#define CS_RF_ON() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_14, false)
#define CS_RF_OFF() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_14, true)

#define PWR_UP_ON() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_5, true)
#define PWR_UP_OFF() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_5, false)

#define TX_EN_ON() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_2, true)
#define TX_EN_OFF() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_2, false)

#define TRX_CE_ON() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_3, true)
#define TRX_CE_OFF() PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_3, false)

// regs
#define WC  0x00  // Write configuration register command
#define RC  0x10  // Read  configuration register command
#define WTP  0x20 // Write TX Payload command
#define RTP  0x21 // Read  TX Payload command
#define WTA  0x22 // Write TX Address command
#define RTA  0x23 // Read  TX Address command
#define RRP  0x24 // Read  RX Payload command

#define AM() PLIB_PORTS_PinGet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_10)

#define DR() PLIB_PORTS_PinGet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_11)

#define CD() PLIB_PORTS_PinGet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_12)

/* ************************************************************************** */

void Nrf905_Init ();
uint32_t Nrf905_GetOwnAddr(void);
void Nrf905_WriteTxBufAndTx (uint8_t* txPckt);
uint8_t Nrf905_ReadRxBuf (uint8_t* rxPckt);

void Nrf905_Activer_Ecoute_RF ();
void Nrf905_Activer_Envoi_RF ();
void Nrf905_Stand_by_RF ();
void Nrf905_Stand_by_RF ();
void Nrf905_MODE_CONFIG ();
void Nrf905_PWR_Down ();

bool Nrf905_GetAm (void);
bool Nrf905_GetCd (void);
bool Nrf905_GetDr (void);

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#end
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

#endif
/* *****************************************************************************
 End of File
 */
