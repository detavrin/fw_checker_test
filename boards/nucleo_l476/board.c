#include <errno.h>
#include <stdio.h>

#include "board.h"

#define LED_PIN_PORT GPIOA
#define LED_PIN_PIN GPIO_PIN_5
#define RST_LINE_PORT GPIOB
#define RST_LINE_PIN GPIO_PIN_3
#define BOOT_LINE_PORT GPIOB
#define BOOT_LINE_PIN GPIO_PIN_5

/* Адрес чтения параметров проверяемой прошивки */
#ifndef CONFIG_FW_META_ADDR
#define CONFIG_FW_META_ADDR ((uint32_t)0x0803F800) /* 127 page */
#endif /* CONFIG_FW_META_ADDR */

extern void SetSysClock(void);

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;

static void dfu_uart_init(void)
{
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_9B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_EVEN;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&hlpuart1);
}

#ifdef DEBUG
static void log_usart_init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart2);
}

int _write(int fd, char* ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

#endif /* DEBUG */

static void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_PIN_PORT, LED_PIN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RST_LINE_PORT, RST_LINE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BOOT_LINE_PORT, BOOT_LINE_PIN, GPIO_PIN_RESET);

    /*Configure GPIO pin : PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PB3 (RST) */
    GPIO_InitStruct.Pin = RST_LINE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RST_LINE_PORT, &GPIO_InitStruct);

    /*Configure GPIO pins : PB5 (BOOT) */
    GPIO_InitStruct.Pin = BOOT_LINE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOOT_LINE_PORT, &GPIO_InitStruct);
}

void board_init(void)
{
    HAL_Init();
    SetSysClock();
        
    dfu_uart_init();
    gpio_init();

#ifdef DEBUG
    log_usart_init();
#endif /* DEBUG */
}

UART_HandleTypeDef* board_get_serial_handle(void) 
{ 
    return &hlpuart1; 
}

void board_led_write(bool value)
{
    HAL_GPIO_WritePin(LED_PIN_PORT, LED_PIN_PIN, (GPIO_PinState)value);
}

void board_reset_write(bool value)
{
    HAL_GPIO_WritePin(RST_LINE_PORT, RST_LINE_PIN, (GPIO_PinState)value);
}

void board_boot0_write(bool value)
{
    HAL_GPIO_WritePin(BOOT_LINE_PORT, BOOT_LINE_PIN, (GPIO_PinState)value);
}

uint32_t board_get_fw_meta_addr(void) 
{ 
    return CONFIG_FW_META_ADDR; 
}
