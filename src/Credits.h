// The intro's scrolling credits: g_CreditsText drawn as one textured quad per
// character (D:\media\font\font.dds), plus the logo (D:\media\logo.tga).
// The same class as AfterShock Pool SX's, without the MD5 check (this game
// checks its media with CRCs instead, see Crc32.h).
#ifndef MADNESS_CREDITS_H
#define MADNESS_CREDITS_H

#include <xtl.h>

// The vertex format of the credits, the quads and the sprites.
struct SPRITEVERTEX
{
    float x, y, z;
    D3DCOLOR color;
    float tu, tv;
};
#define D3DFVF_SPRITEVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

class CCredits
{
public:
    float                   x;                  // +0x00 never used after Reset
    float                   y;                  // +0x04 scroll position
    LPDIRECT3DTEXTURE8      m_pFontTexture;     // +0x08
    LPDIRECT3DTEXTURE8      m_pLogoTexture;     // +0x0C
    LPDIRECT3DVERTEXBUFFER8 m_pVB;              // +0x10

    void Init(LPDIRECT3DDEVICE8 pDevice);
    void Reset();
    HRESULT Render();
};                                              // 0x14 bytes

extern char g_CreditsText[];
extern CCredits g_Credits;

#endif
