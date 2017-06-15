avrdude -c usbasp -p atmega328p -U flash:w:dualoptiboot_at24c512c.hex -U lfuse:w:0xFF:m -U hfuse:w:0xD2:m -U efuse:w:0x06:m
pause