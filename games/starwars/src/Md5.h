// MD5 (RSA Data Security's reference MD5 in a class) with the digest as a
// hex string. Used to check that the intro's text and the release's files
// haven't been changed.
#ifndef STARWARS_MD5_H
#define STARWARS_MD5_H

#include <xtl.h>
#include <string>
#include <stdio.h>

class MD5
{
public:
    MD5();
    virtual ~MD5() {}

    void Update(const BYTE* pInput, unsigned int nLength);
    std::string Final();                // the digest as 32 lowercase hex digits
    void Transform(const BYTE block[64]);

    BYTE  m_Buffer[64];                 // +0x04 input not yet transformed
    DWORD m_Count[2];                   // +0x44 number of bits, modulo 2^64
    DWORD m_State[4];                   // +0x4C A, B, C, D
};                                      // 0x5C bytes

std::string MD5String(const char* strText, unsigned int nLength);
std::string MD5File(FILE* f);
std::string MD5FilePath(const char* strFile);

#endif
