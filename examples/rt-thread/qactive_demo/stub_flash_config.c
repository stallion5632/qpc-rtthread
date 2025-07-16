#include <stdint.h>
#include <stdio.h>

// flash_write的桩实现
int flash_write(uint8_t *data, uint32_t len) {
    printf("[stub] flash_write called, len=%lu\n", (unsigned long)len);
    return 0; // 0表示成功
}

// read_config的桩实现
void read_config(uint32_t key, uint8_t *buf) {
    printf("[stub] read_config called, key=%lu\n", (unsigned long)key);
    if (buf) buf[0] = 0; // 简单赋值
}
