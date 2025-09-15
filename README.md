# EESE

EESE is an **E**mbedded **E**nvironmental **S**ensor **E**xample. This
project is functional, but the main purpose is to show some of the
techniques used in efficient development of a effective product.

Features:

- I2C temperature and humidity sensor,
- UART/serial interface,
- CRC32 and COBS to ensure packet integrity,
- python API for easy integration

## Building

Required packages:

- make
- gcc-arm-none-eabi
- picolibc-arm-none-eabi
- python3
- g++

    make

## Running the tests

Required packages:

- python3-pytest
- python3-pytest-cov
- python3-serial
- sensible-utils

    make test

To see generate and view a HTML coverage report

    make coverage

License: see License file.
