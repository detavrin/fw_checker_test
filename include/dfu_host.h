#ifndef INCLUDE_DFU_HOST_H__
#define INCLUDE_DFU_HOST_H__

#include "cmsis.h"

/**
 *  @brief  Перечисление кодов ошибок модуля DFU_HOST.
 */
typedef enum {
	DFU_HOST_ERR_NONE      = 0,      /* OK */
	DFU_HOST_ERR_EIO       = -1000,  /* Ошибка ввода/вывода */
	DFU_HOST_ERR_EINVAL    = -1001,  /* Передача неверных значений аргументов функции */
	DFU_HOST_ERR_NACK      = -1002,  /* Устройство ответило NACK */
	DFU_HOST_ERR_TIMEOUT   = -1003,  /* Таймаут ожмдания ответа от устройства */
	DFU_HOST_ERR_WRONG_ANS = -1004,  /* Неверный формат ответа от устройства */
	DFU_HOST_ERR_OVERFLOW  = -1005,  /* Переполнение приемного буфера */
} dfu_host_err_t;

/**
 *  @brief Инициализация модуля dfu_host.
 *  Должна вызываться до использования остальных функций из API.
 *
 *  @param handle  Объект UART для взаимодействия с подчиненным устройством.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_init(UART_HandleTypeDef* handle);

/**
 *  @brief Отправить начальный пакет Ping подчиненному устройству.
 *
 *  @param timeout  Длительность ожидания ответа от устройства в миллисекундах.
 *
 *  @return 0 - в случае успеха (устройство ответило ACK), 
 *          код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_ping(uint32_t timeout);

/**
 *  @brief  Запросить у устройства версию бутлоадера.
 *
 *  @return Версия прошивки в виде 1 байтового числа вида 0xNM, где
 *  N - старшая тетрада содержащая двоично-десятичное значение мажорной версии,
 *  M - младшая тетрада содержащая двоично-десятичное значение минорной версии,
 *  Полный вид версии: N.M.
 *
 *  В случае ошибки возвращается код ошибки dfu_host_err_t.
 */
int dfu_host_get_version(void);

/**
 *  @brief  Запросить у устройства идентификатор продукта.
 *  
 *  @param id  Результирующий буфер содержащий последовательность байт идентификатора продукта.
 *  @param id_len  Результирующая длина буфера @p id.
 *  
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_get_id(const uint8_t** id, size_t* id_len);

/** 
 *  @brief  Прочитать непрерывный блок памяти устройства начиная с заданного адреса.
 *  
 *  @param address  Начальный адрес памяти устройства для чтения.
 *  @param result   Указатель на результирующий буфер, хранящий содержимое прочитанной памяти.
 *  @param len      Длина считываемого блока памяти в байтах.
 *  
 *  @return  Значение количества прочитанных байт из памяти устройства,
 *           код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_read_memory(uint32_t address, const uint8_t** result, size_t len);

/**
 *  @brief Записать непрерывный блок данных в память устройства по заданному адресу.
 *
 *  @param address  Начальный адрес памяти устройства для записи блока данных.
 *  @param data     Блок записываемых данных.
 *  @param len      Длина блока записываемых данных в байтах.
 *
 *  @return  Значение количества записанных байт в память устройства,
 *           код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_write_memory(uint32_t address, const uint8_t* data, size_t len);

/**
 *  @brief  Запуск приложения на устройстве используя заданный адрес начала программы.
 *
 *  @brief address  Начальный адрес программы.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_go(uint32_t address);

/**
 *  @brief  Очистить содержимое всей памяти устройства.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_erase_all(void);

/**
 *  @brief  Установить защиту от перезаписи для всех заданных секторов памяти устройства.
 *
 *  @param sectors  Массив перечисления номеров секторов, для которых нужно
 *  установить защиту от перезаписи.
 *  @param count  Размер массива перечисления секторов.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_write_protect_sectors(const uint8_t* sectors, size_t count);

/**
 *  @brief  Установить защиту от перезаписи для заданной области памяти устройства.
 *
 *  @param start  Начальный номер сектора.
 *  @param end    Последний номер сектора.
 *
 *  В пределах данного открытого диапазона память будет защищена от перезаписи.
 *  @p start должен быть меньше или равен @p end.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_write_protect_area(uint16_t start, uint16_t end);

/**
 *  @brief  Снять защиту от перезаписи для всей памяти устройства.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_write_unprotect(void);

/**
 *  @brief  Установить защиту от чтения всей памяти устройства.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_readout_protect(void);

/**
 *  @brief  Снять защиту от чтения всей памяти устройства.
 *
 *  @return  0 - в случае успеха, код ошибки dfu_host_err_t в противном случае.
 */
int dfu_host_readout_unprotect(void);

#endif /* !INCLUDE_DFU_HOST_H__ */