// Baku Baku X2: the classes and the globals. Offsets are the original's
// (checked at compile time by build/layout_check.cpp).
#ifndef BAKU_GAME_H
#define BAKU_GAME_H

#include <xtl.h>
#include <xfont.h>
#include <stdio.h>
#include <math.h>
#include "Input.h"
#include "MusicManager.h"

class CGame;

// ---- textures -----------------------------------------------------------------

// Every texture the game draws, by id (the skins' skin.cfg numbers; see
// original/skins/default/skin.cfg for the files):
//   1-8       the pieces (1-4 food, 5-8 the animals' heads), 9-16 spinning
//   17-20     the four animals walking, 121-124 eating
//   21-38     the menus' items, 39 the title, 40-44 the countdown
//   45/46     the lion's jaws (a lost board), 47-57 "player" and the digits
//   58-63     the controls (pads 1-4, easy, hard), 64-66 free for all, red, blue
//   67-70     players, type, free for all, teams; 71-78 the pause and skins pages
//   79        the menus' background, 80 "great" (the winners), 90-92 the puffs
//   93-96     "here", "we", "are", "you" (the countdowns), 97 the music box
//   98-101    the crumbs, 102-107 the portraits, 109 the teams' help
//   110-120   the backgrounds, 125-129 the help pages, 130 start
#define NUM_TEXTURES 131

class CTextures
{
public:
    LPDIRECT3DTEXTURE8 m_pTextures[NUM_TEXTURES];   // +0x000
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x20C

    void SetDevice(LPDIRECT3DDEVICE8 pDevice);
    void Load(const char* strFile, int id);
    void Set(int id);
};                                                  // 0x210 bytes

// ---- text -------------------------------------------------------------------------

class CFont
{
public:
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x00
    XFONT*             m_pFont;                     // +0x04
    LPDIRECT3DSURFACE8 m_pFrontBuffer;              // +0x08

    void Create(LPDIRECT3DDEVICE8 pDevice, const WCHAR* strFile);
    void DrawText(const WCHAR* strText, float x, float y, DWORD r, DWORD g, DWORD b);
};                                                  // 0x0C bytes

// ---- sound effects (DirectMusic) ------------------------------------------------------

struct IDirectMusicSegment;

class CSoundEffect
{
public:
    IDirectMusicSegment*  m_pSegment;               // +0x00

    void Load(const char* strFile);
    void Play();
};                                                  // 0x04 bytes

// ---- skins ----------------------------------------------------------------------------------

// A skin: a folder of D:\skins with a skin.cfg (its preview's file name,
// then "id file" lines naming the textures it replaces).
struct SSkin
{
    char               strPath[256];                // +0x000 D:\skins\<name>\ (with the backslash)
    LPDIRECT3DTEXTURE8 pPreview;                    // +0x100
};                                                  // 0x104 bytes

class CSkins
{
public:
    SSkin*             m_pSkins;                    // +0x00
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x04
    CTextures*         m_pTextures;                 // +0x08
    int                m_nSkins;                    // +0x0C
    int                m_nCurrent;                  // +0x10

    void Find(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures);
    void Prev();
    void Next();
    void LoadSaved();
    void Apply();
    void RenderPreview();
};                                                  // 0x14 bytes

// ---- the settings file ---------------------------------------------------------------------

// T:\baku2.cfg, with the current skin's folder.
class CConfig
{
public:
    char               m_strSkin[256];              // +0x000

    void Load();
    void Save();
};                                                  // 0x100 bytes

// ---- animation frames -------------------------------------------------------------------

// The texture corners of one frame of a sheet: 1 top left, 2 top right,
// 3 bottom right, 4 bottom left. Each animation keeps its own (in globals
// for the pieces and the animals).
struct SFrameUV
{
    float tu1;                                      // +0x00
    float tv1;                                      // +0x04
    float tu2;                                      // +0x08
    float tv2;                                      // +0x0C
    float tu3;                                      // +0x10
    float tv3;                                      // +0x14
    float tu4;                                      // +0x18
    float tv4;                                      // +0x1C
};                                                  // 0x20 bytes

// The frame being worked out, also kept in globals.
struct SFrame
{
    float fHeight;                                  // +0x00 of a frame, in texture coordinates
    int   nCol;                                     // +0x04
    float fWidth;                                   // +0x08
    int   nRow;                                     // +0x0C
};                                                  // 0x10 bytes

// Works out frame n of a sheet nCols across and nRows down.
static __forceinline void SetFrame(SFrame& f, SFrameUV& uv, int n, int nCols, int nRows)
{
    f.nRow = n / nCols;
    f.nCol = (int)fmod((double)n, (double)nCols);
    f.fWidth = 1.0f / nCols;
    f.fHeight = 1.0f / nRows;
    uv.tu1 = f.nCol * f.fWidth;
    uv.tv1 = f.nRow * f.fHeight;
    uv.tu3 = uv.tu1 + f.fWidth;
    uv.tv3 = uv.tv1 + f.fHeight;
    uv.tu2 = uv.tu3;
    uv.tv2 = uv.tv1;
    uv.tu4 = uv.tu1;
    uv.tv4 = uv.tv3;
}

// ---- particles ------------------------------------------------------------------------------

struct SParticle
{
    float              x;                           // +0x00
    float              y;                           // +0x04
    float              z;                           // +0x08
    DWORD              dwStart;                     // +0x0C
    int                frame;                       // +0x10 0-2: texture 0x5A + frame
    bool               bActive;                     // +0x14
};                                                  // 0x18 bytes

// The puffs: shown for 600 ms each.
class CParticles
{
public:
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x00
    CTextures*         m_pTextures;                 // +0x04
    SParticle*         m_pParticles;                // +0x08
    float              m_fScale;                    // +0x0C
    int                m_nParticles;                // +0x10
    BYTE               pad0014[0x4];

    void Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nParticles, float fScale);
    void Add(float x, float y, float z);
    void Update();
    void Render();
};                                                  // 0x18 bytes

// ---- the match settings --------------------------------------------------------------------

enum
{
    CONTROL_PAD1 = 1,       // 1-4: pads 1-4
    CONTROL_EASY = 5,       // the computer
    CONTROL_HARD = 6,
};

enum
{
    MODE_FREE_FOR_ALL = 1,
    MODE_TEAMS        = 2,
};

enum
{
    TEAM_RED  = 1,
    TEAM_BLUE = 2,
};

struct SMatchSettings
{
    int nControl[8];        // +0x00 CONTROL_*
    int nTeam[8];           // +0x20 TEAM_*
    int nRed;               // +0x40 players in each team
    int nBlue;              // +0x44
    int nPlayers;           // +0x48 2-8
    int nMode;              // +0x4C MODE_*
    int nBoards;            // +0x50 the layout (StartMatch)
};                          // 0x54 bytes

// ---- the menus --------------------------------------------------------------------------------

enum
{
    PAGE_MAIN,              // Play, Options, Skins, Credits
    PAGE_SETUP,             // players, mode, each player's control (and team)
    PAGE_OPTIONS,           // music, random music, piece set, sound effects, back
    PAGE_PAUSE,             // resume, restart, skins, quit
    PAGE_SKINS,             // previous, next, apply, back
    PAGE_HELP,              // the five help pages
};

struct SMenuPage
{
    int nItems;             // +0x00
    int texture;            // +0x04 the first item's texture
    int item;               // +0x08 the highlighted one
};                          // 0x0C bytes

struct MENUVERTEX
{
    float x;                                        // +0x00
    float y;                                        // +0x04
    float z;                                        // +0x08
    float tu;                                       // +0x0C
    float tv;                                       // +0x10
};                                                  // 0x14 bytes

class CMenu
{
public:
    LPDIRECT3DVERTEXBUFFER8 m_pVB[3];               // +0x000 0: plain, 1: highlighted, 2: whole texture
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x00C
    CTextures*         m_pTextures;                 // +0x010
    CParticles*        m_pParticles;                // +0x014
    CInput*            m_pInput;                    // +0x018
    BYTE               pad001C[0x4];
    DWORD              m_dwInputTime;               // +0x020
    BYTE               pad0024[0x4];
    SFrameUV           m_uv;                        // +0x028 the texture corners being built
    MENUVERTEX         m_v[4];                      // +0x048
    SMenuPage          m_pages[6];                  // +0x098
    int                m_page;                      // +0x0E0 PAGE_*
    SMatchSettings     m_settings;                  // +0x0E4
    bool               m_bStart;                    // +0x138 start (or restart) the match
    bool               m_bResume;                   // +0x139
    bool               m_bFromPause;                // +0x13A the skins page was opened from the pause menu
    bool               m_bQuit;                     // +0x13B
    BYTE               pad013C[0x4];
    CSkins             m_skins;                     // +0x140

    void Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, CInput* pInput, CParticles* pParticles);
    void CreateVertexBuffers();
    void Update();
    void Render();
};                                                  // 0x154 bytes

// ---- a board ---------------------------------------------------------------------------------------

#define BOARD_COLS 6
#define BOARD_ROWS 10

// What a board cell holds: 0 empty, 1-4 food, 5-8 the animals' heads (an
// animal eats the food of its own kind: head 5 + n eats food 1 + n).
#define FIRST_HEAD 5

struct QUADVERTEX
{
    float x;                                        // +0x00
    float y;                                        // +0x04
    float z;                                        // +0x08
    float tu;                                       // +0x0C
    float tv;                                       // +0x10
};                                                  // 0x14 bytes

// The crumbs an eaten piece leaves: four bits that fly apart for 2 s.
struct SCrumb
{
    float x;                                        // +0x00
    float y;                                        // +0x04
    float z;                                        // +0x08
    float fScale;                                   // +0x0C
    float dx;                                       // +0x10
    float dy;                                       // +0x14
};                                                  // 0x18 bytes

class CCrumbs
{
public:
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x00
    CTextures*         m_pTextures;                 // +0x04
    SCrumb             m_crumbs[4];                 // +0x08
    DWORD              m_dwStart;                   // +0x68 0: not flying
    int                m_type;                      // +0x6C the piece eaten (its crumbs' texture)

    void Start(float x, float y, int type);
    void Update();
    void Render();
};                                                  // 0x70 bytes

// One of the two pieces falling.
class CPiece
{
public:
    int                m_nCell;                     // +0x00 the board's cell size
    int                m_nScale;                    // +0x04 the quad's half size
    int                m_x;                         // +0x08 the board's corner
    int                m_y;                         // +0x0C
    int                m_z;                         // +0x10
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x14
    CTextures*         m_pTextures;                 // +0x18
    QUADVERTEX         m_v[4];                      // +0x1C the sparkle's frame (UpdateVertices)
    float              m_fx;                        // +0x6C where it's drawn
    float              m_fy;                        // +0x70
    int                m_col;                       // +0x74
    int                m_row;                       // +0x78
    int                m_type;                      // +0x7C 0: none
    bool               m_bLanded;                   // +0x80

    void UpdateVertices();
    void Render();
};                                                  // 0x84 bytes

// A piece in the queue (and the player's portrait).
struct SPreview
{
    LPDIRECT3DDEVICE8  pDevice;                     // +0x00
    CTextures*         pTextures;                   // +0x04
    int                nCell;                       // +0x08
    int                nScale;                      // +0x0C
    int                x;                           // +0x10
    int                y;                           // +0x14
    int                z;                           // +0x18
    float              fx;                          // +0x1C
    float              fy;                          // +0x20
    int                type;                        // +0x24
};                                                  // 0x28 bytes

// An animal on the board's bottom row: when a head lands next to its food,
// it walks to each piece it eats and back.
class CEater
{
public:
    QUADVERTEX         m_v[4];                      // +0x000 its frame
    CParticles*        m_pParticles;                // +0x050
    int                m_frame;                     // +0x054 0-17
    int                m_nCell;                     // +0x058
    int                m_nScale;                    // +0x05C
    int                m_x;                         // +0x060 the board's corner
    int                m_y;                         // +0x064
    int                m_z;                         // +0x068
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x06C
    CTextures*         m_pTextures;                 // +0x070
    int*               m_pGrid;                     // +0x074 the board's cells
    int                m_nSpeed;                    // +0x078 steps a frame
    int                m_index;                     // +0x07C 1-4: which animal (textures 0x10 + index)
    int                m_nChomp;                    // +0x080 >0: chomping (counts to 5)
    float              m_fx;                        // +0x084
    float              m_fy;                        // +0x088
    float              m_tx;                        // +0x08C where it's going (-1: nowhere)
    float              m_ty;                        // +0x090
    float              m_homeX;                     // +0x094
    float              m_homeY;                     // +0x098
    CCrumbs*           m_pCrumbs;                   // +0x09C the board's ten
    float              m_fScaleX;                   // +0x0A0
    float              m_fScaleY;                   // +0x0A4
    float              m_fCell;                     // +0x0A8
    int                m_eat[50];                   // +0x0AC the cells to eat (-1: none)
    int                m_nEat;                      // +0x174
    bool               m_bHome;                     // +0x178

    void Init(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int* pGrid, int index, int nCell, int nScale,
              int x, int y, int z, CParticles* pParticles);
    void Update();
    void EatNext();
    void Aim(int unused);
    void UpdateVertices();
    void Render();
};                                                  // 0x17C bytes

class CBoard
{
public:
    virtual void Set(int col, int row, int value);  // +0x000 vftable
    virtual int Get(int col, int row);

    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x004
    CTextures*         m_pTextures;                 // +0x008
    int                m_nCell;                     // +0x00C
    int                m_nScale;                    // +0x010
    int                m_nBoost;                    // +0x014 extra drop steps this frame (Down)
    SPreview           m_preview[5];                // +0x018 the next pieces
    SPreview           m_portrait;                  // +0x0E0
    int                m_x;                         // +0x108
    int                m_y;                         // +0x10C
    int                m_z;                         // +0x110
    int                m_nAiMove;                   // +0x114 the easy computer's move, 0-2 a pair in turn
    bool               m_bAimed;                    // +0x118 the hard computer has its column
    int                m_nSpeed;                    // +0x11C drop steps a frame
    bool               m_bShowEaters;               // +0x120
    CPiece             m_pieces[2];                 // +0x124
    int                m_rotation;                  // +0x22C 0-3: the second piece right, below, left, above
    LPDIRECT3DDEVICE8  m_pDevice2;                  // +0x230
    CTextures*         m_pTextures2;                // +0x234
    CParticles*        m_pParticles;                // +0x238
    int                m_nUnused23C;                // +0x23C
    bool               m_bEating[4];                // +0x240 each eater
    bool               m_bFalling;                  // +0x244 pieces dropping after eating
    int                m_grid[BOARD_ROWS * BOARD_COLS];  // +0x248 row 0 at the bottom
    CEater             m_eaters[4];                 // +0x338
    bool               m_bUnused928;                // +0x928
    float              m_topX;                      // +0x92C the lion's top jaw (comes down)
    float              m_topY;                      // +0x930
    float              m_botX;                      // +0x934 its bottom jaw (comes up, then chews)
    float              m_botY;                      // +0x938
    bool               m_bChewRight;                // +0x93C the bottom jaw's way across
    bool               m_bShut;                     // +0x93D the jaws have met
    bool               m_bChewUp;                   // +0x93E the bottom jaw's way up
    DWORD              m_dwShutTime;                // +0x940
    CCrumbs            m_crumbs[10];                // +0x944
    bool               m_bLost;                     // +0xDA4 the lion is eating the board
    int                m_bx;                        // +0xDA8 the board's corner
    int                m_by;                        // +0xDAC
    int                m_bz;                        // +0xDB0
    int                m_nCellSize;                 // +0xDB4
    int                m_nPieceScale;               // +0xDB8
    int                m_nLevel;                    // +0xDBC drop speed
    bool               m_bGarbage;                  // +0xDC0 drop the garbage owed (every 60 s)
    int                m_nGarbage;                  // +0xDC4 pieces of garbage owed
    int                m_nSend;                     // +0xDC8 pieces eaten: garbage for the others
    BYTE               pad0DCC[0x4];
    int                m_nWins;                     // +0xDD0
    int                m_nLosses;                   // +0xDD4
    BYTE               pad0DD8[0x4];
    bool               m_bAlive;                    // +0xDDC

    void SetupPieces(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nCell, int nScale, int x, int y, int z,
                     int portrait);
    void NextPair();
    void Rotate();
    void Begin();
    void DropGarbage();
    void Settle();
    void MoveLeft();
    void MoveRight();
    void Think(bool bHard);
    // Not inlined in the original (and only called from Flood).
    __declspec(noinline) bool IsListed(int eater, int cell);
    __declspec(noinline) void List(int eater, int cell);
    void Drop(int nSteps);
    void SlideLeft(int nSteps);
    void SlideRight(int nSteps);
    void Flood(int col, int row, int value);
    void Reset();
    void Init(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nCell, int nScale, int x, int y, int z,
              CParticles* pParticles, int portrait);
    void FindMatches();
    void Update();
    void Render();
};                                                  // 0xDE0 bytes

// ---- the game ----------------------------------------------------------------------------------------

enum
{
    STATE_MENU,             // the menus
    STATE_COUNTDOWN,        // 3, 2, 1, go
    STATE_PLAY,
    STATE_RESULT,           // the winners cheer for 3 s
    STATE_NEXT,             // the menus updated unseen: the next round starts
};

class CGame
{
public:
    LPDIRECT3DDEVICE8  m_pDevice;                   // +0x0000
    CTextures*         m_pTextures;                 // +0x0004
    CInput*            m_pInput;                    // +0x0008
    CBoard             m_boards[8];                 // +0x000C
    CParticles         m_particles;                 // +0x6F0C
    int                m_nCountdown;                // +0x6F24 0-5
    DWORD              m_dwCountdownTime;           // +0x6F28
    int                m_nCountdownStyle;           // +0x6F2C 0-2
    DWORD              m_dwResultTime;              // +0x6F30
    DWORD              m_dwGarbageTime;             // +0x6F34 every 60 s everyone gets garbage
    int                m_nLevel;                    // +0x6F38 1-4
    DWORD              m_dwLevelTime;               // +0x6F3C every 90 s the level goes up
    SMatchSettings*    m_pSettings;                 // +0x6F40
    BYTE               pad6F44[0x4];
    CMenu              m_menu;                      // +0x6F48
    int                m_state;                     // +0x709C STATE_*
    bool               m_bOver;                     // +0x70A0 the match is over

    void Create(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, CInput* pInput);
    void LoadSettings();
    void UpdateInput();
    void Render();
    void StartMatch();
    void UpdateGarbageBar();
    void ShareGarbage();
    void UpdateTimers();
    void Update();
};                                                  // 0x70A4 bytes

// ---- globals (Main.cpp) ---------------------------------------------------------------------------------

extern CInput               g_Input;
extern LPDIRECT3DDEVICE8    g_pDevice;
extern bool                 g_bShowMusicHud;
extern bool                 g_bRandomMusic;
extern int                  g_nBackgrounds;
extern DWORD                g_dwMusicHudTime;
extern SMatchSettings       g_Settings;
extern CSoundEffect         g_SoundGo;
extern LPDIRECT3DVERTEXBUFFER8 g_pColorQuadVB;
extern CFont                g_Font;
extern bool                 g_bSoundEffects;
extern CTextures            g_Textures;
extern CSoundEffect         g_SoundEat;
extern LPDIRECT3DTEXTURE8   g_pSplashTexture;
extern int                  g_nPieceSet;
extern int                  g_nPieceFrame;
extern CConfig              g_Config;
extern LPDIRECT3DVERTEXBUFFER8 g_pBarVB;
extern CMusicManager        g_Music;
extern CGame                g_Game;
extern LPDIRECTSOUND8       g_pDSound;
extern LPDIRECT3DVERTEXBUFFER8 g_pQuadVB;
extern bool                 g_bMusicOn;
extern int                  g_nBackground;

#define D3DFVF_QUADVERTEX  (D3DFVF_XYZ | D3DFVF_TEX1)
#define D3DFVF_COLORVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

struct COLORVERTEX
{
    float x;                                        // +0x00
    float y;                                        // +0x04
    float z;                                        // +0x08
    D3DCOLOR color;                                 // +0x0C
};                                                  // 0x10 bytes

#endif
