/******************************************************************************
 * @file    lcd_i2c.c
 * @brief   16x2 LCD Driver Implementation via PCF8574T I2C I/O Expander
 * @details Implements 4-bit mode LCD communication through the PCF8574T.
 *          Each LCD write requires sending the upper nibble then lower nibble,
 *          each with an Enable pulse, all via I2C byte writes to PCF8574T.
 *
 * Data Byte Format sent to PCF8574T:
 *   Bit 7  6  5  4  3  2  1  0
 *       D7 D6 D5 D4 BL EN RW RS
 *
 * Write Sequence for one LCD byte (command or data):
 *   1. Send upper nibble | EN=1 | RS | BL  (EN pulse high)
 *   2. Send upper nibble | EN=0 | RS | BL  (EN pulse low → latch)
 *   3. Send lower nibble | EN=1 | RS | BL  (EN pulse high)
 *   4. Send lower nibble | EN=0 | RS | BL  (EN pulse low → latch)
 ******************************************************************************/

#include "LPC17xx.h"
#include "lcd_i2c.h"
#include "i2c.h"
#include "delay.h"

/*---------------------------------------------------------------------------
 * Module-level state: backlight control
 *---------------------------------------------------------------------------*/
static uint8_t lcd_backlight = LCD_BL;  /* Backlight ON by default */

/*---------------------------------------------------------------------------
 * Private: Write a byte to the PCF8574T I/O expander
 *---------------------------------------------------------------------------*/
static void PCF8574T_Write(uint8_t data)
{
    I2C0_WriteBuffer(PCF8574T_ADDR, &data, 1);
}

/*---------------------------------------------------------------------------
 * Private: Pulse the Enable (EN) pin
 * The HD44780 latches data on the falling edge of EN.
 *---------------------------------------------------------------------------*/
static void LCD_PulseEnable(uint8_t data)
{
    PCF8574T_Write(data | LCD_EN);    /* EN = 1 (rising edge)   */
    Delay_us(1);                      /* Enable pulse width > 450ns         */
    PCF8574T_Write(data & ~LCD_EN);   /* EN = 0 (falling edge → latch)      */
    Delay_us(50);                     /* Commands need > 37 µs to execute    */
}

/*---------------------------------------------------------------------------
 * Private: Send a 4-bit nibble to the LCD
 *---------------------------------------------------------------------------*/
static void LCD_WriteNibble(uint8_t nibble, uint8_t mode)
{
    /*
     * nibble: upper 4 bits of data byte (already positioned in bits 7:4)
     * mode:   0 = command (RS=0), LCD_RS = data (RS=1)
     */
    uint8_t data = (nibble & 0xF0) | mode | lcd_backlight;
    LCD_PulseEnable(data);
}

/*---------------------------------------------------------------------------
 * Private: Send a full byte to the LCD in two nibbles (4-bit mode)
 *---------------------------------------------------------------------------*/
static void LCD_WriteByte(uint8_t byte, uint8_t mode)
{
    /* Send upper nibble first, then lower nibble */
    LCD_WriteNibble(byte & 0xF0, mode);           /* Upper nibble */
    LCD_WriteNibble((byte << 4) & 0xF0, mode);    /* Lower nibble */
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Init — HD44780 Initialization in 4-bit Mode
 *
 * This follows the exact initialization sequence from the HD44780 datasheet
 * (Figure 24: 4-Bit Interface Initialization).
 *---------------------------------------------------------------------------*/
void LCD_I2C_Init(void)
{
    /*-----------------------------------------------------------------------
     * Wait for LCD power-on (HD44780 requires > 40ms after VCC rises to 2.7V)
     *-----------------------------------------------------------------------*/
    Delay_ms(50);

    /*-----------------------------------------------------------------------
     * Set PCF8574T outputs low initially (with backlight ON)
     *-----------------------------------------------------------------------*/
    PCF8574T_Write(lcd_backlight);
    Delay_ms(10);

    /*-----------------------------------------------------------------------
     * HD44780 Special Initialization Sequence (4-bit mode entry)
     *
     * After power on, the LCD may be in an unknown state (could be 8-bit
     * mode). We must send Function Set (0x03) three times to guarantee
     * we're starting from a known 8-bit state, then switch to 4-bit.
     *
     * NOTE: These are NIBBLE writes (only upper 4 bits), NOT full byte
     * writes, because we haven't entered 4-bit mode yet!
     *-----------------------------------------------------------------------*/

    /* Attempt 1: Send 0x30 (Function Set: 8-bit) — wait > 4.1ms */
    LCD_WriteNibble(0x30, 0);
    Delay_ms(5);

    /* Attempt 2: Send 0x30 again — wait > 100µs */
    LCD_WriteNibble(0x30, 0);
    Delay_us(150);

    /* Attempt 3: Send 0x30 one more time */
    LCD_WriteNibble(0x30, 0);
    Delay_us(150);

    /*-----------------------------------------------------------------------
     * NOW switch to 4-bit mode by sending 0x20 (Function Set: 4-bit)
     * After this point, we can use full-byte (2-nibble) writes.
     *-----------------------------------------------------------------------*/
    LCD_WriteNibble(0x20, 0);
    Delay_us(150);

    /*-----------------------------------------------------------------------
     * Configure LCD settings (these are full-byte commands in 4-bit mode)
     *-----------------------------------------------------------------------*/

    /* Function Set: 4-bit mode, 2 display lines, 5x8 font */
    LCD_I2C_Command(LCD_CMD_FUNCTION_SET);

    /* Display Control: Display ON, Cursor OFF, Blink OFF */
    LCD_I2C_Command(LCD_CMD_DISPLAY_ON);

    /* Clear Display */
    LCD_I2C_Clear();

    /* Entry Mode Set: Increment cursor, No display shift */
    LCD_I2C_Command(LCD_CMD_ENTRY_MODE);
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Command — Send a command to the LCD (RS = 0)
 *---------------------------------------------------------------------------*/
void LCD_I2C_Command(uint8_t cmd)
{
    LCD_WriteByte(cmd, 0);  /* mode = 0 → RS = 0 (command register) */
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Data — Send a character to the LCD (RS = 1)
 *---------------------------------------------------------------------------*/
void LCD_I2C_Data(uint8_t data)
{
    LCD_WriteByte(data, LCD_RS);  /* mode = LCD_RS → RS = 1 (data register) */
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Print — Print a string at the current cursor position
 *---------------------------------------------------------------------------*/
void LCD_I2C_Print(const char *str)
{
    while (*str)
    {
        LCD_I2C_Data((uint8_t)*str);
        str++;
    }
}

/*---------------------------------------------------------------------------
 * LCD_I2C_SetCursor — Set cursor to (row, col)
 *
 * 16x2 LCD DDRAM addresses:
 *   Row 0: 0x00 – 0x0F  (16 columns)
 *   Row 1: 0x40 – 0x4F  (16 columns)
 *---------------------------------------------------------------------------*/
void LCD_I2C_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if (row == 0)
        addr = LCD_ROW0_ADDR + col;
    else
        addr = LCD_ROW1_ADDR + col;

    LCD_I2C_Command(LCD_CMD_SET_DDRAM | addr);
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Clear — Clear display (takes ~1.52 ms to execute)
 *---------------------------------------------------------------------------*/
void LCD_I2C_Clear(void)
{
    LCD_I2C_Command(LCD_CMD_CLEAR);
    Delay_ms(2);  /* Clear command requires > 1.52 ms */
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Home — Return cursor to home (takes ~1.52 ms)
 *---------------------------------------------------------------------------*/
void LCD_I2C_Home(void)
{
    LCD_I2C_Command(LCD_CMD_HOME);
    Delay_ms(2);
}

/*---------------------------------------------------------------------------
 * LCD_I2C_Backlight — Turn backlight ON/OFF
 *---------------------------------------------------------------------------*/
void LCD_I2C_Backlight(uint8_t on)
{
    lcd_backlight = on ? LCD_BL : 0;
    PCF8574T_Write(lcd_backlight);  /* Update PCF8574T output immediately */
}

/*---------------------------------------------------------------------------
 * LCD_I2C_CreateChar — Define a custom character (CGRAM)
 *
 * The HD44780 supports 8 custom 5x8 characters stored in CGRAM.
 * Each character is 8 bytes (one byte per row, lower 5 bits used).
 *
 * After writing to CGRAM, cursor returns to DDRAM automatically
 * when you write data or set DDRAM address.
 *---------------------------------------------------------------------------*/
void LCD_I2C_CreateChar(uint8_t location, const uint8_t pattern[8])
{
    uint8_t i;

    location &= 0x07;  /* Only locations 0-7 are valid */

    /* Set CGRAM address: location * 8 */
    LCD_I2C_Command(LCD_CMD_SET_CGRAM | (location << 3));

    /* Write 8 pattern bytes */
    for (i = 0; i < 8; i++)
    {
        LCD_I2C_Data(pattern[i]);
    }

    /* Return to DDRAM (home position) */
    LCD_I2C_Command(LCD_CMD_SET_DDRAM);
}
