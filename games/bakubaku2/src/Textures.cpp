// The texture table: every texture by id, the skins replacing them.
#include "Game.h"

// 0x0001AAA0
void CTextures::SetDevice(LPDIRECT3DDEVICE8 pDevice)
{
    m_pDevice = pDevice;
}

// 0x0001AAB0
// Magenta (0xFF00FF) is transparent.
void CTextures::Load(const char* strFile, int id)
{
    if (m_pTextures[id])
        m_pTextures[id]->Release();
    D3DXCreateTextureFromFileExA(m_pDevice, strFile, 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                 D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, 0xFFFF00FF, NULL, NULL,
                                 &m_pTextures[id]);
}

// 0x0001AB00
// Id 0 takes the texture off; an id with no texture leaves the last one on.
void CTextures::Set(int id)
{
    if (m_pTextures[id])
        m_pDevice->SetTexture(0, m_pTextures[id]);
    if (id == 0)
        m_pDevice->SetTexture(0, NULL);
}
