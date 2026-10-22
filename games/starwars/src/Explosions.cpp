// Explosions: an animated billboard (explosion0..11.tga) and, but for
// type 1, a flat ring (shock.tga) that grows around it.
#include "Game.h"

// 0x00014550
// Loads twelve frames into the eight slots: the last four go over the
// mesh after the class (CGame::unusedMesh, never loaded).
void CExplosions::Create(LPDIRECT3DDEVICE8 pDevice)
{
    char strFile[100];
    D3DXCreateTextureFromFileA(pDevice, "D:\\media\\shock.tga", &texShock);
    for (int i = 1; i < 13; i++)
    {
        sprintf(strFile, "D:\\media\\explosion%i.tga", i - 1);
        D3DXCreateTextureFromFileA(pDevice, strFile, &texFrames[i - 1]);
    }

    MESHVERTEX quad[4];
    quad[0].x = -2.0f; quad[0].y = 2.0f;  quad[0].z = 0.0f; quad[0].color = 0xFFFFFFFF; quad[0].tu = 0.0f; quad[0].tv = 0.0f;
    quad[1].x = 2.0f;  quad[1].y = 2.0f;  quad[1].z = 0.0f; quad[1].color = 0xFFFFFFFF; quad[1].tu = 1.0f; quad[1].tv = 0.0f;
    quad[2].x = -2.0f; quad[2].y = -2.0f; quad[2].z = 0.0f; quad[2].color = 0xFFFFFFFF; quad[2].tu = 0.0f; quad[2].tv = 1.0f;
    quad[3].x = 2.0f;  quad[3].y = -2.0f; quad[3].z = 0.0f; quad[3].color = 0xFFFFFFFF; quad[3].tu = 1.0f; quad[3].tv = 1.0f;
    if (FAILED(pDevice->CreateVertexBuffer(sizeof(quad), 0, 0, 0, &pQuadVB)))
        return;
    MESHVERTEX* p;
    pQuadVB->Lock(0, 0, (BYTE**)&p, 0);
    memcpy(p, quad, sizeof(quad));

    // The ring: twelve segments from radius 1 to 2 in the xz plane.
    nRingVertices = 72;
    if (FAILED(pDevice->CreateVertexBuffer(72 * sizeof(MESHVERTEX), 0, 0, 0, &pRingVB)))
        return;
    pRingVB->Lock(0, 0, (BYTE**)&p, 0);
    for (float angle = 0.0f; angle < 360.0f; angle += 30.0f)
    {
        float a = D3DXToRadian(angle);
        float b = a + D3DXToRadian(30.0f);
        p[0].x = cosf(a);        p[0].y = 0.0f; p[0].z = sinf(a);        p[0].color = 0xFFFFFFFF; p[0].tu = 0.0f; p[0].tv = 1.0f;
        p[1].x = cosf(a) * 2.0f; p[1].y = 0.0f; p[1].z = sinf(a) * 2.0f; p[1].color = 0xFFFFFFFF; p[1].tu = 0.0f; p[1].tv = 0.0f;
        p[2].x = cosf(b);        p[2].y = 0.0f; p[2].z = sinf(b);        p[2].color = 0xFFFFFFFF; p[2].tu = 1.0f; p[2].tv = 1.0f;
        p[3].x = cosf(a) * 2.0f; p[3].y = 0.0f; p[3].z = sinf(a) * 2.0f; p[3].color = 0xFFFFFFFF; p[3].tu = 0.0f; p[3].tv = 0.0f;
        p[4].x = cosf(b) * 2.0f; p[4].y = 0.0f; p[4].z = sinf(b) * 2.0f; p[4].color = 0xFFFFFFFF; p[4].tu = 1.0f; p[4].tv = 0.0f;
        p[5].x = cosf(b);        p[5].y = 0.0f; p[5].z = sinf(b);        p[5].color = 0xFFFFFFFF; p[5].tu = 1.0f; p[5].tv = 1.0f;
        p += 6;
    }
}

// 0x00013EE0
// matrix places it (a ship's world matrix); r, g, b tint the ring.
void CExplosions::Add(int type, D3DXVECTOR3 vPos, D3DXMATRIX matrix, short r, short g, short b)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        SExplosion* e = &explosions[i];
        if (e->dwStart == 0)
        {
            e->dwStart = GetTickCount();
            e->vPos = vPos;
            e->matrix = matrix;
            e->r = r;
            e->fScale = 0.2f;
            e->frame = 0;
            e->type = type;
            e->g = g;
            e->b = b;
            i = MAX_EXPLOSIONS;
        }
    }
}

// 0x00013F80
// The Death Star's: a big orange ring, tilted.
void CExplosions::AddShockwave(D3DXVECTOR3 vPos)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        SExplosion* e = &explosions[i];
        if (e->dwStart == 0)
        {
            e->dwStart = GetTickCount();
            e->fScale = 1.0f;
            e->frame = 0;
            e->vPos = vPos;
            D3DXMatrixIdentity(&e->matrix);
            D3DXMATRIX matX, matZ, mat;
            D3DXMatrixRotationX(&matX, 0.1f);
            D3DXMatrixRotationZ(&matZ, 1.57f);
            D3DXMatrixMultiply(&mat, &matX, &matZ);
            e->matrix = mat;
            e->type = EXPLOSION_SHOCKWAVE;
            e->r = 255;
            e->g = 58;
            e->b = 0;
            i = MAX_EXPLOSIONS;
        }
    }
}

// 0x000140B0
// Twelve frames: over 600 ms, the shockwave over 1200 ms. The ring grows
// every frame drawn.
void CExplosions::Update()
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        SExplosion* e = &explosions[i];
        if (e->dwStart == 0)
            continue;
        DWORD dwNow = GetTickCount();
        if (e->type == EXPLOSION_SHOCKWAVE)
        {
            int t = dwNow - e->dwStart;
            float f = t * 0.00125f;   // 0..1 over the explosion
            e->frame = (int)(f * 12.0f);
            if (e->frame > 11)
                e->frame = -1;
            if (t >= 1200)
                e->dwStart = 0;
            e->fScale += 0.5f;
        }
        else
        {
            int t = dwNow - e->dwStart;
            float f = t * 0.0025f;   // 0..1 over the explosion
            e->frame = (int)(f * 12.0f);
            if (e->frame > 11)
                e->frame = -1;
            if (t >= 600)
                e->dwStart = 0;
            e->fScale += 0.1f;
        }
    }
}

// 0x00014170
// The ring fades over its thirteen frames. The billboard faces the camera,
// at the matrix's position; once fScale passes 2 its matrix is replaced by
// a plain scaling (quirk: those frames are drawn at the world's origin).
void CExplosions::Render(LPDIRECT3DDEVICE8 pDevice)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        SExplosion* e = &explosions[i];
        if (e->dwStart == 0)
            continue;

        D3DXMATRIX matTrans, matScale, mat;
        D3DXMatrixIdentity(&matTrans);
        D3DXMatrixIdentity(&matScale);
        D3DXMatrixTranslation(&matTrans, e->vPos.x, e->vPos.y, e->vPos.z);    // unused
        D3DXMatrixScaling(&matScale, e->fScale, 1.0f, e->fScale);
        D3DXMatrixMultiply(&mat, &matScale, &e->matrix);

        if (e->type != EXPLOSION_SMALL)
        {
            pDevice->SetTransform(D3DTS_WORLD, &mat);
            pDevice->SetTexture(0, texShock);
            pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR,
                                    D3DCOLOR_ARGB(255 - e->frame * 255 / 13, (BYTE)e->r, (BYTE)e->g, (BYTE)e->b));
            pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
            pDevice->SetStreamSource(0, pRingVB, sizeof(MESHVERTEX));
            pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
            pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, nRingVertices / 3);
            pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
            pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        }

        if (e->frame != -1)
        {
            D3DXMATRIX matView, matBillboard;
            pDevice->GetTransform(D3DTS_VIEW, &matView);
            D3DXMatrixTranspose(&matBillboard, &matView);
            matBillboard._41 = mat._41;
            matBillboard._42 = mat._42;
            matBillboard._43 = mat._43;
            matBillboard._14 = 0.0f;
            matBillboard._24 = 0.0f;
            matBillboard._34 = 0.0f;
            matBillboard._44 = 1.0f;
            if (e->fScale > 2.0f)
                D3DXMatrixScaling(&matBillboard, 5.0f, 5.0f, 5.0f);
            pDevice->SetTransform(D3DTS_WORLD, &matBillboard);
            pDevice->SetStreamSource(0, pQuadVB, sizeof(MESHVERTEX));
            pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
            pDevice->SetTexture(0, texFrames[e->frame]);
            pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
            pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        }
    }
}
