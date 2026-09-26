#ifndef __DATA_ACQUISITION_H__
#define __DATA_ACQUISITION_H__
#include "bq79600.h"

extern osMutexId_t telemetryMutex;
void acquiredata(void *argument);

#endif //__DATA_ACQUISITION_H__
