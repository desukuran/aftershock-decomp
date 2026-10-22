// main(), the device, the splash, the frame (with the music box), and the
// globals.
#include "Game.h"

int g_nBackground;                      // 0x000FF754 the background's texture, 0x6E on
LPDIRECTSOUND8 g_pDSound;               // 0x000FF760
LPDIRECT3DVERTEXBUFFER8 g_pQuadVB;      // 0x000FF764 the unit quad, textured
bool g_bMusicOn;                        // 0x000FF768
CInput g_Input;                         // 0x000FF770
LPDIRECT3DDEVICE8 g_pDevice;            // 0x000FFB38
bool g_bShowMusicHud;                   // 0x000FFB40 Back shows or hides the music box
bool g_bRandomMusic;                    // 0x000FFB41
int g_nBackgrounds;                     // 0x000FFB44 in the skin
DWORD g_dwMusicHudTime;                 // 0x000FFB48 the music box is up for 1.5 s from this
SMatchSettings g_Settings;              // 0x000FFB50 the last match's (saved)
CSoundEffect g_SoundGo;                 // 0x000FFBA4
LPDIRECT3DVERTEXBUFFER8 g_pColorQuadVB; // 0x000FFBA8 the unit quad, colored (an empty cell)
CFont g_Font;                           // 0x000FFBAC
bool g_bSoundEffects;                   // 0x000FFBB8
CTextures g_Textures;                   // 0x000FFBC0
CSoundEffect g_SoundEat;                // 0x000FFDD0
LPDIRECT3DTEXTURE8 g_pSplashTexture;    // 0x000FFDD4
int g_nPieceSet;                        // 0x000FFDD8 0 anything, 1 food in each pair, 2 a head in each
CConfig g_Config;                       // 0x000FFDE0
LPDIRECT3DVERTEXBUFFER8 g_pBarVB;       // 0x000FFEE0 the garbage bar and the volume bar
// Its constructor and destructor run as static initializers.
CMusicManager g_Music;                  // 0x000FFEE8
// Its constructor (0x00017240, setting the boards' vftables) is
// compiler-generated.
CGame g_Game;                           // 0x00100040

// 0x00016810
// 640x480, looking down +z from (0, 0, -500); alpha blending on.
void InitDevice()
{
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_F24S8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.FullScreen_RefreshRateInHz = 0;
    Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_pDevice);

    g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pDevice->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.33333f, 1.0f, 1000.0f);
    g_pDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vEye(0.0f, 0.0f, -500.0f);
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pDevice->SetTransform(D3DTS_VIEW, &matView);
}

// 0x000169A0
// The game, then the music box for 1.5 s after the music changes (if Back
// has not hidden it): soundtrack, song, position, and the volume as a bar
// from green (loud) to red.
void RenderFrame()
{
    g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF2846C8, 1.0f, 0);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_Game.Render();

    if (g_dwMusicHudTime && g_bShowMusicHud)
    {
        if (GetTickCount() - g_dwMusicHudTime >= 1500)
            g_dwMusicHudTime = 0;

        WCHAR strSoundtrack[30], strSong[30];
        WCHAR strSoundtrackName[MAX_SOUNDTRACK_NAME], strSongName[MAX_SONG_NAME];
        WCHAR strTime[100];
        DWORD dwLength;
        float fPosition = g_Music.GetPlaybackPosition();
        g_Music.GetCurrentInfo(strSoundtrackName, strSongName, &dwLength);
        swprintf(strTime, L"%02d:%02d / %02d:%02d", (int)(fPosition * (1.0f / 60.0f)), (DWORD)fPosition % 60,
                 dwLength / 60000, dwLength / 1000 % 60);
        wcsncpy(strSong, strSongName, 23);
        wcsncpy(strSoundtrack, strSoundtrackName, 23);
        g_Font.DrawText(strSoundtrack, 310.0f, 350.0f, 0, 0, 0);
        g_Font.DrawText(strSong, 310.0f, 380.0f, 0, 0, 0);
        g_Font.DrawText(L"Position: ", 310.0f, 410.0f, 0, 0, 0);
        g_Font.DrawText(strTime, 410.0f, 410.0f, 0, 0, 0);

        D3DXMATRIX matScale, matTrans, matWorld;
        D3DXMatrixScaling(&matScale, 135.0f, 75.0f, 1.0f);
        D3DXMatrixTranslation(&matTrans, 105.0f, -110.0f, 1.0f);
        D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
        g_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        g_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        g_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        g_Textures.Set(97);
        g_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        g_Textures.Set(0);

        // 0-100: the volume's amplitude in percent.
        float fHeight = (float)pow(10.0, (double)(g_Music.m_fVolume * 0.0005f)) * 100.0f;
        int nGreen = (int)(fHeight * 2) - 1;
        D3DCOLOR color = D3DCOLOR_ARGB(250, 255 - nGreen, nGreen, 0);
        COLORVERTEX v[4] =
        {
            { -1.0f,  1.0f, 0.0f, color },
            {  1.0f,  1.0f, 0.0f, color },
            { -0.5f, -1.0f, 0.0f, color },
            {  1.0f, -1.0f, 0.0f, color },
        };
        if (g_pBarVB == NULL)
            g_pDevice->CreateVertexBuffer(4 * sizeof(v), 0, 0, 0, &g_pBarVB);
        void* pVertices;
        g_pBarVB->Lock(0, 0, (BYTE**)&pVertices, 0);
        memcpy(pVertices, v, sizeof(v));
        g_pBarVB->Unlock();

        fHeight *= 0.5f;
        D3DXMATRIX matBarScale, matBarTrans, matBar;
        D3DXMatrixScaling(&matBarScale, 10.0f, fHeight, 1.0f);
        D3DXMatrixTranslation(&matBarTrans, 220.0f, fHeight - 170.0f, 1.0f);
        D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
        g_pDevice->SetTransform(D3DTS_WORLD, &matBar);
        g_pDevice->SetTexture(0, NULL);
        g_pDevice->SetStreamSource(0, g_pBarVB, sizeof(COLORVERTEX));
        g_pDevice->SetVertexShader(D3DFVF_COLORVERTEX);
        g_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    g_Textures.Set(0);
    g_pDevice->Present(NULL, NULL, NULL, NULL);
}

// 0x00016DC0
void ShowSplash()
{
    g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF6400FF, 1.0f, 0);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    D3DXMATRIX matScale, matTrans, matWorld;
    D3DXMatrixScaling(&matScale, 272.0f, 206.0f, 1.0f);
    D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 1.0f);
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
    g_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pDevice->SetTexture(0, g_pSplashTexture);
    g_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
    g_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
    g_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    g_pDevice->Present(NULL, NULL, NULL, NULL);
}

// 0x00016E90
// The device and the splash (shown while everything loads), the two unit
// quads, the sound, the font, the default settings (then T:\baku2.cfg's),
// the music, the pads, and the frame loop.
void __cdecl main()
{
    InitDevice();
    D3DXCreateTextureFromFileExA(g_pDevice, "D:\\shizno.png", 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                 D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, 0, NULL, NULL, &g_pSplashTexture);

    COLORVERTEX cv[4] =
    {
        { -1.0f,  1.0f, 0.0f, 0x80FF8080 },
        {  1.0f,  1.0f, 0.0f, 0x80FF8080 },
        { -1.0f, -1.0f, 0.0f, 0x80FF8080 },
        {  1.0f, -1.0f, 0.0f, 0x80FF8080 },
    };
    QUADVERTEX qv[4] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
    };
    void* pVertices;
    if (g_pQuadVB == NULL)
        g_pDevice->CreateVertexBuffer(4 * sizeof(qv), 0, 0, 0, &g_pQuadVB);
    g_pQuadVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, qv, sizeof(qv));
    g_pQuadVB->Unlock();
    if (g_pColorQuadVB == NULL)
        g_pDevice->CreateVertexBuffer(4 * sizeof(cv), 0, 0, 0, &g_pColorQuadVB);
    g_pColorQuadVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, cv, sizeof(cv));
    g_pColorQuadVB->Unlock();

    ShowSplash();
    g_Textures.SetDevice(g_pDevice);

    DirectSoundCreate(NULL, &g_pDSound, NULL);
    DSEFFECTIMAGELOC loc;
    loc.dwI3DL2ReverbIndex = 0;
    loc.dwCrosstalkIndex = 1;
    if (SUCCEEDED(XAudioDownloadEffectsImage("d:\\media\\dsstdfx.bin", &loc, XAUDIO_DOWNLOADFX_EXTERNFILE, NULL)))
        g_Music.Initialize();
    g_Music.SetGlobal(!g_Music.m_bGlobal);

    g_SoundEat.Load("D:\\media\\snd\\eat.wav");
    g_SoundGo.Load("D:\\media\\snd\\go.wav");
    g_Font.Create(g_pDevice, NULL);

    strcpy(g_Config.m_strSkin, "D:\\skins\\default\\");
    g_Settings.nControl[0] = CONTROL_PAD1;
    g_Settings.nControl[1] = CONTROL_PAD1 + 1;
    g_Settings.nControl[2] = CONTROL_PAD1 + 2;
    g_Settings.nControl[3] = CONTROL_PAD1 + 3;
    g_Settings.nControl[4] = CONTROL_EASY;
    g_Settings.nControl[5] = CONTROL_HARD;
    g_Settings.nControl[6] = CONTROL_HARD;
    g_Settings.nControl[7] = CONTROL_HARD;
    g_Settings.nTeam[0] = TEAM_RED;
    g_Settings.nTeam[1] = TEAM_RED;
    g_Settings.nTeam[2] = TEAM_RED;
    g_Settings.nTeam[3] = TEAM_RED;
    g_Settings.nTeam[4] = TEAM_BLUE;
    g_Settings.nTeam[5] = TEAM_BLUE;
    g_Settings.nTeam[6] = TEAM_BLUE;
    g_Settings.nTeam[7] = TEAM_BLUE;
    g_Settings.nRed = 4;
    g_Settings.nBlue = 4;
    g_Settings.nPlayers = 8;
    g_Settings.nMode = MODE_FREE_FOR_ALL;
    g_Settings.nBoards = 8;
    g_Game.Create(g_pDevice, &g_Textures, &g_Input);

    g_bSoundEffects = true;
    g_bMusicOn = true;
    g_bRandomMusic = false;
    g_nPieceSet = 0;
    g_bShowMusicHud = true;
    g_Config.Load();
    g_Music.SetRandom(g_bRandomMusic);
    g_Game.LoadSettings();
    if (g_bMusicOn)
        g_Music.Play();
    g_Input.Init();
    g_pSplashTexture->Release();

    for (;;)
    {
        DirectSoundDoWork();
        g_Music.Process();
        g_Game.Update();
        RenderFrame();
    }
}
