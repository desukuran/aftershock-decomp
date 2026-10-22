// Text drawing with an XFONT bitmap font (D:\media\fonts\normal.bmf).
#ifndef STARWARS_FONT_H
#define STARWARS_FONT_H

#include <xtl.h>
#include <xfont.h>

class CGame;

class CFont
{
public:
    XFONT*             m_pFont;         // +0x00
    LPDIRECT3DSURFACE8 m_pBackBuffer;   // +0x04

    void Create(CGame* pGame, const WCHAR* strFile);
    void DrawText(const WCHAR* strText, float x, float y, D3DCOLOR color);
};                                      // 0x08 bytes

#endif
