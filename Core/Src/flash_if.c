/**
  ******************************************************************************
  * @file    flash_if.c
  * @brief   This file provides code for interfacing the read/write/erase 
             operation on internal flash memory
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

#include "flash_if.h"

static void Flash_Unlock(void);
static void Flash_Lock(void);
static uint32_t Flash_WaitForLastOperation(void);

uint32_t Flash_Sector_Erase(uint32_t Banks, uint32_t FirstSector, uint32_t NbOfSectors)
{
    uint32_t sector_index;
    uint32_t status = FLASH_OK;
    
    Flash_Unlock();
    /* To avoid interrupt while flash erase operation */
    __disable_irq();

    if(Flash_WaitForLastOperation() != FLASH_OK){
        __enable_irq();
        Flash_Lock();
        return FLASH_ERROR;
    }
    for(sector_index = FirstSector; sector_index < (NbOfSectors + FirstSector); sector_index++){
        FLASH->CR2 &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);
        FLASH->CR2 |= (FLASH_CR_SER | FLASH_CR_PSIZE | (FirstSector << FLASH_CR_SNB_Pos) | FLASH_CR_START);

        status = Flash_WaitForLastOperation();

        FLASH->CR2 &= (~(FLASH_CR_SER | FLASH_CR_SNB));

        if(status != FLASH_OK){
            break;
        }
    }
    __enable_irq();
    Flash_Lock();
		
    return status;
}

uint32_t Flash_Program(uint32_t FlashAddress, uint32_t DataAddress, uint32_t NbOfFlashWords)
{
    __IO uint32_t *dest_addr = (__IO uint32_t *)FlashAddress;
    __IO uint32_t *src_addr = (__IO uint32_t *)DataAddress;
    uint32_t status = FLASH_OK;
    uint8_t row_index = FLASH_NB_32BITWORD_IN_FLASHWORD;
    
    if(((FlashAddress) >= FLASH_BANK2_BASE ) && ((FlashAddress) <= FLASH_END)){        
        Flash_Unlock();
        __disable_irq();
        while(NbOfFlashWords != 0){
            status = Flash_WaitForLastOperation();
            if(status != FLASH_OK){
                __enable_irq();
                Flash_Lock();
                return FLASH_ERROR;
            }else{
                SET_BIT(FLASH->CR2, FLASH_CR_PG);
                __ISB();
                __DSB();

                do{
                    *dest_addr = *src_addr;
                    dest_addr++;
                    src_addr++;
                    row_index--;
                }while(row_index != 0);
                
                __ISB();
                __DSB();

                status = Flash_WaitForLastOperation();
                CLEAR_BIT(FLASH->CR2, FLASH_CR_PG);
            }
            
            FlashAddress += 32;
            DataAddress += 32;
            NbOfFlashWords--;
        }
        __enable_irq();
        Flash_Lock();
    }
    return status;
}

static void Flash_Unlock(void)
{
    /* Unlock Flash control register access */
    if(READ_BIT(FLASH->CR2, FLASH_CR_LOCK) != 0U) {
        /* Authorize the FLASH Bank2 Registers access */
        WRITE_REG(FLASH->KEYR2, FLASH_KEY1);
        WRITE_REG(FLASH->KEYR2, FLASH_KEY2);

        /* Verify Flash Bank2 is unlocked */
        if (READ_BIT(FLASH->CR2, FLASH_CR_LOCK) != 0U) {
            while(1){
            } // Need to add exception for error
        }        
    }
}

static void Flash_Lock(void)
{
  /* Set the LOCK Bit to lock the FLASH Bank2 Control Register access */
  SET_BIT(FLASH->CR2, FLASH_CR_LOCK);

  /* Verify Flash Bank2 is locked */
  if (READ_BIT(FLASH->CR2, FLASH_CR_LOCK) == 0U){
      while(1){
      } // Need to add exception for flash error
  }
}

static uint32_t Flash_WaitForLastOperation(void)
{
    /* Wait for the FLASH operation to complete by polling on QW flag to be reset.
       Even if the FLASH operation fails, the QW flag will be reset and an error
       flag will be set */
    __IO uint32_t count = 0;
    uint32_t errorflag = 0;

    /* wait for last operation to bge completed on Bank2 */
    while((READ_BIT(FLASH->SR2, ((FLASH_FLAG_QW_BANK2) & 0x7FFFFFFFU)) == (((FLASH_FLAG_QW_BANK2) & 0x7FFFFFFFU)))){
        // Need to add exception for timeout by pending
        if(count > 0xFFFFFFFFU){
            return FLASH_TIMEOUT;
        }
        count++;
    }
    
    errorflag = (FLASH->SR2 & FLASH_FLAG_ALL_ERRORS_BANK2) | 0x80000000U;

    if((errorflag & 0x7FFFFFFFU) != 0U){
        WRITE_REG(FLASH->CCR2, (errorflag & 0x7FFFFFFFU));
        return FLASH_ERROR;
    }
    /* Check Flash end of operation flag */
    if(READ_BIT(FLASH->SR2, (FLASH_FLAG_EOP_BANK2 & 0x7FFFFFFFU)) == ((FLASH_FLAG_EOP_BANK2 & 0x7FFFFFFFU))){
        WRITE_REG(FLASH->CCR2, (FLASH_FLAG_EOP_BANK2 & 0x7FFFFFFFU));
    }
    return FLASH_OK;
}



