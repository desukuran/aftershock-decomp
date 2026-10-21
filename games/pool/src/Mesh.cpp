// The game's models.
#include "Mesh.h"
#include <stdio.h>
#include <math.h>

// 0x0002F050
CMesh::CMesh()
{
}

// 0x00036520
// The file must exist: a missing one crashes (fopen isn't checked).
HRESULT CMesh::Load(LPDIRECT3DDEVICE8 pDevice, const char* strFile, const char* strTexture, bool bNoTexture)
{
    m_fRadius = 0.0f;
    m_fMaxZ = 0.0f;
    m_fMaxY = 0.0f;
    m_fMaxX = 0.0f;

    MESHVERTEX v;
    v.color = 0xFFFFFFFF;

    FILE* f = fopen(strFile, "rb");
    // The triangle count is stored as a float.
    float fTriangles = 0.0f;
    fread(&fTriangles, 1, 4, f);
    m_nTriangles = (int)fTriangles;

    if (FAILED(pDevice->CreateVertexBuffer((int)fTriangles * 3 * sizeof(MESHVERTEX), 0, 0, 0, &m_pVB)))
        return E_FAIL;

    MESHVERTEX* pVertices;
    m_pVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    for (int i = 0; i < m_nTriangles * 3; i++)
    {
        fread(&v.x, 1, 4, f);
        fread(&v.y, 1, 4, f);
        fread(&v.z, 1, 4, f);

        float fDist = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        if (fDist > m_fRadius)
            m_fRadius = fDist;
        if (fabsf(v.x) > m_fMaxX)
            m_fMaxX = fabsf(v.x);
        if (fabsf(v.y) > m_fMaxY)
            m_fMaxY = fabsf(v.y);
        if (fabsf(v.z) > m_fMaxZ)
            m_fMaxZ = fabsf(v.z);

        fread(&v.nx, 1, 4, f);
        fread(&v.ny, 1, 4, f);
        fread(&v.nz, 1, 4, f);
        fread(&v.tu, 1, 4, f);
        fread(&v.tv, 1, 4, f);
        *pVertices++ = v;
    }
    fclose(f);

    if (!bNoTexture)
    {
        if (FAILED(D3DXCreateTextureFromFileExA(pDevice, strTexture, 0, 0, 0, 0, D3DFMT_UNKNOWN,
                D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_pTexture)))
            return E_FAIL;
    }
    else
    {
        m_pTexture = NULL;
    }
    return S_OK;
}

// 0x00036770
// Draws with the mesh's own texture, or pDefaultTexture if it has none.
// Leaves culling off.
HRESULT CMesh::Render(LPDIRECT3DDEVICE8 pDevice, LPDIRECT3DTEXTURE8 pDefaultTexture)
{
    pDevice->SetStreamSource(0, m_pVB, sizeof(MESHVERTEX));
    pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
    if (m_pTexture != NULL)
        pDefaultTexture = m_pTexture;
    pDevice->SetTexture(0, pDefaultTexture);
    pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_nTriangles);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    return S_OK;
}

// 0x000367C0
CMesh::~CMesh()
{
    if (m_pTexture)
        m_pTexture->Release();
    m_pTexture = NULL;
    if (m_pVB)
        m_pVB->Release();
    m_pVB = NULL;
}
