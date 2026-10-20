// The game's loose globals (everything not in g_Game), with their original
// addresses. Initialized ones keep their original initial values.
#include "Game.h"

float       g_ShotPower;                // 0x000BA0A0 builds up while the shot is drawn back
float       g_CamHeightMin;             // 0x000BA0A4
float       g_PocketRadius;             // 0x000BA0A8
float       g_CamDistanceMax;           // 0x000BA0AC
float       g_CueMinPull;               // 0x000BA0B0
float       g_CueDistance;              // 0x000BA0B4 the cue's distance from the cue ball
int         g_unknownBA1F4;             // 0x000BA1F4
float       g_CamDistanceMin;           // 0x000BA1F8
int         g_NumBalls;                 // 0x000BA1FC
float       g_CamHeightMax;             // 0x000BA200
float       g_CamHeight;                // 0x000BA204
int         g_NumBarriers;              // 0x000BA208
bool        g_bAiming;                  // 0x000BA20C TODO: verify
float       g_CamAngle;                 // 0x000BA210 around the table, radians
float       g_CueMaxPull;               // 0x000BA214
float       g_CamDefaultAngle;          // 0x000BA278
float       g_CamDistance;              // 0x000BA27C
int         g_ViewMode;                 // 0x000BA280 TODO: verify
int         g_unknownBA284;             // 0x000BA284
float       g_CamHeightSpeed;           // 0x000BA288
float       g_CamAngleSpeed;            // 0x000BA28C
bool        g_unknownBA290;             // 0x000BA290
int         g_NumPockets;               // 0x000BA294
int         g_unknownBA298;             // 0x000BA298

D3DXVECTOR3 g_Barriers[MAX_BARRIERS];   // 0x000BA3F0 the table's cushion lines
SPocket     g_Pockets[MAX_POCKETS];     // 0x000BB6B0

// 0x000B6A88
// Where each ball is racked, in table units; set per table (its .cfg).
float g_RackX[NUM_RACK_BALLS] = { -0.8f, 0.7f, 0.94f, 0.76f, 0.88f, 0.94f, 0.88f, 0.88f,
                                  0.82f, 0.88f, 0.82f, 0.82f, 0.94f, 0.94f, 0.76f, 0.94f };
float g_RackZ[NUM_RACK_BALLS] = { 0.0f, 0.0f, -0.07f, -0.035f, -0.035f, 0.14f, 0.105f, -0.105f,
                                  0.0f, 0.035f, -0.07f, 0.07f, 0.07f, 0.0f, 0.035f, -0.14f };

float g_BallDiameter = 2.25f;           // 0x000B6C18
float g_unknownB6C1C = 1.0f;            // 0x000B6C1C
float g_TableHeight = 5.0f;             // 0x000B6C20
float g_BallRadius = g_BallDiameter * 0.5f;   // 0x000BB9E8 (set by a static initializer)
