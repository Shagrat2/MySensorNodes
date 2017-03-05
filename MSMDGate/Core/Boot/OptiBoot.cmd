avrdude -c usbasp -p atmega328p -U flash:w:optiboot_atmega328.hex -U lfuse:w:0xFF:m -U hfuse:w:0xDE:m -U efuse:w:0x05:m
pause