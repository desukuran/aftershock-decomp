// The skins: the folders of D:\skins, each with a skin.cfg naming the
// textures it replaces ("id file" lines after the preview's file name).
#include "Game.h"

// 0x0001A260
// Quirks: the search handles are closed with CloseHandle (not FindClose),
// the first one of each folder is never closed, and nor are the skin.cfg
// files.
void CSkins::Find(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures)
{
    char strPath[256];
    WIN32_FIND_DATA fdSkin;
    WIN32_FIND_DATA fdFolder;
    char strPreview[256];
    char strFile[300];
    WIN32_FIND_DATA fdFile;

    m_pDevice = pDevice;
    m_pTextures = pTextures;

    // Count the folders with a skin.cfg.
    HANDLE hFind = FindFirstFile("D:\\skins\\*", &fdFolder);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        OutputDebugString("FindFirstFile failed.");
    }
    else
    {
        do
        {
            wsprintf(strPath, "D:\\skins\\%s\\*", fdFolder.cFileName);
            HANDLE hSkin = FindFirstFile(strPath, &fdSkin);
            if (hSkin != INVALID_HANDLE_VALUE)
            {
                wsprintf(strPath, "D:\\skins\\%s\\skin.cfg", fdFolder.cFileName);
                hSkin = FindFirstFile(strPath, &fdSkin);
                if (hSkin != INVALID_HANDLE_VALUE)
                    m_nSkins++;
            }
            CloseHandle(hSkin);
        } while (FindNextFile(hFind, &fdFolder));
        CloseHandle(hFind);

        if (m_nSkins)
            m_pSkins = new SSkin[m_nSkins];
    }

    if (m_nSkins == 0 || m_pSkins == NULL)
        return;

    // Then load each one's path and preview.
    hFind = FindFirstFile("D:\\skins\\*", &fdSkin);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        OutputDebugString("FindFirstFile failed.");
        return;
    }
    int i = 0;
    do
    {
        wsprintf(strPath, "D:\\skins\\%s\\*", fdSkin.cFileName);
        HANDLE hSkin = FindFirstFile(strPath, &fdFile);
        if (hSkin != INVALID_HANDLE_VALUE)
        {
            wsprintf(strPath, "D:\\skins\\%s\\skin.cfg", fdSkin.cFileName);
            hSkin = FindFirstFile(strPath, &fdFile);
            if (hSkin != INVALID_HANDLE_VALUE)
            {
                FILE* fp = fopen(strPath, "r");
                if (fp)
                {
                    fscanf(fp, "%s", strPreview);
                    wsprintf(m_pSkins[i].strPath, "D:\\skins\\%s\\", fdSkin.cFileName);
                    wsprintf(strFile, "D:\\skins\\%s\\%s", fdSkin.cFileName, strPreview);
                    D3DXCreateTextureFromFileExA(m_pDevice, strFile, 0, 0, 0, 0, D3DFMT_A8R8G8B8,
                                                 D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR,
                                                 0xFFFF00FF, NULL, NULL, &m_pSkins[i].pPreview);
                    i++;
                }
            }
        }
        CloseHandle(hSkin);
    } while (FindNextFile(hFind, &fdSkin));
    CloseHandle(hFind);
}

// 0x0001A4F0
void CSkins::Prev()
{
    if (m_nCurrent == 0)
        m_nCurrent = m_nSkins - 1;
    else if (m_nCurrent > 0)
        m_nCurrent--;
}

// 0x0001A510
void CSkins::Next()
{
    m_nCurrent++;
    if (m_nCurrent == m_nSkins)
        m_nCurrent = 0;
}

// 0x0001A530
// Loads the textures of the skin saved in T:\baku2.cfg. A skin with
// backgrounds (ids 0x6E-0x78) replaces the whole set: g_nBackgrounds counts
// its own. Quirk: the last line is loaded twice when the file ends in
// a line break, and the file is never closed.
void CSkins::LoadSaved()
{
    char strName[256];
    char strCfg[300];
    char strFile[300];
    int id;
    bool bBackgrounds = false;

    for (int i = 0; i < m_nSkins; i++)
    {
        if (strstr(g_Config.m_strSkin, m_pSkins[i].strPath) == NULL)
            continue;
        m_nCurrent = i;
        sprintf(strCfg, "%sskin.cfg", m_pSkins[i].strPath);
        FILE* fp = fopen(strCfg, "r");
        if (fp == NULL)
            continue;
        int line = 0;
        do
        {
            if (line == 0)
            {
                fscanf(fp, "%s", strName);      // the preview
            }
            else
            {
                fscanf(fp, "%i%s", &id, strName);
                sprintf(strFile, "%s%s", m_pSkins[m_nCurrent].strPath, strName);
                if ((id >= 0x6E) & (id <= 0x78))
                {
                    if (!bBackgrounds)
                        g_nBackgrounds = 0;
                    bBackgrounds = true;
                    g_nBackgrounds++;
                }
                m_pTextures->Load(strFile, id);
            }
            line++;
        } while (!feof(fp));
    }
}

// 0x0001A6A0
// Makes the highlighted skin the current one (saved with the settings),
// and loads its textures, like LoadSaved.
void CSkins::Apply()
{
    char strName[256];
    char strCfg[300];
    char strFile[300];
    int id;
    bool bBackgrounds = false;

    strcpy(g_Config.m_strSkin, m_pSkins[m_nCurrent].strPath);
    sprintf(strCfg, "%sskin.cfg", m_pSkins[m_nCurrent].strPath);
    FILE* fp = fopen(strCfg, "r");
    if (fp != NULL)
    {
        int line = 0;
        do
        {
            if (line == 0)
            {
                fscanf(fp, "%s", strName);
            }
            else
            {
                fscanf(fp, "%i%s", &id, strName);
                sprintf(strFile, "%s%s", m_pSkins[m_nCurrent].strPath, strName);
                if ((id >= 0x6E) & (id <= 0x78))
                {
                    if (!bBackgrounds)
                    {
                        g_nBackgrounds = 0;
                        g_nBackground = 0x6E;
                    }
                    bBackgrounds = true;
                    g_nBackgrounds++;
                }
                m_pTextures->Load(strFile, id);
            }
            line++;
        } while (!feof(fp));
    }
}

// 0x0001A7F0
void CSkins::RenderPreview()
{
    if (m_pSkins == NULL)
        return;

    D3DXMATRIX matScale, matTrans, matWorld;
    D3DXMatrixScaling(&matScale, 100.0f, 70.0f, 1.0f);
    D3DXMatrixTranslation(&matTrans, 100.0f, -50.0f, 1.0f);
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
    m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
    m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
    if (m_nCurrent < m_nSkins)
        m_pDevice->SetTexture(0, m_pSkins[m_nCurrent].pPreview);
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    m_pDevice->SetTexture(0, NULL);
}
