//--------------------------------------------------------
// Mc32gestI2c24AA02.c
//--------------------------------------------------------
// Gestion I2C du Mc32gestI2c24AA02
//	Description :	Fonctions pour 24AA02
//
//	Auteur 		: 	Y. SIMONET
//  Date        :       22.03.2017
//	Version		:	V1.0
//	Compilateur	:	XC32 V1.31
//  Modifications :
//   SCA 5.11.2019 V1.1 reprise, nettoyage du code
//    pour faire fonctionner modules RF 868 ES
/*--------------------------------------------------------*/


//#include "bsp_config.h"
#include "apprf.h"
#include "Mc32gestI2c24AA02.h"
#include "Mc32_I2cUtilCCS.h"

// Définition pour LM92
#define M24AA02_rd    0xA1         // 24AA02 address for read
#define M24AA02_wr    0xA0         // 24AA02 address for write
#define M24AA02_UID_F 0xFC        // adr. UID 32 bit fabricant
//#define M24AA02_UID_P 0x00        // adr. UID 32 bit personnel


// ------------------------------------------------
// Initialisation de la communication I2C et du 24AA02
void I2C_Init24AA02(void)  
{
   bool Fast = true;
   i2c_init(Fast); 
 }

// ------------------------------------------------
// Lecture du registre de UID usine
// Effectue uniquement la lecture
uint32_t I2C_Read_UID_24AA02_F(void)
{
    U_32 uId_32;
    
    i2c_start();    
    i2c_write(M24AA02_wr);	// adresse + écriture
    i2c_write(M24AA02_UID_F);	// sélection registre.
    i2c_reStart();    
    i2c_write(M24AA02_rd);	// adresse + lecture
    uId_32.bytes[3] = i2c_read(1); 	// ack
    uId_32.bytes[2] = i2c_read(1);	
    uId_32.bytes[1] = i2c_read(1);
    uId_32.bytes[0]  = i2c_read(0);// no ack
    i2c_stop();
   
    return uId_32.val32;
} 
 
// ------------------------------------------------
// Lecture du registre de UID ajouter par utilisateur
//int32_t I2C_Read_UID_24AA02_P(void)
//{
//      
//    i2c_start();
//    
//    i2c_write(M24AA02_wr);	// adresse + écriture
//    i2c_write(M24AA02_UID_P);	// sélection registre.
//
//    i2c_reStart();
//    
//    i2c_write(M24AA02_rd);	// adresse + lecture
//    UID_32.shl.msb   = i2c_read(1); 	// ack
//    UID_32.shl.byte2 = i2c_read(1);	
//    UID_32.shl.byte1 = i2c_read(1);
//    UID_32.shl.lsb   = i2c_read(0);// no ack
//    i2c_stop();
//   
//    return UID_32.val32;
//
//} 

// ------------------------------------------------
//modifier UID
//void I2C_Write_UID_24AA02_P(int32_t PnewUID)
//{
//    UID_32.val32 = PnewUID;
//
//    i2c_start();
//    
//    i2c_write(M24AA02_wr);	// adresse + écriture
//    i2c_write(M24AA02_UID_P);	// sélection registre.
//
//    i2c_write(UID_32.shl.msb); 	// ack
//    
//    i2c_write(UID_32.shl.byte2);	
//   
//    i2c_write(UID_32.shl.byte1);
//   
//    i2c_write(UID_32.shl.lsb);//  ack
//    
//    i2c_stop();
//
//} 

// ------------------------------------------------
//écrire des data dans l' EEPROM 
//void I2C_Write_EEPROM_24AA02(int8_t add,int8_t data)
//{
//    
//    i2c_start();
//    
//    i2c_write(M24AA02_wr);	// adresse + écriture
//    i2c_write(add);	// sélection registre
//    i2c_write(data); 	// ack
//
//    i2c_stop();
//
//} 
 



