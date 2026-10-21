// The intro's scrolling credits: g_CreditsText drawn as one textured quad per
// character (D:\media\fonts\font.dds), plus the logo (D:\media\logo.tga).
#ifndef AFTERSHOCK_CREDITS_H
#define AFTERSHOCK_CREDITS_H

#include <xtl.h>

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
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
};

extern char g_CreditsText[];
extern CCredits g_Credits;

#endif
