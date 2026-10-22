// Text drawing with an XFONT bitmap font.
#include "Game.h"

// 0x00027130
// Opens the font if strFile isn't NULL.
void CFont::Create(CGame* pGame, const WCHAR* strFile)
{
    pGame->pDevice->GetBackBuffer(-1, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    if (strFile)
        XFONT_OpenBitmapFont(strFile, 16 * 1024, &m_pFont);
}

// 0x00027160
void CFont::DrawText(const WCHAR* strText, float x, float y, D3DCOLOR color)
{
    WCHAR strBuffer[200] = { 0 };
    wsprintfW(strBuffer, L"%s", strText);
    m_pFont->SetTextColor(color);
    m_pFont->TextOut(m_pBackBuffer, strBuffer, -1, (long)x, (long)y);
}
