#include "loader_api.h"
#include "loader_debug.h"
#include "norflash.h"

#include <stddef.h>
#include <string.h>

#define VERIFY_CHUNK_SIZE 256U

const StorageInfoTypeDef StorageInfo __attribute__((section(".Dev_Info"), used)) =
{
  LOADER_DEVICE_NAME,
  LOADER_DEVICE_TYPE_NOR,
  LOADER_FLASH_BASE,
  LOADER_FLASH_SIZE,
  LOADER_WRITE_BLOCK_SIZE,
  LOADER_ERASE_VALUE,
  {
    { LOADER_SECTOR_COUNT, LOADER_SECTOR_SIZE },
    { 0U, 0U }
  }
};

static uint8_t verify_buffer[VERIFY_CHUNK_SIZE];
static int loader_ready;

static int loader_system_clock_config(void);
static int open_flash(void);

static int address_to_offset(uint32_t address, uint32_t size, uint32_t *offset)
{
  uint32_t local_offset;

  if (address < LOADER_FLASH_BASE)
  {
    return 0;
  }

  local_offset = address - LOADER_FLASH_BASE;
  if (local_offset > LOADER_FLASH_SIZE)
  {
    return 0;
  }

  if (size > (LOADER_FLASH_SIZE - local_offset))
  {
    return 0;
  }

  if (offset != NULL)
  {
    *offset = local_offset;
  }

  return 1;
}

static int ensure_ready(void)
{
  if (loader_ready)
  {
    return 1;
  }

  return Init();
}

int Init(void)
{
  LOADER_DEBUG_MARK(LOADER_DBG_INIT_START, 0U, 0U, 0U);

  SystemInit();

  if (HAL_Init() != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_INIT_HAL_FAIL, 0U, 0U, 0U);
    return 0;
  }

  if (!loader_system_clock_config())
  {
    LOADER_DEBUG_MARK(LOADER_DBG_INIT_CLOCK_FAIL, 0U, 0U, 0U);
    return 0;
  }

  SystemCoreClockUpdate();
  loader_time_init();

  return open_flash();
}

static int loader_system_clock_config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  uint32_t wait = 0x100000U;

  (void)HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY) && (wait > 0U))
  {
    wait--;
  }

  __HAL_RCC_HSI_ENABLE();
  wait = 0x100000U;
  while (!__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) && (wait > 0U))
  {
    wait--;
  }

  if (wait == 0U)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_CLOCK_HSI_TIMEOUT, 0U, 0U, 0U);
    return 0;
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_CLOCK_CFG_FAIL, 0U, 0U, 0U);
    return 0;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_CLOCK_OK, 0U, 0U, 0U);
  return 1;
}

static int open_flash(void)
{
  uint16_t flash_id = 0U;
  HAL_StatusTypeDef result = HAL_OK;

  LOADER_DEBUG_MARK(LOADER_DBG_OPEN_START, (uint32_t)hqspi.Instance, loader_ready, 0U);

  if (hqspi.Instance == QUADSPI)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_OPEN_DEINIT, (uint32_t)hqspi.Instance, 0U, 0U);
    (void)qspi_deinit();
  }

  result = norflash_init();
  if (result != HAL_OK)
  {
    loader_ready = 0;
    return 0;
  }

  flash_id = norflash_read_id();
  LOADER_DEBUG_MARK(LOADER_DBG_OPEN_ID, flash_id, (uint32_t)hqspi.ErrorCode, 0U);

  if (!norflash_is_supported_id(flash_id))
  {
    LOADER_DEBUG_MARK(LOADER_DBG_OPEN_ID_FAIL, flash_id, (uint32_t)hqspi.ErrorCode, 0U);
    loader_ready = 0;
    return 0;
  }

  loader_ready = 1;
  LOADER_DEBUG_MARK(LOADER_DBG_OPEN_OK, flash_id, (uint32_t)hqspi.ErrorCode, 0U);
  return 1;
}

int Read(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
  uint32_t offset = 0U;
  int ok = 0;

  LOADER_DEBUG_MARK(LOADER_DBG_READ_START, Address, Size, (uint32_t)Buffer);

  if (Buffer == NULL)
  {
    return 0;
  }
  LOADER_DEBUG_MARK(LOADER_DBG_READ_BUFFER_OK, Address, Size, (uint32_t)Buffer);

  if (!address_to_offset(Address, Size, &offset))
  {
    return 0;
  }
  LOADER_DEBUG_MARK(LOADER_DBG_READ_ADDR_OK, Address, Size, offset);

  if (!open_flash())
  {
    return 0;
  }
  LOADER_DEBUG_MARK(LOADER_DBG_READ_OPEN_OK, Address, Size, offset);

  LOADER_DEBUG_MARK(LOADER_DBG_READ_PRE_NOR, Address, Size, offset);
  ok = (norflash_read(Buffer, offset, Size) == HAL_OK) ? 1 : 0;
  LOADER_DEBUG_MARK(LOADER_DBG_READ_DONE, Address, Size, (uint32_t)ok);
  return ok;
}

int Write(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
  uint32_t offset = 0U;
  HAL_StatusTypeDef result = HAL_OK;

  LOADER_DEBUG_MARK(LOADER_DBG_WRITE_START, Address, Size, (uint32_t)Buffer);

  if ((Buffer == NULL) || !address_to_offset(Address, Size, &offset) || !ensure_ready())
  {
    LOADER_DEBUG_MARK(LOADER_DBG_WRITE_FAIL, Address, Size, 0U);
    return 0;
  }

  result = norflash_write(Buffer, offset, Size);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_WRITE_FAIL, Address, Size, (uint32_t)result);
    return 0;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_WRITE_DONE, Address, Size, 0U);
  return 1;
}

int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
  uint32_t start_offset = 0U;
  uint32_t end_offset = 0U;
  uint32_t address = 0U;

  LOADER_DEBUG_MARK(LOADER_DBG_ERASE_START, EraseStartAddress, EraseEndAddress, 0U);

  if ((EraseEndAddress < EraseStartAddress) ||
      !address_to_offset(EraseStartAddress, 1U, &start_offset) ||
      !address_to_offset(EraseEndAddress, 1U, &end_offset) ||
      !open_flash())
  {
    LOADER_DEBUG_MARK(LOADER_DBG_ERASE_FAIL, EraseStartAddress, EraseEndAddress, 0U);
    return 0;
  }

  start_offset &= ~(LOADER_SECTOR_SIZE - 1U);
  end_offset &= ~(LOADER_SECTOR_SIZE - 1U);

  address = start_offset;
  while (address <= end_offset)
  {
    HAL_StatusTypeDef result = HAL_OK;

    LOADER_DEBUG_MARK(LOADER_DBG_ERASE_BLOCK, LOADER_FLASH_BASE + address, end_offset, 0U);

    result = norflash_erase_sector(address);
    if (result != HAL_OK)
    {
      LOADER_DEBUG_MARK(LOADER_DBG_ERASE_FAIL, LOADER_FLASH_BASE + address, end_offset, (uint32_t)result);
      return 0;
    }

    if (address > (LOADER_FLASH_SIZE - LOADER_SECTOR_SIZE))
    {
      break;
    }
    address += LOADER_SECTOR_SIZE;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_ERASE_DONE, EraseStartAddress, EraseEndAddress, 0U);
  return 1;
}

int MassErase(void)
{
  HAL_StatusTypeDef result = HAL_OK;

  LOADER_DEBUG_MARK(LOADER_DBG_MASS_ERASE_START, 0U, 0U, 0U);

  if (!open_flash())
  {
    LOADER_DEBUG_MARK(LOADER_DBG_MASS_ERASE_FAIL, 0U, 0U, 0U);
    return 0;
  }

  result = norflash_erase_chip();
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_MASS_ERASE_FAIL, 0U, 0U, (uint32_t)result);
    return 0;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_MASS_ERASE_DONE, 0U, 0U, 0U);
  return 1;
}

uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
  uint32_t checksum = InitVal;
  uint32_t address = StartAddress;
  uint32_t remaining = Size;

  if (!address_to_offset(StartAddress, Size, NULL) || !ensure_ready())
  {
    return checksum;
  }

  while (remaining > 0U)
  {
    uint32_t chunk = (remaining > VERIFY_CHUNK_SIZE) ? VERIFY_CHUNK_SIZE : remaining;
    if (!Read(address, chunk, verify_buffer))
    {
      return checksum;
    }

    for (uint32_t i = 0U; i < chunk; ++i)
    {
      checksum += verify_buffer[i];
    }

    address += chunk;
    remaining -= chunk;
  }

  return checksum;
}

uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement)
{
  uint32_t verified = 0U;
  uint32_t verify_size = Size * 4U;
  uint32_t checksum_start = MemoryAddr + (missalignement & 0xFU);
  uint32_t checksum_size = verify_size - ((missalignement >> 16) & 0xFU);
  uint32_t checksum = CheckSum(checksum_start, checksum_size, 0U);
  uint8_t *ram = (uint8_t *)RAMBufferAddr;

  if (!address_to_offset(MemoryAddr, verify_size, NULL) || (ram == NULL) || !ensure_ready())
  {
    return ((uint64_t)checksum << 32) | MemoryAddr;
  }

  while (verified < verify_size)
  {
    uint32_t chunk = ((verify_size - verified) > VERIFY_CHUNK_SIZE) ? VERIFY_CHUNK_SIZE : (verify_size - verified);
    if (!Read(MemoryAddr + verified, chunk, verify_buffer))
    {
      return ((uint64_t)checksum << 32) | (MemoryAddr + verified);
    }

    if (memcmp(verify_buffer, ram + verified, chunk) != 0)
    {
      for (uint32_t i = 0U; i < chunk; ++i)
      {
        if (verify_buffer[i] != ram[verified + i])
        {
          return ((uint64_t)checksum << 32) | (MemoryAddr + verified + i);
        }
      }
    }

    verified += chunk;
  }

  return ((uint64_t)checksum << 32);
}
