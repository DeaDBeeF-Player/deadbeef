//
// Audio Overload
// Emulated music player
//
// (C) 2000-2008 Richard F. Bannister
//

// mem.h

uint8 memory_read(uint16 addr);
uint8 memory_readop(uint16 addr);
uint8 memory_readport(uint16 addr);
void memory_write(uint16 addr, uint8 byte);
void memory_writeport(uint16 addr, uint8 byte);

uint8 dc_read8(uint32 addr);
uint16 dc_read16(uint32 addr);
uint32 dc_read32(uint32 addr);
void dc_write8(uint32 addr, uint8 byte);
void dc_write16(uint32 addr, uint16 word);
void dc_write32(uint32 addr, uint32 dword);

