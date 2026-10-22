// Sparks: small animated billboards where a laser hits (explosion0..8.tga).
#include "Game.h"

// 0x00014F40
// Nine textures into eight slots: the last one lands on sparks[0].bActive
// (cleared by the first Update; frame 8 is never drawn).
void CSparks::Create(LPDIRECT3DDEVICE8 pDevice)
{
    char strFile[100];
    for (int i = 0; i < 9; i++)
    {
        sprintf(strFile, "D:\\media\\explosion%i.tga", i);
        D3DXCreateTextureFromFileExA(pDevice, strFile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                     D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0xFF000000, NULL, NULL,
                                     &texFrames[i]);
    }

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
}

// 0x00014C00
void CSparks::Add(D3DXVECTOR3 vPos)
{
    for (int i = 0; i < MAX_SPARKS; i++)
    {
        if (!sparks[i].bActive)
        {
            sparks[i].bActive = true;
            sparks[i].fScale = 1.0f;
            sparks[i].dwStart = GetTickCount();
            sparks[i].vPos = vPos;
            return;
        }
    }
}

// 0x00014C60
void CSparks::AddSmall(D3DXVECTOR3 vPos)
{
    for (int i = 0; i < MAX_SPARKS; i++)
    {
        if (!sparks[i].bActive)
        {
            sparks[i].bActive = true;
            sparks[i].fScale = 0.1f;
            sparks[i].dwStart = GetTickCount();
            sparks[i].vPos = vPos;
            return;
        }
    }
}

// 0x00014CC0
// Seven frames over 875 ms; gone after a second.
void CSparks::Update()
{
    DWORD dwNow = GetTickCount();
    for (int i = 0; i < MAX_SPARKS; i++)
    {
        if ((int)(dwNow - sparks[i].dwStart) >= 1000)
        {
            sparks[i].bActive = false;
            sparks[i].dwStart = 0;
        }
        float f = (int)(dwNow - sparks[i].dwStart) * 0.00125f;
        sparks[i].frame = (int)(f * 8.0f);
        if (sparks[i].frame >= 7)
            sparks[i].frame = -1;
    }
}

// 0x00014D30
// Billboards, alpha tested and blended, black keyed out.
void CSparks::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_KILL);

    for (int i = 0; i < MAX_SPARKS; i++)
    {
        if (!sparks[i].bActive || sparks[i].frame == -1)
            continue;
        D3DXMATRIX matView, matBillboard, matScale, mat;
        pDevice->GetTransform(D3DTS_VIEW, &matView);
        D3DXMatrixTranspose(&matBillboard, &matView);
        float s = sparks[i].fScale;
        D3DXMatrixScaling(&matScale, s, s, s);
        matBillboard._41 = sparks[i].vPos.x;
        matBillboard._42 = sparks[i].vPos.y;
        matBillboard._43 = sparks[i].vPos.z;
        matBillboard._14 = 0.0f;
        matBillboard._24 = 0.0f;
        matBillboard._34 = 0.0f;
        matBillboard._44 = 1.0f;
        D3DXMatrixMultiply(&mat, &matScale, &matBillboard);
        matBillboard = mat;
        pDevice->SetTransform(D3DTS_WORLD, &matBillboard);
        pDevice->SetStreamSource(0, pVB, sizeof(MESHVERTEX));
        pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pDevice->SetTexture(0, texFrames[sparks[i].frame]);
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_DISABLE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
