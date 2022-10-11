/**
  ******************************************************************************
  * @file    flash_if.h
  * @brief   This file contains all the function prototypes for
  *          the flash_if.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __FLASH_IF_H__
#define __FLASH_IF_H__

#ifdef __cplusplus
extern "c" {
#endif

#include "main.h"

    enum{
        FLASH_OK      = 0x00,
        FLASH_ERROR   = 0x01,
        FLASH_BUSY    = 0x02,
        FLASH_TIMEOUT = 0x03
    };    

uint32_t Flash_Sector_Erase(uint32_t Banks, uint32_t FirstSector, uint32_t NbOfSectors);
uint32_t Flash_Program(uint32_t FlashAddress, uint32_t DataAddress, uint32_t NbOfFlashWords);
    
#ifdef __cplusplus
}
#endif
#endif /* __FLASH_IF_H__ */
