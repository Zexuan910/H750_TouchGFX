#ifndef __QSPI_H__
#define __QSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern QSPI_HandleTypeDef hqspi;

HAL_StatusTypeDef qspi_init(void);
HAL_StatusTypeDef qspi_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __QSPI_H__ */
