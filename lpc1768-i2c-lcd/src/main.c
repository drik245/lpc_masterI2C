/******************************************************************************
 * @file    main.c
 * @brief   Main Application — I2C LCD Demo on LPC1768
 * @details Demonstrates the bare metal I2C master driver by interfacing with
 *          a 16x2 LCD via the PCF8574T I2C-to-parallel I/O expander.
 *
 * Hardware Setup:
 *   LPC1768           PCF8574T Module         16x2 LCD
 *   --------          ---------------         --------
 *   P0.27 (SDA0) ---> SDA                     (Connected on module)
 *   P0.28 (SCL0) ---> SCL                     (Connected on module)
 *   3.3V         ---> VCC                     VCC
 *   GND          ---> GND                     GND
 *
 *   Pull-up resistors (4.7 kΩ) on SDA and SCL lines to 3.3V
 *   (Most PCF8574T modules include on-board pull-ups)
 *
 * @author  Bare Metal I2C LCD Project
 * @date    2026
 ******************************************************************************/

#include "LPC17xx.h"
#include "i2c.h"
#include "lcd_i2c.h"
#include "delay.h"

/*---------------------------------------------------------------------------
 * Custom Character Definitions (5x8 pixel patterns)
 *---------------------------------------------------------------------------*/

/* Heart symbol (stored in CGRAM location 0) */
static const uint8_t heart_char[8] = {
    0x00,  /*  .....  */
    0x0A,  /*  .X.X.  */
    0x1F,  /*  XXXXX  */
    0x1F,  /*  XXXXX  */
    0x0E,  /*  .XXX.  */
    0x04,  /*  ..X..  */
    0x00,  /*  .....  */
    0x00   /*  .....  */
};

/* Smiley face (stored in CGRAM location 1) */
static const uint8_t smiley_char[8] = {
    0x00,  /*  .....  */
    0x0A,  /*  .X.X.  */
    0x0A,  /*  .X.X.  */
    0x00,  /*  .....  */
    0x11,  /*  X...X  */
    0x0E,  /*  .XXX.  */
    0x00,  /*  .....  */
    0x00   /*  .....  */
};

/*---------------------------------------------------------------------------
 * Helper: Display a scrolling message on the LCD
 *---------------------------------------------------------------------------*/
static void LCD_ScrollMessage(const char *msg, uint8_t row, uint32_t delay_time)
{
    uint8_t i;
    uint8_t len = 0;
    const char *p = msg;

    /* Calculate string length */
    while (*p++) len++;

    /* Scroll through the message if longer than 16 chars */
    if (len <= 16)
    {
        LCD_I2C_SetCursor(row, 0);
        LCD_I2C_Print(msg);
        return;
    }

    for (i = 0; i <= len - 16; i++)
    {
        LCD_I2C_SetCursor(row, 0);
        /* Print 16 visible characters starting from position i */
        {
            uint8_t j;
            for (j = 0; j < 16; j++)
            {
                LCD_I2C_Data((uint8_t)msg[i + j]);
            }
        }
        Delay_ms(delay_time);
    }
}

/*---------------------------------------------------------------------------
 * Main Application Entry Point
 *---------------------------------------------------------------------------*/
int main(void)
{
    volatile uint32_t counter = 0;
    char count_str[17];

    /*-----------------------------------------------------------------------
     * System Initialization
     * SystemInit() is called by the startup code before main().
     * It configures the PLL to run the CPU at 100 MHz.
     *-----------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------
     * Step 1: Initialize delay system (needs SystemCoreClock to be valid)
     *-----------------------------------------------------------------------*/
    Delay_Init();

    /*-----------------------------------------------------------------------
     * Step 2: Initialize I2C0 at 100 kHz (Standard Mode)
     * Configures P0.27 = SDA0, P0.28 = SCL0
     *-----------------------------------------------------------------------*/
    I2C0_Init(I2C_SPEED_100KHZ);

    /*-----------------------------------------------------------------------
     * Step 3: Initialize the 16x2 LCD via I2C
     * Performs HD44780 4-bit mode initialization through PCF8574T
     *-----------------------------------------------------------------------*/
    LCD_I2C_Init();

    /*-----------------------------------------------------------------------
     * Step 4: Create custom characters in CGRAM
     *-----------------------------------------------------------------------*/
    LCD_I2C_CreateChar(0, heart_char);
    LCD_I2C_CreateChar(1, smiley_char);

    /*-----------------------------------------------------------------------
     * Step 5: Display welcome message
     *-----------------------------------------------------------------------*/
    LCD_I2C_SetCursor(0, 0);
    LCD_I2C_Print("LPC1768 I2C LCD");

    LCD_I2C_SetCursor(1, 0);
    LCD_I2C_Print(" Bare Metal ");
    LCD_I2C_Data(0);    /* Display heart (CGRAM location 0) */
    LCD_I2C_Data(1);    /* Display smiley (CGRAM location 1) */

    Delay_ms(3000);

    /*-----------------------------------------------------------------------
     * Step 6: Display driver information
     *-----------------------------------------------------------------------*/
    LCD_I2C_Clear();
    LCD_I2C_SetCursor(0, 0);
    LCD_I2C_Print("I2C Master Mode");
    LCD_I2C_SetCursor(1, 0);
    LCD_I2C_Print("PCF8574T @0x27");

    Delay_ms(3000);

    /*-----------------------------------------------------------------------
     * Step 7: Main loop — Running counter display
     *-----------------------------------------------------------------------*/
    LCD_I2C_Clear();
    LCD_I2C_SetCursor(0, 0);
    LCD_I2C_Print("Running Counter:");

    while (1)
    {
        /* Convert counter to string (simple decimal conversion) */
        {
            uint32_t temp = counter;
            int8_t   i = 0;
            int8_t   j;
            char     rev[11]; /* Max uint32 = 10 digits */

            if (temp == 0)
            {
                count_str[0] = '0';
                count_str[1] = '\0';
            }
            else
            {
                while (temp > 0)
                {
                    rev[i++] = '0' + (temp % 10);
                    temp /= 10;
                }
                for (j = 0; j < i; j++)
                {
                    count_str[j] = rev[i - 1 - j];
                }
                count_str[i] = '\0';
            }

            /* Pad with spaces to clear old digits */
            while (i < 16)
            {
                count_str[i++] = ' ';
            }
            count_str[16] = '\0';
        }

        /* Display counter on row 1 */
        LCD_I2C_SetCursor(1, 0);
        LCD_I2C_Print(count_str);

        counter++;
        Delay_ms(500);
    }

    /* Never reached */
    /* return 0; */
}
