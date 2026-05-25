/******************************************************************************
 * @file    i2c.c
 * @brief   I2C0 Master Driver Implementation for LPC1768 (Bare Metal)
 * @details Polling-based I2C master using direct register access.
 *          I2C0 peripheral: SDA0 = P0.27, SCL0 = P0.28
 *
 * Register Map (I2C0 base: 0x4001C000):
 *   I2CONSET  (0x00) - Control Set Register
 *   I2STAT    (0x04) - Status Register (read-only)
 *   I2DAT     (0x08) - Data Register
 *   I2ADR0    (0x0C) - Slave Address Register 0
 *   I2SCLH    (0x10) - SCL High Duty Cycle
 *   I2SCLL    (0x14) - SCL Low Duty Cycle
 *   I2CONCLR  (0x18) - Control Clear Register
 ******************************************************************************/

#include "LPC17xx.h"
#include "i2c.h"

/*---------------------------------------------------------------------------
 * Private Helper: Wait for the I2C Serial Interrupt (SI) flag to set
 *---------------------------------------------------------------------------*/
static void I2C0_WaitSI(void)
{
    while (!(LPC_I2C0->I2CONSET & I2C_SI))
    {
        /* Poll until SI is set — indicates I2C state machine has new status */
    }
}

/*---------------------------------------------------------------------------
 * I2C0_Init
 *---------------------------------------------------------------------------*/
void I2C0_Init(uint32_t clock_hz)
{
    uint32_t pclk;
    uint32_t scl_div;

    /*-----------------------------------------------------------------------
     * Step 1: Power on I2C0 peripheral
     * PCONP bit 7 = PCI2C0 (enabled by default after reset, but be explicit)
     *-----------------------------------------------------------------------*/
    LPC_SC->PCONP |= (1 << 7);

    /*-----------------------------------------------------------------------
     * Step 2: Configure PCLK for I2C0
     * PCLKSEL0 bits [15:14] control I2C0 clock divider:
     *   00 = CCLK/4 (default)
     *   01 = CCLK/1
     *   10 = CCLK/2
     *   11 = CCLK/8
     * We use CCLK/4 (default) for stability.
     *-----------------------------------------------------------------------*/
    LPC_SC->PCLKSEL0 &= ~(3 << 14);  /* Clear bits [15:14] → PCLK = CCLK/4 */
    pclk = SystemCoreClock / 4;

    /*-----------------------------------------------------------------------
     * Step 3: Configure GPIO pins for I2C0 function
     * PINSEL1 controls P0.16–P0.31:
     *   P0.27 → bits [22:23] = 01 → SDA0
     *   P0.28 → bits [24:25] = 01 → SCL0
     *-----------------------------------------------------------------------*/
    LPC_PINCON->PINSEL1 &= ~((3 << 22) | (3 << 24));  /* Clear pin functions */
    LPC_PINCON->PINSEL1 |=  ((1 << 22) | (1 << 24));   /* Set to I2C0        */

    /*-----------------------------------------------------------------------
     * Step 4: Configure I2C0 pins as open-drain
     * PINMODE_OD0 controls open-drain mode for Port 0:
     *   Bit 27 = P0.27 (SDA0)
     *   Bit 28 = P0.28 (SCL0)
     * I2C pins MUST be open-drain for proper bus operation.
     *-----------------------------------------------------------------------*/
    LPC_PINCON->PINMODE_OD0 |= ((1 << 27) | (1 << 28));

    /*-----------------------------------------------------------------------
     * Step 5: Reset I2C0 control register
     * Clear all control flags before configuration.
     *-----------------------------------------------------------------------*/
    LPC_I2C0->I2CONCLR = I2C_EN | I2C_STA | I2C_SI | I2C_AA;

    /*-----------------------------------------------------------------------
     * Step 6: Set I2C clock speed
     * I2C bit rate = PCLK / (I2SCLH + I2SCLL)
     * For 50% duty cycle: I2SCLH = I2SCLL = PCLK / (2 * clock_hz)
     *
     * Example at 100 MHz CCLK:
     *   PCLK = 25 MHz
     *   For 100 kHz: SCLH = SCLL = 25000000 / (2 * 100000) = 125
     *   For 400 kHz: SCLH = SCLL = 25000000 / (2 * 400000) = 31 (min ~4)
     *-----------------------------------------------------------------------*/
    scl_div = pclk / (2 * clock_hz);
    LPC_I2C0->I2SCLH = scl_div;
    LPC_I2C0->I2SCLL = scl_div;

    /*-----------------------------------------------------------------------
     * Step 7: Enable I2C0 interface
     *-----------------------------------------------------------------------*/
    LPC_I2C0->I2CONSET = I2C_EN;
}

/*---------------------------------------------------------------------------
 * I2C0_Start — Generate START condition
 *---------------------------------------------------------------------------*/
uint8_t I2C0_Start(void)
{
    /*
     * Set STA bit to initiate START condition.
     * Clear SI in case it was already set from a previous operation.
     */
    LPC_I2C0->I2CONCLR = I2C_SI;            /* Clear SI flag                */
    LPC_I2C0->I2CONSET = I2C_STA;           /* Request START condition      */
    I2C0_WaitSI();                           /* Wait for state change        */

    return (uint8_t)(LPC_I2C0->I2STAT & 0xF8);
}

/*---------------------------------------------------------------------------
 * I2C0_RepeatedStart — Generate repeated START condition
 *---------------------------------------------------------------------------*/
uint8_t I2C0_RepeatedStart(void)
{
    LPC_I2C0->I2CONSET = I2C_STA;           /* Request repeated START       */
    LPC_I2C0->I2CONCLR = I2C_SI;            /* Clear SI to proceed          */
    I2C0_WaitSI();                           /* Wait for state change        */

    return (uint8_t)(LPC_I2C0->I2STAT & 0xF8);
}

/*---------------------------------------------------------------------------
 * I2C0_Stop — Generate STOP condition
 *---------------------------------------------------------------------------*/
void I2C0_Stop(void)
{
    LPC_I2C0->I2CONSET = I2C_STO;           /* Request STOP condition       */
    LPC_I2C0->I2CONCLR = I2C_SI;            /* Clear SI to execute STOP     */

    /*
     * Wait for STO bit to auto-clear (hardware clears it after STOP is sent).
     * This ensures the bus is released before we return.
     */
    while (LPC_I2C0->I2CONSET & I2C_STO)
    {
        /* Wait for STOP to complete */
    }
}

/*---------------------------------------------------------------------------
 * I2C0_Write — Transmit one byte (SLA+R/W or data)
 *---------------------------------------------------------------------------*/
uint8_t I2C0_Write(uint8_t data)
{
    LPC_I2C0->I2DAT    = data;              /* Load data byte               */
    LPC_I2C0->I2CONCLR = I2C_STA | I2C_SI;  /* Clear STA and SI            */
    I2C0_WaitSI();                           /* Wait for transmission done   */

    return (uint8_t)(LPC_I2C0->I2STAT & 0xF8);
}

/*---------------------------------------------------------------------------
 * I2C0_ReadACK — Read one byte with ACK (more bytes to follow)
 *---------------------------------------------------------------------------*/
uint8_t I2C0_ReadACK(uint8_t *data)
{
    LPC_I2C0->I2CONSET = I2C_AA;            /* Set AA to send ACK after rx  */
    LPC_I2C0->I2CONCLR = I2C_SI;            /* Clear SI to proceed          */
    I2C0_WaitSI();                           /* Wait for reception           */

    *data = (uint8_t)LPC_I2C0->I2DAT;       /* Read received byte           */

    return (uint8_t)(LPC_I2C0->I2STAT & 0xF8);
}

/*---------------------------------------------------------------------------
 * I2C0_ReadNACK — Read one byte with NACK (last byte in transfer)
 *---------------------------------------------------------------------------*/
uint8_t I2C0_ReadNACK(uint8_t *data)
{
    LPC_I2C0->I2CONCLR = I2C_AA | I2C_SI;   /* Clear AA (NACK) and SI      */
    I2C0_WaitSI();                           /* Wait for reception           */

    *data = (uint8_t)LPC_I2C0->I2DAT;       /* Read received byte           */

    return (uint8_t)(LPC_I2C0->I2STAT & 0xF8);
}

/*---------------------------------------------------------------------------
 * I2C0_WriteBuffer — Complete master-write transaction
 *
 * Transaction sequence:
 *   [START] → [SLA+W] → [DATA0] → [DATA1] → ... → [DATAn] → [STOP]
 *---------------------------------------------------------------------------*/
uint8_t I2C0_WriteBuffer(uint8_t slave_addr, const uint8_t *buf, uint32_t len)
{
    uint8_t  status;
    uint32_t i;

    /* Step 1: Send START */
    status = I2C0_Start();
    if ((status != I2C_STAT_START) && (status != I2C_STAT_REP_START))
    {
        I2C0_Stop();
        return I2C_ERR_START;
    }

    /* Step 2: Send slave address with Write bit (bit 0 = 0) */
    status = I2C0_Write((slave_addr << 1) | 0x00);
    if (status != I2C_STAT_SLAW_ACK)
    {
        I2C0_Stop();
        return I2C_ERR_ADDR_NACK;
    }

    /* Step 3: Send data bytes */
    for (i = 0; i < len; i++)
    {
        status = I2C0_Write(buf[i]);
        if (status != I2C_STAT_DATW_ACK)
        {
            I2C0_Stop();
            return I2C_ERR_DATA_NACK;
        }
    }

    /* Step 4: Send STOP */
    I2C0_Stop();

    return I2C_OK;
}

/*---------------------------------------------------------------------------
 * I2C0_ReadBuffer — Complete master-read transaction
 *
 * Transaction sequence:
 *   [START] → [SLA+R] → [ACK+DATA0] → ... → [NACK+DATAn] → [STOP]
 *---------------------------------------------------------------------------*/
uint8_t I2C0_ReadBuffer(uint8_t slave_addr, uint8_t *buf, uint32_t len)
{
    uint8_t  status;
    uint32_t i;

    if (len == 0)
    {
        return I2C_OK;
    }

    /* Step 1: Send START */
    status = I2C0_Start();
    if ((status != I2C_STAT_START) && (status != I2C_STAT_REP_START))
    {
        I2C0_Stop();
        return I2C_ERR_START;
    }

    /* Step 2: Send slave address with Read bit (bit 0 = 1) */
    status = I2C0_Write((slave_addr << 1) | 0x01);
    if (status != I2C_STAT_SLAR_ACK)
    {
        I2C0_Stop();
        return I2C_ERR_ADDR_NACK;
    }

    /* Step 3: Read data bytes (ACK all except the last byte) */
    for (i = 0; i < len - 1; i++)
    {
        status = I2C0_ReadACK(&buf[i]);
        if (status != I2C_STAT_DATR_ACK)
        {
            I2C0_Stop();
            return I2C_ERR_DATA_NACK;
        }
    }

    /* Step 4: Read last byte with NACK */
    status = I2C0_ReadNACK(&buf[len - 1]);
    if (status != I2C_STAT_DATR_NACK)
    {
        I2C0_Stop();
        return I2C_ERR_DATA_NACK;
    }

    /* Step 5: Send STOP */
    I2C0_Stop();

    return I2C_OK;
}
