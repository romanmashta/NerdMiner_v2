#ifndef DEVICES_H
#define DEVICES_H

#if defined(NERDMINERV2)
#include "devices/nerdMinerV2.h"
#elif defined(NYANMINER_S3_AMOLED)
#include "devices/nyanMinerS3Amoled.h"
#else
#error "No device defined"
#endif

#endif // DEVICES_H