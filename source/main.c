#include <errno.h>

#include "board.h"
#include "dfu_host.h"
#include "core/crc.h"
#include "core/util.h"
#include "core/assert.h"

/************************* LOG SETTINGS ****************************/

#define LOG_MODULE_PRINTABLE_NAME "MAIN"
#define LOG_MODULE_LOG_LEVEL 4U
#define LOG_MODULE_IS_ENABLED (defined(DEBUG))
#define LOG_MODULE_IS_TIMESTAMP_ENABLED 1
#define LOG_MODULE_IS_FUNC_NAME_ENABLED 1

#include "logging.h"

/*******************************************************************/

/**
 *  @brief  Перечисление возможных состояний приложения
 */
typedef enum {
    APP_STATE_INITIAL,
    APP_STATE_READ_META,
    APP_STATE_CHECK_FW_CRC,
    APP_STATE_CHECK_DONE,
    APP_STATE_CHECK_FAILURE,
} app_state_t;

/**
 *  @brief  Структура метаинформации прошивки.
 */
typedef struct __packed {
    uint32_t fw_size; /* Размер прошивки в байтах      */
    uint16_t crc16; /* CRC16 (ANSI) области прошивки */
} fw_meta_t;

/* Текущее состояние автомата приложения */
static app_state_t app_state = APP_STATE_INITIAL;
/* Прочитанная метаинформация о прошивке проверяемого устройства */
static fw_meta_t fw_meta;

/**
 *  @brief  Прочитать метаинформацию о прошивке из внутренней памяти устройства.
 *  @param  fw_meta  Результат чтения области памяти хранящей метаинформацию прошивки.
 *  @return 0 - в случае успеха, отрицательный код ошибки в противном случае.
 */
static inline int read_fw_meta(fw_meta_t* fw_meta)
{
    CHECK(fw_meta, return -EINVAL);

    const fw_meta_t* meta = NULL;

    int rc = dfu_host_read_memory(
        board_get_fw_meta_addr(), (const uint8_t**)&meta, sizeof(fw_meta_t));
    if (rc < 0) {
        return rc;
    }

    *fw_meta = *meta;

    return 0;
}

/**
 *  @brief  Выполнить очередную итерацию основного цикла приложения.
 */
static void app_dispatch(void)
{
    switch (app_state) {
    /* Начальное состояние автомата - сброс подчиненного устройства */
    case APP_STATE_INITIAL: {
        LOG_DBG("Rebooting...");

        /* Начальный сброс внешнего MCU */
        board_reset_write(0);
        HAL_Delay(100);
        board_reset_write(1);
        HAL_Delay(1000);

        int rc = 0;

        /* Получить ответ на Ping */
        for (int i = 0; i < 5; ++i) {
            rc = dfu_host_ping(1000);

            if (rc == 0) {
                LOG_DBG("Device found!");
                app_state = APP_STATE_READ_META;
                return;
            }

            HAL_Delay(1000);
        }
        
        break;
    }

    /* Запрос у устройства вспомогательной информации */
    case APP_STATE_READ_META: {

        const uint8_t* id = NULL;
        size_t id_len = 0;

        /* Прочитать ID продукта */
        int rc = dfu_host_get_id(&id, &id_len);
        if (rc < 0) {
            return;
        }

        LOG_DBG_IF(id_len == 2, "Product ID: %02X %02X", id[0], id[1]);

        /* Прочитать версию загрузчика */
        rc = dfu_host_get_version();
        if (rc < 0) {
            return;
        }

        LOG_DBG("Bootloader version: %d.%d", rc / 10, rc % 10);

        /* Прочитать метаинформацию о прошивке */
        rc = read_fw_meta(&fw_meta);
        if (rc < 0) {
            LOG_ERROR("Read fw meta error: %d", rc);
            app_state = APP_STATE_INITIAL;
            
            /* Ошибка чтения, возможно, произошла из-за выставленной защиты памяти
             * устройства на чтение. Снять защиту чтения памяти на устройстве. */
            dfu_host_readout_unprotect();
            HAL_Delay(1000);
            LOG_DBG("Readout unprotected");
            return;
        }

        LOG_DBG("Firmware size: %lu, CRC: %04X", fw_meta.fw_size, fw_meta.crc16);

        /* Переход в сосотояние валидации памяти устройства */
        app_state = APP_STATE_CHECK_FW_CRC;
        break;
    }

    /* Проверка целостности прошивки на устройстве */
    case APP_STATE_CHECK_FW_CRC: {

        uint32_t data_left = fw_meta.fw_size;
        uint16_t crc = 0xFFFF;
        uint32_t addr = 0x08000000;

        while (data_left != 0) {

            size_t sz = MIN(data_left, 256);

            const uint8_t* rd = NULL;
            uint8_t err_cnt = 0;
            int rc = 0;

            /* Прочитать очередной блок памяти устройства */
            for (uint8_t i = 0; i < 5; ++i) {
                rc = dfu_host_read_memory(addr, &rd, sz);
                if (rc > 0) {
                    err_cnt = 0;
                    break;
                }

                err_cnt += 1;
            }

            /* В процессе чтения блока возникло много ошибок - перезапуск всего автомата */
            if (err_cnt) {
                LOG_ERROR("To many IO errors!");
                app_state = APP_STATE_INITIAL;
                break;
            }

            /* Рассчитать CRC16 (MODBUS) для очередного блока прочитанных данных */
            crc = crc16_reflect(0xA001, crc, rd, rc);

            data_left -= sz;
            addr += sz;
        }

        LOG_DBG("CRC calculated: %04X", crc);

        /* Проверить корректность CRC прошивки */
        if (fw_meta.crc16 != crc) {
            LOG_ERROR("Wrong CRC value");
            app_state = APP_STATE_CHECK_FAILURE;
            break;
        }

        LOG_DBG("CRC match");

        /* Запустить программу на устройстве с начального адреса Flash */
        int rc = dfu_host_go(0x08000000);
        if (rc < 0) {
            LOG_ERROR("Error while starting application: %d", rc);
            app_state = APP_STATE_CHECK_FAILURE;
            break;
        }

        LOG_DBG("Application started");

        app_state = APP_STATE_CHECK_DONE;
        break;
    }

    /* Приложение успешно проверено и запущено */
    case APP_STATE_CHECK_DONE: {
        static bool value = false;
        board_led_write(value = !value);
        HAL_Delay(1000);
        break;
    }

    /* Ошибка во время проверки прошивки */
    case APP_STATE_CHECK_FAILURE: {
        static bool value = false;
        board_led_write(value = !value);
        HAL_Delay(150);
        break;
    }
    }
}

static void app_dispatch_forever(void)
{
    while (1) {
        app_dispatch();
    }
}

int main(void)
{
    /* Начальная инициализация системы */
    board_init();

    dfu_host_init(board_get_serial_handle());

    /* Установить линию BOOT0 внешнего MCU в 1 */
    board_boot0_write(true);

    /* Начать диспетчеризацию автомата основного приложения */
    app_dispatch_forever();

    CODE_UNREACHABLE;
}