/* ************************************************************************** */
/** Descriptive File Name
 
    SCA, 21.10.2019
 
  @Company
    ETML-ES

  @File Name
    spi2.h

  @Summary
    Gestion port SPI 2

  @Description
    Gestion port SPI 2
 */
/* ************************************************************************** */

#ifndef _SPI2_H    /* Guard against multiple inclusion */
#define _SPI2_H

void Spi2_Init(void);
uint8_t Spi2_ReadWrite(uint8_t txData);


#endif /* _SPI2_H */

/* *****************************************************************************
 End of File
 */
