avrdude -c usbasp -p atmega328p -U flash:w:dualoptiboot_atmega328.hex -U lfuse:w:0xFF:m -U hfuse:w:0xD2:m -U efuse:w:0x06:m
pause