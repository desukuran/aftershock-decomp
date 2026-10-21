// CRC-32 (the media check).
#ifndef MADNESS_CRC32_H
#define MADNESS_CRC32_H

#include <xtl.h>
#include <stdio.h>

class CCrc32
{
public:
    DWORD m_adwTable[256];      // +0x000

    CCrc32();
    DWORD FileCrc(const char* strPath);
};                              // 0x400 bytes

extern CCrc32 g_Crc32;

void MediaError(const char* strPath);
void VerifyMedia();

#endif
