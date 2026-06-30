#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "qspi.h"

#define NORFLASH_W25Q80_ID   0xEF13U
#define NORFLASH_W25Q16_ID   0xEF14U
#define NORFLASH_W25Q32_ID   0xEF15U
#define NORFLASH_W25Q64_ID   0xEF16U
#define NORFLASH_W25Q128_ID  0xEF17U
#define NORFLASH_W25Q256_ID  0xEF18U

#define NORFLASH_SIZE_BYTES       (8U * 1024U * 1024U)
#define NORFLASH_SECTOR_SIZE      4096U
#define NORFLASH_PAGE_SIZE        256U

HAL_StatusTypeDef norflash_init(void);
uint16_t norflash_read_id(void);
bool norflash_is_supported_id(uint16_t id);
bool norflash_memory_mapped(void);

HAL_StatusTypeDef norflash_read(uint8_t *buffer, uint32_t address, uint32_t length);
HAL_StatusTypeDef norflash_write_page(const uint8_t *buffer, uint32_t address, uint32_t length);
HAL_StatusTypeDef norflash_write(const uint8_t *buffer, uint32_t address, uint32_t length);
HAL_StatusTypeDef norflash_erase_sector(uint32_t address);
HAL_StatusTypeDef norflash_erase_chip(void);

#ifdef __cplusplus
}
#endif

#endif /* __NORFLASH_H__ */
