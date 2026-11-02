#include "header.h"
#include <stdint.h>

/* Bluetooth commands */
#define CMD_POWER_ON    'S'
#define CMD_POWER_OFF   'd'
#define CMD_FORWARD     '2'
#define CMD_BACKWARD    '4'
#define CMD_RIGHT       '1'
#define CMD_LEFT        '3'
#define CMD_STOP        '0'

/* Direction definitions */
#define DIR_STOP        0U
#define DIR_FORWARD     1U
#define DIR_BACKWARD    2U
#define DIR_RIGHT       3U
#define DIR_LEFT        4U

/* Project settings */
#define MOTOR_SPEED              60U
#define SLOW_SPEED               20U
#define SPEED_STEP               5U

#define LDR_THRESHOLD            1800U
#define MAGNITUDE_LOW_THRESHOLD  65U
#define MAGNITUDE_HIGH_THRESHOLD 68U

/* Vehicle status */
static uint8_t vehicle_power = 0U;
static uint8_t requested_direction = DIR_STOP;
static uint8_t current_direction = DIR_STOP;
static uint8_t current_speed = 0U;

static uint8_t left_indicator = 0U;
static uint8_t right_indicator = 0U;
static uint8_t indicator_phase = 0U;
static uint8_t accident_warning = 0U;

/* Sensor readings */
static uint16_t ldr_value = 0U;

static int8_t accel_x = 0;
static int8_t accel_y = 0;
static int8_t accel_z = 0;

static uint32_t accel_magnitude = 0U;

/* Update timestamps */
static uint32_t last_motor_update = 0U;
static uint32_t last_sensor_update = 0U;
static uint32_t last_lcd_update = 0U;
static uint32_t last_indicator_update = 0U;


/* Integer square root */
static uint32_t IntegerSqrt(uint32_t n)
{
    uint32_t result = 0U;
    uint32_t bit = 1UL << 30;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0U)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }

        bit >>= 2;
    }

    return result;
}


/* Handle Bluetooth commands */
static void HandleBluetoothCommand(char command)
{
    switch (command)
    {
        case CMD_POWER_ON:

            vehicle_power = 1U;
            requested_direction = DIR_STOP;
            current_direction = DIR_STOP;
            current_speed = 0U;

            Motor_AllStop();

            break;


        case CMD_POWER_OFF:

            vehicle_power = 0U;
            requested_direction = DIR_STOP;
            current_direction = DIR_STOP;
            current_speed = 0U;

            left_indicator = 0U;
            right_indicator = 0U;

            Motor_AllStop();

            StatusLED_Set(LEFT_LED, 0U);
            StatusLED_Set(RIGHT_LED, 0U);
            StatusLED_Set(HEADLIGHT_LED, 0U);

            break;


        case CMD_FORWARD:

            if (vehicle_power)
            {
                requested_direction = DIR_FORWARD;

                left_indicator = 0U;
                right_indicator = 0U;
            }

            break;


        case CMD_BACKWARD:

            if (vehicle_power)
            {
                requested_direction = DIR_BACKWARD;

                left_indicator = 0U;
                right_indicator = 0U;
            }

            break;


        case CMD_RIGHT:

            if (vehicle_power)
            {
                requested_direction = DIR_RIGHT;

                left_indicator = 0U;
                right_indicator = 1U;
            }

            break;


        case CMD_LEFT:

            if (vehicle_power)
            {
                requested_direction = DIR_LEFT;

                left_indicator = 1U;
                right_indicator = 0U;
            }

            break;


        case CMD_STOP:

            requested_direction = DIR_STOP;
            current_direction = DIR_STOP;
            current_speed = 0U;

            left_indicator = 0U;
            right_indicator = 0U;

            Motor_AllStop();

            break;


        default:

            break;
    }
}


/* Motor control with gradual acceleration and deceleration */
static void UpdateMotor(void)
{
    uint8_t target_speed;

    /* Stop only for power OFF or STOP command */
    if (!vehicle_power || requested_direction == DIR_STOP)
    {
        current_speed = 0U;
        current_direction = DIR_STOP;

        Motor_AllStop();

        return;
    }

    /* Set motor direction */
    if (current_direction != requested_direction)
    {
        Motor_AllStop();

        current_speed = 0U;

        Motor_SetDirection(requested_direction);

        current_direction = requested_direction;
    }

    /* Select target speed */
    if (accident_warning)
    {
        target_speed = SLOW_SPEED;
    }
    else
    {
        target_speed = MOTOR_SPEED;
    }

    /* Gradual speed reduction */
    if (current_speed > target_speed)
    {
        if (current_speed >= SPEED_STEP)
        {
            current_speed -= SPEED_STEP;
        }
        else
        {
            current_speed = target_speed;
        }

        if (current_speed < target_speed)
        {
            current_speed = target_speed;
        }
    }

    /* Gradual speed increase */
    else if (current_speed < target_speed)
    {
        current_speed += SPEED_STEP;

        if (current_speed > target_speed)
        {
            current_speed = target_speed;
        }
    }

    /* Apply motor speed */
    Motor_SetSpeed(current_speed, current_speed);
}


/* Read sensors and calculate magnitude */
static void UpdateSensors(void)
{
    int32_t x;
    int32_t y;
    int32_t z;

    uint32_t sum;

    /* Read light sensor */
    ldr_value = ADC_ReadLDR();

    /* Read accelerometer */
    LIS302DL_ReadXYZ(&accel_x, &accel_y, &accel_z);

    x = (int32_t)accel_x;
    y = (int32_t)accel_y;
    z = (int32_t)accel_z;

    /* Magnitude = sqrt(X^2 + Y^2 + Z^2) */
    sum = (uint32_t)(x * x + y * y + z * z);

    accel_magnitude = IntegerSqrt(sum);

    /* Accelerometer warning condition */
    if ((accel_magnitude > MAGNITUDE_HIGH_THRESHOLD) ||
        (accel_magnitude < MAGNITUDE_LOW_THRESHOLD))
    {
        accident_warning = 1U;
    }
    else
    {
        accident_warning = 0U;
    }

    /* Automatic headlight control */
    if (vehicle_power && (ldr_value < LDR_THRESHOLD))
    {
        StatusLED_Set(HEADLIGHT_LED, 1U);
    }
    else
    {
        StatusLED_Set(HEADLIGHT_LED, 0U);
    }
}


/* Turn indicator control */
static void UpdateIndicators(void)
{
    if (!vehicle_power)
    {
        StatusLED_Set(LEFT_LED, 0U);
        StatusLED_Set(RIGHT_LED, 0U);

        return;
    }

    if (left_indicator)
    {
        StatusLED_Set(LEFT_LED, indicator_phase);
        StatusLED_Set(RIGHT_LED, 0U);
    }
    else if (right_indicator)
    {
        StatusLED_Set(RIGHT_LED, indicator_phase);
        StatusLED_Set(LEFT_LED, 0U);
    }
    else
    {
        StatusLED_Set(LEFT_LED, 0U);
        StatusLED_Set(RIGHT_LED, 0U);
    }
}


/* Update LCD */
static void UpdateLCD(void)
{
    /* Row 1: Vehicle status */
    LCD_SetCursor(0U, 0U);

    if (vehicle_power)
    {
        LCD_Print("Vehicle: ON        ");
    }
    else
    {
        LCD_Print("Vehicle: OFF       ");
    }

    /* Row 2: Direction */
    LCD_SetCursor(1U, 0U);

    LCD_Print("Direction: ");

    if (!vehicle_power)
    {
        LCD_Print("OFF    ");
    }
    else if (requested_direction == DIR_FORWARD)
    {
        LCD_Print("FORWARD");
    }
    else if (requested_direction == DIR_BACKWARD)
    {
        LCD_Print("BACK   ");
    }
    else if (requested_direction == DIR_RIGHT)
    {
        LCD_Print("RIGHT  ");
    }
    else if (requested_direction == DIR_LEFT)
    {
        LCD_Print("LEFT   ");
    }
    else
    {
        LCD_Print("STOP   ");
    }

    /* Row 3: Accelerometer magnitude */
    LCD_SetCursor(2U, 0U);

    LCD_Print("Magnitude: ");
    LCD_PrintNumber(accel_magnitude);
    LCD_Print("       ");

    /* Row 4: Warning status */
    LCD_SetCursor(3U, 0U);

    if (accident_warning)
    {
        LCD_Print("Reduce speed       ");
    }
    else
    {
        LCD_Print("System Normal      ");
    }
}


/* Main function */
int main(void)
{
    System_Init();

    UART_Init();
    Motor_Init();
    ADC_Init();

    SPI1_Init();
    LIS302DL_Init();

    I2C1_Init();
    LCD_Init();

    StatusLED_Init();

    /* Initial motor configuration */
    Motor_AllStop();
    Motor_SetSpeed(0U, 0U);

    /* Initial LED configuration */
    StatusLED_Set(LEFT_LED, 0U);
    StatusLED_Set(RIGHT_LED, 0U);
    StatusLED_Set(HEADLIGHT_LED, 0U);

    /* Initial LCD display */
    LCD_Clear();

    LCD_SetCursor(0U, 0U);
    LCD_Print("Vehicle Simulator");

    LCD_SetCursor(1U, 0U);
    LCD_Print("Bluetooth Ready");

    Delay_ms(1000U);

    LCD_Clear();


    while (1)
    {
        /* Receive Bluetooth command */
        int received_char = UART_ReadChar();

        if (received_char >= 0)
        {
            HandleBluetoothCommand((char)received_char);
        }


        /* Motor update: every 50 ms */
        if ((uint32_t)(system_ms - last_motor_update) >= 50U)
        {
            last_motor_update = system_ms;

            UpdateMotor();
        }


        /* Sensor update: every 100 ms */
        if ((uint32_t)(system_ms - last_sensor_update) >= 100U)
        {
            last_sensor_update = system_ms;

            UpdateSensors();
        }


        /* Indicator update: every 250 ms */
        if ((uint32_t)(system_ms - last_indicator_update) >= 250U)
        {
            last_indicator_update = system_ms;

            indicator_phase = (uint8_t)!indicator_phase;

            UpdateIndicators();
        }


        /* LCD update: every 300 ms */
        if ((uint32_t)(system_ms - last_lcd_update) >= 300U)
        {
            last_lcd_update = system_ms;

            UpdateLCD();
        }
    }
}
