// The intro's scrolling credits.
#include "Credits.h"
#include "Game.h"
#include "Md5.h"

// 0x000B6248
// Shown 25 characters to a line (one line per string below). Init checks its
// MD5: change one character and the game shows the tamper screen instead.
char g_CreditsText[] =
    "  AFTERSHOCK   PRESENTS  "
    "                         "
    "                         "
    "                         "
    "                         "
    "                         "
    "                         "
    "    CODED BY SHIZNO      "
    "THANKS FOR NOTHING PETER "
    "                         "
    "                         "
    "   YET ANOTHER ALL NEW   "
    "   ORIGINAL AFTERSHOCK   "
    " CREATION. THIS GAME WAS "
    "   RUSHED SO THERE ARE A "
    "  FEW AREAS THAT NEED TO "
    " BE IMPROVED BUT I WILL  "
    "    RELEASE A V2 ONE OF  "
    "       THESE DAYS!!      "
    "                         "
    "                         "
    "                         "
    "    3D GFX.. BY SHIZNO   "
    "  SOUND STOLEN BY SHIZNO "
    " TEXTURES STOLEN BY SHIZ "
    "                         "
    "                         "
    "                         "
    " THIS VERSION OF POOL SX "
    "   HAS _NO_ AI SO YOU    "
    "  WILL NEED A FRIEND TO  "
    "  PLAY WITH UNLESS YOU   "
    " WANT TO PLAY AGAINST..  "
    "         YOURSELF!   .   "
    "                         "
    "                         "
    "                         "
    "                         "
    "         LOOK OUT        "
    "        FOR MY NEXT      "
    "         RELEASE.        "
    "                         "
    "      DETAILS TO BE      "
    "  RELEASED SOON ON OUR   "
    "  WEBSITE. STAY TUNED    "
    "                         "
    "                         "
    "       SHOUTS:           "
    "\t     XANTIUM            "
    "       IRIEZ             "
    "        JSZ              "
    "       LANTUS            "
    "       KL0WN             "
    "       HULKEN            "
    "      H3INRICH           "
    "        XARANN           "
    "       KEN0SIS           "
    " ONEDUMBBITCH(ODB718)    "
    "      CRASHOVER          "
    "EVEN GIANTNEWB..SLACKER!!"
    "    SAME OLD BITCHES_    "
    "   IN #A_SHOCK ON EFNET  "
    " ANYONE IS WELCOME, EVEN "
    "DWORD CHANGERS, YOU KNOW "
    "  WHO YOU ARE _WINK WINK_"
    "                         "
    "                         "
    "                         "
    "  SADLY NOONE HAS BEEFED "
    " WITH ME DURING POOL SX'S"
    "  DEV SO I HAVE NO DISSES"
    "  TO HAND OUT... SIGH    "
    "                         "
    "OH BUT UBERJIM IS A FAG! "
    " AHHH YES NOW WITH THAT  "
    "OUT OF THE WAY...ON WITH "
    "  THE GAME!!!!!!!!!!!!!  "
    "                         "
    "                         "
    "                         "
    "AFTERSHOCK.XBOX-SCENE.COM";

#define CREDITS_MD5 "b949c47e887fcbf5176494b5f3ac145d"

CCredits g_Credits;

// 0x00012400
void CCredits::Init(LPDIRECT3DDEVICE8 pDevice)
{
    MD5 md5;    // unused
    if (MD5String(g_CreditsText, strlen(g_CreditsText)).compare(0, std::string::npos, CREDITS_MD5, 32) != 0)
    {
        // Edited credits: taunt forever.
        for (int y = 0;;)
        {
            pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
            g_Game.font.DrawText(L"Warning, File has been tampered with", 100.0f, 100.0f, 255, 0, 0);
            g_Game.font.DrawText(L"gona need a better haxx0r kid!", 100.0f, 140.0f, 255, 255, 255);
            g_Game.font.DrawText(L"freakdave is my bitch! little noob", 100.0f, (float)(y + 160), 255, 255, 255);
            // (MSVC evaluates the arguments right to left: blue first, x last.)
            for (int n = rand() % 10; n > 0; n--)
            {
                g_Game.font.DrawText(L"NOOB!", (float)(rand() % 640), (float)(rand() % 480), rand() % 255, rand() % 255, rand() % 255);
                g_Game.font.DrawText(L"GAY!", (float)(rand() % 640), (float)(rand() % 480), rand() % 255, rand() % 255, rand() % 255);
                g_Game.font.DrawText(L"FREAK", (float)(rand() % 640), (float)(rand() % 480), rand() % 255, rand() % 255, rand() % 255);
                g_Game.font.DrawText(L"DAVE", (float)(rand() % 640), (float)(rand() % 480), rand() % 255, rand() % 255, rand() % 255);
                g_Game.font.DrawText(L"XXX SUCKS", (float)(rand() % 640), (float)(rand() % 480), rand() % 255, rand() % 255, rand() % 255);
            }
            if (++y > 280)
                y = 0;
            pDevice->Present(NULL, NULL, NULL, NULL);
        }
    }

    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\logo.tga", 0, 0, 0, 0, D3DFMT_UNKNOWN,
            D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL, &m_pLogoTexture)))
        OutputDebugStringA("couldn't load up the logo\n");
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\fonts\\font.dds", 256, 128, 0, 0, D3DFMT_UNKNOWN,
            D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL, &m_pFontTexture)))
        OutputDebugStringA("couldn't load up the font\n");

    if (FAILED(pDevice->CreateVertexBuffer(strlen(g_CreditsText) * 4 * sizeof(MESHVERTEX), 0, 0, 0, &m_pVB)))
        OutputDebugStringA("couldn't create font VB\n");

    // One quad per character, 8x8 units, 25 to a line. font.dds holds 16 x 8
    // glyphs starting at ' '.
    MESHVERTEX* pVertices;
    m_pVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    MESHVERTEX quad[4];     // the normals are left uninitialized; lighting is off
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

// 0x00012080
void CCredits::Reset()
{
    x = -100.0f;
    y = -80.0f;
}

// 0x000120D0
// Scrolls the text up a little each frame (it lies flat, seen from above),
// then draws the logo just behind it.
HRESULT CCredits::Render(LPDIRECT3DDEVICE8 pDevice)
{
    D3DXMATRIX matRot, matWorld, matScale;

    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    y += 0.125f;
    D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, D3DX_PI / 2, 0.0f);
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, y);
    D3DXMatrixMultiply(&matWorld, &matRot, &matWorld);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTexture(0, m_pFontTexture);
    pDevice->SetStreamSource(0, m_pVB, sizeof(MESHVERTEX));
    pDevice->DrawVertices(D3DPT_QUADLIST, 0, strlen(g_CreditsText) * 4);

    D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, D3DX_PI / 2, 0.0f);
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, y - 5.0f);
    D3DXMatrixScaling(&matScale, 180.0f, 80.0f, 0.0f);
    matWorld = matScale * matRot * matWorld;
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    pDevice->SetTexture(0, m_pLogoTexture);
    pDevice->SetStreamSource(0, g_Game.quadVB, sizeof(MESHVERTEX));
    pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
    pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    return S_OK;
}
