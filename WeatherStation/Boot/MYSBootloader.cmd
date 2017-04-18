avrdude -c usbasp -p atmega328p -U flash:w:MYSBL13pre_atmega328_8Mhz.hex 
;-U lfuse:w:0xFF:m -U hfuse:w:0xD2:m -U efuse:w:0x6:m
pause