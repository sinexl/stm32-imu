#!/usr/bin/env sh
#
# set -xe
# 
PROGRAMMER_INSTALLATION_DIR=$HOME/opt/STM32CubeProgrammer/
BUILD=./build/
EXE_NAME=UARTTest
ADDRESS=0x08000000 

BIN=$BUILD/$EXE_NAME.elf

for arg in "$@"; do
  if [ "$arg" = "--raw" ] || [ "$arg" = "-r" ]; then
    BIN="$BUILD/$EXE_NAME.bin $ADDRESS"
  fi
done

echo Flashing $BIN ..

$PROGRAMMER_INSTALLATION_DIR/bin/STM32_Programmer_CLI \
    -c port=SWD \
    -w $BIN \
    -v -rst
