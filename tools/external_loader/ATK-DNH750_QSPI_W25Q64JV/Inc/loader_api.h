#ifndef LOADER_API_H
#define LOADER_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOADER_DEVICE_NAME       "ATK-DNH750_QSPI_W25Q64JV"
#define LOADER_DEVICE_TYPE_NOR   3U
#define LOADER_FLASH_BASE        0x90000000U
#define LOADER_FLASH_SIZE        0x00800000U
#define LOADER_WRITE_BLOCK_SIZE  0x00001000U
#define LOADER_ERASE_VALUE       0x000000FFU
#define LOADER_SECTOR_COUNT      2048U
#define LOADER_SECTOR_SIZE       0x00001000U

typedef struct
{
  uint32_t SectorNum;
  uint32_t SectorSize;
} StorageSectorTypeDef;

typedef struct
{
  char DeviceName[100];
  uint32_t DeviceType;
  uint32_t DeviceStartAddress;
  uint32_t DeviceSize;
  uint32_t PageSize;
  uint32_t EraseValue;
  StorageSectorTypeDef sectors[10];
} StorageInfoTypeDef;

extern const StorageInfoTypeDef StorageInfo;

int Init(void);
int Read(uint32_t Address, uint32_t Size, uint8_t *Buffer);
int Write(uint32_t Address, uint32_t Size, uint8_t *Buffer);
int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
int MassErase(void);
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal);
uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement);

void loader_time_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LOADER_API_H */
