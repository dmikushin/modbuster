#include "ModbusterServer.h"

#include "Arduino.h"
#include "util/word.h"

using namespace ModBuster;

ModbusServer::ModbusServer(void) : ModbusBase() {}

/**
Initialize class object.

Assigns the Modbus slave ID and serial port.
Call once class has been instantiated, typically within setup().

@param slave Modbus slave ID (1..255)
@param &serial reference to serial port object (Serial, Serial1, ... Serial3)
@ingroup setup
*/
void ModbusServer::begin(uint8_t slave, Stream &serial) {
  //  txBuffer = (uint16_t*) calloc(ku8MaxBufferSize, sizeof(uint16_t));
  _u8MBSlave = slave;
  _serial = &serial;
  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;

#if __MODBUSMASTER_DEBUG__
  pinMode(__MODBUSMASTER_DEBUG_PIN_A__, OUTPUT);
  pinMode(__MODBUSMASTER_DEBUG_PIN_B__, OUTPUT);
#endif
}

uint16_t ModbusServer::getResponseTimeOut() const {
  return _u16MBResponseTimeout;
}

void ModbusServer::setResponseTimeOut(uint16_t u16MBResponseTimeout) {
  _u16MBResponseTimeout = u16MBResponseTimeout;
}

void ModbusServer::beginTransmission(uint16_t u16Address) {
  _u16WriteAddress = u16Address;
  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;
}

// eliminate this function in favor of using existing MB request functions
uint8_t ModbusServer::requestFrom(uint16_t address, uint16_t quantity) {
  uint8_t read;
  // clamp to buffer length
  if (quantity > ku8MaxBufferSize) {
    quantity = ku8MaxBufferSize;
  }
  // set rx buffer iterator vars
  _u8ResponseBufferIndex = 0;
  _u8ResponseBufferLength = read;

  return read;
}

void ModbusServer::sendBit(bool data) {
  uint8_t txBitIndex = u16TransmitBufferLength % 16;
  if ((u16TransmitBufferLength >> 4) < ku8MaxBufferSize) {
    if (0 == txBitIndex) {
      _u16TransmitBuffer[_u8TransmitBufferIndex] = 0;
    }
    bitWrite(_u16TransmitBuffer[_u8TransmitBufferIndex], txBitIndex, data);
    u16TransmitBufferLength++;
    _u8TransmitBufferIndex = u16TransmitBufferLength >> 4;
  }
}

void ModbusServer::send(uint16_t data) {
  if (_u8TransmitBufferIndex < ku8MaxBufferSize) {
    _u16TransmitBuffer[_u8TransmitBufferIndex++] = data;
    u16TransmitBufferLength = _u8TransmitBufferIndex << 4;
  }
}

void ModbusServer::send(uint32_t data) {
  send(lowWord(data));
  send(highWord(data));
}

void ModbusServer::send(uint8_t data) { send(word(data)); }

uint8_t ModbusServer::available(void) {
  return _u8ResponseBufferLength - _u8ResponseBufferIndex;
}

uint16_t ModbusServer::receive(void) {
  if (_u8ResponseBufferIndex < _u8ResponseBufferLength) {
    return _u16ResponseBuffer[_u8ResponseBufferIndex++];
  } else {
    return 0xFFFF;
  }
}

/**
Retrieve data from response buffer.

@see ModbusServer::clearResponseBuffer()
@param u8Index index of response buffer array (0x00..0x3F)
@return value in position u8Index of response buffer (0x0000..0xFFFF)
@ingroup buffer
*/
uint16_t ModbusServer::getResponseBuffer(uint8_t u8Index) {
  if (u8Index < ku8MaxBufferSize) {
    return _u16ResponseBuffer[u8Index];
  } else {
    return 0xFFFF;
  }
}

/**
Clear Modbus response buffer.

@see ModbusServer::getResponseBuffer(uint8_t u8Index)
@ingroup buffer
*/
void ModbusServer::clearResponseBuffer() {
  uint8_t i;

  for (i = 0; i < ku8MaxBufferSize; i++) {
    _u16ResponseBuffer[i] = 0;
  }
}

/**
Place data in transmit buffer.

@see ModbusServer::clearTransmitBuffer()
@param u8Index index of transmit buffer array (0x00..0x3F)
@param u16Value value to place in position u8Index of transmit buffer
(0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup buffer
*/
uint8_t ModbusServer::setTransmitBuffer(uint8_t u8Index, uint16_t u16Value) {
  if (u8Index < ku8MaxBufferSize) {
    _u16TransmitBuffer[u8Index] = u16Value;
    return ku8MBSuccess;
  } else {
    return ku8MBIllegalDataAddress;
  }
}

/**
Clear Modbus transmit buffer.

@see ModbusServer::setTransmitBuffer(uint8_t u8Index, uint16_t u16Value)
@ingroup buffer
*/
void ModbusServer::clearTransmitBuffer() {
  uint8_t i;

  for (i = 0; i < ku8MaxBufferSize; i++) {
    _u16TransmitBuffer[i] = 0;
  }
}

/**
Modbus function 0x01 Read Coils.

This function code is used to read from 1 to 2000 contiguous status of
coils in a remote device. The request specifies the starting address,
i.e. the address of the first coil specified, and the number of coils.
Coils are addressed starting at zero.

The coils in the response buffer are packed as one coil per bit of the
data field. Status is indicated as 1=ON and 0=OFF. The LSB of the first
data word contains the output addressed in the query. The other coils
follow toward the high order end of this word and from low order to high
order in subsequent words.

If the returned quantity is not a multiple of sixteen, the remaining
bits in the final data word will be padded with zeros (toward the high
order end of the word).

@param u16ReadAddress address of first coil (0x0000..0xFFFF)
@param u16BitQty quantity of coils to read (1..2000, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusServer::readCoils(uint16_t u16ReadAddress, uint16_t u16BitQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16BitQty;
  return ModbusServerTransaction(ku8MBReadCoils);
}

/**
Modbus function 0x02 Read Discrete Inputs.

This function code is used to read from 1 to 2000 contiguous status of
discrete inputs in a remote device. The request specifies the starting
address, i.e. the address of the first input specified, and the number
of inputs. Discrete inputs are addressed starting at zero.

The discrete inputs in the response buffer are packed as one input per
bit of the data field. Status is indicated as 1=ON; 0=OFF. The LSB of
the first data word contains the input addressed in the query. The other
inputs follow toward the high order end of this word, and from low order
to high order in subsequent words.

If the returned quantity is not a multiple of sixteen, the remaining
bits in the final data word will be padded with zeros (toward the high
order end of the word).

@param u16ReadAddress address of first discrete input (0x0000..0xFFFF)
@param u16BitQty quantity of discrete inputs to read (1..2000, enforced by
remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusServer::readDiscreteInputs(uint16_t u16ReadAddress,
                                         uint16_t u16BitQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16BitQty;
  return ModbusServerTransaction(ku8MBReadDiscreteInputs);
}

/**
Modbus function 0x03 Read Holding Registers.

This function code is used to read the contents of a contiguous block of
holding registers in a remote device. The request specifies the starting
register address and the number of registers. Registers are addressed
starting at zero.

The register data in the response buffer is packed as one word per
register.

@param u16ReadAddress address of the first holding register (0x0000..0xFFFF)
@param u16ReadQty quantity of holding registers to read (1..125, enforced by
remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::readHoldingRegisters(uint16_t u16ReadAddress,
                                           uint16_t u16ReadQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  return ModbusServerTransaction(ku8MBReadHoldingRegisters);
}

/**
Modbus function 0x04 Read Input Registers.

This function code is used to read from 1 to 125 contiguous input
registers in a remote device. The request specifies the starting
register address and the number of registers. Registers are addressed
starting at zero.

The register data in the response buffer is packed as one word per
register.

@param u16ReadAddress address of the first input register (0x0000..0xFFFF)
@param u16ReadQty quantity of input registers to read (1..125, enforced by
remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::readInputRegisters(uint16_t u16ReadAddress,
                                         uint8_t u16ReadQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  return ModbusServerTransaction(ku8MBReadInputRegisters);
}

/**
Modbus function 0x05 Write Single Coil.

This function code is used to write a single output to either ON or OFF
in a remote device. The requested ON/OFF state is specified by a
constant in the state field. A non-zero value requests the output to be
ON and a value of 0 requests it to be OFF. The request specifies the
address of the coil to be forced. Coils are addressed starting at zero.

@param u16WriteAddress address of the coil (0x0000..0xFFFF)
@param u8State 0=OFF, non-zero=ON (0x00..0xFF)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusServer::writeSingleCoil(uint16_t u16WriteAddress,
                                      uint8_t u8State) {
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = (u8State ? 0xFF00 : 0x0000);
  return ModbusServerTransaction(ku8MBWriteSingleCoil);
}

/**
Modbus function 0x06 Write Single Register.

This function code is used to write a single holding register in a
remote device. The request specifies the address of the register to be
written. Registers are addressed starting at zero.

@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16WriteValue value to be written to holding register (0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::writeSingleRegister(uint16_t u16WriteAddress,
                                          uint16_t u16WriteValue) {
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = 0;
  _u16TransmitBuffer[0] = u16WriteValue;
  return ModbusServerTransaction(ku8MBWriteSingleRegister);
}

/**
Modbus function 0x0F Write Multiple Coils.

This function code is used to force each coil in a sequence of coils to
either ON or OFF in a remote device. The request specifies the coil
references to be forced. Coils are addressed starting at zero.

The requested ON/OFF states are specified by contents of the transmit
buffer. A logical '1' in a bit position of the buffer requests the
corresponding output to be ON. A logical '0' requests it to be OFF.

@param u16WriteAddress address of the first coil (0x0000..0xFFFF)
@param u16BitQty quantity of coils to write (1..2000, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusServer::writeMultipleCoils(uint16_t u16WriteAddress,
                                         uint16_t u16BitQty) {
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = u16BitQty;
  return ModbusServerTransaction(ku8MBWriteMultipleCoils);
}
uint8_t ModbusServer::writeMultipleCoils() {
  _u16WriteQty = u16TransmitBufferLength;
  return ModbusServerTransaction(ku8MBWriteMultipleCoils);
}

/**
Modbus function 0x10 Write Multiple Registers.

This function code is used to write a block of contiguous registers (1
to 123 registers) in a remote device.

The requested written values are specified in the transmit buffer. Data
is packed as one word per register.

@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16WriteQty quantity of holding registers to write (1..123, enforced by
remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::writeMultipleRegisters(uint16_t u16WriteAddress,
                                             uint16_t u16WriteQty) {
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = u16WriteQty;
  return ModbusServerTransaction(ku8MBWriteMultipleRegisters);
}

// new version based on Wire.h
uint8_t ModbusServer::writeMultipleRegisters() {
  _u16WriteQty = _u8TransmitBufferIndex;
  return ModbusServerTransaction(ku8MBWriteMultipleRegisters);
}

/**
Modbus function 0x16 Mask Write Register.

This function code is used to modify the contents of a specified holding
register using a combination of an AND mask, an OR mask, and the
register's current contents. The function can be used to set or clear
individual bits in the register.

The request specifies the holding register to be written, the data to be
used as the AND mask, and the data to be used as the OR mask. Registers
are addressed starting at zero.

The function's algorithm is:

Result = (Current Contents && And_Mask) || (Or_Mask && (~And_Mask))

@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16AndMask AND mask (0x0000..0xFFFF)
@param u16OrMask OR mask (0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::maskWriteRegister(uint16_t u16WriteAddress,
                                        uint16_t u16AndMask,
                                        uint16_t u16OrMask) {
  _u16WriteAddress = u16WriteAddress;
  _u16TransmitBuffer[0] = u16AndMask;
  _u16TransmitBuffer[1] = u16OrMask;
  return ModbusServerTransaction(ku8MBMaskWriteRegister);
}

/**
Modbus function 0x17 Read Write Multiple Registers.

This function code performs a combination of one read operation and one
write operation in a single MODBUS transaction. The write operation is
performed before the read. Holding registers are addressed starting at
zero.

The request specifies the starting address and number of holding
registers to be read as well as the starting address, and the number of
holding registers. The data to be written is specified in the transmit
buffer.

@param u16ReadAddress address of the first holding register (0x0000..0xFFFF)
@param u16ReadQty quantity of holding registers to read (1..125, enforced by
remote device)
@param u16WriteAddress address of the first holding register (0x0000..0xFFFF)
@param u16WriteQty quantity of holding registers to write (1..121, enforced by
remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusServer::readWriteMultipleRegisters(uint16_t u16ReadAddress,
                                                 uint16_t u16ReadQty,
                                                 uint16_t u16WriteAddress,
                                                 uint16_t u16WriteQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = u16WriteQty;
  return ModbusServerTransaction(ku8MBReadWriteMultipleRegisters);
}
uint8_t ModbusServer::readWriteMultipleRegisters(uint16_t u16ReadAddress,
                                                 uint16_t u16ReadQty) {
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  _u16WriteQty = _u8TransmitBufferIndex;
  return ModbusServerTransaction(ku8MBReadWriteMultipleRegisters);
}

/* _____PRIVATE FUNCTIONS____________________________________________________ */
/**
Modbus transaction engine.
Sequence:
  - assemble Modbus Request Application Data Unit (ADU),
    based on particular function called
  - transmit request over selected serial port
  - wait for/retrieve response
  - evaluate/disassemble response
  - return status (success/exception)

@param u8MBFunction Modbus function (0x01..0xFF)
@return 0 on success; exception number on failure
*/
uint8_t ModbusServer::ModbusServerTransaction(uint8_t u8MBFunction) {
  uint8_t u8ModbusADU[256];
  uint8_t u8ModbusADUSize = 0;
  uint8_t i, u8Qty;
  uint32_t u32StartTime;
  uint8_t u8BytesLeft = 8;
  uint8_t u8MBStatus = ku8MBSuccess;

  // assemble Modbus Request Application Data Unit
  u8ModbusADU[u8ModbusADUSize++] = _u8MBSlave;
  u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;

  switch (u8MBFunction) {
  case ku8MBReadCoils:
  case ku8MBReadDiscreteInputs:
  case ku8MBReadInputRegisters:
  case ku8MBReadHoldingRegisters:
  case ku8MBReadWriteMultipleRegisters:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadAddress);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadAddress);
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadQty);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadQty);
    break;
  }

  switch (u8MBFunction) {
  case ku8MBWriteSingleCoil:
  case ku8MBMaskWriteRegister:
  case ku8MBWriteMultipleCoils:
  case ku8MBWriteSingleRegister:
  case ku8MBWriteMultipleRegisters:
  case ku8MBReadWriteMultipleRegisters:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteAddress);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteAddress);
    break;
  }

  switch (u8MBFunction) {
  case ku8MBWriteSingleCoil:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
    break;

  case ku8MBWriteSingleRegister:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[0]);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[0]);
    break;

  case ku8MBWriteMultipleCoils:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
    u8Qty =
        (_u16WriteQty % 8) ? ((_u16WriteQty >> 3) + 1) : (_u16WriteQty >> 3);
    u8ModbusADU[u8ModbusADUSize++] = u8Qty;
    for (i = 0; i < u8Qty; i++) {
      switch (i % 2) {
      case 0: // i is even
        u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[i >> 1]);
        break;

      case 1: // i is odd
        u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[i >> 1]);
        break;
      }
    }
    break;

  case ku8MBWriteMultipleRegisters:
  case ku8MBReadWriteMultipleRegisters:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty << 1);

    for (i = 0; i < lowByte(_u16WriteQty); i++) {
      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[i]);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[i]);
    }
    break;

  case ku8MBMaskWriteRegister:
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[0]);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[0]);
    u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[1]);
    u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[1]);
    break;
  }

  // append CRC
  uint16_t u16CRC = crc(u8ModbusADU, u8ModbusADUSize);
  u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize] = 0;

  // Optional additional user-defined work step.
  if (_preWrite) {
    _preWrite();
  }

  // flush receive buffer before transmitting request
  while (_serial->read() != -1)
    ;

#ifdef MODBUS_DEBUG
  debugSerialPort.println();
#endif

  for (i = 0; i < u8ModbusADUSize; i++) {
    _serial->write(u8ModbusADU[i]);

#ifdef MODBUS_DEBUG
    if (u8ModbusADU[i] < 15)
      debugSerialPort.print("0");
    debugSerialPort.print(u8ModbusADU[i], HEX);
    debugSerialPort.print(">");
#endif
  }

#ifdef MODBUS_DEBUG
  debugSerialPort.println();
#endif

  u8ModbusADUSize = 0;
  _serial->flush(); // flush transmit buffer

  // Optional additional user-defined work step.
  if (_postWrite) {
    _postWrite();
  }

  // Optional additional user-defined work step.
  if (_preRead) {
    _preRead();
  }

  // loop until we run out of time or bytes, or an error occurs
  u32StartTime = millis();
  while (u8BytesLeft && !u8MBStatus) {
    if (_serial->available()) {
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, true);
#endif
      uint8_t ch = _serial->read();

#ifdef MODBUS_DEBUG
      if (ch < 15)
        debugSerialPort.print("0");
      debugSerialPort.print(ch, HEX);
      debugSerialPort.print("<");
#endif

      if ((ch == _u8MBSlave) || u8ModbusADUSize) {
        u8ModbusADU[u8ModbusADUSize++] = ch;
        u8BytesLeft--;
      }
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, false);
#endif
    } else {
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, true);
#endif
      // Optional additional user-defined work step.
      if (_idleRead) {
        _idleRead();
      }
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, false);
#endif
    }

    // evaluate slave ID, function code once enough bytes have been read
    if (u8ModbusADUSize == 5) {
      // verify response is for correct Modbus function code (mask exception bit
      // 7)
      if ((u8ModbusADU[1] & 0x7F) != u8MBFunction) {
        u8MBStatus = ku8MBInvalidFunction;
        break;
      }

      // check whether Modbus exception occurred; return Modbus Exception Code
      if (bitRead(u8ModbusADU[1], 7)) {
        u8MBStatus = u8ModbusADU[2];
        break;
      }

      // evaluate returned Modbus function code
      switch (u8ModbusADU[1]) {
      case ku8MBReadCoils:
      case ku8MBReadDiscreteInputs:
      case ku8MBReadInputRegisters:
      case ku8MBReadHoldingRegisters:
      case ku8MBReadWriteMultipleRegisters:
        u8BytesLeft = u8ModbusADU[2];
        break;

      case ku8MBWriteSingleCoil:
      case ku8MBWriteMultipleCoils:
      case ku8MBWriteSingleRegister:
      case ku8MBWriteMultipleRegisters:
        u8BytesLeft = 3;
        break;

      case ku8MBMaskWriteRegister:
        u8BytesLeft = 5;
        break;
      }
    }
    if ((millis() - u32StartTime) > _u16MBResponseTimeout) {
      u8MBStatus = ku8MBResponseTimedOut;
    }
  }

  // verify response is large enough to inspect further
  if (!u8MBStatus && u8ModbusADUSize >= 5) {
    // calculate CRC
    uint16_t u16CRC = crc(u8ModbusADU, u8ModbusADUSize - 2);

    // verify CRC
    if (!u8MBStatus && (highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
                        lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1])) {
      u8MBStatus = ku8MBInvalidCRC;
    }
  }

  // Optional additional user-defined work step.
  if (_postRead) {
    _postRead();
  }

  // disassemble ADU into words
  if (!u8MBStatus) {
    // evaluate returned Modbus function code
    switch (u8ModbusADU[1]) {
    case ku8MBReadCoils:
    case ku8MBReadDiscreteInputs:
      // load bytes into word; response bytes are ordered L, H, L, H, ...
      for (i = 0; i < (u8ModbusADU[2] >> 1); i++) {
        if (i < ku8MaxBufferSize) {
          _u16ResponseBuffer[i] =
              word(u8ModbusADU[2 * i + 4], u8ModbusADU[2 * i + 3]);
        }

        _u8ResponseBufferLength = i;
      }

      // in the event of an odd number of bytes, load last byte into zero-padded
      // word
      if (u8ModbusADU[2] % 2) {
        if (i < ku8MaxBufferSize) {
          _u16ResponseBuffer[i] = word(0, u8ModbusADU[2 * i + 3]);
        }

        _u8ResponseBufferLength = i + 1;
      }
      break;

    case ku8MBReadInputRegisters:
    case ku8MBReadHoldingRegisters:
    case ku8MBReadWriteMultipleRegisters:
      // load bytes into word; response bytes are ordered H, L, H, L, ...
      for (i = 0; i < (u8ModbusADU[2] >> 1); i++) {
        if (i < ku8MaxBufferSize) {
          _u16ResponseBuffer[i] =
              word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
        }

        _u8ResponseBufferLength = i;
      }
      break;
    }
  }

  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;
  _u8ResponseBufferIndex = 0;
  return u8MBStatus;
}

/**
Modbus-like protocols transaction engine.
Sequence:
  - calculate CRC
  - transmit buffer  over selected serial port + CRC
  - wait for/retrieve response
  - return status (success/exception)

@param u8ModbusADU - pointer to buffer
@param u8ModbusADUSize - request  size
@param u8BytesLeft - how many bytes to be collected back (include CRC) - should
be less then buffer size
@return 0 on success; exception number on failure

*/
uint8_t ModbusServer::ModbusRawTransaction(uint8_t *u8ModbusADU,
                                           uint8_t u8ModbusADUSize,
                                           uint8_t u8BytesLeft) {
  uint32_t u32StartTime;

  uint8_t u8MBStatus = ku8MBSuccess;
  u8ModbusADU[0] = _u8MBSlave;

  // calculate CRC
  uint16_t u16CRC = crc(u8ModbusADU, u8ModbusADUSize - 2);

  // flush receive buffer before transmitting request
  while (_serial->read() != -1)
    ;

  // Optional additional user-defined work step.
  if (_preWrite) {
    _preWrite();
  }

#ifdef MODBUS_DEBUG
  debugSerialPort.println();
#endif

  for (uint8_t i = 0; i < u8ModbusADUSize; i++) {
    _serial->write(u8ModbusADU[i]);

#ifdef MODBUS_DEBUG
    if (u8ModbusADU[i] < 15)
      debugSerialPort.print("0");
    debugSerialPort.print(u8ModbusADU[i], HEX);
    debugSerialPort.print(">");
#endif
  }

  _serial->write(highByte(u16CRC));
  _serial->write(lowByte(u16CRC));

#ifdef MODBUS_DEBUG
  if (highByte(u16CRC) < 15)
    debugSerialPort.print("0");
  debugSerialPort.print(lowByte(u16CRC), HEX);

  if (lowByte(u16CRC) < 15)
    debugSerialPort.print("0");
  debugSerialPort.print(highByte(u16CRC), HEX);
  debugSerialPort.print(">");

  debugSerialPort.println();
#endif

  u8ModbusADUSize = 0;
  _serial->flush(); // flush transmit buffer

  // Optional additional user-defined work step.
  if (_postWrite) {
    _postWrite();
  }

  // Optional additional user-defined work step.
  if (_preRead) {
    _preRead();
  }

  // loop until we run out of time or bytes, or an error occurs
  u32StartTime = millis();
  while (u8BytesLeft && !u8MBStatus) {
    if (_serial->available()) {
      uint8_t ch;
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, true);
#endif
      ch = _serial->read();

#ifdef MODBUS_DEBUG
      if (ch < 15)
        debugSerialPort.print("0");
      debugSerialPort.print(ch, HEX);
      debugSerialPort.print("<");
#endif

      if ((ch == _u8MBSlave) || u8ModbusADUSize) {
        u8ModbusADU[u8ModbusADUSize++] = ch;
        u8BytesLeft--;
      }
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, false);
#endif
    } else {
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, true);
#endif
      if (_idleRead) {
        _idleRead();
      }
#if __MODBUSMASTER_DEBUG__
      digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, false);
#endif
    }

    if ((millis() - u32StartTime) > ku16MBResponseTimeout) {
      u8MBStatus = ku8MBResponseTimedOut;
    }
  }

  // verify response is large enough to inspect further
  if (!u8MBStatus && u8ModbusADUSize >= 4) {
    // calculate CRC
    uint16_t u16CRC = crc(u8ModbusADU, u8ModbusADUSize - 2);

    // verify CRC
    if (!u8MBStatus && (highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
                        lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1])) {
      u8MBStatus = ku8MBInvalidCRC;
    }
  }

  _u8TransmitBufferIndex = 0;
  u16TransmitBufferLength = 0;
  _u8ResponseBufferIndex = 0;

  // Optional additional user-defined work step.
  if (_postRead) {
    _postRead();
  }

  return u8MBStatus;
}
