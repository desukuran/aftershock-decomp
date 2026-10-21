// The game's models: .bin files of raw triangles, one texture each.
#ifndef AFTERSHOCK_MESH_H
#define AFTERSHOCK_MESH_H

#include <xtl.h>
#include <string>

// .bin format: a float triangle count, then 3 vertices per triangle, each
// 8 floats: position, normal, texture u, v. The vertex buffer adds a white
// diffuse color.
struct MESHVERTEX
{
    float x, y, z;
    float nx, ny, nz;
    D3DCOLOR color;
    float tu, tv;
};
#define D3DFVF_MESHVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

class CMesh
{
public:
    float                   m_fRadius;      // +0x00 largest distance of a vertex from the origin
    float                   m_fMaxX;        // +0x04 largest |x|, |y| and |z|
    float                   m_fMaxY;        // +0x08
    float                   m_fMaxZ;        // +0x0C
    LPDIRECT3DTEXTURE8      m_pTexture;     // +0x10
    int                     m_nTriangles;   // +0x14
    LPDIRECT3DVERTEXBUFFER8 m_pVB;          // +0x18
    std::string             m_strName;      // +0x1C file name, for the editor's custom pieces

    CMesh();
    ~CMesh();
    HRESULT Load(LPDIRECT3DDEVICE8 pDevice, const char* strFile, const char* strTexture, bool bNoTexture);
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice, LPDIRECT3DTEXTURE8 pDefaultTexture);
};                                          // 0x38 bytes

#endif
