#include <EEPROM.h>

void EEPROMReadStr(int Addr, char* Val, int Size) {
  char c;
  
  for (int i = 0; i < Size; ++i)
    {
      c = char(EEPROM.read(Addr+i));
      if (c >= 0x80) { c = 0x00; }
      Val[i] = c;
    }
}

void EEPROMWriteStr(int Addr, const char* Val, int Size){
  for (int i = 0; i < Size; ++i) { EEPROM.write(Addr+i, Val[i]); }
}

