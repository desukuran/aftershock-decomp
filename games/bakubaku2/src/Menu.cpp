// The menus: main, the match setup, options, pause, skins and help. Pad 1
// only. Each item is a texture: the plain half, or the highlighted half.
#include "Game.h"

// 0x001070E4 the frame being worked out for the vertex buffers
static SFrame s_menuFrame;

// The volume in 1/100 dB: down to -100 dB, up to 0.
static __forceinline FLOAT ClampVolume(FLOAT fVolume)
{
    if (fVolume < -10000.0f)
        fVolume = -10000.0f;
    else if (fVolume > 0.0f)
        fVolume = 0.0f;
    return fVolume;
}

// Music on or off (the options' first item).
static __forceinline void ToggleMusic()
{
    if (g_bMusicOn)
    {
        g_bMusicOn = false;
        g_Music.Stop();
    }
    else
    {
        g_bMusicOn = true;
        g_Music.Play();
    }
}

static __forceinline void ToggleRandomMusic()
{
    if (g_bRandomMusic)
    {
        g_bRandomMusic = false;
        g_Music.SetRandom(FALSE);
    }
    else
    {
        g_bRandomMusic = true;
        g_Music.SetRandom(TRUE);
    }
}

// A setting one step down (from the lowest, round to the highest) and up.
static __forceinline void CycleDown(int& n, int nMin, int nMax)
{
    if (n == nMin)
        n = nMax;
    else if (n > nMin)
        n--;
}

static __forceinline void CycleUp(int& n, int nMin, int nMax)
{
    if (n == nMax)
        n = nMin;
    else if (n < nMax)
        n++;
}

// 0x000172B0
// Puffs by the highlighted item every frame; the pad every 130 ms. Up and
// down pick an item, left and right (and A) change it, A picks it, Back
// goes back; on the setup page Start starts the match (not with every
// player in one team), B and X put a player in the red or the blue team.
// Quirk: A on a player's control goes from the last one (hard) to the
// second (pad 2), past pad 1.
void CMenu::Update()
{
    if (m_page == PAGE_MAIN)
        m_pParticles->Add((float)(rand() % 200 - 200), (float)(50 - m_pages[m_page].item * 50 - rand() % 30),
                          (float)(rand() % 30));
    if (m_page == PAGE_OPTIONS)
    {
        m_pParticles->Add((float)(rand() % 200 - 200), (float)(rand() % 20 - m_pages[m_page].item * 40 + 30),
                          (float)(rand() % 30));
        if (m_pages[PAGE_OPTIONS].item != 4 && rand() % 6 == 1)
            m_pParticles->Add((float)(rand() % 60 + 100), (float)(rand() % 20 - m_pages[m_page].item * 40 + 30),
                              (float)(rand() % 60));
    }
    if (m_page == PAGE_SETUP)
    {
        if (m_pages[PAGE_SETUP].item < 2)
            m_pParticles->Add((float)(rand() % 300 - 230), (float)(rand() % 20 - m_pages[m_page].item * 31 + 178),
                              (float)(rand() % 30));
        if (m_pages[m_page].item >= 2)
            m_pParticles->Add((float)(rand() % 360 - 230), (float)(rand() % 22 - m_pages[m_page].item * 31 + 112),
                              (float)(rand() % 30));
    }
    if (m_page == PAGE_PAUSE)
        m_pParticles->Add((float)(rand() % 200 - 200), (float)(50 - m_pages[m_page].item * 50 - rand() % 30),
                          (float)(rand() % 30));
    if (m_page == PAGE_SKINS)
        m_pParticles->Add((float)(rand() % 180 - 200), (float)(50 - m_pages[m_page].item * 50 - rand() % 30),
                          (float)(rand() % 30));

    if (m_dwInputTime != 0 && GetTickCount() - m_dwInputTime < 130)
        return;
    m_pInput->Update();
    m_dwInputTime = GetTickCount();

    if (m_page != PAGE_SETUP)
    {
        if (m_pInput->YPressed(0))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.NextSoundtrack();
            g_Music.Play();
        }
        if (m_pInput->BPressed(0))
        {
            g_dwMusicHudTime = GetTickCount();
            if (g_Music.m_mmState == MM_PLAYING)
                g_Music.Pause();
            else if (g_Music.m_mmState == MM_PAUSED || g_Music.m_mmState == MM_STOPPED)
                g_Music.Play();
        }
        if (m_pInput->XPressed(0))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.RandomSong(g_Music.m_bGlobal);
            g_Music.Play();
        }
    }
    if (m_pInput->RightThumbDown(0) != 0.0f)
    {
        g_dwMusicHudTime = GetTickCount();
        g_Music.SetVolume(ClampVolume(g_Music.m_fVolume - 200.0f));
    }
    if (m_pInput->RightThumbUp(0) != 0.0f)
    {
        g_dwMusicHudTime = GetTickCount();
        g_Music.SetVolume(ClampVolume(g_Music.m_fVolume + 200.0f));
    }
    if (m_pInput->BlackPressed(0))
    {
        g_dwMusicHudTime = GetTickCount();
        g_Music.Stop();
        g_Music.NextSong();
        g_Music.Play();
    }
    if (m_pInput->WhitePressed(0))
    {
        g_dwMusicHudTime = GetTickCount();
        g_Music.Stop();
        g_Music.PrevSong();
        g_Music.Play();
    }

    // Down and up: the next and previous item, round. The setup page has
    // the players, the mode and a line per player.
    if (m_pInput->DPadDown(0) || m_pInput->LeftThumbDown(0) != 0.0f)
    {
        if (m_page != PAGE_SETUP)
        {
            if (m_pages[m_page].item < m_pages[m_page].nItems - 1)
                m_pages[m_page].item++;
            else
                m_pages[m_page].item = 0;
        }
        if (m_page == PAGE_SETUP)
        {
            if (m_pages[PAGE_SETUP].item < m_settings.nPlayers + 1)
                m_pages[PAGE_SETUP].item++;
            else
                m_pages[PAGE_SETUP].item = 0;
        }
    }
    if (m_pInput->DPadUp(0) || m_pInput->LeftThumbUp(0) != 0.0f)
    {
        if (m_pages[m_page].item > 0)
        {
            m_pages[m_page].item--;
        }
        else
        {
            if (m_page != PAGE_SETUP)
                m_pages[m_page].item = m_pages[m_page].nItems - 1;
            if (m_page == PAGE_SETUP)
                m_pages[PAGE_SETUP].item = m_settings.nPlayers + 1;
        }
    }

    if (m_pInput->LeftThumbLeft(0) != 0.0f || m_pInput->DPadLeft(0))
    {
        if (m_page == PAGE_HELP)
        {
            m_pages[PAGE_HELP].item--;
            if (m_pages[PAGE_HELP].item < 0)
                m_pages[PAGE_HELP].item = 4;
        }
        if (m_page == PAGE_OPTIONS)
        {
            switch (m_pages[PAGE_OPTIONS].item)
            {
            case 0:
                ToggleMusic();
                break;
            case 1:
                ToggleRandomMusic();
                break;
            case 2:
                g_nPieceSet--;
                if (g_nPieceSet < 0)
                    g_nPieceSet = 2;
                break;
            case 3:
                g_bSoundEffects = !g_bSoundEffects;
                break;
            }
        }
        if (m_page == PAGE_SETUP)
        {
            if (m_pages[PAGE_SETUP].item == 0)
                CycleDown(m_settings.nPlayers, 2, 8);
            if (m_pages[PAGE_SETUP].item == 1)
                CycleDown(m_settings.nMode, MODE_FREE_FOR_ALL, MODE_TEAMS);
            if (m_pages[PAGE_SETUP].item > 1)
                CycleDown(m_settings.nControl[m_pages[PAGE_SETUP].item - 2], CONTROL_PAD1, CONTROL_HARD);
        }
    }

    if (m_pInput->LeftThumbRight(0) != 0.0f || m_pInput->DPadRight(0))
    {
        if (m_page == PAGE_HELP)
        {
            m_pages[PAGE_HELP].item++;
            if (m_pages[PAGE_HELP].item > 4)
                m_page = PAGE_MAIN;
        }
        if (m_page == PAGE_OPTIONS)
        {
            switch (m_pages[PAGE_OPTIONS].item)
            {
            case 0:
                ToggleMusic();
                break;
            case 1:
                ToggleRandomMusic();
                break;
            case 2:
                g_nPieceSet++;
                if (g_nPieceSet > 2)
                    g_nPieceSet = 0;
                break;
            case 3:
                g_bSoundEffects = !g_bSoundEffects;
                break;
            }
        }
        if (m_page == PAGE_SETUP)
        {
            if (m_pages[PAGE_SETUP].item == 0)
                CycleUp(m_settings.nPlayers, 2, 8);
            if (m_pages[PAGE_SETUP].item == 1)
                CycleUp(m_settings.nMode, MODE_FREE_FOR_ALL, MODE_TEAMS);
            if (m_pages[PAGE_SETUP].item > 1)
                CycleUp(m_settings.nControl[m_pages[PAGE_SETUP].item - 2], CONTROL_PAD1, CONTROL_HARD);
        }
    }

    if (m_pInput->StartDown(0) && m_page == PAGE_SETUP)
    {
        m_settings.nRed = 0;
        m_settings.nBlue = 0;
        for (int i = 0; i < m_settings.nPlayers; i++)
        {
            if (m_settings.nTeam[i] == TEAM_RED)
                m_settings.nRed++;
            if (m_settings.nTeam[i] == TEAM_BLUE)
                m_settings.nBlue++;
        }
        g_Settings = m_settings;
        g_Config.Save();
        if (m_settings.nMode == MODE_FREE_FOR_ALL ||
            (m_settings.nRed < m_settings.nPlayers && m_settings.nBlue < m_settings.nPlayers))
            m_bStart = true;
    }

    if (m_pInput->BackDown(0))
    {
        if (m_page == PAGE_SETUP || m_page == PAGE_HELP || m_page == PAGE_OPTIONS)
            m_page = PAGE_MAIN;
        else
            g_bShowMusicHud = !g_bShowMusicHud;
    }

    if (m_pInput->BPressed(0) && m_page == PAGE_SETUP && m_pages[PAGE_SETUP].item > 1 &&
        m_settings.nMode == MODE_TEAMS)
        m_settings.nTeam[m_pages[PAGE_SETUP].item - 2] = TEAM_RED;
    if (m_pInput->XPressed(0) && m_page == PAGE_SETUP && m_pages[PAGE_SETUP].item > 1 &&
        m_settings.nMode == MODE_TEAMS)
        m_settings.nTeam[m_pages[PAGE_SETUP].item - 2] = TEAM_BLUE;

    if (m_pInput->APressed(0))
    {
        switch (m_page)
        {
        case PAGE_MAIN:
            switch (m_pages[PAGE_MAIN].item)
            {
            case 0:
                m_page = PAGE_SETUP;
                break;
            case 1:
                m_page = PAGE_OPTIONS;
                break;
            case 2:
                m_page = PAGE_SKINS;
                break;
            case 3:
                m_page = PAGE_HELP;
                m_pages[PAGE_HELP].item = 0;
                break;
            }
            break;
        case PAGE_SETUP:
            if (m_pages[PAGE_SETUP].item == 0)
                CycleUp(m_settings.nPlayers, 2, 8);
            if (m_pages[PAGE_SETUP].item == 1)
                CycleUp(m_settings.nMode, MODE_FREE_FOR_ALL, MODE_TEAMS);
            if (m_pages[PAGE_SETUP].item > 1)
            {
                if (m_settings.nControl[m_pages[PAGE_SETUP].item - 2] == CONTROL_HARD)
                    m_settings.nControl[m_pages[PAGE_SETUP].item - 2] = CONTROL_PAD1;
                if (m_settings.nControl[m_pages[m_page].item - 2] < CONTROL_HARD)
                    m_settings.nControl[m_pages[m_page].item - 2]++;
            }
            break;
        case PAGE_OPTIONS:
            switch (m_pages[PAGE_OPTIONS].item)
            {
            case 0:
                ToggleMusic();
                break;
            case 1:
                ToggleRandomMusic();
                break;
            case 2:
                g_nPieceSet++;
                if (g_nPieceSet > 2)
                    g_nPieceSet = 0;
                break;
            case 3:
                g_bSoundEffects = !g_bSoundEffects;
                break;
            case 4:
                g_Config.Save();
                m_page = PAGE_MAIN;
                break;
            }
            break;
        case PAGE_PAUSE:
            switch (m_pages[PAGE_PAUSE].item)
            {
            case 0:
                m_bResume = true;
                m_bFromPause = false;
                break;
            case 1:
                m_bStart = true;
                break;
            case 2:
                m_bFromPause = true;
                m_page = PAGE_SKINS;
                break;
            case 3:
                m_bQuit = true;
                m_bFromPause = false;
                break;
            }
            break;
        case PAGE_SKINS:
            switch (m_pages[PAGE_SKINS].item)
            {
            case 0:
                m_skins.Prev();
                break;
            case 1:
                m_skins.Next();
                break;
            case 2:
                m_skins.Apply();
                break;
            case 3:
                if (m_bFromPause)
                    m_page = PAGE_PAUSE;
                else
                    m_page = PAGE_MAIN;
                break;
            }
            break;
        case PAGE_HELP:
            m_pages[PAGE_HELP].item++;
            if (m_pages[PAGE_HELP].item > 4)
                m_page = PAGE_MAIN;
            break;
        }
    }
}

// Draws the unit quad from vertex buffer pVB, scaled and moved (with the
// caller's matScale, matTrans and matWorld). A macro: the arguments are
// worked out where they are used.
#define DRAW_QUAD(pVB, texture, sx, sy, x, y, z) \
    D3DXMATRIX matScale, matTrans, matWorld; \
    D3DXMatrixScaling(&matScale, sx, sy, 1.0f); \
    D3DXMatrixTranslation(&matTrans, x, y, z); \
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans); \
    m_pDevice->SetTransform(D3DTS_WORLD, &matWorld); \
    m_pDevice->SetStreamSource(0, pVB, sizeof(MENUVERTEX)); \
    m_pTextures->Set(texture); \
    m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX); \
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2); \
    m_pDevice->SetTexture(0, NULL)

// The same with the highlighted item's buffer when bLit, the plain one
// otherwise.
#define SET_ITEM_VB(bLit) \
    if (bLit) \
        m_pDevice->SetStreamSource(0, m_pVB[1], sizeof(MENUVERTEX)); \
    else \
        m_pDevice->SetStreamSource(0, m_pVB[0], sizeof(MENUVERTEX))

// DRAW_QUAD with SET_ITEM_VB.
#define DRAW_ITEM(bLit, texture, sx, sy, x, y, z) \
    D3DXMATRIX matScale, matTrans, matWorld; \
    D3DXMatrixScaling(&matScale, sx, sy, 1.0f); \
    D3DXMatrixTranslation(&matTrans, x, y, z); \
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans); \
    m_pDevice->SetTransform(D3DTS_WORLD, &matWorld); \
    SET_ITEM_VB(bLit); \
    m_pTextures->Set(texture); \
    m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX); \
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2); \
    m_pDevice->SetTexture(0, NULL)

// 0x000180D0
// The skins page's preview, the help page, or the background; the setup
// page (players, mode, a line per player: number, control, team); the
// other pages' title and items (with the options' values).
void CMenu::Render()
{
    if (m_page == PAGE_SKINS)
        m_skins.RenderPreview();
    if (m_page == PAGE_HELP)
        { DRAW_QUAD(m_pVB[2], m_pages[PAGE_HELP].texture + m_pages[PAGE_HELP].item, 500.0f, 410.0f, 0.0f,
                 0.0f, 500.0f); }
    if (m_page != PAGE_SETUP && m_page != PAGE_HELP)
        { DRAW_QUAD(m_pVB[2], 79, 545.0f, 430.0f, 20.0f, -60.0f, 400.0f); }

    if (m_page == PAGE_SETUP)
    {
        { DRAW_QUAD(m_pVB[2], 130, 130.0f, 10.0f, 0.0f, -178.0f, 1.0f); }
        { DRAW_ITEM(m_pages[m_page].item == 0, 67, 100.0f, 10.0f, -130.0f, 180.0f, 1.0f); }
        { DRAW_QUAD(m_pVB[2], m_pages[m_page].texture + m_settings.nPlayers, 10.0f, 10.0f, 0.0f, 180.0f, 1.0f); }
        if (m_settings.nMode == MODE_TEAMS)
            { DRAW_QUAD(m_pVB[2], 109, 50.0f, 130.0f, 150.0f, -45.0f, 1.0f); }
        { DRAW_ITEM(m_pages[m_page].item == 1, 68, 100.0f, 10.0f, -135.0f, 150.0f, 1.0f); }
        {
            D3DXMATRIX matScale, matTrans, matWorld;
            D3DXMatrixScaling(&matScale, 80.0f, 10.0f, 1.0f);
            D3DXMatrixTranslation(&matTrans, 40.0f, 150.0f, 1.0f);
            D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
            m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            SET_ITEM_VB(m_pages[m_page].item == 1);
            switch (m_settings.nMode)
            {
            case MODE_FREE_FOR_ALL:
                m_pTextures->Set(69);
                break;
            case MODE_TEAMS:
                m_pTextures->Set(70);
                break;
            default:
                m_pTextures->Set(69);
                break;
            }
            m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);
        }

        for (int i = 2; i < m_settings.nPlayers + 2; i++)
        {
            float y = (float)(60 - (i - 2) * 30);
            { DRAW_ITEM(i == m_pages[m_page].item, 47, 100.0f, 10.0f, -130.0f, y, 1.0f); }
            // The player's number (the digits follow the page's texture, 0).
            { DRAW_QUAD(m_pVB[2], m_pages[m_page].texture - 1 + i, 10.0f, 12.0f, -60.0f, y, 1.0f); }
            { DRAW_QUAD(m_pVB[2], m_pages[m_page].texture + 9 + m_settings.nControl[i - 2], 20.0f, 12.0f, 20.0f,
                     y, 1.0f); }

            D3DXMATRIX matScale, matTrans, matWorld;
            D3DXMatrixScaling(&matScale, 20.0f, 12.0f, 1.0f);
            D3DXMatrixTranslation(&matTrans, 80.0f, y, 1.0f);
            D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
            m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            m_pDevice->SetStreamSource(0, m_pVB[2], sizeof(MENUVERTEX));
            if (!m_settings.nTeam[i - 2] || m_settings.nMode != MODE_TEAMS)
                m_pTextures->Set(64);
            if (m_settings.nMode == MODE_TEAMS)
            {
                if (m_settings.nTeam[i - 2] == TEAM_RED)
                    m_pTextures->Set(65);
                if (m_settings.nTeam[i - 2] == TEAM_BLUE)
                    m_pTextures->Set(66);
            }
            m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);
        }
    }

    if (m_page != PAGE_SETUP && m_page != PAGE_HELP)
    {
        { DRAW_QUAD(m_pVB[2], 39, 210.0f, 40.0f, 0.0f, 120.0f, 1.0f); }
        for (int i = 0; i < m_pages[m_page].nItems; i++)
        {
            D3DXMATRIX matScale, matTrans, matWorld;
            if (m_page == PAGE_OPTIONS)
            {
                D3DXMatrixScaling(&matScale, 90.0f, 15.0f, 1.0f);
                D3DXMatrixTranslation(&matTrans, -100.0f, (float)(40 - i * 40), 1.0f);
            }
            else
            {
                D3DXMatrixScaling(&matScale, 100.0f, 20.0f, 1.0f);
                D3DXMatrixTranslation(&matTrans, -100.0f, (float)(40 - i * 50), 1.0f);
            }
            D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
            m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            SET_ITEM_VB(i == m_pages[m_page].item);
            m_pTextures->Set(m_pages[m_page].texture + i);
            m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);

            if (m_page != PAGE_OPTIONS)
                continue;
            if (i == 4)
                break;
            // The option's value.
            D3DXMatrixScaling(&matScale, 40.0f, 15.0f, 1.0f);
            D3DXMatrixTranslation(&matTrans, 130.0f, (float)(40 - i * 40), 1.0f);
            D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
            m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            SET_ITEM_VB(i == m_pages[m_page].item);
            if (i == 0)
            {
                if (g_bMusicOn)
                    m_pTextures->Set(30);
                if (!g_bMusicOn)
                    m_pTextures->Set(31);
            }
            else if (i == 1)
            {
                if (g_bRandomMusic)
                    m_pTextures->Set(32);
                if (!g_bRandomMusic)
                    m_pTextures->Set(33);
            }
            else if (i == 2)
            {
                switch (g_nPieceSet)
                {
                case 0:
                    m_pTextures->Set(34);
                    break;
                case 1:
                    m_pTextures->Set(35);
                    break;
                case 2:
                    m_pTextures->Set(36);
                    break;
                }
            }
            else if (i == 3)
            {
                if (g_bSoundEffects)
                    m_pTextures->Set(30);
                if (!g_bSoundEffects)
                    m_pTextures->Set(31);
            }
            m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);
        }
    }
}

// One vertex buffer: the unit quad with frame n of nCols.
static __forceinline void FillVB(CMenu* pMenu, LPDIRECT3DVERTEXBUFFER8* ppVB, int n, int nCols)
{
    SET_FRAME(s_menuFrame, pMenu->m_uv, n, nCols, 1);
    const SFrameUV& uv = pMenu->m_uv;
    pMenu->m_v[0].tu = uv.tu1; pMenu->m_v[0].tv = uv.tv2;
    pMenu->m_v[1].tu = uv.tu2; pMenu->m_v[1].tv = uv.tv1;
    pMenu->m_v[2].tu = uv.tu4; pMenu->m_v[2].tv = uv.tv3;
    pMenu->m_v[3].tu = uv.tu3; pMenu->m_v[3].tv = uv.tv4;
    pMenu->m_v[0].x = -1.0f; pMenu->m_v[0].y = 1.0f; pMenu->m_v[0].z = 0.0f;
    pMenu->m_v[1].x = 1.0f; pMenu->m_v[1].y = 1.0f; pMenu->m_v[1].z = 0.0f;
    pMenu->m_v[2].x = -1.0f; pMenu->m_v[2].y = -1.0f; pMenu->m_v[2].z = 0.0f;
    pMenu->m_v[3].x = 1.0f; pMenu->m_v[3].y = -1.0f; pMenu->m_v[3].z = 0.0f;
    if (*ppVB == NULL)
        pMenu->m_pDevice->CreateVertexBuffer(4 * sizeof(pMenu->m_v), 0, 0, 0, ppVB);
    void* pVertices;
    (*ppVB)->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, pMenu->m_v, sizeof(pMenu->m_v));
    (*ppVB)->Unlock();
}

// 0x00018CA0
// The items' textures are two frames side by side: 0 the left half
// (plain), 1 the right half (highlighted), 2 the whole texture.
void CMenu::CreateVertexBuffers()
{
    FillVB(this, &m_pVB[0], 0, 2);
    FillVB(this, &m_pVB[1], 1, 2);
    FillVB(this, &m_pVB[2], 0, 1);
}

// 0x00019030
// The pages (items and first texture), the last match's settings, the
// vertex buffers, and the skins (the saved one loaded).
void CMenu::Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, CInput* pInput, CParticles* pParticles)
{
    m_pDevice = pDevice;
    m_pTextures = pTextures;
    m_pParticles = pParticles;
    m_pInput = pInput;
    m_page = PAGE_MAIN;
    m_bStart = false;
    m_pages[PAGE_MAIN].texture = 21;
    m_pages[PAGE_MAIN].nItems = 4;
    m_pages[PAGE_SETUP].nItems = 10;
    m_pages[PAGE_SETUP].texture = 48;
    m_pages[PAGE_OPTIONS].texture = 25;
    m_pages[PAGE_OPTIONS].nItems = 5;
    m_pages[PAGE_PAUSE].nItems = 4;
    m_pages[PAGE_PAUSE].texture = 71;
    m_pages[PAGE_SKINS].nItems = 4;
    m_pages[PAGE_SKINS].texture = 75;
    m_pages[PAGE_HELP].nItems = 5;
    m_pages[PAGE_HELP].texture = 125;
    m_settings = g_Settings;
    CreateVertexBuffers();
    m_skins.Find(pDevice, pTextures);
    m_skins.LoadSaved();
}
