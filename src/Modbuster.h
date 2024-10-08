#ifndef MODBUSTER_H
#define MODBUSTER_H

#include <stdint.h>

// Uncomment MODBUS_DEBUG to print the message content to the serial port
//#define MODBUS_DEBUG

#ifndef debugSerialPort
#define debugSerialPort Serial
#endif

// Set to 1 to enable debugging features within class:
// PIN A cycles for each byte read in the Modbus response
// PIN B cycles for each millisecond timeout during the Modbus response
#define __MODBUSMASTER_DEBUG__ (0)
#define __MODBUSMASTER_DEBUG_PIN_A__ 4
#define __MODBUSMASTER_DEBUG_PIN_B__ 5

namespace ModBuster {

// Modbus exception codes
enum ModbusExpectionCodes {
  /**
  Modbus protocol illegal function exception.

  The function code received in the query is not an allowable action for
  the server (or slave). This may be because the function code is only
  applicable to newer devices, and was not implemented in the unit
  selected. It could also indicate that the server (or slave) is in the
  wrong state to process a request of this type, for example because it is
  unconfigured and is being asked to return register values.

  @ingroup constant
  */
  ku8MBIllegalFunction = 0x01,

  /**
  Modbus protocol illegal data address exception.

  The data address received in the query is not an allowable address for
  the server (or slave). More specifically, the combination of reference
  number and transfer length is invalid. For a controller with 100
  registers, the ADU addresses the first register as 0, and the last one
  as 99. If a request is submitted with a starting register address of 96
  and a quantity of registers of 4, then this request will successfully
  operate (address-wise at least) on registers 96, 97, 98, 99. If a
  request is submitted with a starting register address of 96 and a
  quantity of registers of 5, then this request will fail with Exception
  Code 0x02 "Illegal Data Address" since it attempts to operate on
  registers 96, 97, 98, 99 and 100, and there is no register with address
  100.

  @ingroup constant
  */
  ku8MBIllegalDataAddress = 0x02,

  /**
  Modbus protocol illegal data value exception.

  A value contained in the query data field is not an allowable value for
  server (or slave). This indicates a fault in the structure of the
  remainder of a complex request, such as that the implied length is
  incorrect. It specifically does NOT mean that a data item submitted for
  storage in a register has a value outside the expectation of the
  application program, since the MODBUS protocol is unaware of the
  significance of any particular value of any particular register.

  @ingroup constant
  */
  ku8MBIllegalDataValue = 0x03,

  /**
  Modbus protocol slave device failure exception.

  An unrecoverable error occurred while the server (or slave) was
  attempting to perform the requested action.

  @ingroup constant
  */
  ku8MBSlaveDeviceFailure = 0x04,

  // Class-defined success/exception codes
  /**
  ModbusServer success.

  Modbus transaction was successful; the following checks were valid:
    - slave ID
    - function code
    - response code
    - data
    - CRC

  @ingroup constant
  */
  ku8MBSuccess = 0x00,

  /**
  ModbusServer invalid response slave ID exception.

  The slave ID in the response does not match that of the request.

  @ingroup constant
  */
  ku8MBInvalidSlaveID = 0xE0,

  /**
  ModbusServer invalid response function exception.

  The function code in the response does not match that of the request.

  @ingroup constant
  */
  ku8MBInvalidFunction = 0xE1,

  /**
  ModbusServer response timed out exception.

  The entire response was not received within the timeout period,
  ModbusServer::ku8MBResponseTimeout.

  @ingroup constant
  */
  ku8MBResponseTimedOut = 0xE2,

  /**
  ModbusServer invalid response CRC exception.

  The CRC in the response does not match the one calculated.

  @ingroup constant
  */
  ku8MBInvalidCRC = 0xE3,
};

// Modbus function codes for bit access
enum ModbusFunction {
  ku8MBReadCoils = 0x01,          ///< Modbus function 0x01 Read Coils
  ku8MBReadDiscreteInputs = 0x02, ///< Modbus function 0x02 Read Discrete Inputs
  ku8MBWriteSingleCoil = 0x05,    ///< Modbus function 0x05 Write Single Coil
  ku8MBWriteMultipleCoils = 0x0F, ///< Modbus function 0x0F Write Multiple Coils
};

// Modbus function codes for 16 bit access
enum ModbusFunction16bit {
  ku8MBReadHoldingRegisters =
      0x03, ///< Modbus function 0x03 Read Holding Registers
  ku8MBReadInputRegisters = 0x04, ///< Modbus function 0x04 Read Input Registers
  ku8MBWriteSingleRegister =
      0x06, ///< Modbus function 0x06 Write Single Register
  ku8MBWriteMultipleRegisters =
      0x10, ///< Modbus function 0x10 Write Multiple Registers
  ku8MBMaskWriteRegister = 0x16, ///< Modbus function 0x16 Mask Write Register
  ku8MBReadWriteMultipleRegisters =
      0x17, ///< Modbus function 0x17 Read Write Multiple Registers
};

/**
 * @enum MESSAGE
 * @brief
 * Indexes to telegram frame positions
 */
enum ModbusFramePosition {
  ID = 0,  //!< ID field
  FUNC,    //!< Function code position
  ADD_HI,  //!< Address high byte
  ADD_LO,  //!< Address low byte
  NB_HI,   //!< Number of coils or registers high byte
  NB_LO,   //!< Number of coils or registers low byte
  BYTE_CNT //!< byte counter
};

// Size of response/transmit buffers
const uint8_t ku8MaxBufferSize = 64;

// Slave to master response size
const uint8_t ku8ResponseSize = 6;

// Modbus default timeout [milliseconds]
const uint16_t ku16MBResponseTimeout = 2000;

class ModbusBase {
protected:
  // Optional additional user-defined work step.
  void (*_preRead)() = nullptr;
  void (*_idleRead)() = nullptr;
  void (*_postRead)() = nullptr;
  void (*_preWrite)() = nullptr;
  void (*_postWrite)() = nullptr;

  ModbusBase();

public:
  void preRead(void (*)());
  void idleRead(void (*)());
  void postRead(void (*)());
  void preWrite(void (*)());
  void postWrite(void (*)());
};

uint16_t crc(uint8_t *au8Buffer, uint8_t u8length);

} // namespace ModBuster

#endif // MODBUSTER_H
