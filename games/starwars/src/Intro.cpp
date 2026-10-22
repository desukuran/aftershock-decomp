// The title screen's scrolling text, in the Star Wars style: a slab of
// font.dds letters tilted back and drifting away, under logo.dds.
#include "Game.h"
#include "Md5.h"

// 0x000BA658
// The crawl's text (950 characters), extracted from the original release at
// build time.
char g_strCrawl[] =
#include "crawl.inc"
;

// 0x000BAA10
static float s_fCrawlY = -100.0f;
// 0x000BAA14
static float s_fCrawlZ = -80.0f;

// 0x00017920
// Quits if the text has been edited (its MD5 is checked). Each character is
// an 8x8 quad from the 16x8 grid of font.dds, 24 to a line.
void CIntro::Create(LPDIRECT3DDEVICE8 pDevice)
{
    MD5 md5;
    if (MD5String(g_strCrawl, strlen(g_strCrawl)).compare("e03e1af1697629a53d81ff2b79ff6d1d") != 0)
        exit(1);

    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\logo.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL,
                                            NULL, &texLogo)))
        OutputDebugStringA("couldn't load up the logo\n");
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "d:\\media\\fonts\\font.dds", 256, 128, 0, 0,
                                            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT,
                                            0xFFFF00FF, NULL, NULL, &texFont)))
        OutputDebugStringA("couldn't load up the font\n");

    if (FAILED(pDevice->CreateVertexBuffer(strlen(g_strCrawl) * 4 * sizeof(MESHVERTEX), 0, 0, 0, &pVB)))
        OutputDebugStringA("couldn't create font VB\n");
    MESHVERTEX* p;
    pVB->Lock(0, 0, (BYTE**)&p, 0);

    MESHVERTEX v[4];
    v[0].color = 0xFFFFFFFF;
    v[1].color = 0xFFFFFFFF;
    v[2].color = 0xFFFFFFFF;
    v[3].color = 0xFFFFFFFF;
    const char* pChar = g_strCrawl;
    float x = -96.0f;
    float y = 32.0f;
    for (UINT i = 0; i < strlen(g_strCrawl); i++)
    {
        v[0].x = x;        v[0].y = y;        v[0].z = 0.0f;
        v[1].x = x + 8.0f; v[1].y = y;        v[1].z = 0.0f;
        v[2].x = x + 8.0f; v[2].y = y - 8.0f; v[2].z = 0.0f;
        v[3].x = x;        v[3].y = y - 8.0f; v[3].z = 0.0f;

        int c = *pChar;
        float row = (float)((c - 32) / 16);
        float tu = (float)(c - (int)(row * 16.0f) - 32) * 0.0625f;
        float tv = row * 0.125f;
        v[0].tu = tu;           v[0].tv = tv;
        v[1].tu = tu + 0.0625f; v[1].tv = tv;
        v[2].tu = tu + 0.0625f; v[2].tv = tv + 0.125f;
        v[3].tu = tu;           v[3].tv = tv + 0.125f;
        pChar++;
        memcpy(p, v, sizeof(v));

        x += 8.0f;
        if (x > 96.0f)
        {
            x = -96.0f;
            y -= 8.0f;
        }
        p += 4;
    }
}

// 0x000175D0
// Tilted back 0.8 rad and moved up and away a little every frame.
HRESULT CIntro::Render(LPDIRECT3DDEVICE8 pDevice)
{
    s_fCrawlZ += 0.25f;
    s_fCrawlY += 0.125f;
    D3DXMATRIX matRot, mat;
    D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, 0.8f, 0.0f);
    D3DXMatrixTranslation(&mat, 0.0f, s_fCrawlY, s_fCrawlZ);
    D3DXMatrixMultiply(&mat, &matRot, &mat);
    pDevice->SetTransform(D3DTS_WORLD, &mat);
    pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);

    MESHVERTEX v[4];
    v[0].x = -80.0f; v[0].y = 68.0f; v[0].z = 0.0f; v[0].color = 0xFFFFFFFF; v[0].tu = 0.0f; v[0].tv = 0.0f;
    v[1].x = 80.0f;  v[1].y = 68.0f; v[1].z = 0.0f; v[1].color = 0xFFFFFFFF; v[1].tu = 1.0f; v[1].tv = 0.0f;
    v[2].x = 80.0f;  v[2].y = 32.0f; v[2].z = 0.0f; v[2].color = 0xFFFFFFFF; v[2].tu = 1.0f; v[2].tv = 1.0f;
    v[3].x = -80.0f; v[3].y = 32.0f; v[3].z = 0.0f; v[3].color = 0xFFFFFFFF; v[3].tu = 0.0f; v[3].tv = 1.0f;
    pDevice->SetTexture(0, texLogo);
    pDevice->DrawPrimitiveUP(D3DPT_QUADLIST, 1, v, sizeof(MESHVERTEX));

    pDevice->SetTexture(0, texFont);
    pDevice->SetStreamSource(0, pVB, sizeof(MESHVERTEX));
    pDevice->DrawPrimitive(D3DPT_QUADLIST, 0, strlen(g_strCrawl));
    return S_OK;
}
