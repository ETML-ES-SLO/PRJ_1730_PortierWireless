/*--------------------------------------------------------*/
//      Mc32_I2cUtilCCS.c
/*--------------------------------------------------------*/
//	Description :	utilitaire I2C compatible syntaxe CCS
//                      Obtenu par modification de l'exemple harmony
//
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.2
//	Compilateur	:	XC32 V1.40 & Harmony V1.06
// Modifications :
//      CHR 19.03.2015  Migration sur plib_i2c de Harmony 1.00   CHR
//      CHR 12.04.2016  adaptaion détails pour plib_i2c de Harmony 1.06   CHR
//
//
/*--------------------------------------------------------*/

#include "apprf.h"
#include "Mc32_I2cUtilCCS.h"
#include "peripheral\i2c\plib_i2c.h"
#include "peripheral\osc\plib_osc.h"



// KIT 32MX795F512L Constants
#define I2C_24AA02    I2C_ID_1
//#define KIT_I2C_BUS   I2C_ID_
#define I2C_CLOCK_FAST 400000
#define I2C_CLOCK_SLOW 100000

//------------------------------------------------------------------------------
// i2c_init
//
// Initialisationde l'I2c
//      si bool Fast = false  LOW speed   100 KHz
//      si bool Fast = true   HIGH speed  400 KHz
//------------------------------------------------------------------------------
// Adaptation plib_i2c  : 19.03.2015 CHR
// CHR 12.04.2016 reprise principe init du driver I2C de Harmony 1.06
// = pas OK reprise ancien principe            

//uint32_t I2cConReg, I2cBrg;     // pour controle de la configuration

void i2c_init( bool Fast )
{
    PLIB_I2C_Disable(I2C_24AA02);      // Ajout CHR
      
    PLIB_I2C_HighFrequencyEnable(I2C_24AA02);   // OK comme cela
    if (Fast)  {
       PLIB_I2C_BaudRateSet(I2C_24AA02,
               SYS_CLK_PeripheralFrequencyGet(CLK_BUS_PERIPHERAL_1), I2C_CLOCK_FAST);
    } else {
        PLIB_I2C_BaudRateSet(I2C_24AA02,
               SYS_CLK_PeripheralFrequencyGet(CLK_BUS_PERIPHERAL_1), I2C_CLOCK_SLOW);
    }
    // PLIB_I2C_HighFrequencyDisable(I2C_24AA02);   // celon driver
    
    PLIB_I2C_SlaveClockStretchingEnable(I2C_24AA02);  // ajout CHR
    
    PLIB_I2C_Enable(I2C_24AA02);

//    I2cConReg = I2C2CON;
//    I2cBrg = I2C2BRG;
}


//------------------------------------------------------------------------------
// i2c_start()
//
// Début la transaction I2C master
//
// Adaptation plib_i2c  : 19.03.2015 CHR

void i2c_start(void)
{
    // int DebugCode = 0;

    // Wait for the bus to be idle, then start the transfer
    while(PLIB_I2C_BusIsIdle(I2C_24AA02) == false);

     /* Check for recieve overflow */
    if ( PLIB_I2C_ReceiverOverflowHasOccurred(I2C_24AA02))
    {
        PLIB_I2C_ReceiverOverflowClear(I2C_24AA02);
    }

    /* Check for transmit overflow */
    if (PLIB_I2C_TransmitterOverflowHasOccurred(I2C_24AA02))
    {
        PLIB_I2C_TransmitterOverflowClear(I2C_24AA02);
    }

    PLIB_I2C_MasterStart(I2C_24AA02);

    if (PLIB_I2C_ArbitrationLossHasOccurred(I2C_24AA02));
    {
        // DBPRINTF("Error: Bus collision during transfer Start\n");
        // DebugCode = 1;
        // Handel bus collision
        PLIB_I2C_ArbitrationLossClear(I2C_24AA02);
    }
   
    // Wait for the signal to complete
    while (PLIB_I2C_StartWasDetected(I2C_24AA02) == false);
   
 } // end i2c_start

void i2c_reStart(void)
{
   // int DebugCode = 0;
  
   // Pas d'attente bus en Idle

   /* Check for recieve overflow */
   if ( PLIB_I2C_ReceiverOverflowHasOccurred(I2C_24AA02))
   {
       PLIB_I2C_ReceiverOverflowClear(I2C_24AA02);
   }

   /* Check for transmit overflow */
   if (PLIB_I2C_TransmitterOverflowHasOccurred(I2C_24AA02))
   {
        PLIB_I2C_TransmitterOverflowClear(I2C_24AA02);
   }

   // PLIB_I2C_StartClear(I2C_24AA02);
   PLIB_I2C_MasterStartRepeat(I2C_24AA02);
   
   if (PLIB_I2C_ArbitrationLossHasOccurred(I2C_24AA02));
    {
        // DBPRINTF("Error: Bus collision during transfer Start\n");
        // DebugCode = 1;
        // Handel bus collision
        PLIB_I2C_ArbitrationLossClear(I2C_24AA02);
    }
    
   // Wait for the signal to complete
   while (PLIB_I2C_StartWasDetected(I2C_24AA02) == false);
  
} // end i2c_reStart


//------------------------------------------------------------------------------
// i2c_write()
//
// Syntaxe : 	i2c_write (data);
// Le paramètre data est la valeur 8 bits à transmettre.
// En mode maître, la fonction génère le signal d'horloge, en mode esclave elle attend le signal d'horloge du maître.
// En outre cette fonction retourne le bit ACK, 0 = ACK, 1 = NO_ACK.
//
//------------------------------------------------------------------------------
//
// Modification de  BOOL TransmitOneByte( UINT8 data )
// - Ajout retour du bit Ack
// Adaptation plib_i2c  : 19.03.2015 CHR

bool i2c_write( uint8_t data )
{
    bool  AckBit;
  
    // Wait for the bus to be idle (nécessaire aprés un reStart)
    while(PLIB_I2C_BusIsIdle(I2C_24AA02) == false);

    // Wait for the transmitter to be ready
    while( PLIB_I2C_TransmitterIsReady(I2C_24AA02) == false);

    
    // Transmit the byte
    PLIB_I2C_TransmitterByteSend(I2C_24AA02, data);
    
    while(PLIB_I2C_TransmitterIsBusy(I2C_24AA02));              //Wait as long as TBF = 1
    while(!PLIB_I2C_TransmitterByteHasCompleted(I2C_24AA02));   //Wait as long as TRSTAT == 1
  
    AckBit = PLIB_I2C_TransmitterByteWasAcknowledged(I2C_24AA02);
   
    return AckBit;
} // end i2c_write



//------------------------------------------------------------------------------
// i2c_stop()
//
// termine la transaction I2C master
//
//------------------------------------------------------------------------------
//
// Modification de  void StopTransfer( void )
// - pas de modif sauf le nom de la fonction
// Adaptation plib_i2c  : 19.03.2015 CHR


void i2c_stop( void )
{
    // Attente bus au repos
    while(PLIB_I2C_BusIsIdle(I2C_24AA02) == false);

    PLIB_I2C_MasterStop(I2C_24AA02);

    // Wait for the signal to complete
    while (PLIB_I2C_StopWasDetected(I2C_24AA02) == false);
   
} // end i2c_stop

//------------------------------------------------------------------------------
// i2c_read()
//
// Syntaxe : 	data = i2c_read (ack);
// La fonction retourne l'octet lu.
// Le paramètre ack :
// 1 (true)  signifie qu'il faut effectuer l'acquittement.
// 0 (false) signifie qu'il ne faut pas effectuer l'acquittement.
//
// Adaptation plib_i2c  : 19.03.2015 CHR

uint8_t i2c_read(bool ackTodo)
{
    uint8_t i2cByte;

    // BSP_LEDOn(BSP_LED_5);  // provisoire : pour observation
   
    // ajout idem driver statique I2C de Harmony 1_03
    if ( PLIB_I2C_ReceiverOverflowHasOccurred(I2C_24AA02))
    {
        i2cByte = PLIB_I2C_ReceivedByteGet(I2C_24AA02);
        PLIB_I2C_ReceiverOverflowClear(I2C_24AA02);
    }
    
    // en relation avec stetching
    PLIB_I2C_SlaveClockRelease(I2C_24AA02);

    // Set Rx enable in MSTR which causes SLAVE to send data
    PLIB_I2C_MasterReceiverClock1Byte(I2C_24AA02);

    // Wait till RBF = 1; Which means data is available in I2C2RCV reg
    while(!PLIB_I2C_ReceivedByteIsAvailable(I2C_24AA02));
    
    i2cByte = PLIB_I2C_ReceivedByteGet(I2C_24AA02); //Read from I2CxRCV

    while ( PLIB_I2C_MasterReceiverReadyToAcknowledge ( I2C_24AA02 ) == false );

     if (ackTodo) {
          PLIB_I2C_ReceivedByteAcknowledge ( I2C_24AA02, true );
     } else {
          PLIB_I2C_ReceivedByteAcknowledge ( I2C_24AA02, false );
     }

    // wait till ACK/NACK sequence is complete i.e ACKEN = 0
    while( PLIB_I2C_MasterReceiverReadyToAcknowledge (I2C_24AA02) == false);
   
    // BSP_LEDOff(BSP_LED_5); // provisoire : pour observation

    return i2cByte;
} // end i2c_read


