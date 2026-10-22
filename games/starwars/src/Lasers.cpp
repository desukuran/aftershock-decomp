// A ship's guns. Each bolt is drawn as two boxes per gun, a thin white core
// and a wider glow in the ship's color, moved along by its world matrix.
#include "Game.h"

// The twelve triangles of a box of 8 vertices (see Init).
static const WORD s_wBoxIndices[36] =
{
    0, 1, 2,  1, 3, 2,  1, 5, 3,  3, 5, 7,  0, 5, 1,  0, 4, 5,
    2, 6, 7,  2, 7, 3,  2, 0, 4,  2, 4, 6,  6, 4, 5,  6, 5, 7,
};

// 0x0001AF70
CLaserCannon::CLaserCannon(int guns, int max)
{
    nGuns = guns;
    nMax = max;
    pLasers = new SLaser[nMax];
    for (int i = 0; i < nMax; i++)
        pLasers[i].bActive = false;
    pVB = NULL;
    pIB = NULL;
    dwLastFire = 10000;
}

// 0x000287B0
// The boxes, along +z from each gun (pGuns, in the ship's space): first all
// the cores (0.06 thick, fLength long), then the glows (0.1 thick, 0.02
// longer).
HRESULT CLaserCannon::Init(LPDIRECT3DDEVICE8 pDevice, const D3DXVECTOR3* pGuns)
{
    WORD wIndices[36];
    memcpy(wIndices, s_wBoxIndices, sizeof(wIndices));
    if (FAILED(pDevice->CreateIndexBuffer(nGuns * 2 * 36 * sizeof(WORD), 0, D3DFMT_INDEX16, 0, &pIB)))
        return E_FAIL;
    WORD* pIndices;
    pIB->Lock(0, 0, (BYTE**)&pIndices, 0);
    for (int b = 0; b < nGuns * 2; b++)
    {
        for (int k = 0; k < 36; k++)
        {
            if (b != 0)
                wIndices[k] += 8;
        }
        memcpy(pIndices, wIndices, sizeof(wIndices));
        pIndices += 36;
    }

    if (FAILED(pDevice->CreateVertexBuffer(nGuns * 2 * 8 * sizeof(LASERVERTEX), 0, 0, 0, &pVB)))
        return E_FAIL;
    LASERVERTEX* p;
    pVB->Lock(0, 0, (BYTE**)&p, 0);

    LASERVERTEX v[8];
    float w = 0.03f;
    float len = fLength;
    for (int k = 0; k < 8; k++)
        v[k].color = 0xC8FFFFFF;
    for (int i = 0; i < nGuns; i++)
    {
        const D3DXVECTOR3& g = pGuns[i];
        v[0].x = g.x - w; v[0].y = g.y + w; v[0].z = g.z;
        v[1].x = g.x + w; v[1].y = g.y + w; v[1].z = g.z;
        v[2].x = g.x - w; v[2].y = g.y - w; v[2].z = g.z;
        v[3].x = g.x + w; v[3].y = g.y - w; v[3].z = g.z;
        v[4].x = g.x - w; v[4].y = g.y + w; v[4].z = g.z + len;
        v[5].x = g.x + w; v[5].y = g.y + w; v[5].z = g.z + len;
        v[6].x = g.x - w; v[6].y = g.y - w; v[6].z = g.z + len;
        v[7].x = g.x + w; v[7].y = g.y - w; v[7].z = g.z + len;
        memcpy(p, v, sizeof(v));
        p += 8;
    }

    w = 0.049999997f;     // 0x3D4CCCCC: a bit under 0.05f
    len += 0.02f;
    for (int k = 0; k < 8; k++)
        v[k].color = color;
    for (int i = 0; i < nGuns; i++)
    {
        const D3DXVECTOR3& g = pGuns[i];
        v[0].x = g.x - w; v[0].y = g.y + w; v[0].z = g.z;
        v[1].x = g.x + w; v[1].y = g.y + w; v[1].z = g.z;
        v[2].x = g.x - w; v[2].y = g.y - w; v[2].z = g.z;
        v[3].x = g.x + w; v[3].y = g.y - w; v[3].z = g.z;
        v[4].x = g.x - w; v[4].y = g.y + w; v[4].z = g.z + len;
        v[5].x = g.x + w; v[5].y = g.y + w; v[5].z = g.z + len;
        v[6].x = g.x - w; v[6].y = g.y - w; v[6].z = g.z + len;
        v[7].x = g.x + w; v[7].y = g.y - w; v[7].z = g.z + len;
        memcpy(p, v, sizeof(v));
        p += 8;
    }
    return S_OK;
}

// 0x00028730
// A new bolt from the guns at matWorld, along vDir. False if all are in
// flight. The caller keeps to dwDelay.
bool CLaserCannon::Fire(const D3DXMATRIX* pWorld, const D3DXVECTOR3* pDir)
{
    int i;
    for (i = 0; i < nMax; i++)
    {
        if (!pLasers[i].bActive)
            break;
    }
    if (i == nMax)
        return false;
    pLasers[i].matWorld = *pWorld;
    pLasers[i].vDir = *pDir;
    pLasers[i].bActive = true;
    pLasers[i].dwTime = 0;
    return true;
}

// 0x000286A0
// Moves the bolts by their direction once per frame (not by time) and ends
// them after dwLifetime ms of frame time.
void CLaserCannon::Update(DWORD dwFrameTime)
{
    for (int i = 0; i < nMax; i++)
    {
        if (!pLasers[i].bActive)
            continue;
        pLasers[i].dwTime += dwFrameTime;
        if (pLasers[i].dwTime < dwLifetime)
        {
            pLasers[i].matWorld._41 += pLasers[i].vDir.x;
            pLasers[i].matWorld._42 += pLasers[i].vDir.y;
            pLasers[i].matWorld._43 += pLasers[i].vDir.z;
        }
        else
        {
            pLasers[i].bActive = false;
        }
    }
}

// 0x00028540
// The cores added (source color times itself, plus the scene times the
// core's color), then the glows blended.
HRESULT CLaserCannon::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetTexture(0, NULL);
    pDevice->SetVertexShader(D3DFVF_LASERVERTEX);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetStreamSource(0, pVB, sizeof(LASERVERTEX));
    pDevice->SetIndices(pIB, 0);
    for (int i = 0; i < nMax; i++)
    {
        if (!pLasers[i].bActive)
            continue;
        pDevice->SetTransform(D3DTS_WORLD, &pLasers[i].matWorld);
        pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
        pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
        pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 0, nGuns * 12);
        pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
        pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, nGuns * 36, nGuns * 12);
    }
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    return S_OK;
}
