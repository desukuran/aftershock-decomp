// Text drawing with an XFONT bitmap font (D:\media\font\font.bmf).
#ifndef MADNESS_FONT_H
#define MADNESS_FONT_H

#include <xtl.h>
#include <xfont.h>

class CFont
{
public:
    XFONT*             m_pFont;         // +0x00
    LPDIRECT3DSURFACE8 m_pBackBuffer;   // +0x04

    void Create();
    void DrawText(const WCHAR* strText, float x, float y, DWORD r, DWORD g, DWORD b);
};

#endif
