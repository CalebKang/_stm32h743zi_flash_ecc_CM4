#include "main.h"
#include "flash_if.h"
#include "string.h"

static uint32_t FLASH_Get_Page(uint32_t Addr_base, uint32_t Addr_target);

static int32_t FLASH_BANK1_Access_Lock(void);
static int32_t FLASH_BANK1_Access_Unlock(void);
static int32_t FLASH_BANK1_WaitForLastOperation(uint32_t msTimeout);
static int32_t FLASH_BANK1_Erase_Page(uint32_t FirstPage, uint32_t NbOfPages);

int32_t Flash_Result;

void  FLASH_Program(UINT32 u32Addr, UINT32* p_pu32Data, UINT32 p32Length)
{
  UINT08 i = 0;
  UINT08 i_max = ((((p32Length-1)/32)+1)*32);
  UINT08 j = 0;
  UINT32 pu32Data[40];
  INT32 status = 0;
  UINT08 u08index = FLASH_NB_32BITWORD_IN_FLASHWORD;
  UINT08 u08addroffset = 0;

  memset(&pu32Data, 0xff, sizeof(pu32Data));

  for(i=0; i<i_max; i+=4)
  {
    if(i < p32Length)
    {
      pu32Data[j] = p_pu32Data[j];
    }
    else
    {
      pu32Data[j] = 0xFFFFFFFF;
    }
    //------------------
    j++;
  }

  j = 0;

  for(i=0; i<i_max; i+=32)
  {
    FLASH_BANK1_Access_Unlock();

    status = FLASH_BANK1_WaitForLastOperation(0xFFFFFFFF);
    if( status != 0 )
    {
      return;
    }

    SET_BIT( FLASH->CR1, FLASH_CR_PG );

    __ISB( );
    __DSB( );

    #if 0
    *(uint32_t*)(u32Addr + i)= pu32Data[j];
    #else
    for( u08index = 0; u08index < FLASH_NB_32BITWORD_IN_FLASHWORD ; u08index++ )
    {
      *(uint32_t*)(u32Addr + u08addroffset)= pu32Data[j];
      u08addroffset+=4;
      j++;
    }
    #endif

    __ISB( );
    __DSB( );

    status = FLASH_BANK1_WaitForLastOperation(0xFFFFFFFF); ////////////////// too short period

    CLEAR_BIT( FLASH->CR1, FLASH_CR_PG );

    FLASH_BANK1_Access_Lock();

    //HAL_Delay(1);
  }

}

void FLASH_Erase(UINT32 u32StartAddr, UINT32 u32EndAddr)
{
  UINT08 s08FlashResult = 0;
  UINT32 FirstPage_t = 0;
  UINT32 NbOfPages_t = 0;

  if (u32StartAddr > u32EndAddr)
  {
    u32EndAddr = u32StartAddr;
  }

  //HAL_CLEAR_WATCHDOG();
  FirstPage_t = FLASH_Get_Page(FLASH_BANK1_BASE, u32StartAddr);
  NbOfPages_t = FLASH_Get_Page(FLASH_BANK1_BASE, u32EndAddr) - FirstPage_t + 1;

  //=====================================================================================================
  s08FlashResult = FLASH_BANK1_Access_Unlock();
  if( s08FlashResult != RESET )
  {
    return;
  }
  Flash_Result = FLASH_BANK1_Erase_Page(FirstPage_t, NbOfPages_t);
  FLASH_BANK1_Access_Lock(); // Flash E/W Lock

}


uint32_t FLASH_Get_Page(uint32_t Addr_base, uint32_t Addr_target)
{
  return (Addr_target - Addr_base) / FLASH_PAGE_SIZE;
}

int32_t FLASH_BANK1_Access_Lock(void)
{
  SET_BIT(FLASH->CR1, FLASH_CR_LOCK);

  if (READ_BIT(FLASH->CR1, FLASH_CR_LOCK) != 0U)
  {
    return 0;
  }
  return -1;
}

int32_t FLASH_BANK1_Access_Unlock(void)
{
  if(READ_BIT(FLASH->CR1, FLASH_CR_LOCK) != 0U)
  {
    WRITE_REG(FLASH->KEYR1, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR1, FLASH_KEY2);

    if(READ_BIT(FLASH->CR1, FLASH_CR_LOCK) != 0U)
    {
      return -1;
    }
  }
  return 0;
}

int32_t FLASH_BANK1_WaitForLastOperation(uint32_t msTimeout)
{
  uint32_t loop = 0;
  uint32_t error;

  while((READ_BIT(FLASH->SR1, ((FLASH_FLAG_QW_BANK1) & 0x7FFFFFFFU)) == (((FLASH_FLAG_QW_BANK1) & 0x7FFFFFFFU))))
  {
    if(loop++ > msTimeout)
    {
      return -100;
    }
  }

  error = ( FLASH->SR1 & FLASH_FLAG_ALL_ERRORS_BANK1 ) | 0x80000000U;

  if( ( error & 0x7FFFFFFFU) != 0U )
  {
    WRITE_REG(FLASH->CCR1, (error & 0x7FFFFFFFU));
    return -101;
  }

  if( READ_BIT( FLASH->SR1, (FLASH_FLAG_EOP_BANK1 & 0x7FFFFFFFU)) == ((FLASH_FLAG_EOP_BANK1 & 0x7FFFFFFFU)))
  {
    WRITE_REG(FLASH->CCR1, (FLASH_FLAG_EOP_BANK1 & 0x7FFFFFFFU));
  }

  return 0;
}

int32_t FLASH_BANK1_Erase_Page(uint32_t FirstPage, uint32_t NbOfPages)
{
  uint32_t page_index = 0;
  int32_t status = 0;

  status = FLASH_BANK1_WaitForLastOperation( 0xFFFFFFFFU );
  if( status != 0 )
  {
    return status;
  }

  for (page_index = FirstPage; page_index < (FirstPage + NbOfPages); page_index++)
  {
    FLASH->CR1 &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);
    FLASH->CR1 |= (FLASH_CR_SER | FLASH_CR_PSIZE | (page_index << FLASH_CR_SNB_Pos) | FLASH_CR_START);

    status = FLASH_BANK1_WaitForLastOperation( 0xFFFFFFFFU );
    //HAL_CLEAR_WATCHDOG();

    CLEAR_BIT( FLASH->CR1, ( FLASH_CR_SER | FLASH_CR_SNB ) );

    if (status != 0)
    {
      break;
    }
  }

  return status;
}


