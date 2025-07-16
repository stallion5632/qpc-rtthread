#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Flash write stub implementation */
int32_t flash_write(uint8_t const *data, uint32_t len) {
    if (data == (uint8_t const *)0) {
        return -1;  /* Invalid parameter */
    }

    printf("[stub] flash_write called, len=%lu, first_byte=0x%02X\n",
           (unsigned long)len,
           (len > 0U) ? (unsigned int)data[0] : 0U);
    return 0;  /* Success */
}

/* Config read stub implementation */
void read_config(uint32_t key, uint8_t *buf) {
    printf("[stub] read_config called, key=%lu\n", (unsigned long)key);
    if (buf != (uint8_t *)0) {
        buf[0] = 0U;  /* Simple default value */
        if (key == 1U) {
            buf[0] = 100U;  /* Default sensor rate */
        } else if (key == 2U) {
            buf[0] = 200U;  /* Default storage interval */
        } else {
            /* Default case - already set to 0 */
        }
    }
}
