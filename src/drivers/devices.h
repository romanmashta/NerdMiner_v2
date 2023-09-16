#pragma once

#if defined(NERDMINERV2)
#include "devices/nerdMinerV2.h"
#elif defined(NYANMINER_S3_AMOLED)
#include "devices/nyanMinerS3Amoled.h"
#else
#error "No device defined"
#endif
