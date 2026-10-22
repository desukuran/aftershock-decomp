// Text drawing with an XFONT bitmap font, into the front buffer.
#include "Game.h"

// 0x0001A9B0
// The default font is Arial 18.
void CFont::Create(LPDIRECT3DDEVICE8 pDevice, const WCHAR* strFile)
{
    m_pDevice = pDevice;
    pDevice->GetBackBuffer(-1, D3DBACKBUFFER_TYPE_MONO, &m_pFrontBuffer);
    if (strFile == NULL)
        XFONT_OpenBitmapFont(L"D:\\media\\font\\arial18normal.bmf", 16 * 1024, &m_pFont);
    else
        XFONT_OpenBitmapFont(strFile, 16 * 1024, &m_pFont);
}

// 0x0001A9F0
void CFont::DrawText(const WCHAR* strText, float x, float y, DWORD r, DWORD g, DWORD b)
{
    WCHAR strBuffer[200] = { 0 };
    wsprintfW(strBuffer, L"%s", strText);
    m_pFont->SetTextColor(D3DCOLOR_XRGB(r, g, b));
    m_pFont->TextOut(m_pFrontBuffer, strBuffer, -1, (long)x, (long)y);
}
