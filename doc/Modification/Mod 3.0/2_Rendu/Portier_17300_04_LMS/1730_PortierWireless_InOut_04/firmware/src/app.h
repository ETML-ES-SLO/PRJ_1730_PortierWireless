/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
//DOM-IGNORE-END

#ifndef _APP_H
#define _APP_H

// *****************************************************************************
// Section : Included Files
// *****************************************************************************

#include <stdint.h>             // Standard integer types
#include <stdbool.h>            // Boolean type
#include <stddef.h>             // Size definitions
#include <stdlib.h>             // General utility functions
#include "system_config.h"      // System configuration
#include "system_definitions.h" // System definitions
#include "Mc32Debounce.h"       // Debounce handling for buttons

// DOM-IGNORE-BEGIN
#ifdef __cplusplus              // For C++ compatibility
extern "C" {
#endif
// DOM-IGNORE-END

// *****************************************************************************
// Section : Type Definitions
// *****************************************************************************

// *****************************************************************************
// Application state enumeration
// *****************************************************************************
typedef enum
{
  APP_STATE_INIT = 0,         // Initial state of the application
  APP_STATE_WAIT,             // Waiting for an event
  APP_STATE_SERVICE_TASKS     // Executing service tasks
} APP_STATES;

// *****************************************************************************
// Application data structure
// *****************************************************************************
#define LOWBAT_THRESHOLD 2400   // Low battery threshold (2.4V)

typedef struct
{
  APP_STATES state;           // Current state of the application
  bool isDoor;                // Indicates if the door is detected
  uint16_t batVoltage;        // Battery voltage
  bool isLowBat;              // Indicates if the battery is low
} APP_DATA;

// *****************************************************************************
// External button descriptor declarations
// *****************************************************************************
extern S_SwitchDescriptor swRing;   // Doorbell button descriptor
extern S_SwitchDescriptor swEnter;  // Entry button descriptor
extern S_SwitchDescriptor swWait;   // Waiting button descriptor
extern S_SwitchDescriptor swBusy;   // Busy button descriptor

// *****************************************************************************
// Section : Application Function Prototypes
// *****************************************************************************

/*******************************************************************************
  Function : void APP_Initialize(void)
  Initializes the Harmony application, sets to initial state.
*******************************************************************************/
void APP_Initialize(void);      // Initialize the application

/*******************************************************************************
  Function : void APP_Tasks(void)
  Main state machine of the Harmony application.
*******************************************************************************/
void APP_Tasks(void);           // Execute application tasks

/*******************************************************************************
  Function : void APP_UpdateState(APP_STATES newState)
  Updates the application state.
*******************************************************************************/
void APP_UpdateState(APP_STATES newState); // Update the state

/*******************************************************************************
  Function : bool APP_GetIsDoor(void)
  Returns the door detection state.
*******************************************************************************/
bool APP_GetIsDoor(void);       // Returns if the door is detected

// *****************************************************************************
// End of extern "C" and C++ compatibility
// *****************************************************************************
#ifdef __cplusplus
}
#endif
#endif
/*******************************************************************************
  End of File
*******************************************************************************/
