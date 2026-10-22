// The marker around a ship that's after the player (target.png).
#include "Game.h"

// 0x000272D0
// A quad in red, half transparent.
CTarget::CTarget()
{
    v[0].x = -2.2f; v[0].y = -2.2f; v[0].z = 0.0f; v[0].color = 0x7FFF0000; v[0].tu = 0.0f; v[0].tv = 0.0f;
    v[1].x = 2.2f;  v[1].y = -2.2f; v[1].z = 0.0f; v[1].color = 0x7FFF0000; v[1].tu = 1.0f; v[1].tv = 0.0f;
    v[2].x = 2.2f;  v[2].y = 2.2f;  v[2].z = 0.0f; v[2].color = 0x7FFF0000; v[2].tu = 1.0f; v[2].tv = 1.0f;
    v[3].x = -2.2f; v[3].y = 2.2f;  v[3].z = 0.0f; v[3].color = 0x7FFF0000; v[3].tu = 0.0f; v[3].tv = 1.0f;
}

// 0x000271F0
CTarget::~CTarget()
{
    if (pTexture)
        pTexture->Release();
    pTexture = NULL;
}

// 0x00027290
HRESULT CTarget::Create(LPDIRECT3DDEVICE8 pDevice)
{
    if (FAILED(D3DXCreateTextureFromFileExA(pDevice, "D:\\media\\target.png", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
                                            &pTexture)))
        return E_FAIL;
    return S_OK;
}

// 0x00027210
// With the world matrix already set; drawn over everything.
HRESULT CTarget::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pDevice->SetVertexShader(D3DFVF_TARGETVERTEX);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetTexture(0, pTexture);
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TARGETVERTEX));
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    return S_OK;
}
