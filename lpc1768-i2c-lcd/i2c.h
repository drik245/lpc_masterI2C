/******************************************************************************
 * @file    i2c.h
 * @brief   I2C0 Master Driver for LPC1768 (Bare Metal)
 * @details Register-level I2C master driver using the LPC1768 I2C0 peripheral.
 *          Uses P0.27 (SDA0) and P0.28 (SCL0).
 *          Implements a polling-based state machine for I2C communication.
 *
 * @author  Bare Metal I2C LCD Project
 * @date    2026
 ******************************************************************************/

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/*---------------------------------------------------------------------------
 * I2C Configuration
 *---------------------------------------------------------------------------*/
#define I2C_SPEED_100KHZ    100000U   /* Standard mode: 100 kHz              */
#define I2C_SPEED_400KHZ    400000U   /* Fast mode: 400 kHz                  */

/*---------------------------------------------------------------------------
 * I2C Status Codes (from LPC1768 User Manual - Table 399 / 400)
 *---------------------------------------------------------------------------*/
/* Master Transmitter States */
#define I2C_STAT_START          0x08  /* START condition transmitted          */
#define I2C_STAT_REP_START      0x10  /* Repeated START transmitted           */
#define I2C_STAT_SLAW_ACK       0x18  /* SLA+W sent, ACK received            */
#define I2C_STAT_SLAW_NACK      0x20  /* SLA+W sent, NACK received           */
#define I2C_STAT_DATW_ACK       0x28  /* Data sent, ACK received             */
#define I2C_STAT_DATW_NACK      0x30  /* Data sent, NACK received            */
#define I2C_STAT_ARB_LOST       0x38  /* Arbitration lost                    */

/* Master Receiver States */
#define I2C_STAT_SLAR_ACK       0x40  /* SLA+R sent, ACK received            */
#define I2C_STAT_SLAR_NACK      0x48  /* SLA+R sent, NACK received           */
#define I2C_STAT_DATR_ACK       0x50  /* Data received, ACK returned         */
#define I2C_STAT_DATR_NACK      0x58  /* Data received, NACK returned        */

/*---------------------------------------------------------------------------
 * I2C Control Register Bit Definitions
 *---------------------------------------------------------------------------*/
#define I2C_AA                  (1 << 2)  /* Assert Acknowledge              */
#define I2C_SI                  (1 << 3)  /* Serial Interrupt flag           */
#define I2C_STO                 (1 << 4)  /* STOP condition                  */
#define I2C_STA                 (1 << 5)  /* START condition                 */
#define I2C_EN                  (1 << 6)  /* I2C Interface Enable            */

/*---------------------------------------------------------------------------
 * Return Codes
 *---------------------------------------------------------------------------*/
#define I2C_OK                  0
#define I2C_ERR_START           1
#define I2C_ERR_ADDR_NACK       2
#define I2C_ERR_DATA_NACK       3
#define I2C_ERR_ARB_LOST        4
#define I2C_ERR_UNKNOWN         5

/*---------------------------------------------------------------------------
 * Public Function Prototypes
 *---------------------------------------------------------------------------*/

/**
 * @brief  Initialize I2C0 peripheral in master mode.
 * @param  clock_hz  Desired I2C clock frequency in Hz (e.g., 100000 for 100kHz).
 * @note   Configures P0.27 as SDA0 and P0.28 as SCL0.
 *         Assumes SystemCoreClock is already configured (default 100 MHz).
 */
void I2C0_Init(uint32_t clock_hz);

/**
 * @brief  Send START condition on the I2C bus.
 * @return I2C status code from the I2STAT register.
 */
uint8_t I2C0_Start(void);

/**
 * @brief  Send repeated START condition on the I2C bus.
 * @return I2C status code from the I2STAT register.
 */
uint8_t I2C0_RepeatedStart(void);

/**
 * @brief  Send STOP condition on the I2C bus.
 */
void I2C0_Stop(void);

/**
 * @brief  Write one byte on the I2C bus (address or data).
 * @param  data  Byte to transmit.
 * @return I2C status code from the I2STAT register.
 */
uint8_t I2C0_Write(uint8_t data);

/**
 * @brief  Read one byte from the I2C bus with ACK.
 * @param  data  Pointer to store the received byte.
 * @return I2C status code from the I2STAT register.
 */
uint8_t I2C0_ReadACK(uint8_t *data);

/**
 * @brief  Read one byte from the I2C bus with NACK (last byte).
 * @param  data  Pointer to store the received byte.
 * @return I2C status code from the I2STAT register.
 */
uint8_t I2C0_ReadNACK(uint8_t *data);

/**
 * @brief  Write multiple bytes to an I2C slave device.
 * @param  slave_addr  7-bit slave address (will be left-shifted internally).
 * @param  buf         Pointer to data buffer to transmit.
 * @param  len         Number of bytes to transmit.
 * @return I2C_OK on success, or an error code.
 */
uint8_t I2C0_WriteBuffer(uint8_t slave_addr, const uint8_t *buf, uint32_t len);

/**
 * @brief  Read multiple bytes from an I2C slave device.
 * @param  slave_addr  7-bit slave address (will be left-shifted internally).
 * @param  buf         Pointer to buffer to store received data.
 * @param  len         Number of bytes to read.
 * @return I2C_OK on success, or an error code.
 */
uint8_t I2C0_ReadBuffer(uint8_t slave_addr, uint8_t *buf, uint32_t len);

#endif /* I2C_H */
