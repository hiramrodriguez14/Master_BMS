#ifndef TEST_CMSIS_H
#define TEST_CMSIS_H
#include <stdint.h>
typedef void *osMutexId_t;
#define osWaitForever UINT32_MAX
#define osOK 0
int osMutexAcquire(osMutexId_t, uint32_t);
int osMutexRelease(osMutexId_t);
#endif
