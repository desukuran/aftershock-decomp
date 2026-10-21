// Madness Interactive SX: the game's shared types and globals.
//
// As in AfterShock Pool SX (same author), almost all state lives in one
// global aggregate, g_Game (CGame, at 0x000A6128), used by free functions.
// Member order is the original declaration order; offsets are given for
// each member. The characters, items and weapon types are on the heap.
#ifndef MADNESS_GAME_H
#define MADNESS_GAME_H

#include <xtl.h>
#include <d3dx8.h>
#include <stdio.h>

#include "Credits.h"
#include "Font.h"
#include "Input.h"
#include "MusicManager.h"
#include "Sound.h"

// ---------------------------------------------------------------------------
// Game states (CGame::state)
enum
{
    STATE_LOADING   = 0,
    STATE_INTRO     = 1,    // the scrolling credits
    STATE_MENU      = 2,
    STATE_OPTIONS   = 3,
    STATE_CUSTOMIZE = 4,    // the player's head, suit, hat, mask and eyewear
    STATE_SETTINGS  = 5,    // sound and music on/off
    STATE_STATS     = 6,
    STATE_UNLOCKS   = 7,
    STATE_STORY     = 8,    // the story so far, before level 1
    STATE_PLAYING   = 9,
    STATE_ENDING    = 10,   // the fight with Lantus
    STATE_SANDBOX   = 11,   // spawn weapons, dummies and enemies at will
    STATE_TUTORIAL  = 12,
};

// Weapon types (SItem::type, index into g_Game.pWeaponTypes)
enum
{
    WEAPON_KNIFE        = 0,
    WEAPON_BAT          = 1,
    WEAPON_NIGHTSTICK   = 2,
    WEAPON_SWORD        = 3,
    WEAPON_HANDGUN      = 4,
    WEAPON_SHOTGUN      = 5,
    WEAPON_JETPACK      = 6,
    WEAPON_MACHINEGUN   = 7,
    WEAPON_AXE          = 8,
    WEAPON_CHAINSAW     = 9,
    WEAPON_RIFLE        = 10,
    NUM_WEAPON_TYPES    = 11,
};

// How a thrown item turns in flight (SItem::blade)
enum
{
    BLADE_NONE  = 0,
    BLADE_POINT = 1,    // knife, sword: points along its path
    BLADE_SPIN  = 2,    // axe: spins
};

// Particle types (SParticle::type)
enum
{
    PARTICLE_NONE       = 0,
    PARTICLE_BLOOD      = 1,    // a drop, stretched by its size
    PARTICLE_SHELL      = 2,    // a spent shell (shotgun, handgun, rifle)
    PARTICLE_SMOKE      = 3,
    PARTICLE_TRAIL      = 4,    // behind a bullet
    PARTICLE_BIGSHELL   = 5,    // machine gun
    PARTICLE_BUBBLE     = 6,    // the intro's rising bubbles
    PARTICLE_GUSH       = 7,    // blood from a neck
};

// The floor and the walls of a level.
#define GROUND_Y        -26.0f
#define GROUND_BOUNCE_Y -25.9f

#define NUM_HEADS       35      // head.tga and heads\1..34.tga
#define NUM_BODIES      3       // body.tga and bodys\1..2.tga
#define NUM_GLASSES     4
#define MAX_CHARACTERS  20
#define MAX_ITEMS       10
#define MAX_PARTICLES   300
#define MAX_BULLETS     15

// ---------------------------------------------------------------------------
// A weapon type: its textures and properties. 0x1C bytes.
struct SWeaponType
{
    LPDIRECT3DTEXTURE8 pFlashTexture;   // +0x00 muzzle flash (jetpack: the exhaust)
    LPDIRECT3DTEXTURE8 pTexture;        // +0x04
    LPDIRECT3DTEXTURE8 pStuckTexture;   // +0x08 stuck in a body (knife, sword, axe)
    float        width;                 // +0x0C drawn size
    float        height;                // +0x10
    float        unknown14;             // +0x14
    float        unknown18;             // +0x18
};                                      // 0x1C bytes

// An item: a weapon or the jetpack, lying in the world, flying, held, on a
// character's back or stuck in a body. 0x64 bytes.
struct SItem
{
    int          type;                  // +0x00 WEAPON_*; 5 when unused
    D3DXVECTOR3  pos;                   // +0x04 (stuck in a body: relative to it)
    D3DXVECTOR3  vel;                   // +0x10
    D3DXVECTOR3  lastPos;               // +0x1C where it was when thrown
    bool         bGun;                  // +0x28 fires bullets (handgun, shotgun, machine gun, rifle)
    float        fireDelay;             // +0x2C between shots
    float        unknown30;             // +0x30 only cleared
    int          blade;                 // +0x34 BLADE_*: how it turns in flight
    bool         bSticks;               // +0x38 sticks in a body when thrown hard (knife, sword, axe)
    float        fireTimer;             // +0x3C time to the next shot
    float        angle;                 // +0x40
    int          bounces;               // +0x44 times it hit the ground since thrown
    float        spin;                  // +0x48 axe: turning speed in flight
    BOOL         bFacingRight;          // +0x4C
    int          ammo;                  // +0x50 (chainsaw: fuel)
    bool         bHeld;                 // +0x54
    bool         bStuck;                // +0x55 in a body (drawn with it)
    float        lifeTime;              // +0x58 lying around: seconds until it disappears
    bool         bActive;               // +0x5C
    bool         bSpawned;              // +0x5D sandbox: spawned and not picked up yet
    bool         unknown5E;             // +0x5E never set
    bool         bAutomatic;            // +0x5F keeps firing with the trigger held
    BOOL         bJetpack;              // +0x60 worn on the back
};                                      // 0x64 bytes

// Where a bullet went into a body (drawn as a blood spot). 0xC bytes.
struct SWound
{
    float        x;                     // +0x00 relative to the character (always 0)
    float        y;                     // +0x04
    bool         bUsed;                 // +0x08
};                                      // 0xC bytes

#define MAX_WOUNDS      5

// A character (the player, enemies, dummies). 0x14C bytes.
struct SCharacter
{
    SWound       wounds[MAX_WOUNDS];    // +0x000
    D3DXVECTOR3  pos;                   // +0x03C
    D3DXVECTOR3  headOffset;            // +0x048 the head, relative to pos
    BYTE         pad054[0x50];          // +0x054
    float        fallAngle;             // +0x0A4 over 90 degrees: lying on the ground
    BYTE         pad0A8[0x8];           // +0x0A8
    int          health;                // +0x0B0
    BYTE         pad0B4[0x24];          // +0x0B4
    bool         bActive;               // +0x0D8
    BYTE         pad0D9[0x73];          // +0x0D9
};                                      // 0x14C bytes

#define STANDING_ANGLE  1.570796f       // (float)(D3DX_PI / 2)

// A particle. 0x2C bytes.
struct SParticle
{
    D3DXVECTOR3  pos;                   // +0x00
    bool         bFacingRight;          // +0x0C never set: always drawn flipped
    D3DXVECTOR3  vel;                   // +0x10
    float        size;                  // +0x1C blood spreads on the ground
    bool         bActive;               // +0x20
    short        type;                  // +0x22 PARTICLE_*
    float        gravity;               // +0x24 0: doesn't move
    float        life;                  // +0x28 seconds left
};                                      // 0x2C bytes

class CParticles
{
public:
    SParticle    particles[MAX_PARTICLES];  // +0x0000
    DWORD        dwInitTime;                // +0x3390

    void Init();
    void Add(float x, float y, float z, float vx, float vy, float vz, int type);
    void Update();
    void Render();
};                                          // 0x3394 bytes

// A bullet in flight. 0x20 bytes.
struct SBullet
{
    float        life;                  // +0x00 seconds left
    float        trailTimer;            // +0x04 to the next trail particle
    float        active;                // +0x08 nonzero: in flight
    float        x;                     // +0x0C
    float        y;                     // +0x10
    float        vx;                    // +0x14
    float        vy;                    // +0x18
    bool         bFacingRight;          // +0x1C
};                                      // 0x20 bytes

class CBullets
{
public:
    SBullet      bullets[MAX_BULLETS];  // +0x000

    void Init();
    void Add(float x, float y, float dx, float dy, float active, bool bFacingRight);
    void Update();
    void Render();
};                                      // 0x1E0 bytes

// The settings file (T:\xmadness.cfg); the values are in g_Game.
class CConfig
{
public:
    void Load();
    void Save();
};

// ---------------------------------------------------------------------------
class CGame
{
public:
    int          loadStep;              // +0x000
    int          state;                 // +0x004 STATE_*
    LPDIRECT3DTEXTURE8 texMainMenu;     // +0x008 menu\mainmenu.tga
    LPDIRECT3DTEXTURE8 texOptionsMenu;  // +0x00C menu\optionsmenu.tga
    LPDIRECT3DTEXTURE8 texArrow;        // +0x010 menu\arrow.tga
    LPDIRECT3DTEXTURE8 texUnlock;       // +0x014 menu\unlock.tga
    LPDIRECT3DTEXTURE8 texTick;         // +0x018 menu\tick.tga
    LPDIRECT3DTEXTURE8 texUnlocks[6];   // +0x01C menu\unlock1..6.tga
    LPDIRECT3DTEXTURE8 texMenuPages[11]; // +0x034 menu\1..11.tga
    LPDIRECT3DTEXTURE8 texSnm[2];       // +0x060 menu\snm1..2.tga
    int          settingsSel;           // +0x068
    int          mainMenuSel;           // +0x06C
    int          optionsSel;            // +0x070
    int          unlockSel;             // +0x074
    DWORD        levelClearTime;        // +0x078
    DWORD        storyStartTime;        // +0x07C
    bool         bUnlocked[6];          // +0x080 saved: UNLOCK_*
    bool         bEnabled[6];           // +0x086 saved: unlocked features switched on
    int          cheatStep;             // +0x08C
    DWORD        whiteTime;             // +0x090
    bool         bShowMusicMenu;        // +0x094
    DWORD        musicMenuTime;         // +0x098
    int          numDecapitations;      // +0x09C saved
    bool         bSound;                // +0x0A0 saved
    bool         bMusic;                // +0x0A1 saved
    bool         bPaused;               // +0x0A2
    bool         bConfigSaved;          // +0x0A3
    float        highestHeadHit;        // +0x0A4 saved
    int          totalKills;            // +0x0A8 saved
    LPDIRECT3DTEXTURE8 texMadnessTracks; // +0x0AC madnesstracks.tga
    float        spawnTimer;            // +0x0B0 story: seconds to the next enemy
    CMusicManager music;                // +0x0B4
    CBullets     bullets;               // +0x218
    LPDIRECT3DTEXTURE8 texSplash;       // +0x3F8 aftershock.jpg
    LPDIRECT3DTEXTURE8 texHealthBar;    // +0x3FC healthbar.tga
    LPDIRECT3DTEXTURE8 texHealthBar2;   // +0x400 healthbar2.tga
    LPDIRECT3DTEXTURE8 texHealthBar3;   // +0x404 healthbar3.tga
    LPDIRECT3DTEXTURE8 texBulletTrail;  // +0x408 bullettrail.tga
    LPDIRECT3DTEXTURE8 texBullet;       // +0x40C bullet.tga
    LPDIRECT3DTEXTURE8 texBulletCountHolder; // +0x410 bulletcountholder.tga
    LPDIRECT3DTEXTURE8 texBulletTime;   // +0x414 bullettime.tga
    LPDIRECT3DTEXTURE8 texTarget;       // +0x418 target.tga
    LPDIRECT3DTEXTURE8 texTalk;         // +0x41C talk.tga
    LPDIRECT3DTEXTURE8 texBlood;        // +0x420 blood.tga
    LPDIRECT3DTEXTURE8 texShotgunShell; // +0x424 shotgunshell.tga
    LPDIRECT3DTEXTURE8 texSmoke;        // +0x428 smoke.tga
    LPDIRECT3DTEXTURE8 texLevel;        // +0x42C levels\N.JPG
    LPDIRECT3DTEXTURE8 texCutscenes[2]; // +0x430 levels\cutscene1..2.JPG
    LPDIRECT3DTEXTURE8 texBubble;       // +0x438 bubble.tga
    LPDIRECT3DTEXTURE8 texWhite;        // +0x43C white.tga
    DWORD        bulletTimeSoundTime;   // +0x440
    bool         bThumbClickHeld;       // +0x444
    bool         bBulletTime;           // +0x445
    bool         unknown446;            // +0x446
    DWORD        frameMs;               // +0x448 the last frame's length
    float        frameTime;             // +0x44C
    float        bulletTimeDrain;       // +0x450
    DWORD        menuInputTime;         // +0x454 menus: the last stick move (they repeat every 150 ms)
    LPDIRECT3DVERTEXBUFFER8 pQuadVB;    // +0x458
    LPDIRECT3DVERTEXBUFFER8 pVB45C;     // +0x45C
    LPDIRECT3DVERTEXBUFFER8 pVB460;     // +0x460
    LPDIRECT3DVERTEXBUFFER8 pVB464;     // +0x464
    LPDIRECT3DDEVICE8 pDevice;          // +0x468
    LPDIRECT3DTEXTURE8 texBlack;        // +0x46C black.tga
    int          numHeads;              // +0x470 35
    int          numBodies;             // +0x474 3
    LPDIRECT3DTEXTURE8 texHeads[NUM_HEADS + 2]; // +0x478 then headgib, headgib2
    LPDIRECT3DTEXTURE8 texBodies[NUM_BODIES + 3]; // +0x50C then bodygib, bodygib2 (one unused)
    LPDIRECT3DTEXTURE8 texFoot;         // +0x524 foot.tga
    LPDIRECT3DTEXTURE8 texHand;         // +0x528 hand.tga
    LPDIRECT3DTEXTURE8 texBloodSpot;    // +0x52C bloodspot.tga
    LPDIRECT3DTEXTURE8 texShadow;       // +0x530 shadow.tga
    LPDIRECT3DTEXTURE8 texNinja;        // +0x534 ninja.tga
    LPDIRECT3DTEXTURE8 texHat;          // +0x538 hat.tga
    LPDIRECT3DTEXTURE8 texGlasses[NUM_GLASSES]; // +0x53C glasses\1..4.tga
    XBGAMEPAD*   pPad;                  // +0x54C the first gamepad
    CInput       input;                 // +0x550
    CFont        font;                  // +0x920
    int          levelKills;            // +0x928
    int          lives;                 // +0x92C
    int          endingStep;            // +0x930
    int          endingTalk;            // +0x934
    int          maxEnemies;            // +0x938
    int          level;                 // +0x93C
    float        bulletTimeLeft;        // +0x940 up to 8
    bool         bLevelClear;           // +0x944
    bool         bBarsIn;               // +0x945
    bool         bBarsOut;              // +0x946 the bars have slid away again
    float        unknown948;            // +0x948
    float        barTopY;               // +0x94C the letterbox bars between levels
    float        unknown950;            // +0x950
    float        barBottomY;            // +0x954
    int          sandboxWeapon;         // +0x958
    SWeaponType* pWeaponTypes;          // +0x95C [NUM_WEAPON_TYPES]
    SCharacter*  pCharacters;           // +0x960 [maxCharacters]
    SItem*       pItems;                // +0x964 [maxItems]
    int          maxItems;              // +0x968 10
    int          maxCharacters;         // +0x96C 20
    int          numWeaponTypes;        // +0x970 11
    CParticles   particles;             // +0x974
    bool         bBlink;                // +0x3D08
};                                      // 0x3D0C bytes

extern CGame g_Game;
extern CConfig g_Config;
extern bool g_unknownA6121;

#endif
