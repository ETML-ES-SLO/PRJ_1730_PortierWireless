#ifndef Mc32Gest_RS232_H
#define Mc32Gest_RS232_H
/*--------------------------------------------------------*/
// Mc32Gest_RS232.h
/*--------------------------------------------------------*/
//	Description :	emission et reception spécialisée
//			        pour TP2 2016-2017
//
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.3
//	Compilateur	:	XC32 V1.42 + Harmony 1.08
//
/*--------------------------------------------------------*/

#include <stdint.h>
#include "GesFifoTh32.h"



/*--------------------------------------------------------*/
// Définition des fonctions prototypes
/*--------------------------------------------------------*/

// prototypes des fonctions
void InitFifoComm(void);


// Descripteur des fifos
extern S_fifo descrFifoRX;
extern S_fifo descrFifoTX;

uint8_t Uart_GetMessage(uint8_t* dstArray, uint8_t size);
void Uart_SendMessage(uint8_t* dataToSend, uint8_t nbBytesToSend);
void Uart_CheckForTxInt(void);


#endif
