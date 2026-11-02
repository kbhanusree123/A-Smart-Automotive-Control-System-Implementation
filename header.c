#include "header.h"
#include <stdio.h>

/* ---------- Small GPIO helpers ---------- */
static void GPIO_Mode(volatile uint32_t *moder, uint8_t pin, uint8_t mode)
{
    *moder &= ~(3U << (pin * 2U));
    *moder |= ((uint32_t)mode << (pin * 2U));
}

static void GPIO_AF(volatile uint32_t *afrl, uint8_t pin, uint8_t af)
{
    *afrl &= ~(0xFU << (pin * 4U));
    *afrl |= ((uint32_t)af << (pin * 4U));
}

static void GPIO_Write(volatile uint32_t *odr, uint8_t pin, uint8_t state)
{
    if (state) *odr |= (1U << pin);
    else       *odr &= ~(1U << pin);
}

/* ---------- SysTick: 1 ms tick ---------- */
volatile uint32_t system_ms = 0U;

void SysTick_Handler(void)
{
    system_ms++;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = system_ms;
    while ((uint32_t)(system_ms - start) < ms)
        __asm volatile ("nop");
}

/* ---------- System initialization ---------- */
void System_Init(void)
{
    /* GPIOA, GPIOB, GPIOD, GPIOE clocks */
    RCC_AHB1ENR |= (1U << 0) | (1U << 1) | (1U << 3) | (1U << 4);

    /* TIM2, USART2, I2C1 clocks */
    RCC_APB1ENR |= (1U << 0) | (1U << 17) | (1U << 21);

    /* ADC1 and SPI1 clocks */
    RCC_APB2ENR |= (1U << 8) | (1U << 12);
    (void)RCC_AHB1ENR;

    /* This setting assumes the system clock is 16 MHz (HSI). */
    *(volatile uint32_t *)0xE000E014U = 15999U; /* LOAD */
    *(volatile uint32_t *)0xE000E018U = 0U;     /* VAL */
    *(volatile uint32_t *)0xE000E010U = 7U;     /* enable, interrupt, HCLK */

    UART_Init();
    Motor_Init();
    ADC_Init();
    SPI1_Init();
    I2C1_Init();
    StatusLED_Init();
    LCD_Init();
}

/* ---------- UART2: PA2 TX, PA3 RX, 9600 baud ---------- */
void UART_Init(void)
{
    GPIO_Mode(&GPIOA_MODER, 2U, 2U);
    GPIO_Mode(&GPIOA_MODER, 3U, 2U);
    GPIO_AF(&GPIOA_AFRL, 2U, 7U);
    GPIO_AF(&GPIOA_AFRL, 3U, 7U);

    USART2_BRR = 0x0683U; /* 16 MHz / 9600 */
    USART2_CR1 = (1U << 13) | (1U << 3) | (1U << 2);
    USART2_CR2 = 0U;
    USART2_CR3 = 0U;
}

int UART_ReadChar(void)
{
    if (USART2_SR & (1U << 5))
        return (int)(USART2_DR & 0xFFU);
    return -1;
}

/* ---------- Motor PWM: TIM2 CH1 PA0, CH2 PA1 ---------- */
void Motor_Init(void)
{
    GPIO_Mode(&GPIOA_MODER, 0U, 2U);
    GPIO_Mode(&GPIOA_MODER, 1U, 2U);
    GPIO_AF(&GPIOA_AFRL, 0U, 1U);
    GPIO_AF(&GPIOA_AFRL, 1U, 1U);

    GPIO_Mode(&GPIOB_MODER, LEFT_IN1, 1U);
    GPIO_Mode(&GPIOB_MODER, LEFT_IN2, 1U);
    GPIO_Mode(&GPIOB_MODER, RIGHT_IN1, 1U);
    GPIO_Mode(&GPIOB_MODER, RIGHT_IN2, 1U);

    TIM2_PSC = 15U;       /* 16 MHz / 16 = 1 MHz */
    TIM2_ARR = 999U;      /* 1 kHz PWM */
    TIM2_CCMR1 = (6U << 4) | (1U << 3) |
                 (6U << 12) | (1U << 11);
    TIM2_CCR1 = 0U;
    TIM2_CCR2 = 0U;
    TIM2_CCER = (1U << 0) | (1U << 4);
    TIM2_EGR = 1U;
    TIM2_CR1 = (1U << 7) | 1U;
    Motor_AllStop();
}

void Motor_SetSpeed(uint8_t left, uint8_t right)
{
    if (left > 100U) left = 100U;
    if (right > 100U) right = 100U;
    TIM2_CCR1 = (uint32_t)left * 10U;
    TIM2_CCR2 = (uint32_t)right * 10U;
}

void Motor_SetDirection(uint8_t direction)
{
    switch (direction)
    {
        case 1U: /* Forward */
            GPIO_Write(&GPIOB_ODR, LEFT_IN1, 1U);
            GPIO_Write(&GPIOB_ODR, LEFT_IN2, 0U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN1, 1U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN2, 0U);
            break;
        case 2U: /* Backward */
            GPIO_Write(&GPIOB_ODR, LEFT_IN1, 0U);
            GPIO_Write(&GPIOB_ODR, LEFT_IN2, 1U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN1, 0U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN2, 1U);
            break;
        case 3U: /* Right */
            GPIO_Write(&GPIOB_ODR, LEFT_IN1, 1U);
            GPIO_Write(&GPIOB_ODR, LEFT_IN2, 0U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN1, 0U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN2, 0U);
            break;
        case 4U: /* Left */
            GPIO_Write(&GPIOB_ODR, LEFT_IN1, 0U);
            GPIO_Write(&GPIOB_ODR, LEFT_IN2, 0U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN1, 1U);
            GPIO_Write(&GPIOB_ODR, RIGHT_IN2, 0U);
            break;
        default:
            Motor_AllStop();
            break;
    }
}

void Motor_AllStop(void)
{
    TIM2_CCR1 = 0U;
    TIM2_CCR2 = 0U;
    GPIO_Write(&GPIOB_ODR, LEFT_IN1, 0U);
    GPIO_Write(&GPIOB_ODR, LEFT_IN2, 0U);
    GPIO_Write(&GPIOB_ODR, RIGHT_IN1, 0U);
    GPIO_Write(&GPIOB_ODR, RIGHT_IN2, 0U);
}

/* ---------- ADC1 channel 4 on PA4 for LDR ---------- */
void ADC_Init(void)
{
    GPIO_Mode(&GPIOA_MODER, 4U, 3U);
    GPIOA_PUPDR &= ~(3U << 8);

    ADC1_CR1 = 0U;
    ADC1_CR2 = 0U;
    ADC1_SMPR2 &= ~(7U << 12);
    ADC1_SMPR2 |= (4U << 12);
    ADC1_SQR1 = 0U;
    ADC1_SQR3 = 4U;
    ADC1_CR2 |= 1U;
}

uint16_t ADC_ReadLDR(void)
{
    uint32_t timeout = 1000000U;
    ADC1_SR = 0U;
    ADC1_CR2 |= (1U << 30);
    while (!(ADC1_SR & (1U << 1)))
    {
        if (--timeout == 0U) return 0U;
    }
    return (uint16_t)(ADC1_DR & 0x0FFFU);
}

/* ---------- SPI1 for LIS302DL: PA5/PA6/PA7, PE3 CS ---------- */
void SPI1_Init(void)
{
    GPIO_Mode(&GPIOA_MODER, 5U, 2U);
    GPIO_Mode(&GPIOA_MODER, 6U, 2U);
    GPIO_Mode(&GPIOA_MODER, 7U, 2U);
    GPIO_AF(&GPIOA_AFRL, 5U, 5U);
    GPIO_AF(&GPIOA_AFRL, 6U, 5U);
    GPIO_AF(&GPIOA_AFRL, 7U, 5U);

    GPIO_Mode(&GPIOE_MODER, ACCEL_CS_PIN, 1U);
    GPIO_Write(&GPIOE_ODR, ACCEL_CS_PIN, 1U);

    /* SPI mode 3, master, software NSS, prescaler /16 */
    SPI1_CR1 = (1U << 2) | (1U << 8) | (1U << 9) |
               (1U << 3) | (1U << 1) | (1U << 0);
    SPI1_CR2 = 0U;
    SPI1_CR1 |= (1U << 6);
}

static uint8_t SPI1_Transfer(uint8_t data)
{
    uint32_t timeout = 100000U;
    while (!(SPI1_SR & (1U << 1)))
        if (--timeout == 0U) return 0U;

    *(volatile uint8_t *)&SPI1_DR = data;

    timeout = 100000U;
    while (!(SPI1_SR & (1U << 0)))
        if (--timeout == 0U) return 0U;

    return *(volatile uint8_t *)&SPI1_DR;
}

uint8_t LIS302DL_ReadRegister(uint8_t reg)
{
    uint8_t value;
    GPIO_Write(&GPIOE_ODR, ACCEL_CS_PIN, 0U);
    (void)SPI1_Transfer((uint8_t)(reg | 0x80U));
    value = SPI1_Transfer(0x00U);
    GPIO_Write(&GPIOE_ODR, ACCEL_CS_PIN, 1U);
    return value;
}

void LIS302DL_WriteRegister(uint8_t reg, uint8_t value)
{
    GPIO_Write(&GPIOE_ODR, ACCEL_CS_PIN, 0U);
    (void)SPI1_Transfer((uint8_t)(reg & 0x3FU));
    (void)SPI1_Transfer(value);
    GPIO_Write(&GPIOE_ODR, ACCEL_CS_PIN, 1U);
}

void LIS302DL_Init(void)
{
    LIS302DL_WriteRegister(0x20U, 0x47U);
    Delay_ms(10U);
}

void LIS302DL_ReadXYZ(int8_t *x, int8_t *y, int8_t *z)
{
    *x = (int8_t)LIS302DL_ReadRegister(0x29U);
    *y = (int8_t)LIS302DL_ReadRegister(0x2BU);
    *z = (int8_t)LIS302DL_ReadRegister(0x2DU);
}

/* ---------- I2C1 PB6=SCL, PB7=SDA, 100 kHz ---------- */
void I2C1_Init(void)
{
    GPIO_Mode(&GPIOB_MODER, 6U, 2U);
    GPIO_Mode(&GPIOB_MODER, 7U, 2U);
    GPIO_AF(&GPIOB_AFRL, 6U, 4U);
    GPIO_AF(&GPIOB_AFRL, 7U, 4U);

    GPIOB_OTYPER |= (1U << 6) | (1U << 7);
    GPIOB_PUPDR &= ~((3U << 12) | (3U << 14));
    GPIOB_PUPDR |=  ((1U << 12) | (1U << 14));

    I2C1_CR1 = (1U << 15);
    I2C1_CR1 = 0U;
    I2C1_CR2 = 16U;
    I2C1_CCR = 80U;
    I2C1_TRISE = 17U;
    I2C1_CR1 = 1U;
}

static int I2C_WriteByte(uint8_t data)
{
    uint32_t timeout = 100000U;

    while (I2C1_SR2 & (1U << 1))
        if (--timeout == 0U) return 0;

    I2C1_CR1 |= (1U << 8); /* START */
    timeout = 100000U;
    while (!(I2C1_SR1 & 1U))
        if (--timeout == 0U) return 0;

    I2C1_DR = (uint32_t)(LCD_ADDRESS << 1);
    timeout = 100000U;
    while (!(I2C1_SR1 & (1U << 1)))
    {
        if (I2C1_SR1 & (1U << 10))
        {
            (void)I2C1_SR1;
            I2C1_CR1 |= (1U << 9);
            return 0;
        }
        if (--timeout == 0U)
        {
            I2C1_CR1 |= (1U << 9);
            return 0;
        }
    }

    (void)I2C1_SR1;
    (void)I2C1_SR2;

    I2C1_DR = data;
    timeout = 100000U;
    while (!(I2C1_SR1 & (1U << 2)))
    {
        if (I2C1_SR1 & (1U << 10))
        {
            (void)I2C1_SR1;
            I2C1_CR1 |= (1U << 9);
            return 0;
        }
        if (--timeout == 0U)
        {
            I2C1_CR1 |= (1U << 9);
            return 0;
        }
    }

    I2C1_CR1 |= (1U << 9); /* STOP */
    return 1;
}

/* ---------- HD44780 LCD with PCF8574 I2C backpack ---------- */
static void LCD_ExpanderWrite(uint8_t data)
{
    /* Common backpack mapping: P3=backlight, P2=EN, P0=RS */
    (void)I2C_WriteByte((uint8_t)(data | 0x08U));
}

static void LCD_PulseEnable(uint8_t data)
{
    LCD_ExpanderWrite((uint8_t)(data | 0x04U));
    Delay_ms(1U);
    LCD_ExpanderWrite((uint8_t)(data & (uint8_t)~0x04U));
    Delay_ms(1U);
}

static void LCD_SendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (uint8_t)(nibble & 0xF0U);
    if (rs) data |= 0x01U;
    LCD_ExpanderWrite(data);
    LCD_PulseEnable(data);
}

static void LCD_Send(uint8_t value, uint8_t rs)
{
    LCD_SendNibble((uint8_t)(value & 0xF0U), rs);
    LCD_SendNibble((uint8_t)((value << 4) & 0xF0U), rs);
}

static void LCD_Command(uint8_t command) { LCD_Send(command, 0U); }
static void LCD_Data(uint8_t data)       { LCD_Send(data, 1U); }

void LCD_Init(void)
{
    Delay_ms(50U);
    LCD_SendNibble(0x30U, 0U);
    Delay_ms(5U);
    LCD_SendNibble(0x30U, 0U);
    Delay_ms(2U);
    LCD_SendNibble(0x30U, 0U);
    Delay_ms(2U);
    LCD_SendNibble(0x20U, 0U);
    Delay_ms(2U);
    LCD_Command(0x28U); /* 4-bit, 2-line mode */
    LCD_Command(0x0CU); /* display on, cursor off */
    LCD_Command(0x06U); /* increment cursor */
    LCD_Clear();
}

void LCD_Clear(void)
{
    LCD_Command(0x01U);
    Delay_ms(2U);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;
    switch (row)
    {
        case 0U: address = 0x00U; break;
        case 1U: address = 0x40U; break;
        case 2U: address = 0x14U; break;
        case 3U: address = 0x54U; break;
        default: address = 0x00U; break;
    }
    LCD_Command((uint8_t)(0x80U | (address + col)));
}

void LCD_Print(const char *str)
{
    while (*str != '\0')
    {
        LCD_Data((uint8_t)*str);
        str++;
    }
}

void LCD_PrintNumber(uint32_t value)
{
    char buffer[11];
    (void)snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)value);
    LCD_Print(buffer);
}

/* ---------- Status LEDs ---------- */
void StatusLED_Init(void)
{
    GPIO_Mode(&GPIOD_MODER, RIGHT_LED, 1U);
    GPIO_Mode(&GPIOD_MODER, LEFT_LED, 1U);
    GPIO_Mode(&GPIOD_MODER, HEADLIGHT_LED, 1U);
    StatusLED_Set(RIGHT_LED, 0U);
    StatusLED_Set(LEFT_LED, 0U);
    StatusLED_Set(HEADLIGHT_LED, 0U);
}

void StatusLED_Set(uint8_t pin, uint8_t state)
{
    GPIO_Write(&GPIOD_ODR, pin, state);
}
