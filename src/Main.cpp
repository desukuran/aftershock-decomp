// main(), the frame loop, the intro, and the global objects.
#include "Game.h"
#include "Credits.h"

// Its constructor (0x0002F970) and destructor (0x0002FA30) are compiler-generated.
CGame g_Game;

// The models, in their original declaration order.
CMesh g_BallMesh;
CMesh g_ShadowMesh;
CMesh g_UnusedMesh;
CMesh g_PocketMesh;
CMesh g_CueMesh;
CMesh g_SkyboxMesh;
CMesh g_RoofMesh;
CMesh g_BarTableMesh;
CMesh g_BarStoolMesh;
CMesh g_OverLightMesh;
CMesh g_DartboardMesh;
CMesh g_PoolTableMesh;
CMesh g_TableChairMesh;
CMesh g_PosterMeshes[5];        // only four are loaded
CMesh g_TrashMesh;
CMesh g_DeerMesh;
CMesh g_DeerBackMesh;
CMesh g_DeerLabelMeshes[2];
CMesh g_LaptopMesh;
CMesh g_XbinsMesh;
CMesh g_CrierMesh;
CMesh g_BeerFridgeMesh;
CMesh g_FanMesh;
CMesh g_OutsideMesh;
CMesh g_WindowMesh;
CMesh g_GlassMesh;
CMesh g_DoorMesh;
CMesh g_TapMesh;
CMesh g_SinkMesh;
CMesh g_XboxMesh;
CMesh g_ArrowMesh;

D3DLIGHT8 g_Lights[3];

// 0x000B1308
// The 15-ball triangle with the cue ball, in table units (the main menu,
// intro and editor scale it to the table).
const float kRackShape16X[16] = { -0.8f, 0.0f, 0.24f, 0.06f, 0.18f, 0.24f, 0.18f, 0.18f,
                                  0.12f, 0.18f, 0.12f, 0.12f, 0.24f, 0.24f, 0.06f, 0.24f };
const float kRackShape16Z[16] = { 0.0f, 0.0f, -0.07f, -0.035f, -0.035f, 0.14f, 0.105f, -0.105f,
                                  0.0f, 0.035f, -0.07f, 0.07f, 0.07f, 0.0f, 0.035f, -0.14f };

// 0x000B1388
// The 9 ball diamond with the cue ball (for the editor).
const float kRackShape10X[10] = { -0.8f, 0.0f, 0.06f, 0.06f, 0.12f, 0.18f, 0.12f, 0.18f, 0.24f, 0.12f };
const float kRackShape10Z[10] = { 0.0f, 0.0f, 0.035f, -0.035f, 0.07f, 0.035f, -0.07f, -0.035f, 0.0f, 0.0f };

// The intro's barriers: a fixed square around the balls, then a box around
// each paragraph of the scrolling credits (z follows the scroll), each a
// closed line of five points followed by a (0,0,0) separator. Only x and z
// are written (the separators get y = 0 too). The second box's last point is
// at 72 instead of 70, leaving it slightly open.
struct INTROBARRIER
{
    float x, z;
    BOOL  bScrolls;
};

static const INTROBARRIER kIntroBarriers[] =
{
    { -30.0f, 30.0f, FALSE },
    { 30.0f, 30.0f, FALSE },
    { 30.0f, -30.0f, FALSE },
    { -30.0f, -30.0f, FALSE },
    { -30.0f, 30.0f, FALSE },
    { 0.0f, 0.0f, FALSE },
    { -19.0f, 70.0f, TRUE  },
    { -19.0f, 54.0f, TRUE  },
    { 19.0f, 54.0f, TRUE  },
    { 19.0f, 70.0f, TRUE  },
    { -19.0f, 72.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -17.0f, 51.0f, TRUE  },
    { -17.0f, 36.0f, TRUE  },
    { 17.0f, 36.0f, TRUE  },
    { 17.0f, 51.0f, TRUE  },
    { -17.0f, 51.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -16.0f, 31.0f, TRUE  },
    { -16.0f, 25.0f, TRUE  },
    { 19.0f, 25.0f, TRUE  },
    { 19.0f, 31.0f, TRUE  },
    { -16.0f, 31.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -18.0f, 20.0f, TRUE  },
    { -18.0f, 9.0f, TRUE  },
    { 17.0f, 9.0f, TRUE  },
    { 17.0f, 20.0f, TRUE  },
    { -18.0f, 20.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -6.0f, 2.0f, TRUE  },
    { -6.0f, -3.0f, TRUE  },
    { 10.0f, -3.0f, TRUE  },
    { 10.0f, 2.0f, TRUE  },
    { -6.0f, 2.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -15.0f, -5.0f, TRUE  },
    { -15.0f, -11.0f, TRUE  },
    { 16.0f, -11.0f, TRUE  },
    { 16.0f, -5.0f, TRUE  },
    { -15.0f, -5.0f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -9.0f, -14.5f, TRUE  },
    { -9.0f, -34.0f, TRUE  },
    { 4.0f, -34.0f, TRUE  },
    { 4.0f, -14.5f, TRUE  },
    { -9.0f, -14.5f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -18.0f, -32.5f, TRUE  },
    { -18.0f, -46.0f, TRUE  },
    { 18.0f, -46.0f, TRUE  },
    { 18.0f, -32.5f, TRUE  },
    { -18.0f, -32.5f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -17.0f, -51.5f, TRUE  },
    { -17.0f, -59.0f, TRUE  },
    { 19.0f, -59.0f, TRUE  },
    { 19.0f, -51.5f, TRUE  },
    { -17.0f, -51.5f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -19.0f, -61.5f, TRUE  },
    { -19.0f, -68.0f, TRUE  },
    { 18.0f, -68.0f, TRUE  },
    { 18.0f, -61.5f, TRUE  },
    { -19.0f, -61.5f, TRUE  },
    { 0.0f, 0.0f, FALSE },
    { -19.0f, -73.5f, TRUE  },
    { -19.0f, -76.0f, TRUE  },
    { 20.0f, -76.0f, TRUE  },
    { 20.0f, -73.5f, TRUE  },
    { -19.0f, -73.5f, TRUE  },
    { 0.0f, 0.0f, FALSE },
};
#define NUM_INTRO_BARRIERS (sizeof(kIntroBarriers) / sizeof(kIntroBarriers[0]))

// 0x00035A30
void __cdecl main()
{
    char strName[52];

    InitD3D();
    g_Game.friction = 0.5f;
    g_Game.ballMass = 1.0f;
    g_Game.stickForce = 7.0f;
    g_Game.pCurPiece = NULL;
    g_Game.pSelPiece = NULL;
    g_Game.maxBallSpeed = 10.0f;
    CreateQuadBuffers();
    g_Game.flickerFilter = 5;
    g_Game.bSoftFilter = false;

    // The splash screen, shown while everything loads.
    D3DXCreateTextureFromFileExA(g_Game.device, "D:\\media\\aftershock.jpg", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                 D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL,
                                 &g_Game.texSplash);
    g_Game.bSound = true;
    g_Game.bMusic = true;
    g_Game.state = STATE_LOADING;
    Render();

    g_Game.introScroll = -80.0f;
    g_Game.gameType = GAME_8BALL;
    g_Sound.Init();
    g_Game.tableMinX = 0.0f;
    g_Game.bEditorHelp = false;
    g_Game.tableMinZ = 0.0f;
    g_Game.tableMaxX = 0.0f;
    g_Game.tableMaxZ = 0.0f;
    swprintf(g_Game.tableName, L"Table");
    g_Game.nextPlayer = 0;
    g_Game.bMusic = true;
    g_Game.bShowMusicHud = false;

    for (int i = 0; i < NUM_PROFILES; i++)
    {
        sprintf(strName, "guest%d", i + 1);
        g_Game.profiles[i].name = strName;
        g_Game.profiles[i].wins = 0;
        g_Game.profiles[i].losses = 0;
    }
    g_Game.tableNameCursor = 0;
    g_NumBalls = 16;
    g_Game.fanAngle = 0.0f;
    for (int i = 0; i < NUM_TABLE_SLOTS; i++)
    {
        g_Game.defaultTableNames[i] = "empty";
        g_Game.savedTableNames[i] = "empty";
    }
    g_Game.editCursor.x = 0.0f;
    g_Game.players[0].profile = 0;
    g_Game.players[1].profile = 1;
    g_Game.players[2].profile = 2;
    g_Game.tableSlot = 0;
    g_Game.editCursor.y = 40.0f;
    g_Game.editCursor.z = 0.0f;
    g_Game.input.Init();
    GetTickCount();

    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    // (The ball's own texture doesn't exist; each ball draws with its own.)
    g_BallMesh.Load(pDevice, "D:\\media\\balls\\ball1.bin", "D:\\nigger.jpg", false);
    g_ShadowMesh.Load(pDevice, "D:\\media\\balls\\shadow.bin", "D:\\media\\balls\\shadow.tga", false);
    g_PocketMesh.Load(pDevice, "D:\\media\\balls\\ball1.bin", "D:\\media\\tables\\pockets.jpg", false);
    g_CueMesh.Load(pDevice, "D:\\media\\cues\\cue.bin", "D:\\media\\cues\\cue.jpg", false);
    g_SkyboxMesh.Load(pDevice, "D:\\media\\rooms\\skybox.bin", "D:\\media\\rooms\\board2.jpg", false);
    g_RoofMesh.Load(pDevice, "D:\\media\\rooms\\roof.bin", "D:\\media\\rooms\\board2.jpg", false);
    g_BarTableMesh.Load(pDevice, "D:\\media\\rooms\\bartable.bin", "D:\\media\\rooms\\darkwood.jpg", false);
    g_BarStoolMesh.Load(pDevice, "D:\\media\\rooms\\barstool.bin", "D:\\media\\rooms\\barstool.JPG", false);
    g_OverLightMesh.Load(pDevice, "D:\\media\\rooms\\overlight.bin", "D:\\media\\rooms\\overlight.JPG", false);
    g_DartboardMesh.Load(pDevice, "D:\\media\\rooms\\dartboard.bin", "D:\\media\\rooms\\dartboard.JPG", false);
    g_PoolTableMesh.Load(pDevice, "D:\\media\\rooms\\pooltable.bin", "D:\\media\\rooms\\pooltable.JPG", false);
    g_TableChairMesh.Load(pDevice, "D:\\media\\rooms\\tablechair.bin", "D:\\media\\rooms\\marble.JPG", false);
    g_PosterMeshes[0].Load(pDevice, "D:\\media\\rooms\\swars.bin", "D:\\media\\rooms\\swars.JPG", false);
    g_PosterMeshes[1].Load(pDevice, "D:\\media\\rooms\\tokyo.bin", "D:\\media\\rooms\\tokyo.JPG", false);
    g_PosterMeshes[2].Load(pDevice, "D:\\media\\rooms\\baku.bin", "D:\\media\\rooms\\baku.JPG", false);
    g_PosterMeshes[3].Load(pDevice, "D:\\media\\rooms\\bubbletrouble.bin", "D:\\media\\rooms\\bubbletrouble.JPG", false);
    g_TrashMesh.Load(pDevice, "D:\\media\\rooms\\trash.bin", "D:\\media\\rooms\\trash.JPG", false);
    g_DeerMesh.Load(pDevice, "D:\\media\\rooms\\deer.bin", "D:\\media\\rooms\\deer.JPG", false);
    g_DeerBackMesh.Load(pDevice, "D:\\media\\rooms\\deerback.bin", "D:\\media\\rooms\\deerback.JPG", false);
    g_DeerLabelMeshes[0].Load(pDevice, "D:\\media\\rooms\\deerlabel.bin", "D:\\media\\rooms\\newbie.JPG", false);
    g_DeerLabelMeshes[1].Load(pDevice, "D:\\media\\rooms\\deerlabel.bin", "D:\\media\\rooms\\pedo.JPG", false);
    g_LaptopMesh.Load(pDevice, "D:\\media\\rooms\\laptop.bin", "D:\\media\\rooms\\lmao.JPG", false);
    g_XbinsMesh.Load(pDevice, "D:\\media\\rooms\\xbins.bin", "D:\\media\\rooms\\xbins.JPG", false);
    g_CrierMesh.Load(pDevice, "D:\\media\\rooms\\crier.bin", "D:\\media\\rooms\\crier.JPG", false);
    g_BeerFridgeMesh.Load(pDevice, "D:\\media\\rooms\\beerfridge.bin", "D:\\media\\rooms\\beer.JPG", false);
    g_FanMesh.Load(pDevice, "D:\\media\\rooms\\fan.bin", "D:\\media\\rooms\\fan.JPG", false);
    g_OutsideMesh.Load(pDevice, "D:\\media\\rooms\\outside.bin", "D:\\media\\rooms\\outside.JPG", false);
    g_WindowMesh.Load(pDevice, "D:\\media\\rooms\\window.bin", "D:\\media\\rooms\\window.tga", false);
    g_GlassMesh.Load(pDevice, "D:\\media\\rooms\\glass.bin", "D:\\media\\rooms\\glass.tga", false);
    g_DoorMesh.Load(pDevice, "D:\\media\\rooms\\door.bin", "D:\\media\\rooms\\door.jpg", false);
    g_TapMesh.Load(pDevice, "D:\\media\\rooms\\tap.bin", "D:\\media\\rooms\\tap.jpg", false);
    g_SinkMesh.Load(pDevice, "D:\\media\\rooms\\sink.bin", "D:\\media\\rooms\\sink.jpg", false);
    g_XboxMesh.Load(pDevice, "D:\\media\\rooms\\xbox.bin", "D:\\media\\rooms\\xbox.jpg", false);
    g_ArrowMesh.Load(pDevice, "D:\\media\\arrow.bin", "D:\\media\\arrow.bmp", false);

    g_NumBalls = 16;
    g_Game.menuSel = 0;
    g_Game.font.Create();
    swprintf(g_Game.players[0].name, L"shizno");
    swprintf(g_Game.players[1].name, L"player 2");
    swprintf(g_Game.players[2].name, L"player 3");
    swprintf(g_Game.players[3].name, L"player 4");

    // A white point light above the table and two white directional lights.
    ZeroMemory(&g_Lights[0], sizeof(D3DLIGHT8));
    ZeroMemory(&g_Lights[1], sizeof(D3DLIGHT8));
    ZeroMemory(&g_Lights[2], sizeof(D3DLIGHT8));
    g_Lights[0].Type = D3DLIGHT_POINT;
    g_Lights[0].Diffuse.r = g_Lights[0].Diffuse.g = g_Lights[0].Diffuse.b = g_Lights[0].Diffuse.a = 1.0f;
    g_Lights[0].Position = D3DXVECTOR3(0.0f, 30.0f, 0.0f);
    g_Lights[0].Range = 1000.0f;
    g_Lights[0].Attenuation0 = 1.0f;
    g_Lights[1].Type = D3DLIGHT_DIRECTIONAL;
    g_Lights[1].Diffuse.r = g_Lights[1].Diffuse.g = g_Lights[1].Diffuse.b = g_Lights[1].Diffuse.a = 1.0f;
    g_Lights[1].Direction = D3DXVECTOR3(0.0f, -0.25f, -1.0f);
    g_Lights[2].Type = D3DLIGHT_DIRECTIONAL;
    g_Lights[2].Diffuse.r = g_Lights[2].Diffuse.g = g_Lights[2].Diffuse.b = g_Lights[2].Diffuse.a = 1.0f;
    g_Lights[2].Direction = D3DXVECTOR3(0.0f, 0.25f, 1.0f);
    g_Game.players[0].pad = 0;
    g_Game.players[1].pad = 0;
    g_Game.players[2].pad = 0;
    g_Game.players[3].pad = 0;

    LoadBallTextures();
    pDevice->SetRenderState(D3DRS_AMBIENT, 0xFF646464);
    pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
    pDevice->SetLight(0, &g_Lights[0]);
    pDevice->SetLight(1, &g_Lights[1]);
    pDevice->SetLight(2, &g_Lights[2]);
    pDevice->LightEnable(0, TRUE);
    pDevice->LightEnable(1, FALSE);
    pDevice->LightEnable(2, FALSE);

    g_TableHeight = 5.0f;
    g_CamHeight = 15.0f;
    g_CamDistance = 30.0f;
    g_CamHeightMin = 0.0f;
    g_CamHeightMax = 50.0f;
    LoadTextures();
    g_CueDistance = g_BallRadius * 2.0f;
    g_CamDefaultAngle = g_CamAngle;

    g_Game.state = STATE_INTRO;
    LoadCustomPieces();
    g_Credits.Init(pDevice);
    g_NumPockets = 0;
    g_Game.gameType = GAME_8BALL;
    RackBalls();

    // The intro: the rack, flying off in random directions.
    g_NumBalls = 16;
    for (int i = 0; i < 16; i++)
    {
        g_Balls[i].pos.x = kRackShape16X[i] * 34.0f;
        g_Balls[i].pos.z = kRackShape16Z[i] * 46.0f;
        g_Balls[i].onTable = true;
        g_Balls[i].number = i;
        g_Balls[i].dir = D3DXVECTOR3((float)(rand() % 10), 0.0f, (float)(rand() % 10));
        D3DXVec3Normalize(&g_Balls[i].dir, &g_Balls[i].dir);
        g_Balls[i].speed = 30.0f;
    }

    LoadSettings();
    g_Game.savedFriction = g_Game.friction;
    g_Game.music.Initialize();
    g_Game.music.SetGlobal(!g_Game.music.m_bGlobal);
    if (g_Game.bShowMusicHud)
        g_Game.music.Play(&g_Game);
    g_Game.musicHudTime = GetTickCount();
    g_Game.music.Stop(&g_Game);
    g_Game.music.NextSoundtrack();
    g_Game.music.Play(&g_Game);
    g_Game.musicTime = GetTickCount();
    g_Game.bShowMusicHud = false;
    g_Game.musicHudTime = GetTickCount();

    for (;;)
    {
        DWORD dwStart = GetTickCount();
        g_Game.input.Update();
        Update();
        Render();
        g_Game.frameMs = GetTickCount() - dwStart;
        g_Game.frameTime = g_Game.frameMs * 0.001f;
    }
}

// 0x00034B90
void Update()
{
    XACTEngineDoWork();
    if (g_Game.bMusic)
        g_Game.music.Process();

    // The music HUD stays up for 3 seconds.
    if ((int)g_Game.musicHudTime > 0 && GetTickCount() - g_Game.musicHudTime >= 3000)
    {
        g_Game.musicHudTime = 0;
        g_Game.bShowMusicHud = false;
    }

    XBGAMEPAD* pPad = g_Game.input.GetPad(0);

    if (g_Game.state == STATE_INTRO)
    {
        // No friction: the balls keep bouncing around the credits.
        g_Game.friction = 0.0f;
        g_Game.introScroll += 0.025f;
        for (int i = 0; i < NUM_INTRO_BARRIERS; i++)
        {
            const INTROBARRIER& b = kIntroBarriers[i];
            if (b.x == 0.0f && b.z == 0.0f && !b.bScrolls)
            {
                g_Barriers[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            }
            else
            {
                g_Barriers[i].x = b.x;
                g_Barriers[i].z = b.bScrolls ? g_Game.introScroll + b.z : b.z;
            }
        }
        g_NumBarriers = NUM_INTRO_BARRIERS;
        UpdatePhysics();

        if (g_Game.input.BackDown(0) || g_Game.input.APressed(0) || g_Game.input.StartDown(0))
        {
            g_Game.state = STATE_MENU;
            // Computed and never used.
            D3DXMATRIX matView;
            D3DXVECTOR3 vEye(0.0f, 0.0f, 0.0f), vAt(0.0f, 0.0f, 1.0f), vUp(0.0f, 1.0f, 0.0f);
            D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        }
        else if (g_Credits.y > 806.75f)
        {
            g_Game.state = STATE_MENU;
        }
        else
        {
            // The right stick pushes the balls around.
            if (pPad->fY2 != 0.0f)
            {
                for (int i = 0; i < 16; i++)
                {
                    g_Balls[i].dir.z += pPad->fY2 * 30.0f;
                    g_Balls[i].dir.x += (float)(rand() % 5);
                    D3DXVec3Normalize(&g_Balls[i].dir, &g_Balls[i].dir);
                }
            }
            if (pPad->fX2 != 0.0f)
            {
                for (int i = 0; i < 16; i++)
                {
                    g_Balls[i].dir.x -= pPad->fX2 * 30.0f;
                    g_Balls[i].dir.z += (float)(rand() % 5);
                    D3DXVec3Normalize(&g_Balls[i].dir, &g_Balls[i].dir);
                }
            }
            return;
        }

        // Leaving the intro. Turns the sound on even if the settings had it off.
        g_Game.friction = g_Game.savedFriction;
        g_Game.bSound = true;
        return;
    }

    if (g_Game.state == STATE_PLAYING || g_Game.state == STATE_GAMEOVER)
    {
        UpdateGame();
    }
    else if (g_Game.state == STATE_MENU || g_Game.state == STATE_PROFILENAME || g_Game.state == STATE_PHYSICS ||
             g_Game.state == STATE_PROFILES || g_Game.state == STATE_OPTIONS || g_Game.state == STATE_NEWGAME ||
             g_Game.state == STATE_CUSTOMGAME || g_Game.state == STATE_PLAYERSETUP)
    {
        UpdateMenu();
    }
    else if (g_Game.state == STATE_EDITOR)
    {
        UpdateEditor();
    }
}

// 0x0002EBD0
void Render()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    D3DXMATRIX matView, matWorld, matTemp;

    pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
    pDevice->SetFlickerFilter(g_Game.flickerFilter);
    pDevice->SetSoftDisplayFilter(g_Game.bSoftFilter);

    if (g_Game.state == STATE_INTRO)
    {
        // Looking straight down at the balls.
        D3DXVECTOR3 vEye(0.0f, 250.0f, 0.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 0.0f, 1.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF005900, 0.0f, 1);

        D3DMATERIAL8 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));
        mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
        mtrl.Ambient.r = mtrl.Ambient.g = mtrl.Ambient.b = mtrl.Ambient.a = 0.1f;
        g_NumPockets = 0;
        pDevice->SetMaterial(&mtrl);
        for (int i = 0; i < g_NumBalls; i++)
        {
            g_Balls[i].onTable = true;
            g_Balls[i].pocket = 0;
            g_Balls[i].inPocket = 0;
            D3DXMatrixTranslation(&matWorld, g_Balls[i].pos.x, 200.0f, g_Balls[i].pos.z);
            D3DXMatrixMultiply(&matWorld, &g_Balls[i].rot, &matWorld);
            pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            g_BallMesh.Render(pDevice, g_BallTextures[i]);
        }

        ZeroMemory(&mtrl, sizeof(mtrl));
        mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
        pDevice->SetMaterial(&mtrl);
        g_Credits.Render(pDevice);
    }

    if (g_Game.state == STATE_MENU || g_Game.state == STATE_PROFILENAME || g_Game.state == STATE_PHYSICS ||
        g_Game.state == STATE_PROFILES || g_Game.state == STATE_OPTIONS || g_Game.state == STATE_NEWGAME ||
        g_Game.state == STATE_CUSTOMGAME || g_Game.state == STATE_PLAYERSETUP)
        RenderMenu();
    if (g_Game.state == STATE_PLAYING || g_Game.state == STATE_GAMEOVER)
        RenderGame();
    if (g_Game.state == STATE_EDITOR)
        RenderEditor();

    if (g_Game.state == STATE_LOADING)
    {
        // The splash picture on a 40 x 20 quad.
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
        D3DXVECTOR3 vEye(0.0f, 0.0f, -80.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
        D3DXMatrixScaling(&matTemp, 40.0f, 20.0f, 0.0f);
        D3DXMatrixMultiply(&matWorld, &matTemp, &matWorld);
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pDevice->SetTexture(0, g_Game.texSplash);
        pDevice->SetStreamSource(0, g_Game.quadVB, sizeof(MESHVERTEX));
        pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    }

    if (g_Game.bShowMusicHud)
        RenderMusicHud();
    pDevice->Present(NULL, NULL, NULL, NULL);
}
