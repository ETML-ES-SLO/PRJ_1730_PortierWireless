//--------------------------------------------------------
// Mc32gestI2c24AA02.h
//--------------------------------------------------------
// Gestion I2C du Mc32gestI2c24AA02
//	Description :	Fonctions pour 24AA02
//
//	Auteur 		: 	Y. SIMONET
//  Date        :       22.03.2017
//	Version		:	V1.0
//	Compilateur	:	XC32 V1.31
//  Modifications :
//   SCA 5.11.2019  V1.1 reprise, nettoyage du code
//    pour faire fonctionner modules RF 868 ES
/*--------------------------------------------------------*/

#ifndef Mc32GestI2C24AA02_H
#define Mc32GestI2C24AA02_H


#include <stdint.h>


typedef union 
{
    uint32_t val32;    
//    struct
//    {
//        uint8_t lsb;
//        uint8_t byte1;
//        uint8_t byte2;
//        uint8_t msb;
//    }  
//    U_32_Bytes;    
    uint8_t bytes[4];
} 
U_32;


// prototypes des fonctions
void I2C_Init24AA02(void);
uint32_t I2C_Read_UID_24AA02_F(void);
//int32_t I2C_Read_UID_24AA02_P(void);
//void I2C_Write_UID_24AA02_P(int32_t PnewUID);
//void I2C_Write_EEPROM_24AA02(int8_t add,int8_t data);

#endif //Mc32GestI2C24AA02_H
