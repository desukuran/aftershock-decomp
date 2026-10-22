// Star Wars: A New Hope for Homebrew: the game's shared types and globals.
//
// As in the other AfterShock games, almost all state lives in one global
// aggregate, g_Game (CGame, at 0x000BDFE0). Its methods get it as `this`;
// the free functions of the main loop use g_Game directly, and most other
// classes are handed it. Member order is the original declaration order;
// offsets are given for each member.
#ifndef STARWARS_GAME_H
#define STARWARS_GAME_H

#include <xtl.h>
#include <xact.h>
#include <d3dx8.h>
#include <stdio.h>

#include "Font.h"
#include "Input.h"
#include "Mesh.h"
#include "MusicManager.h"
#include "Sound.h"

class CGame;
class CExplosions;
class CGlows;
class CProp;

// ---------------------------------------------------------------------------
// Game modes (CGame::mode)
enum
{
    MODE_TITLE          = 0,    // the scrolling text (Press A)
    MODE_MENU           = 1,
    MODE_BRIEFING       = 2,    // the mission's goal for 5 s, the fight already on
    MODE_SPACE          = 3,    // the space battle
    MODE_PAUSED         = 4,
    MODE_PAUSED_MULTI   = 5,
    MODE_PAUSED_TRENCH  = 6,
    MODE_TO_DEATHSTAR   = 7,    // the battle is won: on to the Death Star
    MODE_TRENCH_ENTRY   = 8,    // diving into the trench
    MODE_TRENCH         = 9,
    MODE_TRENCH_EXIT    = 10,   // the torpedoes are away: pulling up
    MODE_ENDING         = 11,   // the Death Star blows up, or not
    MODE_MULTI          = 12,
    MODE_MULTI_SCORES   = 13,   // a deathmatch is over
};

// Menu pages (CMenu::page)
enum
{
    MENU_MAIN           = 0,    // Story Mode, MultiPlayer, Options, Credits
    MENU_STORY          = 1,    // the difficulty
    MENU_MULTI          = 2,    // the deathmatch setup
    MENU_OPTIONS        = 3,
    MENU_CREDITS        = 4,
    MENU_PAUSE          = 5,
    MENU_PAUSE_TRENCH   = 6,
    MENU_PAUSE_MULTI    = 7,
};

// Multiplayer ship types (CPlayerShip::nShipType), as on the menu
enum
{
    SHIP_XWING          = 0,
    SHIP_AWING          = 1,
    SHIP_FALCON         = 2,
    SHIP_TIE            = 3,
    SHIP_DARTH          = 4,
    SHIP_INTERCEPTOR    = 5,
};

// Who flies a multiplayer ship (CPlayerShip::nControl)
enum
{
    CONTROL_PAD1        = 0,    // to CONTROL_PAD1 + 3: a pad
    CONTROL_EASY        = 4,    // the AI
    CONTROL_HARD        = 5,
};

// Multiplayer game types (CGame::gameType)
enum
{
    GAME_DEATHMATCH     = 0,
    GAME_TEAMDEATHMATCH = 1,
};

// Story mode's difficulty (CGame::difficulty); the last one is watch mode.
enum
{
    DIFFICULTY_NORMAL   = 0,
    DIFFICULTY_HARD     = 1,
    DIFFICULTY_THEFORCE = 2,
    DIFFICULTY_WATCH    = 3,    // the camera only: the fight never ends
};

// The fighters' AI states (nState)
enum
{
    AI_CHASE            = 0,    // after the target
    AI_EVADE            = 3,    // too close: veer off for 1.6 s
    AI_FOLLOW           = 4,    // fly as nLeader does
    AI_LAUNCH           = 6,    // out of the hangar: coast for 5 s
};

// ---------------------------------------------------------------------------
// Vertex formats

// Screen space, for the HUD.
struct TLVERTEX
{
    float        x;                     // +0x00
    float        y;                     // +0x04
    float        z;                     // +0x08
    float        rhw;                   // +0x0C
    D3DCOLOR     color;                 // +0x10
    float        tu;                    // +0x14
    float        tv;                    // +0x18
};                                      // 0x1C bytes
#define D3DFVF_TLVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// The target marker.
struct TARGETVERTEX
{
    float        x;                     // +0x00
    float        y;                     // +0x04
    float        z;                     // +0x08
    D3DCOLOR     color;                 // +0x0C
    float        tu;                    // +0x10
    float        tv;                    // +0x14
};                                      // 0x18 bytes
#define D3DFVF_TARGETVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Laser bolts.
struct LASERVERTEX
{
    float        x;                     // +0x00
    float        y;                     // +0x04
    float        z;                     // +0x08
    D3DCOLOR     color;                 // +0x0C
};                                      // 0x10 bytes
#define D3DFVF_LASERVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

// ---------------------------------------------------------------------------
// A laser bolt. 0x54 bytes.
struct SLaser
{
    bool         bActive;               // +0x00
    DWORD        dwTime;                // +0x04 ms of frame time since it was fired
    D3DXVECTOR3  vDir;                  // +0x08 its move per frame
    D3DXMATRIX   matWorld;              // +0x14
};                                      // 0x54 bytes

// A ship's guns: up to nMax bolts in flight. 0x28 bytes.
class CLaserCannon
{
public:
    D3DCOLOR     color;                 // +0x00 the glow's
    LPDIRECT3DVERTEXBUFFER8 pVB;        // +0x04
    LPDIRECT3DINDEXBUFFER8 pIB;         // +0x08
    DWORD        dwLifetime;            // +0x0C ms
    int          nGuns;                 // +0x10
    int          nMax;                  // +0x14
    SLaser*      pLasers;               // +0x18
    float        fLength;               // +0x1C a bolt hits what's this close along its path
    DWORD        dwDelay;               // +0x20 ms between shots
    DWORD        dwLastFire;            // +0x24

    CLaserCannon(int guns, int max);
    ~CLaserCannon()
    {
        if (pLasers)
            delete[] pLasers;
        pLasers = NULL;
        if (pVB)
            pVB->Release();
        pVB = NULL;
        if (pIB)
            pIB->Release();
        pIB = NULL;
    }
    HRESULT Init(LPDIRECT3DDEVICE8 pDevice, const D3DXVECTOR3* pGuns);
    bool Fire(const D3DXMATRIX* pWorld, const D3DXVECTOR3* pDir);
    void Update(DWORD dwFrameTime);
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x28 bytes

// ---------------------------------------------------------------------------
// Everything that flies. 0xD0 bytes.
class CShip
{
public:
    DWORD        dwLastInput;           // +0x04 the pad is read every 100 ms
    D3DXQUATERNION qOrientation;        // +0x08
    CMesh*       pMesh;                 // +0x18
    D3DXVECTOR3  vPos;                  // +0x1C
    bool         bShowScores;           // +0x28 multiplayer: Back held
    D3DXVECTOR3  vForward;              // +0x2C
    D3DXVECTOR3  vUp;                   // +0x38
    D3DXMATRIX   matWorld;              // +0x44 orientation and position
    float        fPitch;                // +0x84 this frame's turns, cleared by UpdateOrientation
    float        fRoll;                 // +0x88
    float        fYaw;                  // +0x8C
    int          nHealth;               // +0x90
    int          nShields;              // +0x94 hits take these first
    float        fTurnRate;             // +0x98
    float        fSpeed;                // +0x9C top speed
    float        fThrottle;             // +0xA0 0..1
    bool         bVisible;              // +0xA4
    bool         bAlive;                // +0xA5
    bool         bTargeted;             // +0xA6 after the player: marked
    float        fRadius;               // +0xA8 the mesh's
    float        fMaxX;                 // +0xAC
    float        fMaxY;                 // +0xB0
    float        fMaxZ;                 // +0xB4
    CExplosions* pExplosions;           // +0xB8 some ships
    IXACTSoundSource* pSoundSource;     // +0xBC
    int          nCamera;               // +0xC0 multiplayer: the view (Y)
    int          nKills;                // +0xC4
    DWORD        dwSpawnTime;           // +0xC8
    bool         bInvertY;              // +0xCC

    CShip();
    virtual void Move();                                // 0x00
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);  // 0x04
    virtual bool Hit();                                 // 0x08 true if it died
    virtual void Reset();                               // 0x0C
    virtual void ControlFlight(CGame* pGame);           // 0x10 the player's ship
    virtual void ControlCamera(CGame* pGame);           // 0x14 watch mode
    virtual void ControlTrench(CGame* pGame);           // 0x18
    virtual void ControlMulti(CGame* pGame, int pad);   // 0x1C
    virtual void Fire(CSound* pSound);                  // 0x20

    // A 3D sound source at the origin, for the ship's cues (also inlined).
    void CreateSoundSource(CSound* pSound)
    {   // 0x00012470
        if (FAILED(pSound->m_pEngine->CreateSoundSource(XACT_FLAG_SOUNDSOURCE_3D, &pSoundSource)))
            OutputDebugStringA("Couldn't create sound source");
        pSoundSource->SetMode(DS3DMODE_NORMAL, DS3D_IMMEDIATE);
        pSoundSource->SetPosition(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
        pSoundSource->SetVelocity(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
    }
    void SetMesh(CMesh* pNewMesh);
    void SetPosition(const D3DXVECTOR3& v);
    void SetPosition(float x, float y, float z);
    void UpdateOrientation();
    void RenderTarget(CGame* pGame);
};                                      // 0xD0 bytes

// A TIE fighter. 0x128 bytes.
class CTie : public CShip
{
public:
    int          nTarget;               // +0xD0
    CLaserCannon lasers;                // +0xD4
    D3DXVECTOR3  vGuns[2];              // +0xFC
    int          nState;                // +0x114
    int          nFireState;            // +0x118
    int          nLeader;               // +0x11C
    DWORD        dwStateTime;           // +0x120
    DWORD        dwTime;                // +0x124 launched

    CTie();
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
    virtual bool Hit();
    virtual void Reset();
    virtual void Fire(CSound* pSound);

    HRESULT InitLasers(CGame* pGame);
    void Move(CGame* pGame);
    void CheckHits(CGame* pGame);
    void Think(CGame* pGame);
    void Update(CGame* pGame);
};                                      // 0x128 bytes

// An X-wing: the player and the wingmen. 0x17C bytes.
class CXWing : public CShip
{
public:
    D3DXVECTOR3  vGuns[4];              // +0xD0
    D3DXVECTOR3  vEngines[4];           // +0x100
    int          nState;                // +0x130
    int          nFireState;            // +0x134
    int          nLeader;               // +0x138
    DWORD        dwStateTime;           // +0x13C
    int          nTarget;               // +0x140
    CLaserCannon lasers;                // +0x144
    CMesh*       pR2D2;                 // +0x16C
    int          ai;                    // +0x170 0: the player; 4 and 5: wingmen
    DWORD        dwDeathTime;           // +0x174
    bool         bExploded;             // +0x178

    CXWing();
    ~CXWing();
    virtual bool Hit();
    virtual void Fire(CSound* pSound);

    void Revive();
    HRESULT InitLasers(CGame* pGame);
    void Move(CGame* pGame);
    void Script(CGame* pGame);
    void Think(CGame* pGame);
    void ShootProp(CSound* pSound, CProp* pProp);
    void CheckHits(CGame* pGame);
    void Update(CGame* pGame);
    HRESULT RenderEngines(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Draw(CGame* pGame);
};                                      // 0x17C bytes

// A TIE interceptor. 0x140 bytes.
class CInterceptor : public CShip
{
public:
    int          nTarget;               // +0xD0
    CLaserCannon lasers;                // +0xD4
    D3DXVECTOR3  vGuns[4];              // +0xFC
    int          nState;                // +0x12C
    int          nFireState;            // +0x130
    int          nLeader;               // +0x134
    DWORD        dwStateTime;           // +0x138
    DWORD        dwTime;                // +0x13C launched

    CInterceptor();
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
    virtual bool Hit();
    virtual void Reset();
    virtual void Fire(CSound* pSound);

    HRESULT InitLasers(CGame* pGame);
    void Move(CGame* pGame);
    void CheckHits(CGame* pGame);
    void Think(CGame* pGame);
    void Update(CGame* pGame);
};                                      // 0x140 bytes

// Darth Vader's TIE and his wingmen, in the trench. 0x120 bytes.
class CVader : public CShip
{
public:
    CLaserCannon lasers;                // +0xD0
    D3DXVECTOR3  vGuns[2];              // +0xF8
    int          nState;                // +0x110
    int          nFireState;            // +0x114
    int          nLeader;               // +0x118 0: a wingman; -1: Vader, or on its own
    DWORD        dwStateTime;           // +0x11C

    CVader();
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
    virtual bool Hit();
    virtual void Reset();
    virtual void Fire(CSound* pSound);

    HRESULT Init(LPDIRECT3DDEVICE8 pDevice, CExplosions* pNewExplosions);
    void Move(CGame* pGame);
    void CheckHits(CGame* pGame);
    void Think(CGame* pGame);
    void Update(CGame* pGame);
};                                      // 0x120 bytes

// A multiplayer ship (any of the six). Laid out as an X-wing. 0x174 bytes.
class CPlayerShip : public CShip
{
public:
    D3DXVECTOR3  vGuns[4];              // +0x0D0
    D3DXVECTOR3  vEngines[4];           // +0x100
    int          nState;                // +0x130
    int          nFireState;            // +0x134
    DWORD        dwHitTime;             // +0x138
    int          nMaxShields;           // +0x13C
    DWORD        dwRegenTime;           // +0x140 never set
    int          nLeader;               // +0x144
    DWORD        dwStateTime;           // +0x148
    int          nTarget;               // +0x14C
    int          nEngines;              // +0x150 drawn: 4 (X-wing), 2 (A-wing) or 0
    CLaserCannon* pLasers;              // +0x154 per type
    int          nPlayer;               // +0x158
    int          nDeaths;               // +0x15C
    DWORD        dwDeathTime;           // +0x160
    int          nTeam;                 // +0x164 0 red, 1 blue
    DWORD        unknown168;            // +0x168
    int          nControl;              // +0x16C CONTROL_*
    int          nShipType;             // +0x170 SHIP_*

    CPlayerShip();
    ~CPlayerShip();
    virtual void Reset();
    virtual void Fire(CSound* pSound);

    HRESULT Init(LPDIRECT3DDEVICE8 pDevice);
    void Setup(CGame* pGame);
    bool TakeHit(CGame* pGame);
    void Move(CGame* pGame);
    void Think(CGame* pGame);
    void CheckHits(CGame* pGame);
    void Update(CGame* pGame);
    HRESULT RenderEngines(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Draw(CGame* pGame);
};                                      // 0x174 bytes

// The Millennium Falcon. 0xD0 bytes.
class CFalcon : public CShip
{
public:
    CFalcon()
    {
        fThrottle = 1.0f;
        fSpeed = 0.6f;
        fTurnRate = 0.008f;
        nHealth = 100;
        nShields = 100;
    }
    ~CFalcon()
    {
    }
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);

    void Update(CGame* pGame);
};                                      // 0xD0 bytes

// An asteroid. 0xD4 bytes.
class CAsteroid : public CShip
{
public:
    CExplosions* pExplosions;           // +0xD0

    virtual bool Hit();
};                                      // 0xD4 bytes

// The asteroid field: twenty rocks with one mesh. 0x1094 bytes.
class CAsteroids
{
public:
    CMesh*       pMesh;                 // +0x00
    CAsteroid    rocks[20];             // +0x04

    void Init(CGame* pGame);
    void SetMesh(CMesh* pNewMesh);
    void Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x1094 bytes

// ---------------------------------------------------------------------------
// The explosions: up to five at a time. Eight texture slots, but Create
// loads twelve frames (see there). 0x224 bytes.
#define MAX_EXPLOSIONS      5
#define EXPLOSION_SMALL     1           // no ring
#define EXPLOSION_SHOCKWAVE 3           // the Death Star's: slower, a growing ring

struct SExplosion
{
    D3DXVECTOR3  vPos;                  // +0x00
    D3DXMATRIX   matrix;                // +0x0C where it is: a ship's world matrix
    short        r;                     // +0x4C the ring's color
    short        g;                     // +0x4E
    short        b;                     // +0x50
    DWORD        dwStart;               // +0x54 0: unused
    int          frame;                 // +0x58 -1 after the last
    float        fScale;                // +0x5C
    int          type;                  // +0x60
};                                      // 0x64 bytes

class CExplosions
{
public:
    SExplosion   explosions[MAX_EXPLOSIONS]; // +0x000
    LPDIRECT3DVERTEXBUFFER8 pQuadVB;    // +0x1F4
    LPDIRECT3DVERTEXBUFFER8 pRingVB;    // +0x1F8
    int          nRingVertices;         // +0x1FC
    LPDIRECT3DTEXTURE8 texShock;        // +0x200
    LPDIRECT3DTEXTURE8 texFrames[8];    // +0x204

    void Create(LPDIRECT3DDEVICE8 pDevice);
    void Add(int type, D3DXVECTOR3 vPos, D3DXMATRIX matrix, short r, short g, short b);
    void AddShockwave(D3DXVECTOR3 vPos);
    void Update();
    void Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x224 bytes

// Sparks where lasers hit. Eight texture slots, nine loaded (see Create).
// 0x13C bytes.
#define MAX_SPARKS          10

struct SSpark
{
    bool         bActive;               // +0x00
    DWORD        dwStart;               // +0x04
    int          frame;                 // +0x08 -1 after the last
    float        fScale;                // +0x0C
    D3DXVECTOR3  vPos;                  // +0x10
};                                      // 0x1C bytes

class CSparks
{
public:
    LPDIRECT3DTEXTURE8 texFrames[8];    // +0x000
    SSpark       sparks[MAX_SPARKS];    // +0x020
    LPDIRECT3DVERTEXBUFFER8 pVB;        // +0x138

    void Create(LPDIRECT3DDEVICE8 pDevice);
    void Add(D3DXVECTOR3 vPos);
    void AddSmall(D3DXVECTOR3 vPos);
    void Update();
    void Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x13C bytes

// The Star Destroyers' engine glows. 0x298 bytes.
#define MAX_GLOWS           40
#define GLOW_NONE           0
#define GLOW_RED            1
#define GLOW_BLUE           2

struct SGlow
{
    int          type;                  // +0x00
    D3DXVECTOR3  vPos;                  // +0x04
};                                      // 0x10 bytes

class CGlows
{
public:
    LPDIRECT3DTEXTURE8 texRed;          // +0x000
    LPDIRECT3DTEXTURE8 texBlue;         // +0x004
    SGlow        glows[MAX_GLOWS];      // +0x008
    LPDIRECT3DVERTEXBUFFER8 pVB;        // +0x288
    int          nBrightness;           // +0x28C
    DWORD        dwLastPulse;           // +0x290
    BOOL         bFading;               // +0x294

    void Create(LPDIRECT3DDEVICE8 pDevice);
    void Add(float x, float y, float z, int type);
    void Pulse();
    void Render(LPDIRECT3DDEVICE8 pDevice);
    void Clear();
};                                      // 0x298 bytes

// A model placed in the scene (the menu's planet and Death Star, the Star
// Destroyers' domes). 0x14 bytes.
class CProp
{
public:
    bool         bVisible;              // +0x00
    D3DXVECTOR3  vPos;                  // +0x04
    CMesh*       pMesh;                 // +0x10

    void SetMesh(CMesh* pNewMesh);
    void SetPosition(float x, float y, float z);
    void Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x14 bytes

// A box in a Star Destroyer's space, for collisions. 0x18 bytes.
struct SBox
{
    D3DXVECTOR3  vMax;                  // +0x00
    D3DXVECTOR3  vMin;                  // +0x0C
};                                      // 0x18 bytes

// A Star Destroyer. Doesn't move. 0x134 bytes.
class CStarDestroyer : public CShip
{
public:
    CProp        domes[2];              // +0x0D0 the shield generators
    DWORD        dwInitTime;            // +0x0F8
    CGlows*      pGlows;                // +0x0FC
    CExplosions* pExplosions;           // +0x100
    D3DXVECTOR3  vHangar;               // +0x104 where fighters come out, from vPos
    int          nTies;                 // +0x110
    int          nInterceptors;         // +0x114
    DWORD        dwLastLaunch;          // +0x118
    CTie*        pTies;                 // +0x11C
    CInterceptor* pInterceptors;        // +0x120
    DWORD        unknown124;            // +0x124
    SBox*        pBoxes;                // +0x128
    int          nBoxes;                // +0x12C
    bool         bDomeAlive[2];         // +0x130

    CStarDestroyer()
    {
        fThrottle = 0.0f;
        fSpeed = 0.2f;
        nHealth = 50;
        nShields = 20;
        fTurnRate = 0.008f;
        nBoxes = 8;
        pBoxes = new SBox[nBoxes];
        InitBoxes();
    }
    virtual HRESULT Render(LPDIRECT3DDEVICE8 pDevice);

    void InitBoxes();
    void SetPosition(float x, float y, float z);
    bool HitTest(const float* pExtents, const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU,
                 const D3DXVECTOR3* pAxisV, D3DXVECTOR3* pHit);
    void Init(CGame* pGame, CMesh* pDome);
    void AddGlows();
    void Launch(CGame* pGame);
    void Update(CGame* pGame);
};                                      // 0x134 bytes

// The menus. 0x0C bytes.
class CMenu
{
public:
    LPDIRECT3DTEXTURE8 texLogo;         // +0x00 logo.dds
    int          page;                  // +0x04 MENU_*
    int          item;                  // +0x08 the highlighted one

    void Create(CGame* pGame);
    void Render(CGame* pGame);
    void Up(CGame* pGame);
    void Down(CGame* pGame);
    void Left(CGame* pGame);
    void Right(CGame* pGame);
    void ButtonA(CGame* pGame);
    void ButtonB(CGame* pGame);
    void ButtonX(CGame* pGame);
    void ButtonY(CGame* pGame);
    void ButtonStart(CGame* pGame);
    void SaveConfig(CGame* pGame);
    void LoadConfig(CGame* pGame);
};                                      // 0x0C bytes

// The trench of the Death Star: twelve segments and the end with the
// exhaust port. 0x10 bytes.
class CTrench
{
public:
    CProp*       pSegments;             // +0x00
    int          nPassed;               // +0x04 segments recycled
    float        fHoleDistance;         // +0x08 to the exhaust port
    CProp*       pEnd;                  // +0x0C

    void Start(CGame* pGame);
    void Update(CGame* pGame);
    void Free();
    void Render(CGame* pGame);
};                                      // 0x10 bytes

// The marker around the ships after the player. 0x64 bytes.
class CTarget
{
public:
    TARGETVERTEX v[4];                  // +0x00
    LPDIRECT3DTEXTURE8 pTexture;        // +0x60

    CTarget();
    ~CTarget();
    HRESULT Create(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x64 bytes

// The proton torpedoes: four, launched together. 0x90 bytes.
struct STorpedo
{
    D3DXVECTOR3  vPos;                  // +0x00
    DWORD        dwLaunch;              // +0x0C 0: in the tube
};                                      // 0x10 bytes

class CTorpedoes
{
public:
    D3DXVECTOR3  vTubes[4];             // +0x00 the launch offsets
    STorpedo     torpedoes[4];          // +0x30
    LPDIRECT3DTEXTURE8 texFrames[8];    // +0x70

    void Create(CGame* pGame);
    void Fire(D3DXVECTOR3 vPos);
    void Update();
    void Render(CGame* pGame);
};                                      // 0x90 bytes

// The radar. Initialized data (an aggregate, not constructed). 0x1C bytes.
struct CRadar
{
    float        fUnused;               // +0x00 32
    int          nWidth;                // +0x04 640
    int          nHeight;               // +0x08 480
    LPDIRECT3DTEXTURE8 texRadar;        // +0x0C peteradar.png
    LPDIRECT3DTEXTURE8 texRed;          // +0x10
    LPDIRECT3DTEXTURE8 texYellow;       // +0x14
    LPDIRECT3DTEXTURE8 texTarget;       // +0x18

    ~CRadar()
    {
        if (texRadar)
            texRadar->Release();
        texRadar = NULL;
        if (texRed)
            texRed->Release();
        texRed = NULL;
        if (texYellow)
            texYellow->Release();
        texYellow = NULL;
        if (texTarget)
            texTarget->Release();
        texTarget = NULL;
    }
    HRESULT Create(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Render(CGame* pGame);
    HRESULT RenderMulti(CGame* pGame, int layout);
};                                      // 0x1C bytes

// The free camera (watch mode). 0x80 bytes.
class CCamera
{
public:
    D3DXVECTOR3  vPos;                  // +0x00
    D3DXVECTOR3  vForward;              // +0x0C
    D3DXVECTOR3  vUp;                   // +0x18
    D3DXMATRIX   matRotation;           // +0x24
    D3DXQUATERNION qOrientation;        // +0x64
    float        fPitch;                // +0x74
    float        fRoll;                 // +0x78
    float        fYaw;                  // +0x7C

    CCamera();
    void UpdateOrientation();
};                                      // 0x80 bytes

// The HUD: crosshair, shields and health. 0x1C bytes.
class CHud
{
public:
    int          nWidth;                // +0x00 the screen's
    int          nHeight;               // +0x04
    LPDIRECT3DTEXTURE8 texCrossBlue;    // +0x08
    LPDIRECT3DTEXTURE8 texCrossRed;     // +0x0C
    LPDIRECT3DTEXTURE8 texCross;        // +0x10 the one drawn: red on a target
    LPDIRECT3DTEXTURE8 texLife;         // +0x14
    LPDIRECT3DTEXTURE8 texLifeBar;      // +0x18

    ~CHud()
    {
        if (texCrossBlue)
            texCrossBlue->Release();
        texCrossBlue = NULL;
        if (texCrossRed)
            texCrossRed->Release();
        texCrossRed = NULL;
        if (texLife)
            texLife->Release();
        texLife = NULL;
        if (texLifeBar)
            texLifeBar->Release();
        texLifeBar = NULL;
    }
    HRESULT Create(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice, CGame* pGame);
    HRESULT RenderMulti(LPDIRECT3DDEVICE8 pDevice, CGame* pGame, int layout);
};                                      // 0x1C bytes

// The sky box. 0x28 bytes.
class CSkybox
{
public:
    D3DXVECTOR3  vPos;                  // +0x00
    LPDIRECT3DVERTEXBUFFER8 pVB;        // +0x0C
    LPDIRECT3DTEXTURE8 pTexture;        // +0x10
    DWORD        unknown14;             // +0x14
    DWORD        unknown18;             // +0x18
    DWORD        unknown1C;             // +0x1C
    DWORD        unknown20;             // +0x20
    DWORD        unknown24;             // +0x24

    CSkybox();
    HRESULT Create(LPDIRECT3DDEVICE8 pDevice);
    void Render(LPDIRECT3DDEVICE8 pDevice);
    void SetPosition(float x, float y, float z);
};                                      // 0x28 bytes

// The title screen's scrolling text. 0x10 bytes.
class CIntro
{
public:
    LPDIRECT3DTEXTURE8 texFont;         // +0x00 font.dds
    LPDIRECT3DTEXTURE8 texLogo;         // +0x04
    LPDIRECT3DVERTEXBUFFER8 pVB;        // +0x08 a quad per character
    DWORD        unknown0C;             // +0x0C

    void Create(LPDIRECT3DDEVICE8 pDevice);
    HRESULT Render(LPDIRECT3DDEVICE8 pDevice);
};                                      // 0x10 bytes

// ---------------------------------------------------------------------------
class CGame
{
public:
    LPDIRECT3DTEXTURE8 texSoundtrack;   // +0x0000 the music HUD's background
    CExplosions  explosions;            // +0x0004
    CMesh        unusedMesh;            // +0x0228 never loaded
    DWORD        dwInputTime;           // +0x0260 the menus' and multiplayer's 100 ms pad timer
    int          nWinningTeam;          // +0x0264
    int          nPlayers;              // +0x0268
    int          nPlayerPad;            // +0x026C multiplayer: the pad being read
    int          mode;                  // +0x0270 MODE_*
    DWORD        dwFrameTime;           // +0x0274 ms
    float        fFrameScale;           // +0x0278 frame time / (1000 / 60)
    CAsteroids   asteroids;             // +0x027C
    CXWing*      pXWings;               // +0x1310
    CTie*        pTies;                 // +0x1314
    CVader       vaders[3];             // +0x1318
    CFalcon      falcon;                // +0x1678
    CInterceptor* pInterceptors;        // +0x1748
    CStarDestroyer* pStarDestroyers;    // +0x174C
    CPlayerShip* pPlayerShips;          // +0x1750
    int          nTies;                 // +0x1754
    int          nXWings;               // +0x1758
    int          nVaders;               // +0x175C
    int          nInterceptors;         // +0x1760
    int          nStarDestroyers;       // +0x1764
    int          nPlayerShips;          // +0x1768
    int          difficulty;            // +0x176C DIFFICULTY_*
    bool         bTorpedoFired;         // +0x1770
    DWORD        dwModeTime;            // +0x1774 when the briefing or the flight to the Death Star began
    DWORD        dwEndingTime;          // +0x1778
    DWORD        dwTrenchExitTime;      // +0x177C
    DWORD        dwResultTime;          // +0x1780 the ending's or a lost game's message
    bool         bInvertY;              // +0x1784
    bool         bVibration;            // +0x1785
    float        fCineDistance;         // +0x1788 the cutscene camera
    float        fCineAngle;            // +0x178C
    CMesh        meshTrench;            // +0x1790
    CMesh        meshTrenchEnd;         // +0x17C8
    CTrench      trench;                // +0x1800
    CProp        propDeathStar;         // +0x1810
    CProp        propPlanet;            // +0x1824
    CSparks      sparks;                // +0x1838
    CGlows       glows;                 // +0x1974
    CTarget      target;                // +0x1C0C
    CMenu        menu;                  // +0x1C70
    CSound       sound;                 // +0x1C7C
    CInput       input;                 // +0x1D20
    CFont        font;                  // +0x20F0
    CCamera      camera;                // +0x20F8
    int          nLightKills;           // +0x2178
    int          nDarkKills;            // +0x217C
    BYTE         pad2180[0x4];          // +0x2180
    int          nCameraMode;           // +0x2184
    DWORD        dwBriefingTime;        // +0x2188
    int          gameType;              // +0x218C
    CTorpedoes   torpedoes;             // +0x2190
    int          maxKills;              // +0x2220
    DWORD        dwEndTime;             // +0x2224
    LPDIRECT3DVERTEXBUFFER8 pCubeVB;    // +0x2228
    BYTE         pad222C[0x4];          // +0x222C
    LPDIRECT3DDEVICE8 pDevice;          // +0x2230
    CMusicManager music;                // +0x2234
    LPDIRECTSOUND pDSound;              // +0x2398
    DWORD        dwMusicHudTime;        // +0x239C
    bool         bShowMusicHud;         // +0x23A0
    DWORD        dwLastSoundtrack;      // +0x23A4
};                                      // 0x23A8 bytes

extern CGame g_Game;
extern CHud g_Hud;
extern CRadar g_Radar;
extern CSkybox g_Sky;
extern CIntro g_Intro;

// The models (Main.cpp)
extern CMesh g_MeshXWing;
extern CMesh g_MeshR2D2;
extern CMesh g_MeshTie;
extern CMesh g_MeshInterceptor;
extern CMesh g_MeshSphere;
extern CMesh g_MeshStarDestroyer;
extern CMesh g_MeshAsteroid;
extern CMesh g_MeshPlanet;
extern CMesh g_MeshDeathStar;
extern CMesh g_MeshDarth;
extern CMesh g_MeshDome;
extern CMesh g_MeshAWing;
extern CMesh g_MeshFalcon;

void RenderMusicHud(CGame* pGame);
void InvalidFile(const char* strFile);

// Collision.cpp
bool RaySphereIntersect(const D3DXVECTOR3* pCenter, float fRadius, const D3DXVECTOR3* pOrigin,
                        const D3DXVECTOR3* pDir, float* pT);
bool RayBoxIntersect(const D3DXVECTOR3* pOrigin, const D3DXVECTOR3* pDir, const D3DXVECTOR3* pExtents,
                     const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU, const D3DXVECTOR3* pAxisV,
                     float* pT);
bool BoxTest(const SBox* pBox, const float* pExtents, const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU,
             const D3DXVECTOR3* pAxisV);

#endif
