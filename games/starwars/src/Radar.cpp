// The radar: two discs, what's ahead of the player and what's behind,
// with a dot for each ship by its direction from the player.
#include "Game.h"

static inline TLVERTEX TLVertex(float x, float y, float tu, float tv)
{
    TLVERTEX v;
    v.x = x;
    v.y = y;
    v.z = 0.0f;
    v.rhw = 0.1f;
    v.color = 0x64FFFFFF;
    v.tu = tu;
    v.tv = tv;
    return v;
}

// A disc of half size r at (x, y).
static void DrawDisc(LPDIRECT3DTEXTURE8 pTexture, float x, float y, float r)
{
    TLVERTEX v[4];
    v[0] = TLVertex(x - r, y - r, 0.0f, 0.0f);
    v[1] = TLVertex(x + r, y - r, 1.0f, 0.0f);
    v[2] = TLVertex(x + r, y + r, 1.0f, 1.0f);
    v[3] = TLVertex(x - r, y + r, 0.0f, 1.0f);
    g_Game.pDevice->SetTexture(0, pTexture);
    g_Game.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(TLVERTEX));
}

// 0x000229B0
HRESULT CRadar::Create(LPDIRECT3DDEVICE8 pDevice)
{
    if (SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, "d:\\hud\\peteradar.png", &texRadar)) &&
        SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, "D:\\hud\\radardotred.png", &texRed)) &&
        SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, "D:\\hud\\radardotyellow.png", &texYellow)) &&
        SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, "D:\\hud\\radartargdot.png", &texTarget)))
        return S_OK;
    OutputDebugStringA("Couldn't load a radar texture\n");
    return E_FAIL;
}

// A ship's dot: its direction in the player's frame (right, up, ahead),
// on the front disc if it's ahead. Inline, as in the original: the code is
// written out in each loop there, and the multiplayer copy adds its
// products up in another order (bMulti), which the float results show.
static __forceinline void DrawDot(LPDIRECT3DTEXTURE8 pTexture, const D3DXVECTOR3* pPos, const D3DXVECTOR3* pPlayer,
                    const D3DXVECTOR3* pForward, const D3DXVECTOR3* pUp, float frontX, float frontY,
                    float backX, float backY, float r, bool bMulti)
{
    float dx = pPos->x - pPlayer->x;
    float dy = pPos->y - pPlayer->y;
    float dz = pPos->z - pPlayer->z;
    D3DXVECTOR3 vRight;
    vRight.x = pUp->y * pForward->z - pUp->z * pForward->y;
    vRight.y = pUp->z * pForward->x - pForward->z * pUp->x;
    vRight.z = pForward->y * pUp->x - pUp->y * pForward->x;
    float ahead;
    D3DXVECTOR3 v;
    // Added up one term at a time: the compiler keeps that order (a single
    // expression it regroups, not always the same way).
    if (!bMulti)
    {
        ahead = pForward->z * dz;
        ahead += pForward->y * dy;
        ahead += dx * pForward->x;
        v.x = vRight.z * dz;
        v.x += vRight.y * dy;
        v.x += dx * vRight.x;
        v.y = pUp->z * dz;
        v.y += pUp->y * dy;
        v.y += pUp->x * dx;
    }
    else
    {
        ahead = dx * pForward->x;
        ahead += pForward->z * dz;
        ahead += pForward->y * dy;
        v.x = dx * vRight.x;
        v.x += vRight.z * dz;
        v.x += vRight.y * dy;
        v.y = pUp->x * dx;
        v.y += pUp->z * dz;
        v.y += pUp->y * dy;
    }
    v.z = ahead;
    D3DXVec3Normalize(&v, &v);
    float cx = backX, cy = backY;
    if (ahead >= 0.0f)
    {
        cx = frontX;
        cy = frontY;
    }
    float x = (v.x * r + cx) - 4.0f;
    float y = (v.y * -r + cy) - 4.0f;
    TLVERTEX q[4];
    q[0] = TLVertex(x, y, 0.0f, 0.0f);
    q[1] = TLVertex(x + 5.0f, y, 1.0f, 0.0f);
    q[2] = TLVertex(x + 5.0f, y + 5.0f, 1.0f, 1.0f);
    q[3] = TLVertex(x, y + 5.0f, 0.0f, 1.0f);
    g_Game.pDevice->SetTexture(0, pTexture);
    g_Game.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, q, sizeof(TLVERTEX));
}

// 0x00022A20
// Story mode: the TIEs and interceptors in red (the ones after the player
// in the target color), the wingmen in yellow.
HRESULT CRadar::Render(CGame* pGame)
{
    pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
    float frontX = 540.0f, frontY = 400.0f;
    float backX = 540.0f, backY = 400.0f;
    DrawDisc(texRadar, frontX, frontY, 48.0f);
    backX = 100.0f;
    DrawDisc(texRadar, backX, backY, 48.0f);

    CXWing* pPlayer = &pGame->pXWings[0];
    D3DXVECTOR3 vPos = pPlayer->vPos;
    D3DXVECTOR3 vForward = pPlayer->vForward;
    D3DXVECTOR3 vUp = pPlayer->vUp;
    D3DXMATRIX matUnused;
    D3DXMatrixLookAtRH(&matUnused, &vForward, &vPos, &vUp);

    int i;
    for (i = 0; i < pGame->nTies; i++)
    {
        if (!pGame->pTies[i].bAlive)
            continue;
        pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
        D3DXVECTOR3 v = pGame->pTies[i].vPos;
        DrawDot(pGame->pTies[i].bTargeted ? texTarget : texRed, &v, &vPos, &vForward, &vUp, frontX, frontY,
                backX, backY, 48.0f, false);
    }
    for (i = 0; i < pGame->nInterceptors; i++)
    {
        if (!pGame->pInterceptors[i].bAlive)
            continue;
        pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
        D3DXVECTOR3 v = pGame->pInterceptors[i].vPos;
        DrawDot(pGame->pInterceptors[i].bTargeted ? texTarget : texRed, &v, &vPos, &vForward, &vUp, frontX,
                frontY, backX, backY, 48.0f, false);
    }
    for (i = 1; i < pGame->nXWings; i++)
    {
        if (!pGame->pXWings[i].bAlive)
            continue;
        pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
        D3DXVECTOR3 v = pGame->pXWings[i].vPos;
        DrawDot(texYellow, &v, &vPos, &vForward, &vUp, frontX, frontY, backX, backY, 48.0f, false);
    }
    return S_OK;
}

// 0x000238B0
// Multiplayer, in one player's part of the screen (layouts as
// CHud::RenderMulti). Quirk: the ships from the second on are shown, for
// everyone, so the first player never shows and the others see themselves.
HRESULT CRadar::RenderMulti(CGame* pGame, int layout)
{
    pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
    float frontX, frontY, backX, backY;
    int nSize;
    float fRight = 270.0f;
    switch (layout)
    {
    case 0:
        nSize = 96;
        frontX = 540.0f; frontY = 400.0f; backX = 100.0f; backY = 400.0f;
        break;
    case 1:
        frontX = 540.0f; frontY = 200.0f; nSize = 72; backX = 100.0f; backY = 200.0f;
        break;
    case 2:
        nSize = 72;
        frontX = 540.0f; frontY = 400.0f; backX = 100.0f; backY = 400.0f;
        break;
    case 3:
        backX = 50.0f; frontY = 190.0f;
        nSize = 48; frontX = fRight; backY = frontY;
        break;
    case 4:
        frontY = 190.0f;
        backX = 370.0f; fRight = 590.0f;
        nSize = 48; frontX = fRight; backY = frontY;
        break;
    case 5:
        backX = 50.0f; frontY = 430.0f;
        nSize = 48; frontX = fRight; backY = frontY;
        break;
    case 6:
        frontY = 430.0f;
        backX = 370.0f; fRight = 590.0f;
        nSize = 48; frontX = fRight; backY = frontY;
        break;
    }
    float r = (float)(nSize / 2);
    DrawDisc(texRadar, frontX, frontY, r);
    DrawDisc(texRadar, backX, backY, r);

    int nPlayer = layout;
    if (layout >= 3)
        nPlayer = layout - 3;
    else if (layout >= 1)
        nPlayer = layout - 1;
    CPlayerShip* pPlayer = &pGame->pPlayerShips[nPlayer];
    D3DXVECTOR3 vPos = pPlayer->vPos;
    D3DXVECTOR3 vForward = pPlayer->vForward;
    D3DXVECTOR3 vUp = pPlayer->vUp;
    D3DXMATRIX matUnused;
    D3DXMatrixLookAtRH(&matUnused, &vForward, &vPos, &vUp);

    for (int i = 1; i < pGame->nPlayerShips; i++)
    {
        if (!pGame->pPlayerShips[i].bAlive)
            continue;
        pGame->pDevice->SetVertexShader(D3DFVF_TLVERTEX);
        D3DXVECTOR3 v = pGame->pPlayerShips[i].vPos;
        DrawDot(pGame->pPlayerShips[i].bTargeted ? texTarget : texRed, &v, &vPos, &vForward, &vUp, frontX,
                frontY, backX, backY, r, true);
    }
    return S_OK;
}
