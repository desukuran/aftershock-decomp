// The puffs: a quad with one of three textures, shown for 600 ms.
#include "Game.h"

// 0x00019C40
// Quirk: the old array is freed with free(), not delete.
void CParticles::Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nParticles, float fScale)
{
    m_pDevice = pDevice;
    m_pTextures = pTextures;
    m_nParticles = nParticles;
    m_fScale = fScale;
    if (m_pParticles)
        free(m_pParticles);
    m_pParticles = new SParticle[m_nParticles];
}

// 0x00019C90
// Takes the first free one; none free, no puff.
void CParticles::Add(float x, float y, float z)
{
    bool bAdded = false;
    for (int i = 0; i < m_nParticles; i++)
    {
        if (bAdded)
            return;
        if (!m_pParticles[i].bActive)
        {
            m_pParticles[i].dwStart = GetTickCount();
            m_pParticles[i].bActive = true;
            m_pParticles[i].frame = rand() % 3;
            m_pParticles[i].x = x;
            m_pParticles[i].y = y;
            m_pParticles[i].z = z;
            bAdded = true;
        }
    }
}

// 0x00019D20
void CParticles::Update()
{
    for (int i = 0; i < m_nParticles; i++)
    {
        if (m_pParticles[i].bActive)
        {
            SParticle* p = &m_pParticles[i];
            if (GetTickCount() - p->dwStart >= 600)
                p->bActive = false;
        }
    }
}

// 0x00019D70
void CParticles::Render()
{
    D3DXMATRIX matScale, matTrans, matWorld;
    for (int i = 0; i < m_nParticles; i++)
    {
        if (!m_pParticles[i].bActive)
            continue;
        D3DXMatrixScaling(&matScale, m_fScale, m_fScale, 1.0f);
        D3DXMatrixTranslation(&matTrans, m_pParticles[i].x, m_pParticles[i].y, m_pParticles[i].z);
        D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(0x5A + m_pParticles[i].frame);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);
    }
}
