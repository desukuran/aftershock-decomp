// The game: the menus, the countdown, a match of 2-8 boards (free for all
// or two teams), the result, and the pads during play.
#include "Game.h"

// 0x00014740
void CGame::Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, CInput* pInput)
{
    m_state = STATE_MENU;
    m_particles.Create(pDevice, pTextures, 200, 5.0f);
    m_menu.Create(pDevice, pTextures, pInput, &m_particles);
    m_pInput = pInput;
    m_pDevice = pDevice;
    m_pTextures = pTextures;
}

// 0x000147A0
// After T:\baku2.cfg is read: its skin and its match settings.
void CGame::LoadSettings()
{
    m_menu.m_skins.LoadSaved();
    m_menu.m_settings = g_Settings;
}

// The volume in 1/100 dB: down to -100 dB, up to 0.
static __forceinline FLOAT ClampVolume(FLOAT fVolume)
{
    if (fVolume < -10000.0f)
        fVolume = -10000.0f;
    else if (fVolume > 0.0f)
        fVolume = 0.0f;
    return fVolume;
}

// 0x000147D0
// Every 75 ms: on any pad, Start pauses, Back shows or hides the music box,
// the right thumbstick changes the background (across) and the volume (up
// and down), the colored and black/white buttons the music; then each
// player's pad (or the computer) moves their pair. Quirk: player 1 on pad
// 1 restarts the 75 ms each frame, so pad 1 is read every frame it is
// playing.
void CGame::UpdateInput()
{
    static DWORD s_dwInputTime = GetTickCount();
    if (GetTickCount() - s_dwInputTime < 75)
        return;
    m_pInput->Update();

    for (int i = 0; i < 4; i++)
    {
        if (m_pInput->StartDown(i))
        {
            m_menu.m_page = PAGE_PAUSE;
            m_state = STATE_MENU;
        }
        if (m_pInput->BackDown(i))
            g_bShowMusicHud = !g_bShowMusicHud;
        if (m_pInput->RightThumbRight(i) != 0.0f)
        {
            if (g_nBackgrounds == 0)
                g_nBackgrounds = 1;
            if (g_nBackground == g_nBackgrounds + 0x6D)
                g_nBackground = 0x6E;
            else
                g_nBackground++;
        }
        if (m_pInput->RightThumbLeft(i) != 0.0f)
        {
            if (g_nBackgrounds == 0)
                g_nBackgrounds = 1;
            if (g_nBackground == 0x6E)
                g_nBackground = g_nBackgrounds + 0x6D;
            else
                g_nBackground--;
        }
        if (m_pInput->YPressed(i))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.NextSoundtrack();
            g_Music.Play();
        }
        if (m_pInput->BPressed(i))
        {
            g_dwMusicHudTime = GetTickCount();
            if (g_Music.m_mmState == MM_PLAYING)
                g_Music.Pause();
            else if (g_Music.m_mmState == MM_PAUSED || g_Music.m_mmState == MM_STOPPED)
                g_Music.Play();
        }
        if (m_pInput->XPressed(i))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.RandomSong(g_Music.m_bGlobal);
            g_Music.Play();
        }
        if (m_pInput->RightThumbDown(i) != 0.0f)
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.SetVolume(ClampVolume(g_Music.m_fVolume - 200.0f));
        }
        if (m_pInput->RightThumbUp(i) != 0.0f)
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.SetVolume(ClampVolume(g_Music.m_fVolume + 200.0f));
        }
        if (m_pInput->BlackPressed(i))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.NextSong();
            g_Music.Play();
        }
        if (m_pInput->WhitePressed(i))
        {
            g_dwMusicHudTime = GetTickCount();
            g_Music.Stop();
            g_Music.PrevSong();
            g_Music.Play();
        }
    }

    for (int i = 0; i < m_pSettings->nPlayers; i++)
    {
        CBoard* pBoard = &m_boards[i];
        switch (m_pSettings->nControl[i])
        {
        case CONTROL_PAD1:
            s_dwInputTime = GetTickCount();
            if (m_pInput->StartDown(0))
            {
                m_menu.m_page = PAGE_PAUSE;
                m_state = STATE_MENU;
            }
            if (pBoard->m_bAlive)
            {
                if (m_pInput->APressed(0))
                    pBoard->Rotate();
                if (m_pInput->DPadDown(0) || m_pInput->LeftThumbDown(0) != 0.0f)
                    pBoard->m_nBoost = 8;
                if (m_pInput->DPadLeft(0) || m_pInput->LeftThumbLeft(0) != 0.0f)
                    pBoard->MoveLeft();
                if (m_pInput->DPadRight(0) || m_pInput->LeftThumbRight(0) != 0.0f)
                    pBoard->MoveRight();
            }
            break;
        case CONTROL_PAD1 + 1:
            if (pBoard->m_bAlive)
            {
                if (m_pInput->DPadDown(1) || m_pInput->LeftThumbDown(1) != 0.0f)
                    pBoard->m_nBoost = 8;
                if (m_pInput->DPadLeft(1) || m_pInput->LeftThumbLeft(1) != 0.0f)
                    pBoard->MoveLeft();
                if (m_pInput->DPadRight(1) || m_pInput->LeftThumbRight(1) != 0.0f)
                    pBoard->MoveRight();
                if (m_pInput->APressed(1))
                    pBoard->Rotate();
            }
            break;
        case CONTROL_PAD1 + 2:
            if (pBoard->m_bAlive)
            {
                if (m_pInput->DPadDown(2) || m_pInput->LeftThumbDown(2) != 0.0f)
                    pBoard->m_nBoost = 8;
                if (m_pInput->DPadLeft(2) || m_pInput->LeftThumbLeft(2) != 0.0f)
                    pBoard->MoveLeft();
                if (m_pInput->DPadRight(2) || m_pInput->LeftThumbRight(2) != 0.0f)
                    pBoard->MoveRight();
                if (m_pInput->APressed(2))
                    pBoard->Rotate();
            }
            break;
        case CONTROL_PAD1 + 3:
            if (pBoard->m_bAlive)
            {
                if (m_pInput->DPadDown(3) || m_pInput->LeftThumbDown(3) != 0.0f)
                    pBoard->m_nBoost = 8;
                if (m_pInput->DPadLeft(3) || m_pInput->LeftThumbLeft(3) != 0.0f)
                    pBoard->MoveLeft();
                if (m_pInput->DPadRight(3) || m_pInput->LeftThumbRight(3) != 0.0f)
                    pBoard->MoveRight();
                if (m_pInput->APressed(3))
                    pBoard->Rotate();
            }
            break;
        case CONTROL_EASY:
            pBoard->Think(false);
            break;
        case CONTROL_HARD:
            pBoard->Think(true);
            break;
        }
    }
}

// 0x00014E20
// The background and the boards; the countdown's words over them (three
// styles: ready 3 2 1 go, ready... here we go, are you ready... go); the
// winners' "great" and puffs; then the puffs and, in the menus, the menus.
void CGame::Render()
{
    D3DXMATRIX matScale, matTrans, matWorld;

    if (m_state == STATE_PLAY || m_state == STATE_RESULT || m_state == STATE_COUNTDOWN)
    {
        D3DXMatrixScaling(&matScale, 550.0f, 420.0f, 1.0f);
        D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 500.0f);
        D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(g_nBackground);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);

        for (int i = 0; i < m_pSettings->nPlayers; i++)
            m_boards[i].Render();

        if (m_state == STATE_COUNTDOWN)
        {
            D3DXMatrixScaling(&matScale, 120.0f, 40.0f, 1.0f);
            D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 1.0f);
            D3DXMatrixMultiply(&matTrans, &matScale, &matWorld);
            m_pDevice->SetTransform(D3DTS_WORLD, &matTrans);
            m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
            m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
            switch (m_nCountdown)
            {
            case 0:
            case 1:
                if (m_nCountdownStyle == 0 || m_nCountdownStyle == 1)
                    m_pTextures->Set(40);
                if (m_nCountdownStyle == 2)
                    m_pTextures->Set(95);
                break;
            case 2:
                if (m_nCountdownStyle == 0)
                    m_pTextures->Set(41);
                if (m_nCountdownStyle != 0)
                    m_pTextures->Set(40);
                if (m_nCountdownStyle == 2)
                    m_pTextures->Set(96);
                break;
            case 3:
                if (m_nCountdownStyle == 0)
                    m_pTextures->Set(42);
                if (m_nCountdownStyle != 0)
                    m_pTextures->Set(93);
                if (m_nCountdownStyle == 2)
                    m_pTextures->Set(40);
                break;
            case 4:
                if (m_nCountdownStyle == 0)
                    m_pTextures->Set(43);
                if (m_nCountdownStyle != 0)
                    m_pTextures->Set(94);
                if (m_nCountdownStyle == 2)
                    m_pTextures->Set(40);
                break;
            default:
                m_pTextures->Set(44);
                break;
            }
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);
        }

        if (m_state == STATE_RESULT)
        {
            for (int i = 0; i < m_pSettings->nPlayers; i++)
            {
                CBoard* pBoard = &m_boards[i];
                if (pBoard->m_bLost)
                    continue;
                m_particles.Add((float)(pBoard->m_bx + rand() % (pBoard->m_nPieceScale * 5) * 2),
                                (float)(pBoard->m_nPieceScale * 5 - rand() % (pBoard->m_nPieceScale * 10) +
                                        pBoard->m_nCellSize * 4 + pBoard->m_by),
                                (float)(rand() % 30 + pBoard->m_bz));
                m_particles.Add((float)(pBoard->m_bx + rand() % (pBoard->m_nPieceScale * 5) * 2),
                                (float)(pBoard->m_nPieceScale * 5 - rand() % (pBoard->m_nPieceScale * 10) +
                                        pBoard->m_nCellSize * 4 + pBoard->m_by),
                                (float)(rand() % 30 + pBoard->m_bz));
                D3DXMatrixScaling(&matScale, (float)(pBoard->m_nPieceScale * 5), (float)(pBoard->m_nPieceScale * 5),
                                  1.0f);
                D3DXMatrixTranslation(&matWorld,
                                      (float)(pBoard->m_nCellSize * 3 - pBoard->m_nCellSize / 2 + pBoard->m_bx),
                                      (float)(pBoard->m_by + pBoard->m_nCellSize * 4), (float)pBoard->m_bz);
                D3DXMatrixMultiply(&matTrans, &matScale, &matWorld);
                m_pDevice->SetTransform(D3DTS_WORLD, &matTrans);
                m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
                m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
                m_pTextures->Set(80);
                m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
                m_pDevice->SetTexture(0, NULL);
            }
        }
    }

    m_particles.Render();
    if (m_state == STATE_MENU)
        m_menu.Render();
}

// 0x000152B0
// Level 1, a countdown style, a background, and the boards laid out for
// the number of players: two or three side by side; four on a far row (or
// near ones for two players and two computers far behind them); more in two
// far rows. The portraits: each player's control, or their team's color.
void CGame::StartMatch()
{
    m_nLevel = 1;
    m_dwLevelTime = GetTickCount();
    m_nCountdownStyle = rand() % 3;
    if (g_nBackgrounds == 0)
        g_nBackgrounds = 1;
    g_nBackground = rand() % g_nBackgrounds + 0x6E;

    int portraits[8];
    for (int i = 0; i < m_pSettings->nPlayers; i++)
    {
        if (m_pSettings->nMode == MODE_FREE_FOR_ALL)
        {
            portraits[i] = m_pSettings->nControl[i] + 101;
        }
        else if (m_pSettings->nMode == MODE_TEAMS)
        {
            if (m_pSettings->nTeam[i] == TEAM_RED)
                portraits[i] = 65;
            if (m_pSettings->nTeam[i] == TEAM_BLUE)
                portraits[i] = 66;
        }
    }

    m_pSettings->nBoards = m_pSettings->nPlayers;
    if (m_pSettings->nBoards < 4)
        m_pSettings->nBoards--;
    if (m_pSettings->nBoards == 4 && m_pSettings->nControl[2] > 3 && m_pSettings->nControl[3] > 3)
        m_pSettings->nBoards = 3;

    switch (m_pSettings->nBoards)
    {
    case 1:
        for (int i = 0; i < 2; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -200 + i * 250, -170, 1, &m_particles, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        break;
    case 2:
        for (int i = 0; i < 3; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -260 + i * 190, -170, 100, &m_particles, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        break;
    case 3:
        for (int i = 0; i < 2; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -260 + i * 190, -170, 100, &m_particles, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        for (int i = 0; i < 2; i++)
        {
            m_boards[2 + i].Init(m_pDevice, m_pTextures, 30, 14, 200, -300 + i * 350, 500, NULL, portraits[2 + i]);
            m_boards[2 + i].Reset();
            m_boards[2 + i].m_bShowEaters = false;
        }
        break;
    case 4:
        for (int i = 0; i < 4; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, -160, 430, NULL, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        break;
    case 5:
    {
        int i;
        for (i = 0; i < 4; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, 35, 430, NULL, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        m_boards[4].Init(m_pDevice, m_pTextures, 30, 14, -95, -320, 430, NULL, portraits[i]);
        m_boards[4].Reset();
        m_boards[4].m_bShowEaters = true;
        break;
    }
    case 6:
        for (int i = 0; i < 4; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, 35, 430, NULL, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        for (int i = 0; i < 2; i++)
        {
            m_boards[4 + i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 570, -320, 430, NULL, portraits[4 + i]);
            m_boards[4 + i].Reset();
            m_boards[4 + i].m_bShowEaters = true;
        }
        break;
    case 7:
        for (int i = 0; i < 4; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, 35, 430, NULL, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        for (int i = 0; i < 3; i++)
        {
            m_boards[4 + i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 280, -320, 410, NULL, portraits[4 + i]);
            m_boards[4 + i].Reset();
            m_boards[4 + i].m_bShowEaters = true;
        }
        break;
    case 8:
        for (int i = 0; i < 4; i++)
        {
            m_boards[i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, 35, 430, NULL, portraits[i]);
            m_boards[i].Reset();
            m_boards[i].m_bShowEaters = true;
        }
        for (int i = 0; i < 4; i++)
        {
            m_boards[4 + i].Init(m_pDevice, m_pTextures, 30, 14, -350 + i * 190, -320, 410, NULL, portraits[4 + i]);
            m_boards[4 + i].Reset();
            m_boards[4 + i].m_bShowEaters = true;
        }
        break;
    }
}

// 0x000158F0
// The garbage bar's color: green to red over the minute to the next drop.
void CGame::UpdateGarbageBar()
{
    DWORD dwSeconds = (GetTickCount() - m_dwGarbageTime) / 1000;
    DWORD dwRed = dwSeconds * 4;
    D3DCOLOR color = D3DCOLOR_ARGB(250, dwRed, 255 - dwRed, 0);
    COLORVERTEX v[4] =
    {
        { -1.0f,  1.0f, 0.0f, color },
        {  1.0f,  1.0f, 0.0f, color },
        { -1.0f, -1.0f, 0.0f, color },
        {  1.0f, -1.0f, 0.0f, color },
    };
    if (g_pBarVB == NULL)
        g_pDevice->CreateVertexBuffer(4 * sizeof(v), 0, 0, 0, &g_pBarVB);
    void* pVertices;
    g_pBarVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, v, sizeof(v));
    g_pBarVB->Unlock();
}

// 0x000159D0
// Sends each board's garbage to the others: free for all, to anyone alive
// at random; teams, to the other team's living boards, until that team
// is out. Quirk: free for all, with nobody else alive it never ends.
void CGame::ShareGarbage()
{
    if (m_state != STATE_PLAY)
        return;
    for (int i = 0; i < m_pSettings->nPlayers; i++)
    {
        CBoard* pBoard = &m_boards[i];
        if (pBoard->m_nSend == 0)
            continue;
        if (m_pSettings->nMode == MODE_FREE_FOR_ALL)
        {
            while (pBoard->m_nSend)
            {
                int j = rand() % m_pSettings->nPlayers;
                if (j != i && m_boards[j].m_bAlive)
                {
                    m_boards[j].m_nGarbage++;
                    pBoard->m_nSend--;
                }
            }
        }
        if (m_pSettings->nMode == MODE_TEAMS)
        {
            while (pBoard->m_nSend)
            {
                int nRedOut = 0;
                int nBlueOut = 0;
                for (int k = 0; k < m_pSettings->nPlayers; k++)
                {
                    if (!m_boards[k].m_bAlive)
                    {
                        if (m_pSettings->nTeam[k] == TEAM_RED)
                            nRedOut++;
                        if (m_pSettings->nTeam[k] == TEAM_BLUE)
                            nBlueOut++;
                    }
                }
                if (m_pSettings->nTeam[i] == TEAM_RED)
                {
                    if (nBlueOut >= m_pSettings->nBlue)
                        pBoard->m_nSend = 0;
                    int j = rand() % m_pSettings->nPlayers;
                    if (m_pSettings->nTeam[j] == TEAM_BLUE && j != i && m_boards[j].m_bAlive && pBoard->m_nSend)
                    {
                        m_boards[j].m_nGarbage++;
                        pBoard->m_nSend--;
                    }
                }
                if (m_pSettings->nTeam[i] == TEAM_BLUE)
                {
                    if (nRedOut >= m_pSettings->nRed)
                        pBoard->m_nSend = 0;
                    int j = rand() % m_pSettings->nPlayers;
                    if (m_pSettings->nTeam[j] == TEAM_RED && j != i && m_boards[j].m_bAlive && pBoard->m_nSend)
                    {
                        m_boards[j].m_nGarbage++;
                        pBoard->m_nSend--;
                    }
                }
            }
        }
    }
}

// 0x00015BB0
// Every minute of play, every board drops the garbage it is owed.
void CGame::UpdateTimers()
{
    if (m_dwGarbageTime == 0)
        m_dwGarbageTime = GetTickCount();
    if (m_state != STATE_PLAY)
        m_dwGarbageTime = GetTickCount();
    if (m_state == STATE_PLAY)
    {
        if (GetTickCount() - m_dwGarbageTime >= 60000)
        {
            for (int i = 0; i < m_pSettings->nPlayers; i++)
                m_boards[i].m_bGarbage = true;
            m_dwGarbageTime = GetTickCount();
        }
    }
    UpdateGarbageBar();
}

// 0x00015C40
// A frame: the level goes up every 90 s of play (to 4, the drop speed);
// the menus start, quit or resume a match; the countdown, half a second a
// step; play until one board (or team) is left, the wins and losses
// counted; then 3 s of result and the next round.
void CGame::Update()
{
    m_particles.Update();
    UpdateTimers();
    ShareGarbage();
    if (GetTickCount() - m_dwLevelTime >= 90000 && m_state == STATE_PLAY)
    {
        if (m_nLevel < 4)
            m_nLevel++;
        m_dwLevelTime = GetTickCount();
        for (int i = 0; i < m_pSettings->nPlayers; i++)
            m_boards[i].m_nLevel = m_nLevel;
    }

    if (m_state == STATE_MENU || m_state == STATE_NEXT)
    {
        m_menu.Update();
        if (m_menu.m_bStart)
        {
            m_menu.m_bStart = false;
            m_bOver = false;
            m_pSettings = &m_menu.m_settings;
            StartMatch();
            m_nCountdown = 0;
            m_state = STATE_COUNTDOWN;
        }
        if (m_menu.m_bQuit)
        {
            m_menu.m_bQuit = false;
            for (int i = 0; i < m_pSettings->nPlayers; i++)
            {
                m_boards[i].m_bAlive = false;
                m_boards[i].Reset();
                m_menu.m_page = PAGE_MAIN;
                m_state = STATE_MENU;
            }
        }
        if (m_menu.m_bResume)
        {
            m_menu.m_bResume = false;
            m_state = STATE_PLAY;
        }
    }

    if (m_state == STATE_COUNTDOWN)
    {
        if (m_nCountdown > 5)
        {
            for (int i = 0; i < m_pSettings->nPlayers; i++)
                m_boards[i].Begin();
            m_state = STATE_PLAY;
            m_dwGarbageTime = GetTickCount();
        }
        if (m_nCountdown < 6)
        {
            DWORD dwTime = m_dwCountdownTime;
            if (GetTickCount() - dwTime >= 500 || dwTime == 0)
            {
                if (m_nCountdown == 4 && g_bSoundEffects)
                    g_SoundGo.Play();
                m_dwCountdownTime = GetTickCount();
                m_nCountdown++;
            }
        }
    }

    if (m_state == STATE_PLAY)
    {
        UpdateInput();
        int nOut = 0;
        int nRedOut = 0;
        int nBlueOut = 0;
        switch (m_pSettings->nMode)
        {
        case MODE_FREE_FOR_ALL:
            for (int i = 0; i < m_pSettings->nPlayers; i++)
            {
                if (!m_boards[i].m_bAlive)
                    nOut++;
            }
            if (nOut >= m_pSettings->nPlayers - 1)
            {
                for (int i = 0; i < m_pSettings->nPlayers; i++)
                {
                    if (!m_boards[i].m_bAlive)
                        m_boards[i].m_nLosses++;
                    if (m_boards[i].m_bAlive)
                        m_boards[i].m_nWins++;
                    m_boards[i].m_bAlive = false;
                }
                m_bOver = false;
                m_dwResultTime = GetTickCount();
                m_state = STATE_RESULT;
            }
            break;
        case MODE_TEAMS:
            for (int i = 0; i < m_pSettings->nPlayers; i++)
            {
                if (!m_boards[i].m_bAlive)
                {
                    if (m_pSettings->nTeam[i] == TEAM_RED)
                        nRedOut++;
                    if (m_pSettings->nTeam[i] == TEAM_BLUE)
                        nBlueOut++;
                }
            }
            if (nRedOut >= m_pSettings->nRed)
            {
                for (int i = 0; i < m_pSettings->nPlayers; i++)
                {
                    if (m_pSettings->nTeam[i] == TEAM_RED)
                        m_boards[i].m_nLosses++;
                    if (m_pSettings->nTeam[i] == TEAM_BLUE)
                        m_boards[i].m_nWins++;
                    m_boards[i].m_bAlive = false;
                }
                m_bOver = false;
                m_dwResultTime = GetTickCount();
                m_state = STATE_RESULT;
            }
            else if (nBlueOut >= m_pSettings->nBlue)
            {
                for (int i = 0; i < m_pSettings->nPlayers; i++)
                {
                    if (m_pSettings->nTeam[i] == TEAM_BLUE)
                        m_boards[i].m_nLosses++;
                    if (m_pSettings->nTeam[i] == TEAM_RED)
                        m_boards[i].m_nWins++;
                    m_boards[i].m_bAlive = false;
                }
                m_bOver = false;
                m_dwResultTime = GetTickCount();
                m_state = STATE_RESULT;
            }
            break;
        }
        for (int i = 0; i < m_pSettings->nPlayers; i++)
            m_boards[i].Update();
    }

    if (m_state == STATE_RESULT)
    {
        for (int i = 0; i < m_pSettings->nPlayers; i++)
            m_boards[i].Update();
        if (GetTickCount() - m_dwResultTime >= 3000)
        {
            if (m_pSettings->nMode == MODE_FREE_FOR_ALL)
            {
                m_state = STATE_NEXT;
                if (m_bOver)
                {
                    m_state = STATE_MENU;
                }
                else
                {
                    m_menu.m_bStart = true;
                    Update();
                }
            }
            if (m_pSettings->nMode == MODE_TEAMS)
            {
                m_state = STATE_NEXT;
                m_menu.m_bStart = true;
                Update();
            }
        }
    }
}
