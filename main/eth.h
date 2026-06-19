#ifndef ETH_H
#define ETH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
void eth_start();

#if CONFIG_ETH_DEINIT_CODE
void eth_stop();
#endif

#ifdef __cplusplus
}
#endif

#endif
