#ifndef HEADER_H
#define HEADER_H

#include <stdint.h>

/* STM32F4 register-level project: assumes HSI = 16 MHz */

#define RCC_AHB1ENR     (*(volatile uint32_t *)0x40023830U)
#define RCC_APB1ENR     (*(volatile uint32_t *)0x40023840U)
#define RCC_APB2ENR     (*(volatile uint32_t *)0x40023844U)

/* GPIOA */
#define GPIOA_MODER     (*(volatile uint32_t *)0x40020000U)
#define GPIOA_OTYPER    (*(volatile uint32_t *)0x40020004U)
#define GPIOA_PUPDR     (*(volatile uint32_t *)0x4002000CU)
#define GPIOA_IDR       (*(volatile uint32_t *)0x40020010U)
#define GPIOA_ODR       (*(volatile uint32_t *)0x40020014U)
#define GPIOA_AFRL      (*(volatile uint32_t *)0x40020020U)

/* GPIOB */
#define GPIOB_MODER     (*(volatile uint32_t *)0x40020400U)
#define GPIOB_OTYPER    (*(volatile uint32_t *)0x40020404U)
#define GPIOB_PUPDR     (*(volatile uint32_t *)0x4002040CU)
#define GPIOB_ODR       (*(volatile uint32_t *)0x40020414U)
#define GPIOB_AFRL      (*(volatile uint32_t *)0x40020420U)

/* GPIOD */
#define GPIOD_MODER     (*(volatile uint32_t *)0x40020C00U)
#define GPIOD_ODR       (*(volatile uint32_t *)0x40020C14U)

/* GPIOE */
#define GPIOE_MODER     (*(volatile uint32_t *)0x40021000U)
#define GPIOE_ODR       (*(volatile uint32_t *)0x40021014U)

/* USART2 */
#define USART2_SR       (*(volatile uint32_t *)0x40004400U)
#define USART2_DR       (*(volatile uint32_t *)0x40004404U)
#define USART2_BRR      (*(volatile uint32_t *)0x40004408U)
#define USART2_CR1      (*(volatile uint32_t *)0x4000440CU)
#define USART2_CR2      (*(volatile uint32_t *)0x40004410U)
#define USART2_CR3      (*(volatile uint32_t *)0x40004414U)

/* TIM2 */
#define TIM2_CR1        (*(volatile uint32_t *)0x40000000U)
#define TIM2_EGR        (*(volatile uint32_t *)0x40000014U)
#define TIM2_CCMR1      (*(volatile uint32_t *)0x40000018U)
#define TIM2_CCER       (*(volatile uint32_t *)0x40000020U)
#define TIM2_PSC        (*(volatile uint32_t *)0x40000028U)
#define TIM2_ARR        (*(volatile uint32_t *)0x4000002CU)
#define TIM2_CCR1       (*(volatile uint32_t *)0x40000034U)
#define TIM2_CCR2       (*(volatile uint32_t *)0x40000038U)

/* ADC1 */
#define ADC1_SR         (*(volatile uint32_t *)0x40012000U)
#define ADC1_CR1        (*(volatile uint32_t *)0x40012004U)
#define ADC1_CR2        (*(volatile uint32_t *)0x40012008U)
#define ADC1_SMPR2      (*(volatile uint32_t *)0x40012010U)
#define ADC1_SQR1       (*(volatile uint32_t *)0x4001202CU)
#define ADC1_SQR3       (*(volatile uint32_t *)0x40012034U)
#define ADC1_DR         (*(volatile uint32_t *)0x4001204CU)

/* SPI1 */
#define SPI1_CR1        (*(volatile uint32_t *)0x40013000U)
#define SPI1_CR2        (*(volatile uint32_t *)0x40013004U)
#define SPI1_SR         (*(volatile uint32_t *)0x40013008U)
#define SPI1_DR         (*(volatile uint32_t *)0x4001300CU)

/* I2C1 */
#define I2C1_CR1        (*(volatile uint32_t *)0x40005400U)
#define I2C1_CR2        (*(volatile uint32_t *)0x40005404U)
#define I2C1_DR         (*(volatile uint32_t *)0x40005410U)
#define I2C1_SR1        (*(volatile uint32_t *)0x40005414U)
#define I2C1_SR2        (*(volatile uint32_t *)0x40005418U)
#define I2C1_CCR        (*(volatile uint32_t *)0x4000541CU)
#define I2C1_TRISE      (*(volatile uint32_t *)0x40005420U)

/* Motor driver direction pins on GPIOB */
#define LEFT_IN1        0U
#define LEFT_IN2        1U
#define RIGHT_IN1       10U
#define RIGHT_IN2       11U

/* On STM32F4 Discovery, user LEDs are PD12-PD15 */
#define RIGHT_LED       12U
#define LEFT_LED        13U
#define HEADLIGHT_LED   14U

#define LCD_ADDRESS     0x27U
#define ACCEL_CS_PIN    3U

extern volatile uint32_t system_ms;

void System_Init(void);
void Delay_ms(uint32_t ms);
void UART_Init(void);
int UART_ReadChar(void);

void Motor_Init(void);
void Motor_SetSpeed(uint8_t left, uint8_t right);
void Motor_SetDirection(uint8_t direction);
void Motor_AllStop(void);

void ADC_Init(void);
uint16_t ADC_ReadLDR(void);

void SPI1_Init(void);
uint8_t LIS302DL_ReadRegister(uint8_t reg);
void LIS302DL_WriteRegister(uint8_t reg, uint8_t value);
void LIS302DL_Init(void);
void LIS302DL_ReadXYZ(int8_t *x, int8_t *y, int8_t *z);

void I2C1_Init(void);
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_PrintNumber(uint32_t value);

void StatusLED_Init(void);
void StatusLED_Set(uint8_t pin, uint8_t state);

#endif
