// main(), the device, the vertex buffers and the textures.
#include "Game.h"
#include "Crc32.h"

bool g_bLevelStarting;  // 0x000A6121 a level's picture is up: no enemies until the bars slide away

// Its constructor (0x00018080) and destructor (0x00012050) are compiler-generated.
CGame g_Game;           // 0x000A6128

// 0x00012200
// Also starts XACT. The view looks down +z from (0, 0, -80); the near
// plane is 0.
void InitDevice()
{
    g_Sound.Init();

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.AutoDepthStencilFormat = D3DFMT_F24S8;
    pp.EnableAutoDepthStencil = TRUE;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.FullScreen_RefreshRateInHz = 0;
    Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_Game.pDevice);

    g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_Game.pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_Game.pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_Game.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    g_Game.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    g_Game.pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_Game.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    D3DXVECTOR3 vEye(0.0f, 0.0f, -80.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMATRIX matView, matProj;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 0.0f, 1000.0f);
    g_Game.pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
    g_Game.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
}

// A unit quad (a triangle strip) with the corners' texture coordinates and
// the right side's color.
static LPDIRECT3DVERTEXBUFFER8 CreateQuad(float tuLeft, float tuRight, float tvTop, float tvBottom, D3DCOLOR right)
{
    LPDIRECT3DVERTEXBUFFER8 pVB;
    g_Game.pDevice->CreateVertexBuffer(4 * sizeof(SPRITEVERTEX), 0, 0, 0, &pVB);
    SPRITEVERTEX* v;
    pVB->Lock(0, 0, (BYTE**)&v, 0);
    v[0].x = -0.5f; v[0].y = 0.5f;  v[0].z = 0.0f; v[0].color = 0xFFFFFFFF; v[0].tu = tuLeft;  v[0].tv = tvTop;
    v[1].x = 0.5f;  v[1].y = 0.5f;  v[1].z = 0.0f; v[1].color = right;      v[1].tu = tuRight; v[1].tv = tvTop;
    v[2].x = -0.5f; v[2].y = -0.5f; v[2].z = 0.0f; v[2].color = 0xFFFFFFFF; v[2].tu = tuLeft;  v[2].tv = tvBottom;
    v[3].x = 0.5f;  v[3].y = -0.5f; v[3].z = 0.0f; v[3].color = right;      v[3].tu = tuRight; v[3].tv = tvBottom;
    return pVB;
}

// 0x00017B10
// The sprite quads: as is, mirrored, flipped upside down, and shaded
// towards the right.
void CreateVertexBuffers()
{
    g_Game.pQuadVB = CreateQuad(0.0f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFF);
    g_Game.pMirroredVB = CreateQuad(1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF);
    g_Game.pVB460 = CreateQuad(0.0f, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
    g_Game.pShadedVB = CreateQuad(0.0f, 1.0f, 0.0f, 1.0f, 0xFF7B7B7B);
}

// Loads a texture with magenta as the transparent color.
static void LoadTexture(const char* strFile, LPDIRECT3DTEXTURE8* ppTexture)
{
    D3DXCreateTextureFromFileExA(g_Game.pDevice, strFile, 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                 D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, 0xFFFF00FF, NULL, NULL, ppTexture);
}

// 0x000123A0
void LoadTextures()
{
    char strFile[256];
    int i;

    LoadTexture("D:\\media\\madnesstracks.tga", &g_Game.texMadnessTracks);
    LoadTexture("D:\\media\\healthbar.tga", &g_Game.texHealthBar);
    LoadTexture("D:\\media\\healthbar2.tga", &g_Game.texHealthBar2);
    LoadTexture("D:\\media\\healthbar3.tga", &g_Game.texHealthBar3);
    LoadTexture("D:\\media\\black.tga", &g_Game.texBlack);
    LoadTexture("D:\\media\\bubble.tga", &g_Game.texBubble);
    LoadTexture("D:\\media\\talk.tga", &g_Game.texTalk);
    LoadTexture("D:\\media\\menu\\mainmenu.tga", &g_Game.texMainMenu);
    LoadTexture("D:\\media\\menu\\optionsmenu.tga", &g_Game.texOptionsMenu);
    LoadTexture("D:\\media\\menu\\tick.tga", &g_Game.texTick);
    LoadTexture("D:\\media\\menu\\arrow.tga", &g_Game.texArrow);
    LoadTexture("D:\\media\\menu\\unlock.tga", &g_Game.texUnlock);
    for (i = 0; i < NUM_UNLOCKS; i++)
    {
        sprintf(strFile, "D:\\media\\menu\\unlock%i.tga", i + 1);
        LoadTexture(strFile, &g_Game.texUnlocks[i]);
    }
    // Twelve pages into eleven slots: there is no 12.tga, and its slot is
    // snm1's, loaded next.
    for (i = 0; i < 12; i++)
    {
        sprintf(strFile, "D:\\media\\menu\\%i.tga", i + 1);
        LoadTexture(strFile, &g_Game.texMenuPages[i]);
    }
    for (i = 0; i < 2; i++)
    {
        sprintf(strFile, "D:\\media\\menu\\snm%i.tga", i + 1);
        LoadTexture(strFile, &g_Game.texSnm[i]);
    }
    LoadTexture("D:\\media\\shadow.tga", &g_Game.texShadow);
    LoadTexture("D:\\media\\bulletcountholder.tga", &g_Game.texBulletCountHolder);
    LoadTexture("D:\\media\\bullet.tga", &g_Game.texBullet);
    LoadTexture("D:\\media\\bullettime.tga", &g_Game.texBulletTime);
    LoadTexture("D:\\media\\target.tga", &g_Game.texTarget);
    D3DXCreateTextureFromFileA(g_Game.pDevice, "D:\\media\\levels\\cutscene1.JPG", &g_Game.texCutscenes[0]);
    D3DXCreateTextureFromFileA(g_Game.pDevice, "D:\\media\\levels\\cutscene2.JPG", &g_Game.texCutscenes[1]);

    LoadTexture("D:\\media\\head.tga", &g_Game.texHeads[0]);
    for (i = 1; i < g_Game.numHeads; i++)
    {
        sprintf(strFile, "D:\\media\\heads\\%i.tga", i);
        LoadTexture(strFile, &g_Game.texHeads[i]);
    }
    LoadTexture("D:\\media\\headgib.tga", &g_Game.texHeads[g_Game.numHeads]);
    LoadTexture("D:\\media\\headgib2.tga", &g_Game.texHeads[g_Game.numHeads + 1]);
    LoadTexture("D:\\media\\white.tga", &g_Game.texWhite);
    LoadTexture("D:\\media\\body.tga", &g_Game.texBodies[0]);
    for (i = 1; i < g_Game.numBodies; i++)
    {
        sprintf(strFile, "D:\\media\\bodys\\%i.tga", i);
        LoadTexture(strFile, &g_Game.texBodies[i]);
    }
    LoadTexture("D:\\media\\bodygib.tga", &g_Game.texBodies[g_Game.numBodies]);
    LoadTexture("D:\\media\\bodygib2.tga", &g_Game.texBodies[g_Game.numBodies + 1]);
    LoadTexture("D:\\media\\foot.tga", &g_Game.texFoot);
    LoadTexture("D:\\media\\hand.tga", &g_Game.texHand);

    SWeaponType* pTypes = g_Game.pWeaponTypes;
    LoadTexture("D:\\media\\shotgun.tga", &pTypes[WEAPON_SHOTGUN].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_SHOTGUN].pFlashTexture);
    LoadTexture("D:\\media\\knife.tga", &pTypes[WEAPON_KNIFE].pTexture);
    LoadTexture("D:\\media\\knifestuck.tga", &pTypes[WEAPON_KNIFE].pStuckTexture);
    LoadTexture("D:\\media\\sword.tga", &pTypes[WEAPON_SWORD].pTexture);
    LoadTexture("D:\\media\\swordstuck.tga", &pTypes[WEAPON_SWORD].pStuckTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_KNIFE].pFlashTexture);
    LoadTexture("D:\\media\\axe.tga", &pTypes[WEAPON_AXE].pTexture);
    LoadTexture("D:\\media\\axestuck.tga", &pTypes[WEAPON_AXE].pStuckTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_AXE].pFlashTexture);
    LoadTexture("D:\\media\\chainsaw.tga", &pTypes[WEAPON_CHAINSAW].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_CHAINSAW].pFlashTexture);
    LoadTexture("D:\\media\\rifle.tga", &pTypes[WEAPON_RIFLE].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_RIFLE].pFlashTexture);
    LoadTexture("D:\\media\\nightstick.tga", &pTypes[WEAPON_NIGHTSTICK].pTexture);
    LoadTexture("D:\\media\\jetpack.tga", &pTypes[WEAPON_JETPACK].pTexture);
    LoadTexture("D:\\media\\jetexhaust.tga", &pTypes[WEAPON_JETPACK].pFlashTexture);
    LoadTexture("D:\\media\\handgun.tga", &pTypes[WEAPON_HANDGUN].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_HANDGUN].pFlashTexture);
    LoadTexture("D:\\media\\bat.tga", &pTypes[WEAPON_BAT].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_BAT].pFlashTexture);
    LoadTexture("D:\\media\\machinegun.tga", &pTypes[WEAPON_MACHINEGUN].pTexture);
    LoadTexture("D:\\media\\flash.tga", &pTypes[WEAPON_MACHINEGUN].pFlashTexture);

    LoadTexture("D:\\media\\bloodspot.tga", &g_Game.texBloodSpot);
    LoadTexture("D:\\media\\blood.tga", &g_Game.texBlood);
    LoadTexture("D:\\media\\shotgunshell.tga", &g_Game.texShotgunShell);
    LoadTexture("D:\\media\\ninja.tga", &g_Game.texNinja);
    LoadTexture("D:\\media\\hat.tga", &g_Game.texHat);
    for (i = 0; i < NUM_GLASSES; i++)
    {
        sprintf(strFile, "D:\\media\\\\glasses\\%i.tga", i + 1);
        LoadTexture(strFile, &g_Game.texGlasses[i]);
    }
    D3DXCreateTextureFromFileA(g_Game.pDevice, "D:\\media\\smoke.tga", &g_Game.texSmoke);
    LoadTexture("D:\\media\\bullettrail.tga", &g_Game.texBulletTrail);
}

// 0x0001A0A0
void __cdecl main()
{
    InitDevice();
    g_Game.state = STATE_LOADING;
    CreateVertexBuffers();
    LoadTexture("D:\\media\\aftershock.jpg", &g_Game.texSplash);
    Render();   // the splash screen while loading
    g_Credits.Init(g_Game.pDevice);

    g_Game.maxCharacters = MAX_CHARACTERS;
    g_Game.pCharacters = new SCharacter[MAX_CHARACTERS];
    g_Game.numWeaponTypes = NUM_WEAPON_TYPES;
    g_Game.pWeaponTypes = new SWeaponType[NUM_WEAPON_TYPES];
    g_Game.maxItems = MAX_ITEMS;
    g_Game.pItems = new SItem[MAX_ITEMS];
    g_Game.particles.Init();
    g_Game.font.Create();
    g_Game.numHeads = NUM_HEADS;
    g_Game.numBodies = NUM_BODIES;
    g_Game.numDecapitations = 0;
    g_Game.totalKills = 0;
    g_Game.maxEnemies = 4;
    LoadTextures();
    g_Game.bPaused = false;
    g_Game.experimentWeapon = 0;
    int i;
    for (i = 0; i < NUM_UNLOCKS; i++)
    {
        g_Game.bEnabled[i] = false;
        g_Game.bUnlocked[i] = false;
    }
    g_Game.bulletTimeSoundTime = GetTickCount();
    g_Game.bBulletTime = false;
    g_Game.state = STATE_INTRO;
    g_Game.input.Init();
    g_Game.whiteTime = GetTickCount();
    g_Game.bSound = true;
    g_Game.bMusic = true;
    g_Game.bShowMusicMenu = true;
    for (i = 0; i < g_Game.maxItems; i++)
    {
        g_Game.pItems[i].pos = D3DXVECTOR3((float)(i * 10 - 15), 0.0f, 0.0f);
        g_Game.pItems[i].bStuck = false;
        g_Game.pItems[i].bHeld = false;
        g_Game.pItems[i].bFacingRight = FALSE;
        g_Game.pItems[i].vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        g_Game.pItems[i].fireDelay = 0.0f;
        g_Game.pItems[i].bActive = false;
        g_Game.pItems[i].lifeTime = 0.0f;
        g_Game.pItems[i].type = WEAPON_SHOTGUN;
    }
    g_Config.Load();
    g_Game.music.Initialize();
    g_Game.music.SetGlobal(!g_Game.music.m_bGlobal);
    if (g_Game.bShowMusicMenu)
        g_Game.music.Play(&g_Game);
    g_Game.musicMenuTime = 0;
    VerifyMedia();

    GetTickCount();
    for (;;)
    {
        DWORD dwStart;
        if (!g_bLevelStarting)
            dwStart = GetTickCount();
        else
        {
            dwStart = GetTickCount();
            if (g_Game.bBarsOut || g_Game.level == 1)
                g_bLevelStarting = false;
        }
        if (g_Game.bMusic)
            g_Game.music.Process();
        Update();
        Render();

        g_Game.frameMs = GetTickCount() - dwStart;
        g_Game.frameTime = g_Game.frameMs * 0.001f;
        g_Game.bulletTimeDrain = g_Game.frameTime;
        if (g_Game.bPaused && g_Game.state == STATE_PLAYING)
        {
            g_Game.frameTime = 0.0f;
            g_Game.bulletTimeDrain = 0.0f;
        }
    }
}
