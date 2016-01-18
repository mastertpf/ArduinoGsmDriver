/**
 * Arduino - Gsm driver
 * 
 * SIM900.cpp
 * 
 * SIM900 implementation of the SIM900 modem.
 * 
 * @author Dalmir da Silva <dalmirdasilva@gmail.com>
 */

#ifndef __ARDUINO_DRIVER_GSM_SIM900_CPP__
#define __ARDUINO_DRIVER_GSM_SIM900_CPP__ 1

#include <Arduino.h>
#include "SIM900.h"

SIM900::SIM900(unsigned char receivePin, unsigned char transmitPin)
        : SIM900(receivePin, transmitPin, 0, 0) {
}

SIM900::SIM900(unsigned char receivePin, unsigned char transmitPin, unsigned char resetPin, unsigned char powerPin)
        : SoftwareSerial(receivePin, transmitPin), rxBufferPos(0), echo(true), resetPin(resetPin), powerPin(powerPin), responseFullyRead(
        true) {
    rxBuffer[0] = '\0';
    pinMode(resetPin, OUTPUT);
    pinMode(powerPin, OUTPUT);
    softResetAndPowerEnabled = !(resetPin == 00 && powerPin == 0);
}

SIM900::~SIM900() {
}

unsigned char SIM900::begin(long bound) {
    SoftwareSerial::begin(bound);
    if (sendCommandExpecting("AT", "OK")) {
        return 1;
    }
    softPower();
    return (unsigned char) (waitUntilReceive("Call Ready", SIM900_INITIALIZATION_TIMEOUT) >= 0);
}

void SIM900::softReset() {
    if (softResetAndPowerEnabled) {
        digitalWrite(resetPin, HIGH);
        delay(100);
        digitalWrite(resetPin, LOW);
    }
}

void SIM900::softPower() {
    if (softResetAndPowerEnabled) {
        digitalWrite(powerPin, HIGH);
        delay(1000);
        digitalWrite(powerPin, LOW);
    }
}

bool SIM900::sendCommandExpecting(const char *command, const char *expectation, bool append, unsigned long timeout) {
    if (sendCommand(command, append, timeout) == 0) {
        return false;
    }
    return doesResponseContains(expectation);
}

bool SIM900::doesResponseContains(const char *expectation) {
    return findInResponse(expectation) != NULL;
}

unsigned int SIM900::sendCommand(const char *command, bool append, unsigned long timeout) {
    rxBufferPos = 0;
    rxBuffer[0] = '\0';
    flush();
    if (append) {
        print("AT");
    }
    println(command);
    return readResponse(timeout);
}

unsigned int SIM900::readResponse(unsigned long timeout, bool append) {
    int availableBytes;
    unsigned long pos, last, now, start;
    unsigned int read;
    last = start = millis();
    if (!append) {
        rxBufferPos = 0;
    }
    pos = rxBufferPos;
    responseFullyRead = true;
    while (available() <= 0 && (millis() - start) < timeout)
        ;
    start = millis();
    do {
        availableBytes = available();
        if (availableBytes > 0) {
            if (rxBufferPos + availableBytes >= SIM900_RX_BUFFER_SIZE) {
                availableBytes = SIM900_RX_BUFFER_SIZE - (rxBufferPos + 1);
                responseFullyRead = false;
            }
            if (availableBytes > 0) {
                last = millis();
                // we have the guaranty that is not going to be too big because it is constrained by the buffer size.
                read = readBytes((char *) &rxBuffer[rxBufferPos], availableBytes);
                rxBufferPos += read;
                rxBuffer[rxBufferPos] = '\0';
            }
        }
        now = millis();
    } while ((availableBytes > 0 || (now - last) < SIM900_SERIAL_INTERBYTE_TIMEOUT) && (now - start) < timeout && responseFullyRead);
    return rxBufferPos - pos;
}

void SIM900::setEcho(bool echo) {
    this->echo = echo;
    char command[] = "E0";
    if (echo) {
        command[1] = '1';
    }
    sendCommand(command, true, 100);
}

unsigned char SIM900::disconnect(DisconnectParamter param) {
    // TODO
    return 3;
}

bool SIM900::wasResponseFullyRead() {
    return responseFullyRead;
}

const char *SIM900::findInResponse(const char *str) {
    return strstr((const char *) &rxBuffer[0], str);
}

int SIM900::waitUntilReceive(const char *str, unsigned int timeout) {
    const char *pos;
    while ((pos = findInResponse(str)) == NULL && readResponse(timeout, responseFullyRead) > 0)
        ;
    if (pos != NULL) {
        return (int) (pos - (const char *) &rxBuffer[0]);
    }
    return -1;
}

void SIM900::discardBuffer() {
    rxBuffer[0] = '\0';
}

#endif /* __ARDUINO_DRIVER_GSM_SIM900_CPP__ */
