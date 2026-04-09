#ifndef ZMOD4510_HAL_PORT_H
#define ZMOD4510_HAL_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal.h"

    /*
     * Fill Renesas Interface_t with ESP32-specific callbacks.
     * Returns 0 on success.
     */
    int zmod4510_fill_interface(Interface_t *hal);

#ifdef __cplusplus
}
#endif

#endif // ZMOD4510_HAL_PORT_H