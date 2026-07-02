#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include <stdint.h>

/* Keep debug records away from the external loader stack at the top of SRAM. */
#define LOADER_DEBUG_BASE   0x20004000U
#define LOADER_DEBUG_MAGIC  0x51445049U
#define LOADER_DEBUG_HISTORY_COUNT 16U

#define LOADER_DBG_INIT_START          0x00000100U
#define LOADER_DBG_INIT_HAL_FAIL       0x00000101U
#define LOADER_DBG_INIT_CLOCK_FAIL     0x00000102U
#define LOADER_DBG_CLOCK_HSI_TIMEOUT   0x00000103U
#define LOADER_DBG_CLOCK_CFG_FAIL      0x00000104U
#define LOADER_DBG_CLOCK_OK            0x00000105U
#define LOADER_DBG_OPEN_START          0x00000110U
#define LOADER_DBG_OPEN_DEINIT         0x00000111U
#define LOADER_DBG_OPEN_NOR_INIT_FAIL  0x00000112U
#define LOADER_DBG_OPEN_ID             0x00000113U
#define LOADER_DBG_OPEN_ID_FAIL        0x00000114U
#define LOADER_DBG_OPEN_OK             0x00000115U

#define LOADER_DBG_READ_START          0x00000200U
#define LOADER_DBG_READ_DONE           0x00000201U
#define LOADER_DBG_READ_BUFFER_OK      0x00000202U
#define LOADER_DBG_READ_ADDR_OK        0x00000203U
#define LOADER_DBG_READ_OPEN_OK        0x00000204U
#define LOADER_DBG_READ_PRE_NOR        0x00000205U
#define LOADER_DBG_WRITE_START         0x00000300U
#define LOADER_DBG_WRITE_DONE          0x00000301U
#define LOADER_DBG_WRITE_FAIL          0x00000302U
#define LOADER_DBG_ERASE_START         0x00000400U
#define LOADER_DBG_ERASE_BLOCK         0x00000401U
#define LOADER_DBG_ERASE_FAIL          0x00000402U
#define LOADER_DBG_ERASE_DONE          0x00000403U
#define LOADER_DBG_MASS_ERASE_START    0x00000410U
#define LOADER_DBG_MASS_ERASE_DONE     0x00000411U
#define LOADER_DBG_MASS_ERASE_FAIL     0x00000412U

#define LOADER_DBG_NOR_QSPI_INIT_FAIL  0x00000500U
#define LOADER_DBG_NOR_WAIT_START      0x00000510U
#define LOADER_DBG_NOR_WAIT_STATUS     0x00000511U
#define LOADER_DBG_NOR_WAIT_DONE       0x00000512U
#define LOADER_DBG_NOR_WAIT_READ_FAIL  0x00000513U
#define LOADER_DBG_NOR_WAIT_TIMEOUT    0x00000514U
#define LOADER_DBG_NOR_WREN_CMD        0x00000520U
#define LOADER_DBG_NOR_WREN_STATUS     0x00000521U
#define LOADER_DBG_NOR_WREN_TIMEOUT    0x00000522U
#define LOADER_DBG_NOR_QUAD_STATUS2    0x00000530U
#define LOADER_DBG_NOR_QUAD_WRITE_FAIL 0x00000531U
#define LOADER_DBG_NOR_READ_ID         0x00000540U
#define LOADER_DBG_NOR_READ_FAIL       0x00000550U
#define LOADER_DBG_NOR_READ_CMD_START  0x00000551U
#define LOADER_DBG_NOR_READ_CMD_DONE   0x00000552U
#define LOADER_DBG_NOR_READ_RX_START   0x00000553U
#define LOADER_DBG_NOR_READ_RX_DONE    0x00000554U
#define LOADER_DBG_NOR_WRITE_PAGE      0x00000560U
#define LOADER_DBG_NOR_WRITE_PAGE_FAIL 0x00000561U
#define LOADER_DBG_NOR_ERASE_CMD       0x00000570U
#define LOADER_DBG_NOR_ERASE_DONE      0x00000571U
#define LOADER_DBG_NOR_CHIP_ERASE_CMD  0x00000580U
#define LOADER_DBG_NOR_CHIP_ERASE_DONE 0x00000581U
#define LOADER_DBG_NOR_STATUS_CMD_FAIL 0x00000590U
#define LOADER_DBG_NOR_STATUS_RX_FAIL  0x00000591U
#define LOADER_DBG_NOR_STATUS_RX_OK    0x00000592U
#define LOADER_DBG_NOR_RECOVER_CMD     0x000005A0U
#define LOADER_DBG_NOR_RECOVER_DONE    0x000005A1U
#define LOADER_DBG_NOR_RECOVER_SKIP    0x000005A2U

#define LOADER_DBG_QSPI_INIT_START     0x00000600U
#define LOADER_DBG_QSPI_INIT_DONE      0x00000601U
#define LOADER_DBG_QSPI_INIT_FAIL      0x00000602U
#define LOADER_DBG_QSPI_MSP_START      0x00000610U
#define LOADER_DBG_QSPI_MSP_CLK_FAIL   0x00000611U
#define LOADER_DBG_QSPI_MSP_DONE       0x00000612U

#define LOADER_DBG_TIME_DWT_READY      0x00000700U
#define LOADER_DBG_TIME_FALLBACK       0x00000701U

static inline void loader_debug_mark(uint32_t stage,
                                     uint32_t value0,
                                     uint32_t value1,
                                     uint32_t value2,
                                     uint32_t line)
{
  volatile uint32_t *record = (volatile uint32_t *)LOADER_DEBUG_BASE;
  uint32_t previous_stage = record[1];
  uint32_t previous_value0 = record[2];
  uint32_t history_index = record[8];
  volatile uint32_t *history_entry;

  record[0] = LOADER_DEBUG_MAGIC;
  record[1] = stage;
  record[2] = value0;
  record[3] = value1;
  record[4] = value2;
  record[5] = line;
  record[6] = previous_stage;
  record[7] = previous_value0;

  if (history_index >= LOADER_DEBUG_HISTORY_COUNT)
  {
    history_index = 0U;
  }

  history_entry = &record[16U + (history_index * 5U)];
  history_entry[0] = stage;
  history_entry[1] = value0;
  history_entry[2] = value1;
  history_entry[3] = value2;
  history_entry[4] = line;
  record[8] = history_index + 1U;
}

#define LOADER_DEBUG_MARK(stage, value0, value1, value2) \
  loader_debug_mark((stage), (value0), (value1), (value2), (uint32_t)__LINE__)

#endif /* LOADER_DEBUG_H */
