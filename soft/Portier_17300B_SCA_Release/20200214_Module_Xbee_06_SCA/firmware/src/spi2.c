/* ************************************************************************** */
/** Descriptive File Name
 
    SCA, 21.10.2019
 
  @Company
    ETML-ES

  @File Name
    spi2.c

  @Summary
    Gestion port SPI 2

  @Description
    Gestion port SPI 2
 */
/* ************************************************************************** */

#include "peripheral/spi/plib_spi.h"
#include "system_definitions.h"

/* ************************************************************************** */
/* ************************************************************************** */

// SYS_MODULE_OBJ DRV_SPI0_Initialize(void)
void Spi2_Init(void)
{
//    DRV_SPI_OBJ *dObj = (DRV_SPI_OBJ*)NULL;
//
//    dObj = &gDrvSPI0Obj;

    /* Disable the SPI module to configure it*/
    PLIB_SPI_Disable ( SPI_ID_2 );

    /* Set up Master or Slave Mode*/
    PLIB_SPI_MasterEnable ( SPI_ID_2 );
    PLIB_SPI_PinDisable(SPI_ID_2, SPI_PIN_SLAVE_SELECT);

    /* Set up if the SPI is allowed to run while the rest of the CPU is in idle mode*/
    PLIB_SPI_StopInIdleEnable( SPI_ID_2 );

    /* Set up clock Polarity and output data phase*/
    PLIB_SPI_ClockPolaritySelect( SPI_ID_2, SPI_CLOCK_POLARITY_IDLE_LOW );
    PLIB_SPI_OutputDataPhaseSelect( SPI_ID_2, SPI_OUTPUT_DATA_PHASE_ON_ACTIVE_TO_IDLE_CLOCK );

    /* Set up the Input Sample Phase*/
    PLIB_SPI_InputSamplePhaseSelect ( SPI_ID_2, SPI_INPUT_SAMPLING_PHASE_AT_END);

    /* Communication Width Selection */
    PLIB_SPI_CommunicationWidthSelect ( SPI_ID_2, SPI_COMMUNICATION_WIDTH_8BITS );

    /* Baud rate selection */
    PLIB_SPI_BaudRateSet( SPI_ID_2 , SYS_CLK_PeripheralFrequencyGet(CLK_BUS_PERIPHERAL_2), 1000000 );

    /* Protocol selection */
    PLIB_SPI_FramedCommunicationDisable( SPI_ID_2  );
    #if defined (PLIB_SPI_ExistsAudioProtocolControl)
            if (PLIB_SPI_ExistsAudioProtocolControl(SPI_ID_2))
            {
                PLIB_SPI_AudioProtocolDisable(SPI_ID_2);
            }
    #endif

    /* Buffer type selection */
    #if defined (PLIB_SPI_ExistsFIFOControl)
        if (PLIB_SPI_ExistsFIFOControl( SPI_ID_2 ))
        {
            PLIB_SPI_FIFOEnable( SPI_ID_2 );
            PLIB_SPI_FIFOInterruptModeSelect(SPI_ID_2, SPI_FIFO_INTERRUPT_WHEN_TRANSMIT_BUFFER_IS_COMPLETELY_EMPTY);
            PLIB_SPI_FIFOInterruptModeSelect(SPI_ID_2, SPI_FIFO_INTERRUPT_WHEN_RECEIVE_BUFFER_IS_NOT_EMPTY);
        }
    #else
        {
            SYS_ASSERT(false, "\r\nInvalid SPI Configuration.");
            return SYS_MODULE_OBJ_INVALID;
        }
    #endif

    PLIB_SPI_BufferClear( SPI_ID_2 );
    PLIB_SPI_ReceiverOverflowClear ( SPI_ID_2 );

//    /* Initialize Queue only once for all instances of SPI driver*/
//    if (DRV_SPI_SYS_QUEUE_Initialize(&qmInitData, &hQueueManager) != DRV_SPI_SYS_QUEUE_SUCCESS)
//    {
//        SYS_ASSERT(false, "\r\nSPI Driver: Could not create queuing system.");
//        return SYS_MODULE_OBJ_INVALID;
//    }
//
//    /* Update the Queue parameters. */
//    qInitData.maxElements = 10; //Queue size
//    qInitData.reserveElements = 1; //Mininmum number of job queues reserved
//
//    /* Create Queue for this instance of SPI */
//    if (DRV_SPI_SYS_QUEUE_CreateQueue(hQueueManager, &qInitData, &dObj->queue) != DRV_SPI_SYS_QUEUE_SUCCESS)
//    {
//        SYS_ASSERT(false, "\r\nSPI Driver: Could not set up driver instance queue.");
//        return SYS_MODULE_OBJ_INVALID;
//
//    }
//
//    /* Update the SPI OBJECT parameters. */
//    dObj->operationStarting = NULL;
//    dObj->operationEnded = NULL;

    SYS_INT_SourceDisable(INT_SOURCE_SPI_2_TRANSMIT);
    SYS_INT_SourceDisable(INT_SOURCE_SPI_2_RECEIVE);
    //SYS_INT_SourceEnable(INT_SOURCE_SPI_2_ERROR);
    SYS_INT_SourceDisable(INT_SOURCE_SPI_2_ERROR);

    /* Clear all interrupt sources */
    SYS_INT_SourceStatusClear(INT_SOURCE_SPI_2_TRANSMIT);
    SYS_INT_SourceStatusClear(INT_SOURCE_SPI_2_RECEIVE);
    SYS_INT_SourceStatusClear(INT_SOURCE_SPI_2_ERROR);

    /* Enable the Module */
    PLIB_SPI_Enable(SPI_ID_2);

//    return (SYS_MODULE_OBJ)DRV_SPI_INDEX_0 ;
}

/* ************************************************************************** */
/* ************************************************************************** */

uint8_t Spi2_ReadWrite(uint8_t txData)
{
    PLIB_SPI_BufferWrite(SPI_ID_2,txData);
    do
    {      
    }while (PLIB_SPI_IsBusy(SPI_ID_2));
    return(PLIB_SPI_BufferRead(SPI_ID_2));
}  


/* *****************************************************************************
 End of File
 */
