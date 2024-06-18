//--------------------------------------------------------
// BellFct.c
//--------------------------------------------------------
// Gestion communication	
//
//	Auteur 		: 	DFA
//  Date        :   5.12.2024
//	Version		:	V1.0
//	Compilateur	:	XC32 V2.15
//  Modifications :
//   -
/*--------------------------------------------------------*/

#include "app.h"
#include "SerialGestion.h"
#include "Mc32NVMUtil.h"
#include "RF.h"

uint32_t doorSerialList[NBR_MODULE_MAX];
uint8_t doorSerialIndex;

void GetSerialList()
{
    //lit les num�ro de s�rie enregistrer
    NVM_ReadBlock(doorSerialList,NBR_MODULE_MAX);
    //hardcode pour d�bug et enregistrer l'autre module
    doorSerialList[0] = 0x02303216;
}

void SaveSerialInFlash()
{
    
}

void RemoveSerial()
{
    
}

void ResetSerialList()
{
    
}

uint32_t GetActifSerialNbr()
{
    return doorSerialList[doorSerialIndex];
}

uint8_t Checkcorrespondance(uint8_t *msg)
{
    doorSerialIndex = 0;
    uint32_t getSerial = 0;
    uint8_t getSerialCharIndex = 0;
    uint8_t getSerialChar[8];
    
    if (msg[1]=='B') //message bien en provenance de la sonnerie ?
    {
        //scan le num�ro de s�rie dans le message
        while(msg[getSerialCharIndex+2] != 'T')
        {
            getSerialChar[getSerialCharIndex] = msg[getSerialCharIndex+2];
            getSerialCharIndex++;
        }
        getSerialCharIndex = 0;
        //converti le num�ro string en uint32
        while(getSerialChar[getSerialCharIndex] != 0x00)
        {
            getSerial = getSerial << 4;
            if((getSerialChar[getSerialCharIndex] >= '0') && (getSerialChar[getSerialCharIndex] <= '9'))
                getSerial += getSerialChar[getSerialCharIndex]-'0';
            else
                getSerial += 0x55-getSerialChar[getSerialCharIndex];
            getSerialCharIndex++;
        }
        //si correspondance avec le num�ro de s�rie du module
        if(getSerial == getSerialRF())
            return 1; 
    }
    //indique que le num�ro de s�rie n'est pas compatible
    return 0;
}

uint8_t CheckSerialDoor(uint8_t *msg)
{
    doorSerialIndex = 0;
    uint32_t getSerial = 0;
    uint8_t getSerialCharIndex = 0;
    uint8_t getSerialChar[8];
    //si le message vient de la porte
    if (msg[1]=='D')
    {
        //scan le num�ro de s�rie dans le message
        while(msg[getSerialCharIndex+2] != 'T')
        {
            getSerialChar[getSerialCharIndex] = msg[getSerialCharIndex+2];
            getSerialCharIndex++;
        }
        getSerialCharIndex = 0;
        //converti le num�ro string en uint32
        while(getSerialChar[getSerialCharIndex] != 0x00)
        {
            getSerial = getSerial << 4;
            if((getSerialChar[getSerialCharIndex] >= '0') && (getSerialChar[getSerialCharIndex] <= '9'))
                getSerial += getSerialChar[getSerialCharIndex]-'0';
            else
                getSerial += 0x55-getSerialChar[getSerialCharIndex];
            getSerialCharIndex++;
        }
        //check si le num�ro de s�rie existe dans la liste
        while(doorSerialList[doorSerialIndex] != 0)
        {
            //si un module est reconnu
            if(doorSerialList[doorSerialIndex] == getSerial)
                return 1;
            (doorSerialIndex)++;
        }
    }
    return 0;
}