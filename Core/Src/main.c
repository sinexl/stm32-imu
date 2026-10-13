#include "main.h"
#include <stdbool.h>
#include <string.h>
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_uart.h"
#include "stm32g4xx_hal_i2c.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_nucleo.h"
#include "stm32g474xx.h"
#define MPU6050_IMPL
#include "MPU6050.h"
UART_HandleTypeDef huart2;

// updated through BSP_PB_Callback()
bool button_pressed = false;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(UART_HandleTypeDef*);
void panic(const char* msg);

void I2C1_Init(I2C_HandleTypeDef* handle) {
    *handle = (I2C_HandleTypeDef) {
        .Instance = I2C1, 
        .Init = {
            .Timing = 0x40B285C2,
            .OwnAddress1 = 0x00,
            .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
            .DualAddressMode = I2C_DUALADDRESS_DISABLE,
            .OwnAddress2 = 0x00, 
            .OwnAddress2Masks = I2C_OA2_NOMASK, 
            .GeneralCallMode = I2C_GENERALCALL_DISABLE,
            .NoStretchMode = I2C_NOSTRETCH_DISABLE,
        }
    }; 
}


void MPU6050_print(MPU6050_Measurements data) {
    printf(
        "Accelerometer = {\r\n"
        "  x = %fg,\r\n"
        "  y = %fg,\r\n"
        "  z = %fg.\r\n"
        "}\r\n",

        /* "x: %f, y: %f, z: %f\r\n", */
        /* "%f, %f, %f\r\n", */
        data.x, data.y, data.z
    );
}


int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();


    // GPIOA(PIN10) Init
    GPIO_InitTypeDef gpio = 
    {
        .Pin = GPIO_PIN_10, 
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_HIGH, 
    };
    HAL_GPIO_Init(GPIOA, &gpio);

    // GPIOA(USART2)  Init
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF7_USART2; 
    HAL_GPIO_Init(GPIOA, &gpio);

    // USART2 Init
    MX_USART2_UART_Init(&huart2);

    // GPIOA(I2C1) Init
    // PB7 -  SDA1
    // PA15 - SCL1
    gpio = (GPIO_InitTypeDef) {
        .Pin = GPIO_PIN_15, // PA15 (SCL1)
        .Mode = GPIO_MODE_AF_OD, 
        .Pull = GPIO_NOPULL, // pull-up is done by external resistors.
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF4_I2C1, 
    }; 
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_7; // PB7 (SDA1)
    HAL_GPIO_Init(GPIOB, &gpio);
    
    // I2C1 Init.
    I2C_HandleTypeDef i2c1 = {0};
    I2C1_Init(&i2c1);
    if (HAL_I2C_Init(&i2c1) != HAL_OK) {
        panic("Could not initialize I2C1\r\n");
    }
    HAL_I2CEx_ConfigAnalogFilter(&i2c1, I2C_ANALOGFILTER_ENABLE);

    // MPU6050 Init
    MPU6050_Handle handle = {
        .i2c = &i2c1,
        .address_lsb = false,
    };
    if (!MPU6050_init(handle)) panic("Could not initialize MPU6050\r\n");
    else printf("Initialized MPU6050 successfully\r\n");
    

    BSP_LED_Init(LED_GREEN);
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
    MPU6050_Measurements gyroscope;
    MPU6050_Measurements accelerometer;
    bool output_data = false; 
    while (1)
    {
        if (button_pressed) {
            button_pressed = false;
            BSP_LED_Toggle(LED_GREEN);
            output_data = !output_data;
            
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, output_data ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        if (output_data) {
            if (!MPU6050_read_gyro(handle, &gyroscope)) panic("Could not read MPU6050 gyro measurements.\r\n");
            if (!MPU6050_read_accelerometer(handle, &accelerometer)) panic("Could not read MPU6050 accelerometer measurements.\r\n");
            // Output comma-separated motion data to UART2 (st-link uart-to-usb)
            printf("%f, %f, %f, %f, %f, %f\r\n",
                   accelerometer.x, accelerometer.y, accelerometer.z,
                   gyroscope.x, gyroscope.y, gyroscope.z);
        }
        /* MPU6050_print(accel); */
    }
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (const uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void BSP_PB_Callback(Button_TypeDef Button) {
    if (Button == BUTTON_USER) {
        button_pressed = true;
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
        |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(UART_HandleTypeDef* huart)
{

    huart->Instance = USART2;
    huart->Init.BaudRate = 115200;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;

    // Experiment with other configuration
    /* huart->Init.BaudRate = 230400; */
    /* huart->Init.WordLength = UART_WORDLENGTH_9B; */
    /* huart->Init.StopBits = UART_STOPBITS_1; */
    /* huart->Init.Parity = UART_PARITY_EVEN; */

    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

void panic(const char* msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    Error_Handler();
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
    printf("%s:%d: ASSERTION FAILED");
    Error_Handler();
}
#endif /* USE_FULL_ASSERT */
