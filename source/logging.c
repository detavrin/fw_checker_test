#include "logging.h"

/* Максимальное количество байт в одной линии вывода HEX_ARRAY */
#ifndef CONFIG_HEXDUMP_BYTES_IN_LINE
#define CONFIG_HEXDUMP_BYTES_IN_LINE      16
#endif /* CONFIG_HEXDUMP_BYTES_IN_LINE */

void log_hexdump_buffer(const unsigned char* buffer, size_t size)
{
    if(buffer == NULL || size == 0) {
        return;
    }

    const unsigned char* data_buffer = buffer;

    while (size > 0U) {

        for (size_t i = 0U; i < CONFIG_HEXDUMP_BYTES_IN_LINE; i++) {
            if (i < size) {
                LOG_PRINTF("%02X ", (unsigned char)data_buffer[i] & 0xFFu);
            } else {
                LOG_PRINTF("   ");
            }
        }

        LOG_PRINTF("\r\n");

        if (size < CONFIG_HEXDUMP_BYTES_IN_LINE) {
            break;
        }

        size -= CONFIG_HEXDUMP_BYTES_IN_LINE;
        data_buffer += CONFIG_HEXDUMP_BYTES_IN_LINE;
    }

    LOG_PRINTF("\r\n");
}

