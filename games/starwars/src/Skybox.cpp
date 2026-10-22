// The sky: a textured box around the camera.
#include "Game.h"

// 0x00025850
CSkybox::CSkybox()
{
    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;
    pVB = NULL;
    pTexture = NULL;
    unknown14 = 0;
    unknown18 = 0;
    unknown1C = 0;
    unknown20 = 0;
    unknown24 = 0;
}

// 0x00025950
// Six faces of 10000 units, the texture tiled twice across each. The
// normals are left unset (the sky is drawn unlit).
HRESULT CSkybox::Create(LPDIRECT3DDEVICE8 pDevice)
{
    MESHVERTEX v[24];
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
            v[i * 4 + j].color = 0xFFFFFFFF;
        v[i * 4 + 0].tu = 0.0f; v[i * 4 + 0].tv = 0.0f;
        v[i * 4 + 1].tu = 2.0f; v[i * 4 + 1].tv = 0.0f;
        v[i * 4 + 2].tu = 0.0f; v[i * 4 + 2].tv = 2.0f;
        v[i * 4 + 3].tu = 2.0f; v[i * 4 + 3].tv = 2.0f;
    }

    const float s = 5000.0f;
    // front (+z), right (+x), left (-x), back (-z), bottom (-y), top (+y)
    v[0].x = -s;  v[0].y = s;   v[0].z = s;
    v[1].x = s;   v[1].y = s;   v[1].z = s;
    v[2].x = -s;  v[2].y = -s;  v[2].z = s;
    v[3].x = s;   v[3].y = -s;  v[3].z = s;
    v[4].x = s;   v[4].y = s;   v[4].z = s;
    v[5].x = s;   v[5].y = s;   v[5].z = -s;
    v[6].x = s;   v[6].y = -s;  v[6].z = s;
    v[7].x = s;   v[7].y = -s;  v[7].z = -s;
    v[8].x = -s;  v[8].y = s;   v[8].z = s;
    v[9].x = -s;  v[9].y = s;   v[9].z = -s;
    v[10].x = -s; v[10].y = -s; v[10].z = s;
    v[11].x = -s; v[11].y = -s; v[11].z = -s;
    v[12].x = -s; v[12].y = s;  v[12].z = -s;
    v[13].x = s;  v[13].y = s;  v[13].z = -s;
    v[14].x = -s; v[14].y = -s; v[14].z = -s;
    v[15].x = s;  v[15].y = -s; v[15].z = -s;
    v[16].x = -s; v[16].y = -s; v[16].z = s;
    v[17].x = s;  v[17].y = -s; v[17].z = s;
    v[18].x = -s; v[18].y = -s; v[18].z = -s;
    v[19].x = s;  v[19].y = -s; v[19].z = -s;
    v[20].x = -s; v[20].y = s;  v[20].z = -s;
    v[21].x = s;  v[21].y = s;  v[21].z = -s;
    v[22].x = -s; v[22].y = s;  v[22].z = s;
    v[23].x = s;  v[23].y = s;  v[23].z = s;

    if (FAILED(pDevice->CreateVertexBuffer(sizeof(v), 0, 0, 0, &pVB)))
        return E_FAIL;
    MESHVERTEX* p;
    pVB->Lock(0, 0, (BYTE**)&p, 0);
    memcpy(p, v, sizeof(v));
    D3DXCreateTextureFromFileA(pDevice, "d:\\media\\sky.tga", &pTexture);
    return S_OK;
}

// 0x00025880
// Unlit, one strip per face; lighting goes back on.
void CSkybox::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    for (int i = 0; i < 24; i += 4)
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, vPos.x, vPos.y, vPos.z);
        pDevice->SetTransform(D3DTS_WORLD, &mat);
        pDevice->SetStreamSource(0, pVB, sizeof(MESHVERTEX));
        pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pDevice->SetTexture(0, pTexture);
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i, 2);
    }
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}

// 0x00025920
void CSkybox::SetPosition(float x, float y, float z)
{
    vPos.x = x;
    vPos.y = y;
    vPos.z = z;
}
