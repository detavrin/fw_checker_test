#ifndef BOARD_H__
#define BOARD_H__

#include <stdbool.h>

#include "cmsis.h"

/**
 *  @brief  Инициализация системы и периферии MCU.
 **/
void board_init(void);

/**
 *  @brief  Получить управляющий объект UART для взаимодействия с бутлоадером.
 *  
 *  @return Указатель на инициализировнный объект UART.
 **/
UART_HandleTypeDef* board_get_serial_handle(void);

/**
 *  @brief  Получить начальный адрес памяти размещения структуры метаинформации
 *  прошивки подчиненного устройства.
 *  
 *  @return  Начальный адрес структуры метаинформации прошивки.
 **/
uint32_t board_get_fw_meta_addr(void);

/**
 *  @brief  Управление выводом статусного светодиода.
 *  
 *  @param  value  true - включить светодиод, false - выключить светодиод.
 **/
void board_led_write(bool value);

/**
 *  @brief  Управление линией сброса подчиненного устройства.
 *
 *  @param  value  true - отпустить линию RST, false - подтянуть вывод к GND.
 **/
void board_reset_write(bool value);

/**
 *  @brief  Управление линией BOOT0 подчиненного устройства.
 *
 *  @param  value  true - линия BOOT0 в 1, линия BOOT0 в 0.
 **/
void board_boot0_write(bool value);

#endif /* !BOARD_H__ */