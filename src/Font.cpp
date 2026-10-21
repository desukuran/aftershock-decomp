// Text drawing with an XFONT bitmap font.
#include "Font.h"
#include "Game.h"

// 0x00011190
void CFont::Create()
{
    g_Game.pDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    XFONT_OpenBitmapFont(L"D:\\media\\font\\font.bmf", 16 * 1024, &m_pFont);
}

// 0x000111B0
void CFont::DrawText(const WCHAR* strText, float x, float y, DWORD r, DWORD g, DWORD b)
{
    WCHAR strBuffer[202] = { 0 };
    wsprintfW(strBuffer, L"%s", strText);
    m_pFont->SetTextColor(D3DCOLOR_XRGB(r, g, b));
    m_pFont->TextOut(m_pBackBuffer, strBuffer, -1, (long)x, (long)y);
}
