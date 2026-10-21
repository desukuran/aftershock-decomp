// The intro's scrolling credits.
#include "Credits.h"
#include "Game.h"

// 0x000A2278
// Shown 25 characters to a line. The text is the original release's; the
// build extracts it from default.xbe (tools/extract_data.py).
char g_CreditsText[] =
#include "credits.inc"
    ;

CCredits g_Credits;     // 0x000A6108

// 0x00011C50
void CCredits::Init(LPDIRECT3DDEVICE8 pDevice)
{
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\logo.tga", 0, 0, 0, 0, D3DFMT_UNKNOWN,
            D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL, &m_pLogoTexture)))
        OutputDebugStringA("couldn't load up the logo\n");
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\font\\font.dds", 256, 128, 0, 0, D3DFMT_UNKNOWN,
            D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL, &m_pFontTexture)))
        OutputDebugStringA("couldn't load up the font\n");

    if (FAILED(pDevice->CreateVertexBuffer(strlen(g_CreditsText) * 4 * sizeof(SPRITEVERTEX), 0, 0, 0, &m_pVB)))
        OutputDebugStringA("couldn't create font VB\n");

    // One quad per character, 8x8 units, 25 to a line. font.dds holds 16 x 8
    // glyphs starting at ' '.
    SPRITEVERTEX* pVertices;
    m_pVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    SPRITEVERTEX quad[4];
    quad[0].color = quad[1].color = quad[2].color = quad[3].color = 0xFFFFFFFF;
    float cx = -96.0f;
    float cy = 32.0f;
    for (unsigned int i = 0; i < strlen(g_CreditsText); i++)
    {
        int c = g_CreditsText[i];
        int row = (c - ' ') / 16;
        float tu = (float)(c - (int)(row * 16.0f) - ' ') * (1.0f / 16);
        float tv = row * (1.0f / 8);

        quad[0].x = cx;        quad[0].y = cy;        quad[0].z = 0.0f; quad[0].tu = tu;            quad[0].tv = tv;
        quad[1].x = cx + 8.0f; quad[1].y = cy;        quad[1].z = 0.0f; quad[1].tu = tu + 1.0f / 16; quad[1].tv = tv;
        quad[2].x = cx + 8.0f; quad[2].y = cy - 8.0f; quad[2].z = 0.0f; quad[2].tu = tu + 1.0f / 16; quad[2].tv = tv + 1.0f / 8;
        quad[3].x = cx;        quad[3].y = cy - 8.0f; quad[3].z = 0.0f; quad[3].tu = tu;            quad[3].tv = tv + 1.0f / 8;
        memcpy(pVertices, quad, sizeof(quad));
        pVertices += 4;

        cx += 8.0f;
        if (cx > 96.0f)
        {
            cy -= 9.0f;
            cx = -96.0f;
        }
    }

    x = -100.0f;
    y = -80.0f;
}

// 0x00011990
void CCredits::Reset()
{
    x = -100.0f;
    y = -80.0f;
}

// 0x000119B0
// Scrolls the text up a little each frame, with the logo just below it and
// a particle (type 6) rising from the bottom every half second. Once the
// text has scrolled off, on to the main menu.
HRESULT CCredits::Render()
{
    static DWORD s_dwLastParticle = GetTickCount();    // 0x000A6100
    if (GetTickCount() - s_dwLastParticle >= 500)
    {
        s_dwLastParticle = GetTickCount();
        g_Game.particles.Add((float)(rand() % 80 - 40), -40.0f, 0.0f, 0.0f, 10.0f, 0.0f, PARTICLE_6);
    }

    y += 0.125f;
    if (y > 806.75f)
        g_Game.state = STATE_MENU;

    D3DXMATRIX matRot, matWorld, matScale, matTemp;
    D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, 0.0f, 0.0f);
    D3DXMatrixTranslation(&matWorld, 0.0f, y, 150.0f);
    D3DXMatrixMultiply(&matWorld, &matRot, &matWorld);
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
    g_Game.pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_Game.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_Game.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    g_Game.pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
    g_Game.pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);
    g_Game.pDevice->SetTexture(0, m_pFontTexture);
    g_Game.pDevice->SetStreamSource(0, m_pVB, sizeof(SPRITEVERTEX));
    g_Game.pDevice->DrawVertices(D3DPT_QUADLIST, 0, strlen(g_CreditsText) * 4);

    D3DXMatrixTranslation(&matWorld, 0.0f, y - 5.0f, 150.0f);
    D3DXMatrixScaling(&matScale, 180.0f, 80.0f, 0.0f);
    D3DXMatrixMultiply(&matTemp, &matScale, &matWorld);
    matWorld = matTemp;
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_Game.pDevice->SetTexture(0, m_pLogoTexture);
    g_Game.pDevice->SetStreamSource(0, g_Game.pQuadVB, sizeof(SPRITEVERTEX));
    g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
    g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
    return S_OK;
}
