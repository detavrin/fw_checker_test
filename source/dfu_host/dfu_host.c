#include <string.h>

#include "dfu_host.h"
#include "core/assert.h"
#include "core/critical_section.h"

/************************* LOG SETTINGS ****************************/

#define LOG_MODULE_PRINTABLE_NAME "DFU"
#define LOG_MODULE_LOG_LEVEL 4U
#define LOG_MODULE_IS_ENABLED (!defined(NDEBUG))
#define LOG_MODULE_IS_TIMESTAMP_ENABLED 1
#define LOG_MODULE_IS_FUNC_NAME_ENABLED 1

#include "logging.h"

/*******************************************************************/

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif /* __GNUC__ */

/* Размер приемного буфера в байтах */
#define CONFIG_DFU_HOST_RX_BUFFER_SIZE     256
/* Тайимаут ожидания прихода ответа от устройства в милисекундах */
#define CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS 1000

/**
 * @brief  Перечисление идентификаторов команд протокола USART bootloader в
 * соответствии с (AN3155).
 */
typedef enum {
    DFU_HOST_CMD_ID_PING              = 0x7F,
    DFU_HOST_CMD_ID_GET               = 0x00,
    DFU_HOST_CMD_ID_GET_VERSION       = 0x01,
    DFU_HOST_CMD_ID_GET_ID            = 0x02,
    DFU_HOST_CMD_ID_READ_MEM          = 0x11,
    DFU_HOST_CMD_ID_GO                = 0x21,
    DFU_HOST_CMD_ID_WRITE_MEM         = 0x31,
    DFU_HOST_CMD_ID_WRITE_EXT_ERASE   = 0x44,
    DFU_HOST_CMD_ID_WRITE_PROTECT     = 0x63,
    DFU_HOST_CMD_ID_WRITE_UNPROTECT   = 0x73,
    DFU_HOST_CMD_ID_READOUT_PROTECT   = 0x82,
    DFU_HOST_CMD_ID_READOUT_UNPROTECT = 0x92,
} cmd_id_t;

/**
 * @brief  Перечисление возможных ответов бутлоадера по USART.
 */
typedef enum {
    DFU_HOST_RESP_ACK  = 0x79,
    DFU_HOST_RESP_NACK = 0x1F,
} resp_value_t;

static UART_HandleTypeDef* huart = NULL;

static uint8_t rcv_buffer[256] = { 0 }; /* Приемный буфер UART */
static size_t  rcv_count       = 0;     /* Текущее количество принятых байт */

static volatile bool rcv_cplt  = false; /* Флаг окончания приема данных */
static volatile int  rcv_err   = 0;     /* Последняя ошибка при приеме данных */

/* Отправить произвольную последовательность данных и дождаться подтверждения */
static int send_data(const uint8_t* buffer, size_t size, uint32_t ack_timeout);

/* Отправить команду и дождаться подтверждения за отведенный таймаут и дождаться подтверждения */
static int send_command(cmd_id_t cmd, uint32_t ack_timeout);

/* Принимать данные до тех пор пока не придет ACK/NACK за отведенный таймаут */
static ssize_t recv(uint32_t timeout);

/* Принимать до тех пор пока не будут получены все данные за отведенный таймаут */
static ssize_t recv_fixed(size_t len, uint32_t timeout);

/* Коллбэк завершения приема данных */
static void rcv_complete_cb(UART_HandleTypeDef* handle);

/* Расчет XOR8 для заданной последовательности байт */
static inline uint8_t calc_xor8(const uint8_t *data, size_t len)
{
    uint8_t result = 0;
    for (size_t s = 0; s < len; ++s) {
        result = (result ^ data[s]) & 0xFF;
    }

    return result;
}

/* Начать прием следующего байта по UART */
static inline int start_rcv_next_byte(void)
{
    if (rcv_count == ARRAY_SIZE(rcv_buffer)) {
        return DFU_HOST_ERR_OVERFLOW;
    }
    
    if(HAL_UART_Receive_IT(huart, rcv_buffer + rcv_count, 1) != HAL_OK) {
        return DFU_HOST_ERR_EIO;
    }
    
    return DFU_HOST_ERR_NONE;
}

static int send_data(const uint8_t* buffer, size_t size, uint32_t rx_timeout_ms)
{
    CHECK(buffer        != NULL, return DFU_HOST_ERR_EINVAL);
    CHECK(size          != 0,    return DFU_HOST_ERR_EINVAL);
    CHECK(rx_timeout_ms > 0,     return DFU_HOST_ERR_EINVAL);
    
    int rc = 0;
    
    //LOG_HEX_ARRAY_DBG("> ", buffer, sz);
    
    /* Отправить команду бутлоадеру */
    HAL_StatusTypeDef result = HAL_UART_Transmit(huart, buffer, size, HAL_MAX_DELAY);
    
    if (result != HAL_OK) {
        LOG_ERROR("Send error: %d", result);
        return DFU_HOST_ERR_EIO;
    }
    
    /* Ожидаем получить ACK или NACK за отведенное время */
    rc = recv_fixed(1, rx_timeout_ms);
    
    if (rc < 0) {
        return rc;
    }
    
    //LOG_HEX_ARRAY_DBG("< ", rcv_buffer, rc);
    
    /* Проверить ответ */
    switch (*rcv_buffer) {
    case DFU_HOST_RESP_ACK:
        rc = DFU_HOST_ERR_NONE;
        break;
        
    case DFU_HOST_RESP_NACK:
        rc = DFU_HOST_ERR_NACK;
        break;
        
    default: 
        rc = DFU_HOST_ERR_WRONG_ANS;
        break;
    }
    
    return rc;
}

static int send_command(cmd_id_t cmd, uint32_t ack_timeout)
{
    uint8_t buffer[2] = { cmd, 0xFF ^ cmd };
    return send_data(buffer, ARRAY_SIZE(buffer), ack_timeout);
}

static ssize_t recv_fixed(size_t len, uint32_t timeout)
{
    ASSERT_NO_MSG(len <= ARRAY_SIZE(rcv_buffer));
    ASSERT_NO_MSG(timeout > 0);
    
    if (HAL_UART_Receive(huart, rcv_buffer, len, timeout) != HAL_OK) {
        return DFU_HOST_ERR_EIO;
    }
    
    return len;
}

static ssize_t recv(uint32_t timeout)
{
    ASSERT_NO_MSG(timeout > 0);
    
    HAL_UART_RegisterCallback(huart, HAL_UART_RX_COMPLETE_CB_ID, rcv_complete_cb);
    // HAL_UART_RegisterCallback(huart, HAL_UART_ERROR_CB_ID, /* TODO */);
    
    rcv_count  = 0;
    rcv_cplt   = false;
    
    /* Начать цепочку приема данных */
    int rc = start_rcv_next_byte();
    
    if (rc < 0) {
        return rc;
    }
    
    uint32_t end_tp = HAL_GetTick() + timeout;
    
    /* Дождаться окончания приема ответного сообщения за отведенное время */
    while (rcv_cplt == false) {
        if(HAL_GetTick() >= end_tp) {
            HAL_UART_AbortReceive_IT(huart);
            return DFU_HOST_ERR_TIMEOUT;
        }
    }
    
    /* Если во время приема возникла ошибка - вернуть ее */
    if (rcv_err != 0) {
        return rcv_err;
    }
    
    // LOG_HEX_ARRAY_DBG("Recv:", rcv_buffer, rcv_count);
    
    /* Сообщение принято нормально - вернуть фактическое количество принятых байт */
    return rcv_count;
}

static void rcv_complete_cb(UART_HandleTypeDef* handle)
{
    uint8_t data = *(rcv_buffer + rcv_count);
    int rc = 0;
    
    switch (data) {
        
    /* В ответ пришел NACK - остановить прием с ошибкой */
    case DFU_HOST_RESP_NACK:
        rcv_err  = DFU_HOST_ERR_NACK;
    
    /* В ответ пришел ACK - остановить прием */
    case DFU_HOST_RESP_ACK:	
        rcv_cplt = true;
        break;
        
    default:
        
        rcv_count += 1;
        
        /* Продолжить прием данных */
        rc = start_rcv_next_byte();
        if(rc < 0) {
            rcv_err  = rc;
            rcv_cplt = true;
        }
        
        break;
    }
}

////////

int dfu_host_init(UART_HandleTypeDef* handle)
{
    ASSERT_NO_MSG(handle != NULL);
    
    huart = handle;	
    
    return 0;
}

int dfu_host_ping(uint32_t timeout)
{
    CHECK(timeout > 0, return DFU_HOST_ERR_EINVAL);
    
    uint8_t data = DFU_HOST_CMD_ID_PING;
    return send_data(&data, sizeof(data), timeout);
}

int dfu_host_get_version(void)
{
    int rc = 0;
    
    /* Отправка команды 01 FE */
    rc = send_command(DFU_HOST_CMD_ID_GET_VERSION, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    /* Принять данные */
    rc = recv(CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if(rc < 0) {
        return rc;
    }
    
    /* Длина ответа известна заранее */
    if (rc != 3) {
        return DFU_HOST_ERR_WRONG_ANS;
    }

    return bcd2bin(*rcv_buffer);
}

int dfu_host_get_id(const uint8_t** id, size_t* id_len)
{
    CHECK(id     != NULL, return DFU_HOST_ERR_EINVAL);
    CHECK(id_len != NULL, return DFU_HOST_ERR_EINVAL);
    
    int rc = 0;
    
    /* Отправка команды 02 FD */
    rc = send_command(DFU_HOST_CMD_ID_GET_ID, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    /* Принять данные */
    rc = recv(CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    if (rc == 0) {
        return DFU_HOST_ERR_WRONG_ANS;
    }
    
    *id     = rcv_buffer + 1;
    *id_len = rc - 1;
    
    return DFU_HOST_ERR_NONE;
}

int dfu_host_read_memory(uint32_t address, const uint8_t** result, size_t len)
{
    CHECK(result != NULL, return DFU_HOST_ERR_EINVAL);
    CHECK(len    != 0,    return DFU_HOST_ERR_EINVAL);
    CHECK(len <= sizeof(rcv_buffer), return DFU_HOST_ERR_EINVAL);

    int rc = 0;
    
    /* Отправка команды 11 EE */
    rc = send_command(DFU_HOST_CMD_ID_READ_MEM, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    uint8_t buff[5] = { 0 };
    
    buff[0] = address >> 24;
    buff[1] = address >> 16;
    buff[2] = address >> 8;
    buff[3] = address;
    buff[4] = calc_xor8(buff, 4);
    
    /* Отправить начальный адрес чтения памяти */
    rc = send_data(buff, 5, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if(rc < 0) {
        return rc;
    }
    
    buff[0] = len - 1;
    buff[1] = 0xFF ^ buff[0];
    
    /* Отправить количество считываемых байт */
    rc = send_data(buff, 2, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    /* Принять содержимое памяти по заданному адресу */
    rc = recv_fixed(len, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    
    if(rc < 0) {
        return rc;
    }
    
    *result = rcv_buffer;
    
    return rc;
}

int dfu_host_write_memory(uint32_t address, const uint8_t* data, size_t len)
{
    CHECK(data != NULL, return DFU_HOST_ERR_EINVAL);
    CHECK(len  != 0,    return DFU_HOST_ERR_EINVAL);
    CHECK(len  <= 256,  return DFU_HOST_ERR_EINVAL);
    
    int rc = 0;
    
    /* Отправка команды 31 СE */
    rc = send_command(DFU_HOST_CMD_ID_WRITE_MEM, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if(rc < 0) {
        return rc;
    }
    
    uint8_t buff[5] = { 0 };
    
    buff[0] = address >> 24;
    buff[1] = address >> 16;
    buff[2] = address >> 8;
    buff[3] = address;
    buff[4] = calc_xor8(buff, 4);
    
    /* Отправить начальный адрес записи данных */
    rc = send_data(buff, 5, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    uint8_t buf[256 + 1];
    
    buf[0] = len - 1;
    memcpy(buf + 1, data, len);
    buf[len + 1] = calc_xor8(buf, len + 1);
    
    /* Отправить блок данных для записи в память устройства */
    rc = send_data(buf, len + 2, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if(rc < 0) {
        return rc;
    }
    
    return len;
}

int dfu_host_go(uint32_t address)
{			
    /* Отправка команды 21 DE */
    int rc = send_command(DFU_HOST_CMD_ID_GO, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    
    uint8_t buff[5] = { 0 };
    
    buff[0] = address >> 24;
    buff[1] = address >> 16;
    buff[2] = address >> 8;
    buff[3] = address;
    buff[4] = calc_xor8(buff, 4);
    
    /* Отправить адрес исполнения программы */
    return send_data(buff, 5, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}

int dfu_host_erase_all(void)
{
	/* Отправка команды 44 BB */
	int rc = send_command(DFU_HOST_CMD_ID_WRITE_EXT_ERASE, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
	if (rc < 0) {
		return rc;
	}
	
	uint8_t buf[3] = { 0 };
	
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = calc_xor8(buf, 2);
	
	/* Отправить спец значение для стирания всей внутренней Flash */
	return send_data(buf, ARRAY_SIZE(buf), CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}

int dfu_host_write_protect_sectors(const uint8_t* sectors, size_t count)
{
	CHECK(sectors != NULL, return DFU_HOST_ERR_EINVAL);
	CHECK(count   != 0,    return DFU_HOST_ERR_EINVAL);
	
	int rc = 0;
	
	/* Отправка команды 63 9C */
	rc = send_command(DFU_HOST_CMD_ID_WRITE_PROTECT, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
	if(rc < 0) {
		return rc;
	}
	
	uint8_t buf[count + 2];
	
	buf[0] = count - 1;
	memcpy(buf + 1, sectors, count);
	buf[count + 1] = calc_xor8(buf, count + 1);
	
	/* Отправить номера секторов для установки защиты на запись */
	return send_data(buf, count + 2, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}

int dfu_host_write_protect_area(uint16_t start, uint16_t end)
{
	CHECK(start <= end, return DFU_HOST_ERR_EINVAL);
	
	const uint8_t len = end - start + 1;
	
	uint8_t sectors[len];
	
	for(uint8_t i = 0; i < len; ++i) {
		sectors[i] = start + i;
	}
	
	return dfu_host_write_protect_sectors(sectors, len);
}

int dfu_host_write_unprotect(void)
{
	/* Отправка команды 73 8C */
	int rc = send_command(DFU_HOST_CMD_ID_WRITE_UNPROTECT, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
	if (rc < 0) {
		return rc;
	}
	
	/* Дождаться ACK */
	return recv(CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}

int dfu_host_readout_protect(void)
{	
	/* Отправка команды 82 7D */
	int rc = send_command(DFU_HOST_CMD_ID_READOUT_PROTECT, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
	if (rc < 0) {
		return rc;
	}
	
	/* Дождаться ACK */
	return recv(CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}

int dfu_host_readout_unprotect(void)
{
	/* Отправка команды 92 6D */
	int rc = send_command(DFU_HOST_CMD_ID_READOUT_UNPROTECT, CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
	if (rc < 0) {
		return rc;
	}
	
	/* Дождаться ACK */
	return recv(CONFIG_DFU_HOST_RECEIVE_TIMEOUT_MS);
}