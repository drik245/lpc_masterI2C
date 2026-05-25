# LPC1768 Bare Metal I2C Master — 16x2 LCD via PCF8574T

Bare metal I2C master driver for the **NXP LPC1768** (ARM Cortex-M3) that interfaces with a **16x2 HD44780 LCD** through the **PCF8574T I2C-to-parallel I/O expander**.

All driver code is written at the register level — no HAL, no Mbed API calls. Pure bare metal.
The Mbed library is used only for CMSIS headers (`LPC17xx.h`) and startup code.

## 📁 Project Structure

```
lpc1768-i2c-lcd/
├── .mbed               ← Target config (LPC1768 / ARM Compiler)
├── mbed.bld            ← Mbed 2 library reference
├── .gitignore
├── README.md
├── main.c              ← Application entry point & demo
├── i2c.c               ← I2C0 master driver (register-level)
├── i2c.h               ← I2C driver API & status codes
├── lcd_i2c.c           ← LCD control via PCF8574T
├── lcd_i2c.h           ← LCD API & HD44780 commands
├── delay.c             ← SysTick-based delay (polling)
└── delay.h             ← Delay function prototypes
```

## 🚀 Importing into Keil Studio Cloud (Mbed)

### Method 1: Clone from GitHub (Recommended)

1. Go to [Keil Studio Cloud](https://studio.keil.arm.com)
2. Click **File → Clone...**
3. Paste: `https://github.com/drik245/lpc_masterI2C.git`
4. Open the `lpc1768-i2c-lcd` folder as the active project
5. The IDE will automatically import the Mbed library from `mbed.bld`
6. Set target to **LPC1768** (bottom-left of IDE)
7. Click **Build** (hammer icon)

### Method 2: Import from Mbed Compiler

1. Go to [Keil Studio Cloud](https://studio.keil.arm.com)
2. Click **File → Import from Mbed Online Compiler...**
3. Follow the prompts to import from your Mbed repo

## 🔧 Hardware Connections

### I2C Bus Wiring

| LPC1768 Pin | Function | Connection       |
|-------------|----------|------------------|
| P0.27       | SDA0     | PCF8574T SDA     |
| P0.28       | SCL0     | PCF8574T SCL     |
| 3.3V        | VCC      | PCF8574T VCC     |
| GND         | GND      | PCF8574T GND     |

> **Note:** Most PCF8574T LCD adapter modules include on-board 4.7kΩ pull-up resistors. If your module doesn't, add external 4.7kΩ pull-ups on SDA and SCL to 3.3V.

### PCF8574T → LCD Pin Mapping

| PCF8574T Pin | LCD Pin | Function         |
|--------------|---------|------------------|
| P0           | RS      | Register Select  |
| P1           | RW      | Read/Write       |
| P2           | EN      | Enable           |
| P3           | BL      | Backlight        |
| P4           | D4      | Data Bit 4       |
| P5           | D5      | Data Bit 5       |
| P6           | D6      | Data Bit 6       |
| P7           | D7      | Data Bit 7       |

### Default I2C Address

The PCF8574T default address is **0x27** (A0=A1=A2 connected to VCC). If your module uses a different address, modify `PCF8574T_ADDR` in `lcd_i2c.h`.

## 📋 Driver API Reference

### I2C Driver (`i2c.h`)

```c
void    I2C0_Init(uint32_t clock_hz);      // Init I2C0, e.g., 100000 for 100kHz
uint8_t I2C0_Start(void);                  // Send START condition
uint8_t I2C0_RepeatedStart(void);          // Send repeated START
void    I2C0_Stop(void);                   // Send STOP condition
uint8_t I2C0_Write(uint8_t data);          // Write one byte (addr or data)
uint8_t I2C0_ReadACK(uint8_t *data);       // Read byte with ACK
uint8_t I2C0_ReadNACK(uint8_t *data);      // Read byte with NACK (last byte)
uint8_t I2C0_WriteBuffer(uint8_t addr, const uint8_t *buf, uint32_t len);
uint8_t I2C0_ReadBuffer(uint8_t addr, uint8_t *buf, uint32_t len);
```

### LCD Driver (`lcd_i2c.h`)

```c
void LCD_I2C_Init(void);                   // Initialize LCD (4-bit mode via I2C)
void LCD_I2C_Command(uint8_t cmd);         // Send command byte
void LCD_I2C_Data(uint8_t data);           // Send data (character) byte
void LCD_I2C_Print(const char *str);       // Print null-terminated string
void LCD_I2C_SetCursor(uint8_t row, uint8_t col);  // Set cursor (row: 0-1, col: 0-15)
void LCD_I2C_Clear(void);                  // Clear display
void LCD_I2C_Home(void);                   // Cursor to (0,0)
void LCD_I2C_Backlight(uint8_t on);        // Backlight ON(1) / OFF(0)
void LCD_I2C_CreateChar(uint8_t loc, const uint8_t pattern[8]);  // Custom char
```

### Delay Functions (`delay.h`)

```c
void Delay_Init(void);         // Initialize (call once at startup)
void Delay_us(uint32_t us);    // Blocking microsecond delay
void Delay_ms(uint32_t ms);    // Blocking millisecond delay
```

## ⚙️ I2C State Machine

The I2C driver uses the LPC1768 hardware state machine. Key status codes:

| Code   | State                          | Next Action              |
|--------|--------------------------------|--------------------------|
| `0x08` | START transmitted              | Send SLA+R/W             |
| `0x10` | Repeated START transmitted     | Send SLA+R/W             |
| `0x18` | SLA+W sent, ACK received       | Send data                |
| `0x20` | SLA+W sent, NACK received      | STOP or retry            |
| `0x28` | Data sent, ACK received        | Send more data or STOP   |
| `0x40` | SLA+R sent, ACK received       | Receive data with ACK    |
| `0x50` | Data received, ACK returned    | Receive more data        |
| `0x58` | Data received, NACK returned   | STOP (last byte)         |

## 📌 Configuration

| Parameter         | File          | Default   | Description                |
|-------------------|---------------|-----------|----------------------------|
| I2C Address       | `lcd_i2c.h`  | `0x27`    | PCF8574T slave address     |
| I2C Speed         | `main.c`     | 100 kHz   | Standard mode              |
| System Clock      | Mbed startup  | 100 MHz   | LPC1768 PLL configuration  |
| I2C PCLK          | `i2c.c`      | 25 MHz    | CCLK/4 (default divider)   |

## 🔬 Troubleshooting

| Issue                        | Possible Cause                     | Solution                                   |
|------------------------------|------------------------------------|--------------------------------------------|
| LCD shows nothing            | Wrong I2C address                  | Try `0x27`, `0x3F`, or scan with I2C scanner |
| LCD shows blocks on row 1    | Initialization timing              | Increase power-on delay in `LCD_I2C_Init()` |
| Garbled text                 | I2C speed too fast                 | Use 100 kHz (standard mode)                |
| Backlight off                | BL jumper on module removed        | Check BL jumper or call `LCD_I2C_Backlight(1)` |
| I2C hangs                    | Missing pull-up resistors          | Add 4.7kΩ pull-ups on SDA & SCL            |
| No ACK from PCF8574T         | Wiring or power issue              | Check VCC, GND, SDA, SCL connections       |
| Build errors in Keil Studio  | Mbed library not imported          | Right-click project → Add Mbed Library     |

## 📜 License

This project is provided as-is for educational purposes. Free to use and modify.
