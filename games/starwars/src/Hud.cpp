// The HUD: crosshair, shield bar and health ticks, and the music HUD.
#include "Game.h"

static inline TLVERTEX TLVertex(float x, float y, D3DCOLOR color, float tu, float tv)
{
    TLVERTEX v;
    v.x = x;
    v.y = y;
    v.z = 0.0f;
    v.rhw = 0.1f;
    v.color = color;
    v.tu = tu;
    v.tv = tv;
    return v;
}

// 0x00016250
HRESULT CHud::Create(LPDIRECT3DDEVICE8 pDevice)
{
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "D:\\hud\\lifebar.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
                                            &texLifeBar)))
        return E_FAIL;
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "D:\\hud\\life.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
                                            &texLife)))
        return E_FAIL;
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "D:\\hud\\ch2blue.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
                                            &texCrossBlue)))
        return E_FAIL;
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "D:\\hud\\ch2red.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
                                            &texCrossRed)))
        return E_FAIL;
    texCross = texCrossBlue;
    return S_OK;
}

// 0x00015BF0
// The player's X-wing: shields as a bar, health as up to ten ticks that go
// from blue to red (and back to full red under 20).
HRESULT CHud::Render(LPDIRECT3DDEVICE8 pDevice, CGame* pGame)
{
    int nShields = pGame->pXWings[0].nShields;
    int nHealth = pGame->pXWings[0].nHealth;
    TLVERTEX v[4];

    v[0] = TLVertex((float)(nWidth / 2 - 32) - 0.5f, (float)(nHeight / 2 - 32) - 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    v[1] = TLVertex((float)(nWidth / 2 + 32) - 0.5f, (float)(nHeight / 2 - 32) - 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    v[2] = TLVertex((float)(nWidth / 2 + 32) - 0.5f, (float)(nHeight / 2 + 32) - 0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    v[3] = TLVertex((float)(nWidth / 2 - 32) - 0.5f, (float)(nHeight / 2 + 32) - 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    pDevice->SetTexture(0, texCross);
    pDevice->SetVertexShader(D3DFVF_TLVERTEX);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    v[0] = TLVertex(471.5f, 39.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    v[1] = TLVertex(599.5f, 39.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    v[2] = TLVertex(599.5f, 69.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    v[3] = TLVertex(471.5f, 69.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    pDevice->SetTexture(0, texLifeBar);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    float x = (598.0f - nShields * 1.24f) - 0.5f;
    v[0] = TLVertex(x, 41.5f, 0x640000FF, 0.0f, 0.0f);
    v[1] = TLVertex(597.5f, 41.5f, 0x640000FF, 1.0f, 0.0f);
    v[2] = TLVertex(597.5f, 67.5f, 0x640000FF, 1.0f, 1.0f);
    v[3] = TLVertex(x, 67.5f, 0x640000FF, 0.0f, 1.0f);
    pDevice->SetTexture(0, NULL);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    pDevice->SetTexture(0, texLife);
    float fHealth = nHealth * 0.01f;   // 0..1
    int a = (int)(fHealth * 255.0f);
    int nTicks = (int)(fHealth * 10.0f);
    if (a < 50)
        a = 255;
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(255 - a, 0, a));
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
    for (int right = 600; nTicks > 0; nTicks--, right -= 13)
    {
        v[0] = TLVertex((right - 10.0f) - 0.5f, 74.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        v[1] = TLVertex(right - 0.5f, 74.5f, 0xFFFFFFFF, 1.0f, 0.0f);
        v[2] = TLVertex(right - 0.5f, 90.5f, 0xFFFFFFFF, 1.0f, 1.0f);
        v[3] = TLVertex((right - 10.0f) - 0.5f, 90.5f, 0xFFFFFFFF, 0.0f, 1.0f);
        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));
    }
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    return S_OK;
}

// 0x00015160
// One player's HUD in a split screen. layout: 0 the whole screen, 1 and 2
// the top and bottom halves, 3 to 6 the quarters (players 1 to 4 left to
// right, top to bottom). Shields and health count out of 25. Another layout
// leaves everything unset.
HRESULT CHud::RenderMulti(LPDIRECT3DDEVICE8 pDevice, CGame* pGame, int layout)
{
    int nHealth, nShields;
    float crossLeft, crossTop, crossRight, crossBottom;
    int barLeft, barRight, barTop, barBottom, barLength;
    int tickWidth, tickTop, tickBottom;

    switch (layout)
    {
    case 0:
        nHealth = pGame->pPlayerShips[0].nHealth;
        nShields = pGame->pPlayerShips[0].nShields;
        crossTop = (float)(nHeight / 2 - 32);
        crossBottom = (float)(nHeight / 2 + 32);
        crossLeft = (float)(nWidth / 2 - 32);
        crossRight = (float)(nWidth / 2 + 32);
        barLength = 128; barTop = 40; barBottom = 70; barRight = 600; barLeft = 472;
        tickWidth = 10; tickTop = 75; tickBottom = 91;
        break;
    case 1:
        nHealth = pGame->pPlayerShips[0].nHealth;
        nShields = pGame->pPlayerShips[0].nShields;
        crossTop = (float)(nHeight / 4 - 24);
        crossBottom = (float)(nHeight / 4 + 24);
        crossLeft = (float)(nWidth / 2 - 24);
        crossRight = (float)(nWidth / 2 + 24);
        barLength = 128; barTop = 40; barBottom = 60; barRight = 600; barLeft = 472;
        tickWidth = 10; tickTop = 62; tickBottom = 78;
        break;
    case 2:
        nHealth = pGame->pPlayerShips[1].nHealth;
        nShields = pGame->pPlayerShips[1].nShields;
        crossTop = (float)(nHeight * 3 / 4 - 24);
        crossBottom = (float)(nHeight * 3 / 4 + 24);
        crossLeft = (float)(nWidth / 2 - 24);
        crossRight = (float)(nWidth / 2 + 24);
        barLength = 128; barTop = 280; barBottom = 300; barRight = 600; barLeft = 472;
        tickWidth = 10; tickTop = 302; tickBottom = 318;
        break;
    case 3:
        nHealth = pGame->pPlayerShips[0].nHealth;
        nShields = pGame->pPlayerShips[0].nShields;
        crossTop = (float)(nHeight / 4 - 16);
        crossBottom = (float)(nHeight / 4 + 16);
        barRight = 320; barLeft = 224;
        crossLeft = (float)(nWidth / 4 - 16);
        crossRight = (float)(nWidth / 4 + 16);
        barLength = 96; barTop = 40; barBottom = 58;
        tickWidth = 10; tickTop = 60; tickBottom = 76;
        break;
    case 4:
        nHealth = pGame->pPlayerShips[1].nHealth;
        nShields = pGame->pPlayerShips[1].nShields;
        crossTop = (float)(nHeight / 4 - 16);
        crossBottom = (float)(nHeight / 4 + 16);
        barRight = 600; barLeft = 504;
        crossLeft = (float)(nWidth * 3 / 4 - 16);
        crossRight = (float)(nWidth * 3 / 4 + 16);
        barLength = 96; barTop = 40; barBottom = 58;
        tickWidth = 10; tickTop = 60; tickBottom = 76;
        break;
    case 5:
        nHealth = pGame->pPlayerShips[2].nHealth;
        nShields = pGame->pPlayerShips[2].nShields;
        crossTop = (float)(nHeight * 3 / 4 - 16);
        crossBottom = (float)(nHeight * 3 / 4 + 16);
        barRight = 320; barLeft = 224;
        crossLeft = (float)(nWidth / 4 - 16);
        crossRight = (float)(nWidth / 4 + 16);
        barLength = 96; barTop = 250; barBottom = 268;
        tickWidth = 10; tickTop = 270; tickBottom = 286;
        break;
    case 6:
        nHealth = pGame->pPlayerShips[3].nHealth;
        nShields = pGame->pPlayerShips[3].nShields;
        crossTop = (float)(nHeight * 3 / 4 - 16);
        crossBottom = (float)(nHeight * 3 / 4 + 16);
        barRight = 600; barLeft = 504;
        crossLeft = (float)(nWidth * 3 / 4 - 16);
        crossRight = (float)(nWidth * 3 / 4 + 16);
        barLength = 96; barTop = 250; barBottom = 268;
        tickWidth = 10; tickTop = 270; tickBottom = 286;
        break;
    }

    TLVERTEX v[4];
    v[0] = TLVertex(crossLeft - 0.5f, crossTop - 0.5f, 0x64FFFFFF, 0.0f, 0.0f);
    v[1] = TLVertex(crossRight - 0.5f, crossTop - 0.5f, 0x64FFFFFF, 1.0f, 0.0f);
    v[2] = TLVertex(crossRight - 0.5f, crossBottom - 0.5f, 0x64FFFFFF, 1.0f, 1.0f);
    v[3] = TLVertex(crossLeft - 0.5f, crossBottom - 0.5f, 0x64FFFFFF, 0.0f, 1.0f);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetTexture(0, texCrossBlue);
    pDevice->SetVertexShader(D3DFVF_TLVERTEX);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    v[0] = TLVertex((float)barLeft - 0.5f, (float)barTop - 0.5f, 0x64FFFFFF, 0.0f, 0.0f);
    v[1] = TLVertex((float)barRight - 0.5f, (float)barTop - 0.5f, 0x64FFFFFF, 1.0f, 0.0f);
    v[2] = TLVertex((float)barRight - 0.5f, (float)barBottom - 0.5f, 0x64FFFFFF, 1.0f, 1.0f);
    v[3] = TLVertex((float)barLeft - 0.5f, (float)barBottom - 0.5f, 0x64FFFFFF, 0.0f, 1.0f);
    pDevice->SetTexture(0, texLifeBar);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    int right = barRight - 2;
    float x = (float)(int)((float)right - (float)(barLength - 4) * (float)nShields * 0.04f) - 0.5f;
    v[0] = TLVertex(x, (float)(barTop + 2) - 0.5f, 0x640000FF, 0.0f, 0.0f);
    v[1] = TLVertex((float)right - 0.5f, (float)(barTop + 2) - 0.5f, 0x640000FF, 1.0f, 0.0f);
    v[2] = TLVertex((float)right - 0.5f, (float)(barBottom - 2) - 0.5f, 0x640000FF, 1.0f, 1.0f);
    v[3] = TLVertex(x, (float)(barBottom - 2) - 0.5f, 0x640000FF, 0.0f, 1.0f);
    pDevice->SetTexture(0, NULL);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    pDevice->SetTexture(0, texLife);
    float fHealth = nHealth * 0.04f;   // 0..1
    int a = (int)(fHealth * 255.0f);
    int nTicks = (int)(fHealth * 10.0f);
    if (a < 50)
        a = 255;
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(255 - a, 0, a));
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
    for (; nTicks > 0; nTicks--, right -= tickWidth + 3)
    {
        v[0] = TLVertex((float)(right - tickWidth) - 0.5f, (float)tickTop - 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        v[1] = TLVertex((float)right - 0.5f, (float)tickTop - 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
        v[2] = TLVertex((float)right - 0.5f, (float)tickBottom - 0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
        v[3] = TLVertex((float)(right - tickWidth) - 0.5f, (float)tickBottom - 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));
    }
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    return S_OK;
}

// 0x00016320
// The soundtrack and song (cut to 23 characters, unterminated if longer)
// and the position in the song, on sndtrck.tga. Shown for 1.5 s after a
// song changes.
void RenderMusicHud(CGame* pGame)
{
    pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
    TLVERTEX v[4];
    v[0] = TLVertex(309.5f, 339.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    v[1] = TLVertex(609.5f, 339.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    v[2] = TLVertex(609.5f, 467.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    v[3] = TLVertex(309.5f, 467.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    pGame->pDevice->SetTexture(0, pGame->texSoundtrack);
    pGame->pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));

    WCHAR strSong[30], strSoundtrack[30];
    WCHAR strSoundtrackName[MAX_SOUNDTRACK_NAME], strSongName[MAX_SONG_NAME];
    WCHAR strTime[100];
    DWORD dwLength;
    float fPosition = pGame->music.GetPlaybackPosition();
    pGame->music.GetCurrentInfo(strSoundtrackName, strSongName, &dwLength);
    swprintf(strTime, L"%02d:%02d / %02d:%02d", (int)(fPosition * (1.0f / 60.0f)), (DWORD)fPosition % 60,
             dwLength / 60000, dwLength / 1000 % 60);
    wcsncpy(strSong, strSongName, 23);
    wcsncpy(strSoundtrack, strSoundtrackName, 23);
    pGame->font.DrawText(strSoundtrack, 325.0f, 375.0f, 0xFFFFFFFF);
    pGame->font.DrawText(strSong, 325.0f, 405.0f, 0xFFFFFFFF);
    pGame->font.DrawText(L"Position: ", 325.0f, 435.0f, 0xFFFFFFFF);
    pGame->font.DrawText(strTime, 425.0f, 435.0f, 0xFFFFFFFF);
}
