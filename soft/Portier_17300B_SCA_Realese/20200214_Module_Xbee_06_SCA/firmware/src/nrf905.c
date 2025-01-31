//--------------------------------------------------------
// nrf905.c
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

#include "nrf905.h"
#include "spi2.h"
#include "Mc32gestI2c24AA02.h"
#include "Mc32_I2cUtilCCS.h"
#include "Mc32Delays.h"

#define DUMMY_BYTE 0x00

U_32 ownAddress = {.val32 = 0}; //unique ID 32 bits. Lu dans EEPROM série

//adresse de broadcast pour s'adresser à tous les modules (valeur arbitraire)
//par défaut 0xE7E7E7E7
const U_32 BCAST_ADDRESS = {.val32 = 0x12345678};   


/* ************************************************************************** */
// Prototypes

void Nrf905_Activer_Ecoute_RF (void);
void Nrf905_Activer_Envoi_RF (void);
void Nrf905_Stand_by_RF (void);
void Nrf905_MODE_CONFIG (void);
void Nrf905_Lecture_Rx (void);
void Nrf905_Ecriture_TX (void);
void Nrf905_PWR_Down(void);


/* ************************************************************************** */
void  Nrf905_Init (void)
{
    uint8_t tmp [10] = {0};
    uint32_t i; 
    //unsigned char Tab_config [10] = {0xEA,0x0E,0x44,0x08,0x08,0x00,0x00,0x00,0x00,0x5E}; //réglages d'origine Martins (Attention fréq hors bande 868?)
    //unsigned char Tab_config [10] = {0x75,0x0E,0x44,0x08,0x08,0x00,0x00,0x00,0x00,0x5E}; //réglages d'origine Martins modif 868.2 MHz
    unsigned char Tab_config [10] = {0x75,0x0E,0x44,/*0x08,0x08*/32,32,0x00,0x00,0x00,0x00,0xDE}; //SCA
    // CH_NO = 0x75 (117d) (défaut 108) => center frequency 868.2 MHz (défaut 866.4 MHz)
    // HFREQ_PLL = 1    => 868 or 915 MHz band
    // PA_PWR = 11      => +10 dBm output power
    // RX_RED_PWR = 0   => normal operation (pas reduced power)
    // AUTO_RETRAN = 0  => No retransmission
    // RX_AFW = 4       => 4 bytes RX address field width
    // TX_AFW = 4       => 4 bytes TX address field width
    // RX_PW = 32        => RX payload width 32 bytes
    // TX_PW = 32        => TX payload width 32 bytes
    // RX_ADDRESS       => RX address identity voir ci-dessous
    // UP_CLK_FREQ = 10 =>  1 MHz
    // UP_CLK_EN =      => ext. clk enabled
    // XOF = 011        => quartz 16 MHz
    // CRC_EN = 1       => CRC check enable
    // CRC_MODE = 1     => CRC mode 16 bits
    
    //Initialisation du module I2C
    I2C_Init24AA02();
    
    //Récupération de l'addresse stockée dans l'EEPROM
    //UID_32.val32 = I2C_Read_UID_24AA02_P();   //lecture EEPROM user
    ownAddress.val32 = I2C_Read_UID_24AA02_F();     //lecture SN stocké par fabricant
    
    //mettre une adresse autre que celle par défaut. ici 0x12345678 / défaut 0xE7E7E7E7
    //Décomposition en Bytes RX address identity
    Tab_config [5] = BCAST_ADDRESS.bytes[0];//0xE7;
    Tab_config [6] = BCAST_ADDRESS.bytes[1];//0xE7;
    Tab_config [7] = BCAST_ADDRESS.bytes[2];//0xE7;
    Tab_config [8] = BCAST_ADDRESS.bytes[3];//0xE7;
    
    Spi2_Init();
    
    //nRF905 en mode configuration
    Nrf905_MODE_CONFIG();
    
    //Temps d'attente de minimum 3 ms pour passage PWR_DWN -> STBY
    delay_msCt(3);
    
    //*** écriture des registres de configuration ***
    for(i = 0; i < 10; i ++)
    {          
        CS_RF_ON(); //chip select nRF905
        Spi2_ReadWrite(WC+i);   //Selection Registre de configuration (write config command)
        Spi2_ReadWrite(Tab_config[i]);  //le byte de config   
        CS_RF_OFF();    //chip select nRF905
               
        delay_usCt(1);  // CSN inactive time min 500 ns
    }
     
    //Debug, lecture des registres de configuration
    CS_RF_ON();
    Spi2_ReadWrite(RC);   
    for( i = 0; i < 10; i ++)
    {
        tmp[i] = Spi2_ReadWrite(DUMMY_BYTE);
    }    
    CS_RF_OFF(); 
    
    //*** écriture adresse TX ***
    delay_usCt(1);  // CSN inactive time min 500 ns   
    CS_RF_ON();      
    Spi2_ReadWrite(WTA); //selection du registre adresse TX (Write TX Address command)
    //écriture de l'adresse destination
    Spi2_ReadWrite(BCAST_ADDRESS.bytes[0]); //0xE7);
    Spi2_ReadWrite(BCAST_ADDRESS.bytes[1]); //0xE7);
    Spi2_ReadWrite(BCAST_ADDRESS.bytes[2]); //0xE7);
    Spi2_ReadWrite(BCAST_ADDRESS.bytes[3]); //0xE7);   
    CS_RF_OFF();
    
    //Debug, lecture des registres de configuration
    delay_usCt(1);  // CSN inactive time min 500 ns
    CS_RF_ON();
    Spi2_ReadWrite(RTA);    
    for( i = 0; i < 4; i ++)
    {
        tmp[i] = Spi2_ReadWrite(DUMMY_BYTE);
    }    
    CS_RF_OFF();
        
    Nrf905_Activer_Ecoute_RF();   
      
    delay_usCt(750);    //min 650 us pour passage STBY -> RX
}

/* ************************************************************************** */
//lecture UID32
uint32_t Nrf905_GetOwnAddr(void)
{
    return(ownAddress.val32);
}

/* ************************************************************************** */
//écriture buffer TX RF et émission
void Nrf905_WriteTxBufAndTx (uint8_t* txPckt)
{
    uint8_t i;
    
    Nrf905_Ecriture_TX ();
    delay_usCt(750);    //min 650 us pour passage STBY -> TX 
    
    // *** écriture payload *** 
    CS_RF_ON();   
    Spi2_ReadWrite(WTP); //selection du registre adresse TX (Write TX Payload command)
    for(i=0 ; i<32 ; i++)
    {
        Spi2_ReadWrite(txPckt[i]);    
    }    
    CS_RF_OFF();
    
    delay_usCt(750);    //min 650 us pour passage STBY -> TX
    
    Nrf905_Activer_Envoi_RF ();  
    delay_usCt(11);    // pulse TRX_CE min 10 us   
    while(!Nrf905_GetDr()); //DR is set when transmission is completed
   
    Nrf905_Activer_Ecoute_RF ();
    delay_usCt(750);    //min 550 us pour passage TX -> RX  
}

/* ************************************************************************** */
//Lecture buffer réception RF
//Renvoie le nb de bytes lus
uint8_t Nrf905_ReadRxBuf (uint8_t* rxPckt)
{
    uint8_t i;
      
    CS_RF_ON();   
    Spi2_ReadWrite(RRP); //selection du registre RX Payload (Read RX Payload command)    
    i = 0;
    while (Nrf905_GetDr()==1 && i<32)
    { 
        rxPckt[i] = Spi2_ReadWrite(DUMMY_BYTE);                
        i ++;       
    }     
    CS_RF_OFF();
                
    return i;    
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_Activer_Ecoute_RF (void)
{
    TRX_CE_ON();
    TX_EN_OFF();        
    PWR_UP_ON();
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_Activer_Envoi_RF (void)
{
    TRX_CE_ON();
    TX_EN_ON();    
    PWR_UP_ON();
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_Stand_by_RF (void)
{
    TRX_CE_OFF();
    TX_EN_OFF();       
    PWR_UP_ON();
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_MODE_CONFIG (void)
{
    TRX_CE_OFF();
    TX_EN_OFF();       
    PWR_UP_ON();   
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_Lecture_Rx (void)
{
    TRX_CE_ON();
    TX_EN_OFF();       
    PWR_UP_ON();             
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_Ecriture_TX (void)
{
    TRX_CE_OFF();
    TX_EN_ON();       //modif SCA ne pas activer transmission lorsque écriture des données
    //TX_EN_OFF();
    PWR_UP_ON();             
}

/* ************************************************************************** */
/* ************************************************************************** */
void Nrf905_PWR_Down(void)
{
    TRX_CE_OFF();
    TX_EN_OFF();       
    PWR_UP_OFF();             
}

/* ************************************************************************** */
/* ************************************************************************** */
// Address Match
bool Nrf905_GetAm (void)
{
    bool val;
    val = AM();
    return val;
}

/* ************************************************************************** */
/* ************************************************************************** */
// Carrier Detect
bool Nrf905_GetCd (void)
{
    bool val;
    val = CD();
    return val;
}

/* ************************************************************************** */
/* ************************************************************************** */
// Data Ready
bool Nrf905_GetDr (void)
{
    bool val;
    val = DR();
    return val;
}