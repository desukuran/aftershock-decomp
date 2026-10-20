// AfterShock Pool SX: the game's shared types and globals.
//
// The original keeps almost all of its state in one global aggregate,
// g_Game (CGame), used by free functions. Member order is the original
// declaration order (the binary's constructor and layout show it); offsets
// are given for each member. Separate globals (balls, meshes, barriers,
// pockets) follow.
#ifndef AFTERSHOCK_GAME_H
#define AFTERSHOCK_GAME_H

#include <xtl.h>
#include <d3dx8.h>
#include <stdio.h>
#include <string>

#include "Camera.h"
#include "Font.h"
#include "Input.h"
#include "Mesh.h"
#include "MusicManager.h"
#include "Sound.h"

// ---------------------------------------------------------------------------
// Game states (CGame::state)
enum
{
    STATE_LOADING   = 0,    // the splash screen while loading
    STATE_INTRO     = 1,    // balls bouncing on the AFTERSHOCK letters, credits
    STATE_MENU      = 2,    // the main menu
    STATE_PLAYING   = 3,
    STATE_GAMEOVER  = 4,
    STATE_EDITOR    = 5,
    STATE_OPTIONS   = 6,
    STATE_PHYSICS   = 7,
    STATE_PROFILES  = 8,
    STATE_PROFILENAME = 9,  // entering a profile's name
    STATE_NEWGAME   = 10,   // picking one of the built-in tables
    STATE_CUSTOMGAME = 11,  // picking one of the player's tables
    STATE_PLAYERSETUP = 12, // profiles and controllers
};

// Editor screens (g_EditorScreen)
enum
{
    EDIT_MENU       = 0,
    EDIT_BARRIERS   = 1,    // the cushion lines ("poles")
    EDIT_POCKETS    = 2,
    EDIT_PIECES     = 3,    // the table's visible pieces ("geometry")
    EDIT_BALLS      = 4,    // where the balls are racked
    EDIT_INFO       = 5,    // game type and room rendering
    EDIT_TEST       = 6,    // play on the table
    EDIT_SAVENAME   = 7,
    EDIT_SAVE       = 8,
    EDIT_LOAD       = 9,
};

// Game types (CGame::gameType)
enum
{
    GAME_8BALL      = 0,
    GAME_9BALL      = 1,
    GAME_REDVSBLUE  = 2,    // "Colored"
    GAME_GOLF       = 3,    // Mini-Golf-Pool
    GAME_CUTTHROAT  = 4,
};

// Room render modes (CGame::roomMode), set per table in the editor
enum
{
    ROOM_EXTERIOR   = 0,    // "Exterior Only": just the walls
    ROOM_NOTABLES   = 1,    // "No Tables": the furniture, no other pool tables
    ROOM_ENTIRE     = 2,    // "Entire Room"
};

// Ball types (SBall::type, SPlayer::group)
enum
{
    BALL_CUE        = 0,
    BALL_SOLID      = 1,    // red in Red vs Blue
    BALL_STRIPE     = 2,    // blue in Red vs Blue
    BALL_EIGHT      = 3,
    BALL_GROUP4     = 4,    // the three cut throat groups
    BALL_GROUP5     = 5,    // also every 9 ball ball
    BALL_GROUP6     = 6,
};

// Ball textures beyond Ball0..15.tga (SBall::number)
#define BALLTEX_RED     16
#define BALLTEX_BLUE    17
#define BALLTEX_BLACK   18

#define MAX_BALLS       40
#define NUM_RACK_BALLS  16
#define MAX_POCKETS     20
#define MAX_BARRIERS    400
#define NUM_PROFILES    3
#define MAX_PLAYERS     4
#define NUM_TABLE_SLOTS 10

// A screen-space vertex (DrawRect and the HUD panels).
struct SCREENVERTEX
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float tu, tv;
};
#define D3DFVF_SCREENVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// ---------------------------------------------------------------------------
struct SBall
{
    int         type;           // +0x00 BALL_*
    D3DXVECTOR3 pos;            // +0x04
    D3DXVECTOR3 dir;            // +0x10 unit direction of travel
    D3DXMATRIX  rot;            // +0x1C rolling orientation
    int         inPocket;       // +0x5C over a pocket, falling in
    int         pocket;         // +0x60 which one
    float       speed;          // +0x64
    bool        onTable;        // +0x68
    bool        needsReplace;   // +0x69 pocketed by foul (the cue ball): put back
    int         number;         // +0x6C texture index (BALLTEX_* for the colored ones)

    SBall();
};                              // 0x70 bytes

struct SPocket
{
    D3DXVECTOR3 pos;            // +0x00
    int         color;          // +0x0C 0, or the only ball type it accepts (Red vs Blue)
};                              // 0x10 bytes

// A piece placed in the table editor. Tables are singly linked lists of
// them, saved in the .cfg files.
enum
{
    PIECE_MAT       = 0,        // cloth (and decals): D:\media\custom\mats
    PIECE_CUSHION   = 1,        // D:\media\custom\cushions
    PIECE_BIG       = 2,        // whole tables etc.: D:\media\custom\bigpieces
};

struct STablePiece
{
    D3DXVECTOR3  pos;           // +0x00
    int          type;          // +0x0C PIECE_*
    int          mesh;          // +0x10 index into that type's SPieceSet
    float        rotation;      // +0x14 in quarter turns
    STablePiece* next;          // +0x18

    STablePiece();
    ~STablePiece();
    void Append(float x, float y, float z, int type);
    void Save(FILE* f);
    void Load(FILE* f);
    void Render(LPDIRECT3DDEVICE8 pDevice, struct SPieceSet* pSets);
};                              // 0x1C bytes

struct SPieceSet
{
    CMesh* meshes;              // +0x00
    int    count;               // +0x04
};                              // 0x08 bytes

struct SProfile
{
    std::string name;           // +0x00
    int         wins;           // +0x1C
    int         losses;         // +0x20
};                              // 0x24 bytes

struct SPlayer
{
    WCHAR name[50];             // +0x00
    int   group;                // +0x64 the BALL_* type to sink; 0 while the table is open
    int   pad;                  // +0x68 controller port
    int   unknown6C;            // +0x6C
    int   profile;              // +0x70 index into CGame::profiles
};                              // 0x74 bytes

// ---------------------------------------------------------------------------
class CGame
{
public:
    SPieceSet    pieceSets[3];          // +0x000 PIECE_MAT, PIECE_CUSHION, PIECE_BIG
    STablePiece  table;                 // +0x018 the table being played (list head)
    STablePiece  editTable;             // +0x034 the table in the editor
    STablePiece* pCurPiece;             // +0x050 list iterator
    STablePiece* pSelPiece;             // +0x054 editor: the selected piece
    bool         unknown058;            // +0x058
    bool         bSound;                // +0x059
    bool         bMusic;                // +0x05A
    float        tableMinX;             // +0x05C the loaded tables' pieces' extent (never reset)
    float        tableMinZ;             // +0x060
    float        tableMaxX;             // +0x064
    float        tableMaxZ;             // +0x068
    bool         bEditorHelp;           // +0x06C editor: Start held, show the help page

    LPDIRECT3DTEXTURE8 texMainMenu;     // +0x070 menu\mainmenu.tga
    LPDIRECT3DTEXTURE8 texMenuPages[6]; // +0x074 menu\1.tga (only the first is loaded)
    LPDIRECT3DTEXTURE8 texArrow;        // +0x08C menu\arrow.tga
    LPDIRECT3DTEXTURE8 texMain;         // +0x090 menu\main.tga
    LPDIRECT3DTEXTURE8 texNew;          // +0x094 menu\new.tga
    LPDIRECT3DTEXTURE8 texCustom;       // +0x098 menu\custom.tga
    LPDIRECT3DTEXTURE8 texNewGame;      // +0x09C menu\newgame.tga
    LPDIRECT3DTEXTURE8 texOptions;      // +0x0A0 menu\options.tga
    LPDIRECT3DTEXTURE8 texPreview;      // +0x0A4 menu\preview.tga
    LPDIRECT3DTEXTURE8 texAftershock;   // +0x0A8 menu\aftershock.tga
    LPDIRECT3DTEXTURE8 texGreenButton;  // +0x0AC the controller buttons for the help bars
    LPDIRECT3DTEXTURE8 texRedButton;    // +0x0B0
    LPDIRECT3DTEXTURE8 texYellowButton; // +0x0B4
    LPDIRECT3DTEXTURE8 texBlueButton;   // +0x0B8
    LPDIRECT3DTEXTURE8 texWhiteButton;  // +0x0BC
    LPDIRECT3DTEXTURE8 texBlackButton;  // +0x0C0
    LPDIRECT3DTEXTURE8 texBackButton;   // +0x0C4
    LPDIRECT3DTEXTURE8 texStartButton;  // +0x0C8

    int          menuSel;               // +0x0CC
    int          menuPage;              // +0x0D0
    int          menuProfile;           // +0x0D4
    float        menuX;                 // +0x0D8
    float        menuY;                 // +0x0DC
    float        unknown0E0;            // +0x0E0
    float        menuVelY;              // +0x0E4
    float        menuVelX;              // +0x0E8
    LPDIRECT3DTEXTURE8 texEdit[7];      // +0x0EC edit\1..7.tga
    int          editorMode;            // +0x108
    int          unknown10C;            // +0x10C

    SProfile     profiles[NUM_PROFILES];// +0x110

    LPDIRECT3DTEXTURE8 texNameTag;      // +0x17C hud\nametag.tga
    LPDIRECT3DTEXTURE8 texSolid;        // +0x180 solid.tga
    LPDIRECT3DTEXTURE8 texStripe;       // +0x184 stripe.tga
    LPDIRECT3DTEXTURE8 texRed;          // +0x188 red.tga
    LPDIRECT3DTEXTURE8 texBlue;         // +0x18C blue.tga
    LPDIRECT3DTEXTURE8 texBlack;        // +0x190 black.tga
    LPDIRECT3DTEXTURE8 texSplash;       // +0x194 aftershock.jpg
    LPDIRECT3DTEXTURE8 texWinner;       // +0x198 winner.tga
    int          state;                 // +0x19C STATE_*

    SPlayer      players[MAX_PLAYERS];  // +0x1A0
    int          curPlayer;             // +0x370
    int          numPlayers;            // +0x374
    bool         bPottedOwnBall;        // +0x378 sank one of their own: shoots again
    bool         bFirstHitDone;         // +0x379 the cue ball has touched a ball this shot
    bool         bHitLowestFirst;       // +0x37A 9 ball: the first ball hit was the lowest
    bool         bHitOwnGroupFirst;     // +0x37B 8 ball, Red vs Blue: the first ball hit was legal
    bool         bShotInProgress;       // +0x37C
    int          unknown380;            // +0x380
    int          unknown384;            // +0x384
    int          aimBall;               // +0x388
    DWORD        turnStartTime;         // +0x38C
    bool         bPlacingBall;          // +0x390 ball in hand: moving a replaced ball
    bool         bBallsMoving;          // +0x391
    int          shots[2];              // +0x394 shots taken by players 1 and 2 (Mini-Golf's score)
    int          nextPlayer;            // +0x39C
    int          roomMode;              // +0x3A0 ROOM_*: how much of the room to draw (saved with a table)
    bool         bShowPottedBalls;      // +0x3A4 the potted balls in a row by the table TODO: verify
    bool         bUnstickBalls;         // +0x3A5 push overlapping balls apart (see MoveBall)
    int          unstickCount;          // +0x3A8

    CMusicManager music;                // +0x3AC
    bool         bShowMusicHud;         // +0x510
    DWORD        musicHudTime;          // +0x514
    DWORD        musicTime;             // +0x518 when the music started
    LPDIRECT3DTEXTURE8 texMusic;        // +0x51C music.tga
    CFont        font;                  // +0x520
    CCamera      camera;                // +0x528
    CInput       input;                 // +0x62C
    DWORD        lastInputTime;         // +0x9FC
    DWORD        unknownA00;            // +0xA00
    LPDIRECT3DDEVICE8 device;           // +0xA04
    DWORD        frameMs;               // +0xA08
    int          editSel;               // +0xA0C editor: the highlighted barrier point or pocket
    bool         bEditGrab;             // +0xA10 editor: the highlighted item is picked up (A)
    D3DXVECTOR3  editCursor;            // +0xA14 editor: the camera, looking down from y at (x, z)
    LPDIRECT3DVERTEXBUFFER8 quadVB;     // +0xA20 unit quads (CreateQuadBuffers)
    LPDIRECT3DVERTEXBUFFER8 quadMirroredVB;  // +0xA24 the same, mirrored left-right
    LPDIRECT3DVERTEXBUFFER8 quadFlippedVB;   // +0xA28 flipped upside down
    LPDIRECT3DVERTEXBUFFER8 quadShadedVB;    // +0xA2C shaded gray on the right
    float        frameTime;             // +0xA30 seconds the last frame took
    WCHAR        tableName[15];         // +0xA34 the loaded table's name; edited in the editor
    float        introScroll;           // +0xA54 the intro's barriers scroll with the credits
    WCHAR        editName[15];          // +0xA58 the profile name being entered
    int          editNameCursor;        // +0xA78
    int          tableSlot;             // +0xA7C the slot being picked (0-9)
    int          tableNameCursor;       // +0xA80
    std::string  savedTableNames[NUM_TABLE_SLOTS];      // +0xA84 T:\N.cfg
    std::string  defaultTableNames[NUM_TABLE_SLOTS];    // +0xB9C D:\media\tables\N.cfg

    int          gameType;              // +0xCB4 GAME_*
    int          editGameType;          // +0xCB8 the game type of the table in the editor
    float        fanAngle;              // +0xCBC the ceiling fans turn at 5x this
    float        friction;              // +0xCC0
    float        ballMass;              // +0xCC4
    float        stickForce;            // +0xCC8
    float        maxBallSpeed;          // +0xCCC
    float        savedFriction;         // +0xCD0 friction is zeroed during the intro
    int          flickerFilter;         // +0xCD4 0-5
    bool         bSoftFilter;           // +0xCD8
    D3DXVECTOR3  pieceOffset;           // +0xCDC scratch: a point of a piece, rotated (BuildTableBarriers/Pockets)
    D3DXVECTOR3  camEye;                // +0xCE8 the table camera's position
};                                      // 0xCF4 bytes

// ---------------------------------------------------------------------------
extern CGame        g_Game;
extern SBall        g_Balls[MAX_BALLS];
extern SBall        g_RackBalls[NUM_RACK_BALLS];
extern SPocket      g_Pockets[MAX_POCKETS];
extern int          g_NumPockets;
extern D3DXVECTOR3  g_Barriers[MAX_BARRIERS];
extern int          g_NumBalls;
extern float        g_BallDiameter;
extern float        g_BallRadius;
extern float        g_TableHeight;
extern float        g_PocketRadius;
extern int          g_NumBarriers;
extern float        g_ShotPower;
extern float        g_CueDistance, g_CueMinPull, g_CueMaxPull;
extern float        g_CamHeight, g_CamHeightMin, g_CamHeightMax, g_CamHeightSpeed;
extern float        g_CamAngle, g_CamDefaultAngle, g_CamAngleSpeed;
extern float        g_CamDistance, g_CamDistanceMin, g_CamDistanceMax;
extern bool         g_bBallsStopped;
extern D3DXVECTOR3  g_AimDir;
extern D3DXVECTOR3  g_CamTarget;
extern float        g_CuePitch;
extern float        g_RollFactor;
extern int          g_ViewMode;
extern int          g_unknownBA1F4, g_EditorScreen, g_unknownBA298;
extern bool         g_bCueMode;
extern const float  kRackShape16X[16], kRackShape16Z[16];
extern const float  kRackShape10X[10], kRackShape10Z[10];
extern float        g_RackX[NUM_RACK_BALLS];
extern float        g_RackZ[NUM_RACK_BALLS];
extern D3DLIGHT8    g_Lights[3];
extern LPDIRECT3DTEXTURE8 g_BallTextures[16];
extern LPDIRECT3DTEXTURE8 g_RedBallTexture;
extern LPDIRECT3DTEXTURE8 g_BlueBallTexture;
extern LPDIRECT3DTEXTURE8 g_BlackBallTexture;
extern CSound       g_Sound;

extern CMesh g_BallMesh, g_ShadowMesh, g_UnusedMesh, g_PocketMesh, g_CueMesh, g_SkyboxMesh,
             g_RoofMesh, g_BarTableMesh, g_BarStoolMesh, g_OverLightMesh, g_DartboardMesh,
             g_PoolTableMesh, g_TableChairMesh, g_PosterMeshes[5], g_TrashMesh, g_DeerMesh,
             g_DeerBackMesh, g_DeerLabelMeshes[2], g_LaptopMesh, g_XbinsMesh, g_CrierMesh,
             g_BeerFridgeMesh, g_FanMesh, g_OutsideMesh, g_WindowMesh, g_GlassMesh, g_DoorMesh,
             g_TapMesh, g_SinkMesh, g_XboxMesh, g_ArrowMesh;

// ---------------------------------------------------------------------------
// Main.cpp
void Update();
void Render();

// Graphics.cpp
void InitD3D();
void LoadBallTextures();
void LoadTextures();
void CreateQuadBuffers();
void DrawRect(float x, float y, float w, float h, LPDIRECT3DTEXTURE8 pTexture);

// Physics.cpp
void RackBalls();
float BallBallCollisionTime(int a, int b);
void BallBallCollide(int a, int b);
void MoveBall(int ball, float time);
void UpdateBallInPocket(int ball);
void ClampBallToPocket(int ball);
void CollideBallWithBarriers(int ball);
void UpdatePhysics();

// Table.cpp
void LoadCustomPieces();
void LoadDefaultTable(int slot, STablePiece* pHead);
void LoadSavedTable(int slot, STablePiece* pHead);
void SaveTable(int slot);
void LoadDefaultTableNames();
void LoadSavedTableName(int slot);
void LoadSavedTableNames();
void BuildTableBarriers();
void BuildTablePockets();

// Game.cpp, GameRender.cpp
void UpdateGame();
void RenderGame();
void RenderRoom();
void DrawAimGuide();
void RenderMusicHud();

// Menu.cpp, MenuRender.cpp
void UpdateMenu();
void RenderMenu();

// Editor.cpp, EditorRender.cpp
void UpdateEditor();
void RenderEditor();

// Settings.cpp
void LoadSettings();
void SaveSettings();

#endif
