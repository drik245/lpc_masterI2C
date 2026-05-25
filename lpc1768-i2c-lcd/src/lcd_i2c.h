/******************************************************************************
 * @file    lcd_i2c.h
 * @brief   16x2 LCD Driver via PCF8574T I2C I/O Expander
 * @details Controls an HD44780-compatible 16x2 LCD in 4-bit mode through
 *          the PCF8574T I2C-to-parallel expander chip.
 *
 * PCF8574T Pin Mapping (typical LCD I2C adapter module):
 *   P0 = RS   (Register Select: 0=Command, 1=Data)
 *   P1 = RW   (Read/Write: 0=Write, 1=Read)
 *   P2 = EN   (Enable: pulse high-to-low to latch data)
 *   P3 = BL   (Backlight: 1=ON, 0=OFF)
 *   P4 = D4   (LCD data bit 4)
 *   P5 = D5   (LCD data bit 5)
 *   P6 = D6   (LCD data bit 6)
 *   P7 = D7   (LCD data bit 7)
 *
 * @author  Bare Metal I2C LCD Project
 * @date    2026
 ******************************************************************************/

#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>

/*---------------------------------------------------------------------------
 * PCF8574T I2C Address
 * Default: 0x27 (A0=A1=A2 pulled high on most modules)
 * Change this if your module has a different address (check with I2C scanner)
 *---------------------------------------------------------------------------*/
#define PCF8574T_ADDR       0x27

/*---------------------------------------------------------------------------
 * PCF8574T Pin Bit Masks (directly map to port expander output pins)
 *---------------------------------------------------------------------------*/
#define LCD_RS              (1 << 0)   /* P0: Register Select               */
#define LCD_RW              (1 << 1)   /* P1: Read/Write                    */
#define LCD_EN              (1 << 2)   /* P2: Enable                        */
#define LCD_BL              (1 << 3)   /* P3: Backlight                     */

/*---------------------------------------------------------------------------
 * HD44780 LCD Commands
 *---------------------------------------------------------------------------*/
#define LCD_CMD_CLEAR           0x01   /* Clear display                     */
#define LCD_CMD_HOME            0x02   /* Return cursor to home             */
#define LCD_CMD_ENTRY_MODE      0x06   /* Increment cursor, no shift        */
#define LCD_CMD_DISPLAY_OFF     0x08   /* Display OFF                       */
#define LCD_CMD_DISPLAY_ON      0x0C   /* Display ON, cursor OFF, blink OFF */
#define LCD_CMD_DISPLAY_CURSOR  0x0E   /* Display ON, cursor ON, blink OFF  */
#define LCD_CMD_DISPLAY_BLINK   0x0F   /* Display ON, cursor ON, blink ON   */
#define LCD_CMD_SHIFT_LEFT      0x18   /* Shift display left                */
#define LCD_CMD_SHIFT_RIGHT     0x1C   /* Shift display right               */
#define LCD_CMD_FUNCTION_SET    0x28   /* 4-bit, 2-line, 5x8 dots           */
#define LCD_CMD_SET_CGRAM       0x40   /* Set CGRAM address                 */
#define LCD_CMD_SET_DDRAM       0x80   /* Set DDRAM address                 */

/*---------------------------------------------------------------------------
 * LCD Row Start Addresses (DDRAM)
 *---------------------------------------------------------------------------*/
#define LCD_ROW0_ADDR           0x00
#define LCD_ROW1_ADDR           0x40

/*---------------------------------------------------------------------------
 * Public Function Prototypes
 *---------------------------------------------------------------------------*/

/**
 * @brief  Initialize the 16x2 LCD in 4-bit mode via I2C.
 * @note   Must be called after I2C0_Init(). Performs the HD44780 standard
 *         initialization sequence with proper timing delays.
 */
void LCD_I2C_Init(void);

/**
 * @brief  Send a command byte to the LCD.
 * @param  cmd  HD44780 command byte.
 */
void LCD_I2C_Command(uint8_t cmd);

/**
 * @brief  Send a data byte (character) to the LCD.
 * @param  data  ASCII character to display.
 */
void LCD_I2C_Data(uint8_t data);

/**
 * @brief  Print a null-terminated string to the LCD.
 * @param  str  String to display.
 */
void LCD_I2C_Print(const char *str);

/**
 * @brief  Set the cursor position on the LCD.
 * @param  row  Row number (0 or 1).
 * @param  col  Column number (0 to 15).
 */
void LCD_I2C_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief  Clear the entire LCD display.
 */
void LCD_I2C_Clear(void);

/**
 * @brief  Return cursor to home position (0,0).
 */
void LCD_I2C_Home(void);

/**
 * @brief  Turn the LCD backlight ON or OFF.
 * @param  on  1 to turn on, 0 to turn off.
 */
void LCD_I2C_Backlight(uint8_t on);

/**
 * @brief  Create a custom character in CGRAM.
 * @param  location  CGRAM slot (0-7).
 * @param  pattern   Array of 8 bytes defining the 5x8 pixel pattern.
 */
void LCD_I2C_CreateChar(uint8_t location, const uint8_t pattern[8]);

#endif /* LCD_I2C_H */
