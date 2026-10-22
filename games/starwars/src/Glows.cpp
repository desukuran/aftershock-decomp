// The Star Destroyers' engine glows: additive billboards (redengine.tga,
// engine.tga) that pulse together.
#include "Game.h"

// 0x00013CD0
void CGlows::Create(LPDIRECT3DDEVICE8 pDevice)
{
    D3DXCreateTextureFromFileExA(pDevice, "D:\\media\\redengine.tga", D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0,
                                 D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0xFF000000, NULL,
                                 NULL, &texRed);
    D3DXCreateTextureFromFileExA(pDevice, "D:\\media\\engine.tga", D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0,
                                 D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0xFF000000, NULL,
                                 NULL, &texBlue);

    MESHVERTEX quad[4];
    quad[0].x = -2.0f; quad[0].y = 2.0f;  quad[0].z = 0.0f; quad[0].color = 0xE6FFFFFF; quad[0].tu = 0.0f; quad[0].tv = 0.0f;
    quad[1].x = 2.0f;  quad[1].y = 2.0f;  quad[1].z = 0.0f; quad[1].color = 0xE6FFFFFF; quad[1].tu = 1.0f; quad[1].tv = 0.0f;
    quad[2].x = -2.0f; quad[2].y = -2.0f; quad[2].z = 0.0f; quad[2].color = 0xE6FFFFFF; quad[2].tu = 0.0f; quad[2].tv = 1.0f;
    quad[3].x = 2.0f;  quad[3].y = -2.0f; quad[3].z = 0.0f; quad[3].color = 0xE6FFFFFF; quad[3].tu = 1.0f; quad[3].tv = 1.0f;
    if (FAILED(pDevice->CreateVertexBuffer(sizeof(quad), 0, 0, 0, &pVB)))
        return;
    MESHVERTEX* p;
    pVB->Lock(0, 0, (BYTE**)&p, 0);
    memcpy(p, quad, sizeof(quad));
    nBrightness = 0;
}

// 0x00013890
// type: GLOW_RED or GLOW_BLUE. Ignored when all 40 are taken.
void CGlows::Add(float x, float y, float z, int type)
{
    for (int i = 0; i < MAX_GLOWS; i++)
    {
        if (glows[i].type < 1)
        {
            glows[i].vPos.x = x;
            glows[i].vPos.y = y;
            glows[i].vPos.z = z;
            glows[i].type = type;
            return;
        }
    }
}

// 0x000138D0
// One step every 10 ms between 1 and 55 and back. Written as a loop over
// the glows, but the first pass resets the timer, so it steps once.
void CGlows::Pulse()
{
    DWORD dwNow = GetTickCount();
    for (int i = 0; i < MAX_GLOWS; i++)
    {
        if ((int)(dwNow - dwLastPulse) >= 10)
        {
            if (!bFading)
                nBrightness++;
            else
                nBrightness--;
            if (nBrightness > 54 || nBrightness < 1)
                bFading = !bFading;
            dwLastPulse = dwNow;
        }
    }
}

// 0x00013A70
// Added on top of the scene, brightened by nBrightness. The red ones are
// stretched sideways, the blue ones half size.
void CGlows::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(nBrightness, nBrightness, nBrightness));
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    for (int i = 0; i < MAX_GLOWS; i++)
    {
        SGlow* g = &glows[i];
        if (g->type == GLOW_NONE)
            continue;
        D3DXMATRIX matScale, matTrans, mat;
        if (g->type == GLOW_RED)
            D3DXMatrixScaling(&matScale, 1.3f, 1.0f, 1.0f);
        if (g->type == GLOW_BLUE)
            D3DXMatrixScaling(&matScale, 0.5f, 0.5f, 0.5f);
        D3DXMatrixTranslation(&matTrans, g->vPos.x, g->vPos.y, g->vPos.z);
        D3DXMatrixMultiply(&mat, &matScale, &matTrans);
        matTrans = mat;
        pDevice->SetTransform(D3DTS_WORLD, &matTrans);
        pDevice->SetStreamSource(0, pVB, sizeof(MESHVERTEX));
        pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        if (g->type == GLOW_RED)
            pDevice->SetTexture(0, texRed);
        if (g->type == GLOW_BLUE)
            pDevice->SetTexture(0, texBlue);
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}

// 0x00013C80
void CGlows::Clear()
{
    for (int i = 0; i < MAX_GLOWS; i++)
    {
        glows[i].vPos.x = 0.0f;
        glows[i].type = GLOW_NONE;
        glows[i].vPos.y = 0.0f;
        glows[i].vPos.z = 0.0f;
    }
}
