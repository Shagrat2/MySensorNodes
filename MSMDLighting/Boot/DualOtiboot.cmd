avrdude -c usbasp -p atmega328p -U flash:w:DualOtiboot_atmega328_1k_d9LED_debugOFF.hex -U lfuse:w:0xFF:m -U hfuse:w:0xD2:m -U efuse:w:0x6:m
pause