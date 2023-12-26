#include <errno.h>
#include <stdio.h>

#include "board.h"
#include "core/assert.h"

#define LED_PIN_PORT GPIOA
#define LED_PIN_PIN GPIO_PIN_5
#define RST_LINE_PORT GPIOB
#define RST_LINE_PIN GPIO_PIN_0
#define BOOT_LINE_PORT GPIOB
#define BOOT_LINE_PIN GPIO_PIN_1

/* Адрес чтения параметров проверяемой прошивки */
#ifndef CONFIG_FW_META_ADDR
#define CONFIG_FW_META_ADDR ((uint32_t)0x0803F800) /* 127-th page */
#endif /* CONFIG_FW_META_ADDR */


UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

void Error_Handler(void);

static void dfu_uart_init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart1);
}

#ifdef DEBUG
static void log_uart_init(void)
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

    /*Configure GPIO pins : PB0 (RST) */
    GPIO_InitStruct.Pin = RST_LINE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RST_LINE_PORT, &GPIO_InitStruct);

    /*Configure GPIO pins : PB1 (BOOT) */
    GPIO_InitStruct.Pin = BOOT_LINE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOOT_LINE_PORT, &GPIO_InitStruct);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        ASSERT_NO_MSG(0);
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

void board_init(void)
{
    HAL_Init();
    SystemClock_Config();
        
    dfu_uart_init();
    gpio_init();

#ifdef DEBUG
    log_uart_init();
#endif /* DEBUG */
}

UART_HandleTypeDef* board_get_serial_handle(void) 
{ 
    return &huart1; 
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

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1);
}
