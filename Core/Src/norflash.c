#include "norflash.h"

#ifdef EXTERNAL_LOADER_BUILD
#include "loader_debug.h"
#else
#define LOADER_DEBUG_MARK(stage, value0, value1, value2) ((void)0)
#endif

#define NORFLASH_CMD_WRITE_ENABLE        0x06U
#define NORFLASH_CMD_READ_STATUS_REG1    0x05U
#define NORFLASH_CMD_READ_STATUS_REG2    0x35U
#define NORFLASH_CMD_WRITE_STATUS_REG2   0x31U
#define NORFLASH_CMD_READ_ID             0x90U
#define NORFLASH_CMD_FAST_READ_QUAD_IO   0xEBU
#define NORFLASH_CMD_PAGE_PROGRAM        0x02U
#define NORFLASH_CMD_SECTOR_ERASE        0x20U
#define NORFLASH_CMD_CHIP_ERASE          0xC7U
#define NORFLASH_CMD_RELEASE_POWER_DOWN  0xABU
#define NORFLASH_CMD_ENABLE_RESET        0x66U
#define NORFLASH_CMD_RESET_DEVICE        0x99U
#define NORFLASH_CMD_EXIT_QPI            0xFFU

#define NORFLASH_STATUS_BUSY             0x01U
#define NORFLASH_STATUS_WEL              0x02U
#define NORFLASH_STATUS_QE               0x02U
#define NORFLASH_SECTOR_ERASE_TIMEOUT_MS 5000U
#define NORFLASH_CHIP_ERASE_TIMEOUT_MS   200000U

static HAL_StatusTypeDef norflash_command(uint8_t instruction,
                                          uint32_t address_mode,
                                          uint32_t address,
                                          uint32_t address_size,
                                          uint32_t dummy_cycles,
                                          uint32_t data_mode,
                                          uint32_t length)
{
  QSPI_CommandTypeDef command = {0};
  HAL_StatusTypeDef result = HAL_OK;

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = instruction;
  command.AddressMode = address_mode;
  command.AddressSize = address_size;
  command.Address = address;
  command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  command.DummyCycles = dummy_cycles;
  command.DataMode = data_mode;
  command.NbData = length;
  command.DdrMode = QSPI_DDR_MODE_DISABLE;
  command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  result = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
#ifdef EXTERNAL_LOADER_BUILD
  if ((result == HAL_BUSY) &&
      (hqspi.Instance == QUADSPI) &&
      (__HAL_QSPI_GET_FLAG(&hqspi, QSPI_FLAG_BUSY) == RESET))
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_SKIP,
                      instruction,
                      (uint32_t)hqspi.State,
                      (uint32_t)hqspi.Lock);
    __HAL_UNLOCK(&hqspi);
    hqspi.State = HAL_QSPI_STATE_READY;
    hqspi.ErrorCode = HAL_QSPI_ERROR_NONE;
    result = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
#endif

  return result;
}

static HAL_StatusTypeDef norflash_simple_instruction(uint32_t instruction_mode, uint8_t instruction)
{
  QSPI_CommandTypeDef command = {0};

  command.InstructionMode = instruction_mode;
  command.Instruction = instruction;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  command.DummyCycles = 0;
  command.DataMode = QSPI_DATA_NONE;
  command.DdrMode = QSPI_DDR_MODE_DISABLE;
  command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  return HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static void norflash_reinit_qspi(void)
{
  (void)qspi_deinit();
  (void)qspi_init();
}

static HAL_StatusTypeDef norflash_recover_spi_mode(void)
{
  HAL_StatusTypeDef result = HAL_OK;

  result = norflash_simple_instruction(QSPI_INSTRUCTION_4_LINES, NORFLASH_CMD_EXIT_QPI);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_CMD, NORFLASH_CMD_EXIT_QPI, QSPI_INSTRUCTION_4_LINES, (uint32_t)result);
  if (result != HAL_OK)
  {
    norflash_reinit_qspi();
  }
  HAL_Delay(1U);

  result = norflash_simple_instruction(QSPI_INSTRUCTION_1_LINE, NORFLASH_CMD_EXIT_QPI);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_CMD, NORFLASH_CMD_EXIT_QPI, QSPI_INSTRUCTION_1_LINE, (uint32_t)result);
  if (result != HAL_OK)
  {
    norflash_reinit_qspi();
  }
  HAL_Delay(1U);

  result = norflash_simple_instruction(QSPI_INSTRUCTION_1_LINE, NORFLASH_CMD_RELEASE_POWER_DOWN);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_CMD, NORFLASH_CMD_RELEASE_POWER_DOWN, QSPI_INSTRUCTION_1_LINE, (uint32_t)result);
  if (result != HAL_OK)
  {
    norflash_reinit_qspi();
  }
  HAL_Delay(2U);

  result = norflash_simple_instruction(QSPI_INSTRUCTION_1_LINE, NORFLASH_CMD_ENABLE_RESET);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_CMD, NORFLASH_CMD_ENABLE_RESET, QSPI_INSTRUCTION_1_LINE, (uint32_t)result);
  if (result != HAL_OK)
  {
    norflash_reinit_qspi();
  }

  result = norflash_simple_instruction(QSPI_INSTRUCTION_1_LINE, NORFLASH_CMD_RESET_DEVICE);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_CMD, NORFLASH_CMD_RESET_DEVICE, QSPI_INSTRUCTION_1_LINE, (uint32_t)result);
  if (result != HAL_OK)
  {
    norflash_reinit_qspi();
  }
  HAL_Delay(2U);

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_RECOVER_DONE, 0U, 0U, 0U);
  return HAL_OK;
}

static HAL_StatusTypeDef norflash_read_status(uint8_t command, uint8_t *status)
{
  HAL_StatusTypeDef result = norflash_command(command,
                                             QSPI_ADDRESS_NONE,
                                             0,
                                             QSPI_ADDRESS_24_BITS,
                                             0,
                                             QSPI_DATA_1_LINE,
                                             1);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_STATUS_CMD_FAIL, command, (uint32_t)hqspi.ErrorCode, (uint32_t)result);
    return result;
  }

  result = HAL_QSPI_Receive(&hqspi, status, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_STATUS_RX_FAIL, command, (uint32_t)hqspi.ErrorCode, (uint32_t)result);
    return result;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_STATUS_RX_OK, command, *status, 0U);
  return HAL_OK;
}

static HAL_StatusTypeDef norflash_wait_busy(uint32_t timeout_ms)
{
  uint8_t status = 0;
  uint32_t tickstart = HAL_GetTick();

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_WAIT_START, timeout_ms, tickstart, 0U);

  do
  {
    HAL_StatusTypeDef result = norflash_read_status(NORFLASH_CMD_READ_STATUS_REG1, &status);
    if (result != HAL_OK)
    {
      return result;
    }

    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WAIT_STATUS, timeout_ms, status, HAL_GetTick() - tickstart);

    if ((status & NORFLASH_STATUS_BUSY) == 0U)
    {
      LOADER_DEBUG_MARK(LOADER_DBG_NOR_WAIT_DONE, timeout_ms, status, HAL_GetTick() - tickstart);
      return HAL_OK;
    }
  } while ((HAL_GetTick() - tickstart) < timeout_ms);

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_WAIT_TIMEOUT, timeout_ms, status, HAL_GetTick() - tickstart);
  return HAL_TIMEOUT;
}

static HAL_StatusTypeDef norflash_write_enable(void)
{
  uint8_t status = 0;
  uint32_t tickstart = HAL_GetTick();
  HAL_StatusTypeDef result = norflash_command(NORFLASH_CMD_WRITE_ENABLE,
                                              QSPI_ADDRESS_NONE,
                                              0,
                                              QSPI_ADDRESS_24_BITS,
                                              0,
                                              QSPI_DATA_NONE,
                                              0);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_WREN_CMD, 0U, 0U, (uint32_t)result);
  if (result != HAL_OK)
  {
    return result;
  }

  do
  {
    result = norflash_read_status(NORFLASH_CMD_READ_STATUS_REG1, &status);
    if (result != HAL_OK)
    {
      LOADER_DEBUG_MARK(LOADER_DBG_NOR_WREN_STATUS, status, HAL_GetTick() - tickstart, (uint32_t)result);
      return result;
    }

    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WREN_STATUS, status, HAL_GetTick() - tickstart, 0U);
    if ((status & NORFLASH_STATUS_WEL) != 0U)
    {
      return HAL_OK;
    }
  } while ((HAL_GetTick() - tickstart) < HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_WREN_TIMEOUT, status, HAL_GetTick() - tickstart, 0U);
  return HAL_TIMEOUT;
}

static HAL_StatusTypeDef norflash_write_status2(uint8_t status)
{
  HAL_StatusTypeDef result = norflash_write_enable();
  if (result != HAL_OK)
  {
    return result;
  }

  result = norflash_command(NORFLASH_CMD_WRITE_STATUS_REG2,
                            QSPI_ADDRESS_NONE,
                            0,
                            QSPI_ADDRESS_24_BITS,
                            0,
                            QSPI_DATA_1_LINE,
                            1);
  if (result != HAL_OK)
  {
    return result;
  }

  result = HAL_QSPI_Transmit(&hqspi, &status, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_QUAD_WRITE_FAIL, status, 0U, (uint32_t)result);
    return result;
  }

  return norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef norflash_enable_quad_io(void)
{
  uint8_t status2 = 0;
  HAL_StatusTypeDef result = norflash_read_status(NORFLASH_CMD_READ_STATUS_REG2, &status2);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_QUAD_STATUS2, status2, 0U, (uint32_t)result);
    return result;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_QUAD_STATUS2, status2, 0U, 0U);

  if ((status2 & NORFLASH_STATUS_QE) != 0U)
  {
    return HAL_OK;
  }

  status2 |= NORFLASH_STATUS_QE;
  return norflash_write_status2(status2);
}

HAL_StatusTypeDef norflash_init(void)
{
  HAL_StatusTypeDef result = qspi_init();
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_QSPI_INIT_FAIL, 0U, (uint32_t)hqspi.ErrorCode, (uint32_t)result);
    return result;
  }

  result = norflash_recover_spi_mode();
  if (result != HAL_OK)
  {
    return result;
  }

  result = norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    return result;
  }

  return norflash_enable_quad_io();
}

uint16_t norflash_read_id(void)
{
  uint8_t id[2] = {0, 0};

  HAL_StatusTypeDef result = norflash_command(NORFLASH_CMD_READ_ID,
                                             QSPI_ADDRESS_1_LINE,
                                             0,
                                             QSPI_ADDRESS_24_BITS,
                                             0,
                                             QSPI_DATA_1_LINE,
                                             2);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_READ_ID, 0U, 0U, (uint32_t)result);
    return 0;
  }

  result = HAL_QSPI_Receive(&hqspi, id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_READ_ID, 0U, (uint32_t)hqspi.ErrorCode, (uint32_t)result);
    return 0;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_READ_ID, id[0], id[1], 0U);
  return ((uint16_t)id[0] << 8) | id[1];
}

bool norflash_is_supported_id(uint16_t id)
{
  switch (id)
  {
  case NORFLASH_W25Q80_ID:
  case NORFLASH_W25Q16_ID:
  case NORFLASH_W25Q32_ID:
  case NORFLASH_W25Q64_ID:
  case NORFLASH_W25Q128_ID:
  case NORFLASH_W25Q256_ID:
    return true;
  default:
    return false;
  }
}

bool norflash_memory_mapped(void)
{
  QSPI_CommandTypeDef command = {0};
  QSPI_MemoryMappedTypeDef memory_mapped = {0};

  if (norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = NORFLASH_CMD_FAST_READ_QUAD_IO;
  command.AddressMode = QSPI_ADDRESS_4_LINES;
  command.AddressSize = QSPI_ADDRESS_24_BITS;
  command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  command.DummyCycles = 6;
  command.DataMode = QSPI_DATA_4_LINES;
  command.DdrMode = QSPI_DDR_MODE_DISABLE;
  command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  memory_mapped.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  memory_mapped.TimeOutPeriod = 0;

  return HAL_QSPI_MemoryMapped(&hqspi, &command, &memory_mapped) == HAL_OK;
}

HAL_StatusTypeDef norflash_read(uint8_t *buffer, uint32_t address, uint32_t length)
{
  if (length == 0U)
  {
    return HAL_OK;
  }

  HAL_StatusTypeDef result = norflash_command(NORFLASH_CMD_FAST_READ_QUAD_IO,
                                             QSPI_ADDRESS_4_LINES,
                                             address,
                                             QSPI_ADDRESS_24_BITS,
                                             6,
                                             QSPI_DATA_4_LINES,
                                             length);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_READ_FAIL, address, length, (uint32_t)result);
    return result;
  }

  result = HAL_QSPI_Receive(&hqspi, buffer, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_READ_FAIL, address, length, (uint32_t)result);
  }

  return result;
}

HAL_StatusTypeDef norflash_write_page(const uint8_t *buffer, uint32_t address, uint32_t length)
{
  if (length == 0U)
  {
    return HAL_OK;
  }

  if ((length > NORFLASH_PAGE_SIZE) || (((address & (NORFLASH_PAGE_SIZE - 1U)) + length) > NORFLASH_PAGE_SIZE))
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef result = norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WRITE_PAGE_FAIL, address, length, (uint32_t)result);
    return result;
  }

  result = norflash_write_enable();
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WRITE_PAGE_FAIL, address, length, (uint32_t)result);
    return result;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_WRITE_PAGE, address, length, 0U);

  result = norflash_command(NORFLASH_CMD_PAGE_PROGRAM,
                            QSPI_ADDRESS_1_LINE,
                            address,
                            QSPI_ADDRESS_24_BITS,
                            0,
                            QSPI_DATA_1_LINE,
                            length);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WRITE_PAGE_FAIL, address, length, (uint32_t)result);
    return result;
  }

  result = HAL_QSPI_Transmit(&hqspi, (uint8_t *)buffer, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_WRITE_PAGE_FAIL, address, length, (uint32_t)result);
    return result;
  }

  return norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef norflash_write(const uint8_t *buffer, uint32_t address, uint32_t length)
{
  HAL_StatusTypeDef result = HAL_OK;

  while (length > 0U)
  {
    uint32_t page_remaining = NORFLASH_PAGE_SIZE - (address & (NORFLASH_PAGE_SIZE - 1U));
    uint32_t chunk = (length < page_remaining) ? length : page_remaining;

    result = norflash_write_page(buffer, address, chunk);
    if (result != HAL_OK)
    {
      return result;
    }

    buffer += chunk;
    address += chunk;
    length -= chunk;
  }

  return result;
}

HAL_StatusTypeDef norflash_erase_sector(uint32_t address)
{
  HAL_StatusTypeDef result = norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_ERASE_CMD, address, 0U, (uint32_t)result);
    return result;
  }

  result = norflash_write_enable();
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_ERASE_CMD, address, 1U, (uint32_t)result);
    return result;
  }

  address &= ~(NORFLASH_SECTOR_SIZE - 1U);
  LOADER_DEBUG_MARK(LOADER_DBG_NOR_ERASE_CMD, address, NORFLASH_CMD_SECTOR_ERASE, 0U);
  result = norflash_command(NORFLASH_CMD_SECTOR_ERASE,
                            QSPI_ADDRESS_1_LINE,
                            address,
                            QSPI_ADDRESS_24_BITS,
                            0,
                            QSPI_DATA_NONE,
                            0);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_ERASE_CMD, address, NORFLASH_CMD_SECTOR_ERASE, (uint32_t)result);
    return result;
  }

  result = norflash_wait_busy(NORFLASH_SECTOR_ERASE_TIMEOUT_MS);
  if (result == HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_ERASE_DONE, address, 0U, 0U);
  }
  return result;
}

HAL_StatusTypeDef norflash_erase_chip(void)
{
  HAL_StatusTypeDef result = norflash_wait_busy(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_CHIP_ERASE_CMD, 0U, 0U, (uint32_t)result);
    return result;
  }

  result = norflash_write_enable();
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_CHIP_ERASE_CMD, 0U, 1U, (uint32_t)result);
    return result;
  }

  LOADER_DEBUG_MARK(LOADER_DBG_NOR_CHIP_ERASE_CMD, 0U, NORFLASH_CMD_CHIP_ERASE, 0U);
  result = norflash_command(NORFLASH_CMD_CHIP_ERASE,
                            QSPI_ADDRESS_NONE,
                            0,
                            QSPI_ADDRESS_24_BITS,
                            0,
                            QSPI_DATA_NONE,
                            0);
  if (result != HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_CHIP_ERASE_CMD, 0U, NORFLASH_CMD_CHIP_ERASE, (uint32_t)result);
    return result;
  }

  result = norflash_wait_busy(NORFLASH_CHIP_ERASE_TIMEOUT_MS);
  if (result == HAL_OK)
  {
    LOADER_DEBUG_MARK(LOADER_DBG_NOR_CHIP_ERASE_DONE, 0U, 0U, 0U);
  }
  return result;
}
