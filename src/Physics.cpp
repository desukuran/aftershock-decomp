// The balls: racking, movement, collisions and pockets.
#include "Game.h"
#include <math.h>

SBall g_Balls[MAX_BALLS];                   // 0x000BD298
SBall g_RackBalls[NUM_RACK_BALLS];          // 0x000BCA70

// 0x00012FA0 (and inlined in the arrays' static initializers)
SBall::SBall()
{
    pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    dir = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    needsReplace = false;
    onTable = true;
    number = 0;
    type = BALL_CUE;
}

// 0x00013B60
// Resets every ball and racks the first g_NumBalls at g_RackX/Z for the
// current game type. Quirks kept: the "random" directions are always zero
// (integer division), 23 of the 40 balls are reset, and Mini-Golf leaves the
// balls' types as the last game set them.
void RackBalls()
{
    g_Game.bPlacingBall = false;
    g_Game.bUnstickBalls = true;
    g_Game.unstickCount = 0;

    int nCounter = 0;   // unused
    for (int i = 0; i < 23; i++)
    {
        SBall& ball = g_Balls[i];
        ball.dir = D3DXVECTOR3((float)((rand() % 10) / 10), 0.0f, (float)((rand() % 10) / 10));
        D3DXVec3Normalize(&ball.dir, &ball.dir);
        if (++nCounter > 3)
            nCounter = 0;
        D3DXMatrixIdentity(&ball.rot);
        ball.inPocket = 0;
        ball.pocket = 0;
        ball.speed = 0.0f;
        ball.pos = D3DXVECTOR3(-5.0f, 0.0f, 0.0f);
        ball.onTable = false;
        ball.needsReplace = false;
    }

    int nGameType = g_Game.gameType;
    for (int i = 0; i < g_NumBalls; i++)
    {
        SBall& ball = g_Balls[i];
        ball.pos.x = g_RackX[i];
        ball.pos.z = g_RackZ[i];
        ball.pos.y = g_BallRadius + g_TableHeight;

        if (nGameType == GAME_9BALL)
        {
            ball.onTable = true;
            ball.number = i;
            ball.type = (i == 0) ? BALL_CUE : BALL_GROUP5;
        }
        else if (nGameType == GAME_8BALL)
        {
            ball.number = i;
            ball.onTable = true;
            if (i == 0)
                ball.type = BALL_CUE;
            else if (i < 8)
                ball.type = BALL_SOLID;
            else if (i == 8)
                ball.type = BALL_EIGHT;
            else
                ball.type = BALL_STRIPE;
        }
        else if (nGameType == GAME_REDVSBLUE)
        {
            ball.onTable = true;
            if (i == 0)
            {
                ball.number = 0;
                ball.type = BALL_CUE;
            }
            else if (i < 8)
            {
                ball.number = BALLTEX_RED;
                ball.type = BALL_SOLID;
            }
            else if (i == 8)
            {
                ball.number = BALLTEX_BLACK;
                ball.type = BALL_EIGHT;
            }
            else
            {
                ball.number = BALLTEX_BLUE;
                ball.type = BALL_STRIPE;
            }
        }
        else if (nGameType == GAME_GOLF)
        {
            // Only the cue ball and one other.
            ball.onTable = i < 2;
            g_Balls[0].number = 0;
            g_Balls[1].number = 1;
        }
        else if (nGameType == GAME_CUTTHROAT)
        {
            g_Game.numPlayers = 3;
            ball.onTable = true;
            if (i == 0)
            {
                ball.type = BALL_CUE;
                ball.number = 0;
            }
            else if (i < 6)
            {
                ball.type = BALL_GROUP4;
                ball.number = BALLTEX_RED;
            }
            else if (i < 11)
            {
                ball.type = BALL_GROUP5;
                ball.number = BALLTEX_BLUE;
            }
            else
            {
                ball.type = BALL_GROUP6;
                ball.number = BALLTEX_BLACK;
            }
        }
    }

    g_Game.curPlayer = 0;
    g_Game.players[0].group = 0;
    g_Game.players[1].group = 0;
    g_Game.players[2].group = 0;
    g_Game.players[3].group = 0;
    g_Game.shots[0] = 0;
    g_Game.shots[1] = 0;
    g_Game.bShotInProgress = false;
    g_Game.bPottedOwnBall = g_Game.bFirstHitDone = g_Game.bHitLowestFirst = g_Game.bHitOwnGroupFirst = false;
}

// 0x00013F00
// When during this frame's move (0..1) balls a and b touch, or 0 if they
// don't. The moving-sphere test from "Pool Hall Lessons" (Gamasutra), in the
// table plane; quirk kept: the "moving apart" early out is skipped whenever
// either ball is moving at all.
float BallBallCollisionTime(int a, int b)
{
    SBall& A = g_Balls[a];
    SBall& B = g_Balls[b];
    D3DXVECTOR3 vMove = A.dir * A.speed * g_Game.frameTime - B.dir * B.speed * g_Game.frameTime;
    float dx = B.pos.x - A.pos.x;
    float dz = B.pos.z - A.pos.z;
    float fMoveLen = sqrtf(vMove.x * vMove.x + vMove.y * vMove.y + vMove.z * vMove.z);
    float fDist = sqrtf(dx * dx + dz * dz);

    if (fDist - g_BallDiameter <= fMoveLen &&
        (dz * vMove.z + dx * vMove.x + vMove.y * 0.0f > 0.0f || A.speed > 0.0f || B.speed > 0.0f))
    {
        D3DXVECTOR3 n;
        D3DXVec3Normalize(&n, &vMove);
        float d = n.y * 0.0f + n.x * dx + n.z * dz;
        float f = sqrtf(fDist * fDist - d * d);
        if (f <= g_BallDiameter)
        {
            float t = d - sqrtf(g_BallDiameter * g_BallDiameter - f * f);
            if (t <= fMoveLen)
                return t / fMoveLen;
        }
    }
    return 0.0f;
}

// 0x000140E0
// An elastic collision of two equal balls along the line between them.
void BallBallCollide(int a, int b)
{
    SBall& A = g_Balls[a];
    SBall& B = g_Balls[b];
    D3DXVECTOR3 n(A.pos.x - B.pos.x, 0.0f, A.pos.z - B.pos.z);
    D3DXVec3Normalize(&n, &n);

    D3DXVECTOR3 va = A.dir * A.speed * g_Game.frameTime;
    D3DXVECTOR3 vb = B.dir * B.speed * g_Game.frameTime;
    float p = ((D3DXVec3Dot(&va, &n) - D3DXVec3Dot(&vb, &n)) * 2.0f) / (g_Game.ballMass * 2.0f);
    va -= n * p;
    vb += n * p;

    A.dir = va;
    B.dir = vb;
    D3DXVec3Normalize(&A.dir, &A.dir);
    D3DXVec3Normalize(&B.dir, &B.dir);
    float fInvTime = 1.0f / g_Game.frameTime;
    A.speed = sqrtf(va.x * va.x + va.y * va.y + va.z * va.z) * fInvTime;
    B.speed = sqrtf(vb.x * vb.x + vb.y * vb.y + vb.z * vb.z) * fInvTime;
}

// 0x000143C0
// Moves a ball by `time` of this frame's travel. While bUnstickBalls is on
// (from the rack until player 2's first shot, or 30 overlaps), a ball that
// ends up inside another is put back and bounced off it. Quirks kept: that
// bounce sets this ball's speed but not its direction, and the retry loop
// always stops after one pass over the balls.
void MoveBall(int ball, float time)
{
    SBall& B = g_Balls[ball];

    if (g_Game.state == STATE_INTRO || g_Game.gameType == GAME_GOLF || !g_Game.bUnstickBalls)
    {
        B.pos += B.dir * B.speed * g_Game.frameTime * time;
        return;
    }

    if (g_Game.shots[1] > 0)
        g_Game.bUnstickBalls = false;

    D3DXVECTOR3 vOld = B.pos;
    bool bClear = false;
    bool bAllChecked = false;
    while (time > 0.0f && !bClear && !bAllChecked)
    {
        B.pos += B.dir * B.speed * g_Game.frameTime * time;
        bClear = false;
        bAllChecked = false;
        for (int j = 0; j < g_NumBalls; j++)
        {
            SBall& O = g_Balls[j];
            if (j != ball && O.onTable)
            {
                D3DXVECTOR3 d = B.pos - O.pos;
                if (sqrtf(d.x * d.x + d.y * d.y + d.z * d.z) > g_BallRadius + g_BallRadius)
                {
                    bClear = true;
                }
                else
                {
                    if (++g_Game.unstickCount > 30)
                        g_Game.bUnstickBalls = false;
                    B.pos = vOld;

                    D3DXVECTOR3 n(B.pos.x - O.pos.x, 0.0f, B.pos.z - O.pos.z);
                    D3DXVec3Normalize(&n, &n);
                    D3DXVECTOR3 vb = B.dir * B.speed * g_Game.frameTime;
                    D3DXVECTOR3 vo = O.dir * O.speed * g_Game.frameTime;
                    float p = ((D3DXVec3Dot(&vb, &n) - D3DXVec3Dot(&vo, &n)) * 2.0f) / (g_Game.ballMass * 2.0f);
                    vb -= n * p;
                    vo += n * p;
                    O.dir = vo;
                    D3DXVec3Normalize(&O.dir, &O.dir);
                    float fInvTime = 1.0f / g_Game.frameTime;
                    B.speed = sqrtf(vb.x * vb.x + vb.y * vb.y + vb.z * vb.z) * fInvTime;
                    if (O.speed < 20.0f)
                        O.speed = D3DXVec3Length(&vo) * fInvTime;
                    time -= 0.1f;
                    bClear = false;
                }
            }
            if (j >= g_NumBalls - 1)
                bAllChecked = true;
        }
    }
}

// 0x000159E0
// Keeps a ball that is dropping into a pocket inside the hole.
void ClampBallToPocket(int ball)
{
    SBall& B = g_Balls[ball];
    int p = B.pocket;
    if (p < MAX_POCKETS + 1 && p >= 0)      // (one past the end of g_Pockets)
    {
        float x = B.pos.x < g_Pockets[p].pos.x + g_PocketRadius ? B.pos.x : g_Pockets[p].pos.x + g_PocketRadius;
        B.pos.x = x;
        B.pos.x = x > g_Pockets[p].pos.x - g_PocketRadius ? x : g_Pockets[p].pos.x - g_PocketRadius;
        float z = B.pos.z < g_Pockets[p].pos.z + g_PocketRadius ? B.pos.z : g_Pockets[p].pos.z + g_PocketRadius;
        B.pos.z = z;
        B.pos.z = z > g_Pockets[p].pos.z - g_PocketRadius ? z : g_Pockets[p].pos.z - g_PocketRadius;
    }
}

// 0x00015A90
// Bounces a ball off the cushion lines in g_Barriers: polylines separated by
// (0,0,0) points. The test is the distance to the segment's line (with a
// rough check that the ball is beside the segment). A line only bounces
// balls travelling toward its right-hand side (it's one way, as the editor's
// help says); the ball is stepped back to the contact, reflected, and moved
// on.
void CollideBallWithBarriers(int ball)
{
    SBall& B = g_Balls[ball];
    D3DXVECTOR3 vZero(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < g_NumBarriers - 1; i++)
    {
        D3DXVECTOR3& p0 = g_Barriers[i];
        D3DXVECTOR3& p1 = g_Barriers[i + 1];
        if (p1 == vZero || p0 == vZero)
        {
            i++;    // skip the separator and the segment after it
            continue;
        }

        D3DXVECTOR3 vToBall(B.pos.x - p0.x, 0.0f, B.pos.z - p0.z);
        D3DXVECTOR3 vSeg(p1.x - p0.x, 0.0f, p1.z - p0.z);
        D3DXVECTOR3 vFromEnd(B.pos.x - p1.x, 0.0f, B.pos.z - p1.z);
        float fToBall = sqrtf(vToBall.x * vToBall.x + vToBall.z * vToBall.z + 0.0f);
        float fSeg = sqrtf(vSeg.x * vSeg.x + vSeg.z * vSeg.z + 0.0f);
        if (fToBall >= fSeg || sqrtf(vFromEnd.x * vFromEnd.x + vFromEnd.z * vFromEnd.z + 0.0f) >= fSeg)
            continue;

        float fAngle = acosf((vToBall.z * vSeg.z + vToBall.x * vSeg.x) / (fSeg * fToBall));
        float fDist = sinf(fAngle) * fToBall;
        if (fDist >= g_BallRadius)
            continue;

        MoveBall(ball, -1.0f);
        MoveBall(ball, fDist / g_BallRadius);

        D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
        D3DXVec3Normalize(&vSeg, &vSeg);
        D3DXVECTOR3 n(vUp.z * vSeg.y - vUp.y * vSeg.z,
                      vUp.x * vSeg.z - vUp.z * vSeg.x,
                      vUp.y * vSeg.x - vSeg.y * vUp.x);
        D3DXVECTOR3 d = B.dir;
        float fDot = d.z * n.z + d.x * n.x;
        if (fDot > 0.0f)
        {
            float fIn = acosf(fDot);
            float fTurn = fIn <= D3DX_PI / 4 ? D3DX_PI - fIn * 2.0f : (D3DX_PI / 2 - fIn) * 2.0f;
            D3DXVECTOR3 c(d.y * n.z - n.y * d.z, d.z * n.x - n.z * d.x, n.y * d.x - d.y * n.x);
            D3DXMATRIX matRot;
            D3DXMatrixRotationY(&matRot, c.y >= 0.0f ? -fTurn : fTurn);
            D3DXVec3TransformCoord(&B.dir, &B.dir, &matRot);
            B.dir.y = 0.0f;
        }
        MoveBall(ball, 1.0f - fDist / g_BallRadius);
    }
}

// 0x00024E50
// One frame of the balls: take the shot if the cue has hit the cue ball,
// slow and roll every ball, pockets and cushions, ball-ball collisions, and
// when everything has stopped, the next player's turn.
void UpdatePhysics()
{
    int nShooter = g_Game.curPlayer;
    g_Game.bBallsMoving = false;

    // The cue reached the ball while everything was still: shoot.
    if (g_CueDistance < g_BallRadius && g_bBallsStopped)
    {
        if (g_Game.curPlayer == 0)
            g_Game.shots[0]++;
        else if (g_Game.curPlayer == 1)
            g_Game.shots[1]++;

        D3DXVECTOR3* pDir;
        if (g_Game.gameType == GAME_GOLF)
        {
            // Mini-Golf: every player has a ball of their own.
            g_Balls[g_Game.curPlayer].speed = g_ShotPower / g_Game.stickForce;
            pDir = &g_Balls[nShooter].dir;
            *pDir = D3DXVECTOR3(0.0f - g_AimDir.x, 0.0f - g_AimDir.y, 0.0f - g_AimDir.z);
        }
        else
        {
            // Every other game: always the same strength.
            g_Balls[0].speed = 506.0f / g_Game.stickForce;
            g_Balls[0].dir = D3DXVECTOR3(0.0f - g_AimDir.x, 0.0f - g_AimDir.y, 0.0f - g_AimDir.z);
            pDir = &g_Balls[0].dir;
        }
        D3DXVec3Normalize(pDir, pDir);

        g_CueDistance = g_BallRadius * 2.0f;
        g_unknownBA1F4 = 0;
        g_bBallsStopped = false;
        g_ShotPower = 0.0f;
        g_bCueMode = false;
        g_Game.bShotInProgress = true;
        g_Game.bPottedOwnBall = false;
        g_Game.unknown380 = 0;
        g_Sound.Play(SOUND_CUE0);
        g_Game.bFirstHitDone = false;
        g_Game.bHitLowestFirst = false;
        g_Game.bHitOwnGroupFirst = false;
    }

    g_bBallsStopped = true;
    for (int i = 0; i < g_NumBalls; i++)
    {
        SBall& B = g_Balls[i];
        if (!B.onTable)
            continue;

        B.speed = (1.0f - g_Game.frameTime * g_Game.friction) * B.speed;
        if (B.speed >= 0.5f)
        {
            g_bBallsStopped = false;
            g_Game.bBallsMoving = true;
        }
        else
        {
            B.speed = 0.0f;
        }

        // Roll: spin around the horizontal axis across the direction.
        D3DXMATRIX matTurn, matRoll;
        D3DXVECTOR3 vAxis;
        D3DXMatrixRotationY(&matTurn, D3DX_PI / 2);
        D3DXVec3TransformCoord(&vAxis, &B.dir, &matTurn);
        D3DXMatrixRotationAxis(&matRoll, &vAxis, B.speed * g_RollFactor * g_Game.frameTime);
        D3DXMatrixMultiply(&B.rot, &B.rot, &matRoll);

        UpdateBallInPocket(i);
        if (B.inPocket == 0)
            CollideBallWithBarriers(i);
        else
            ClampBallToPocket(i);
    }

    // Ball-ball collisions (over the first 23 balls, whatever g_NumBalls is).
    int bMoved[23] = { 0 };
    for (int i = 0; i < 23; i++)
    {
        if (!g_Balls[i].onTable)
            continue;
        for (int j = 0; j < 23; j++)
        {
            if (!g_Balls[j].onTable || i == j)
                continue;
            float t = BallBallCollisionTime(i, j);
            if (t <= 0.0f)
                continue;

            bMoved[j] = 1;
            bMoved[i] = 1;
            MoveBall(i, t);
            MoveBall(j, t);

            // The first contact of the shot decides the fouls.
            if (!g_Game.bFirstHitDone)
            {
                for (int k = 1; k < g_NumBalls; k++)
                {
                    if (g_Balls[k].onTable)
                    {
                        int nGroup = g_Game.players[g_Game.curPlayer].group;
                        g_Game.bHitOwnGroupFirst = nGroup == 0 ? true : g_Balls[j].type == nGroup;
                        g_Game.bHitLowestFirst = g_Balls[j].number <= g_Balls[k].number;
                        break;
                    }
                }
            }
            g_Game.bFirstHitDone = true;
            if (g_Game.state != STATE_INTRO)
                g_Sound.Play(SOUND_CUE1);

            BallBallCollide(i, j);
            MoveBall(j, 1.0f - t);
            MoveBall(i, 1.0f - t);
        }
    }
    for (int i = 0; i < 23; i++)
    {
        if (!bMoved[i])
            MoveBall(i, 1.0f);
    }

    if (!g_bBallsStopped)
        return;

    // Everything has stopped after a shot: whose turn next?
    if (g_Game.bShotInProgress && !g_Game.bPottedOwnBall)
    {
        if ((g_Game.gameType == GAME_8BALL || g_Game.gameType == GAME_REDVSBLUE) && !g_Game.bHitOwnGroupFirst)
            g_Balls[0].needsReplace = true;     // foul: ball in hand

        if (++g_Game.curPlayer > g_Game.numPlayers - 1)
            g_Game.curPlayer = 0;

        if (g_Game.gameType == GAME_GOLF)
        {
            // Skip players whose ball is in; when both are, fewer shots wins.
            if (!g_Balls[g_Game.curPlayer].onTable)
            {
                if (++g_Game.curPlayer > g_Game.numPlayers - 1)
                    g_Game.curPlayer = 0;
                if (!g_Balls[g_Game.curPlayer].onTable)
                {
                    g_Game.state = STATE_GAMEOVER;
                    int nLoser;
                    if (g_Game.shots[0] < g_Game.shots[1])
                    {
                        g_Game.nextPlayer = 0;
                        g_Game.profiles[g_Game.players[0].profile].wins++;
                        nLoser = g_Game.players[1].profile;
                    }
                    else
                    {
                        g_Game.nextPlayer = 1;
                        g_Game.profiles[g_Game.players[1].profile].wins++;
                        nLoser = g_Game.players[0].profile;
                    }
                    g_Game.profiles[nLoser].losses++;
                }
            }
        }
        else if (g_Game.gameType == GAME_CUTTHROAT)
        {
            // Skip players with no balls left. Quirk kept: whether everyone
            // is still in is decided by the current player's group.
            bool bAlive[MAX_PLAYERS];
            int nGroup = g_Game.players[g_Game.curPlayer].group;
            for (int k = 0; k < MAX_PLAYERS; k++)
            {
                bAlive[k] = false;
                if (nGroup == 0)
                {
                    bAlive[k] = true;
                }
                else
                {
                    for (int b = 1; b < g_NumBalls; b++)
                    {
                        if (g_Balls[b].onTable && g_Balls[b].type == g_Game.players[k].group)
                        {
                            bAlive[k] = true;
                            break;
                        }
                    }
                }
            }
            for (int n = 0; !bAlive[g_Game.curPlayer] && n < 5; n++)
            {
                if (++g_Game.curPlayer > g_Game.numPlayers - 1)
                    g_Game.curPlayer = 0;
            }
        }

        if (g_Game.curPlayer > g_Game.numPlayers - 1)
            g_Game.curPlayer = 0;
        g_Game.bPottedOwnBall = false;
        g_Game.turnStartTime = GetTickCount();
    }
    if (g_bBallsStopped)
        g_Game.bShotInProgress = false;
}

// Has the ball dropped through the table (into the pocket)?
static bool BelowTable(const SBall& B)
{
    return B.pos.y < g_TableHeight - g_BallDiameter;
}

// Takes a pocketed ball off the table.
static void RemoveBall(SBall& B)
{
    B.pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    B.onTable = false;
    g_Sound.Play(SOUND_POCKET);
}

// 8 ball and Red vs Blue: the first ball sunk on an open table decides the
// groups (type 1 to the shooter, else type 2).
static void AssignGroups(int nType)
{
    int nOther = g_Game.curPlayer == 0 ? 1 : 0;
    if (nType == BALL_SOLID)
    {
        g_Game.players[g_Game.curPlayer].group = BALL_SOLID;
        g_Game.players[nOther].group = BALL_STRIPE;
    }
    else
    {
        g_Game.players[g_Game.curPlayer].group = BALL_STRIPE;
        g_Game.players[nOther].group = BALL_SOLID;
    }
}

// The eight ball went down: the shooter wins if their group is cleared,
// else loses. Ends the game.
static void EightBallSunk()
{
    bool bCleared = true;
    for (int i = 1; i < 16; i++)
    {
        if (g_Balls[i].onTable && g_Balls[i].type == g_Game.players[g_Game.curPlayer].group)
            bCleared = false;
    }

    int nCur = g_Game.curPlayer;
    if (g_Game.players[nCur].group == 0 || !bCleared)
    {
        g_Game.nextPlayer = nCur + 1;
        if (g_Game.nextPlayer > 1)
            g_Game.nextPlayer = 0;
        g_Game.profiles[g_Game.players[nCur].profile].losses++;
        g_Game.profiles[g_Game.players[g_Game.nextPlayer].profile].wins++;
    }
    else
    {
        int nOther = nCur + 1;
        if (nOther > 1)
            nOther = 0;
        g_Game.profiles[g_Game.players[nCur].profile].wins++;
        g_Game.profiles[g_Game.players[nOther].profile].losses++;
        g_Game.nextPlayer = nCur;
    }
    g_Game.state = STATE_GAMEOVER;
    g_Sound.Play(SOUND_POCKET);
}

// Cut throat: a player without a group who sinks a ball of type t takes
// another group nobody has (t's neighbours first, t itself last).
static void AssignCutThroatGroup(int& nMine, int nOther1, int nOther2, int t)
{
    if (t == BALL_GROUP4)
    {
        if (nOther1 == BALL_GROUP5 || nOther2 == BALL_GROUP5)
            nMine = (nOther1 == BALL_GROUP6 || nOther2 == BALL_GROUP6) ? BALL_GROUP4 : BALL_GROUP6;
        else
            nMine = BALL_GROUP5;
    }
    if (t == BALL_GROUP5)
    {
        if (nOther1 == BALL_GROUP4 || nOther2 == BALL_GROUP4)
            nMine = (nOther1 == BALL_GROUP6 || nOther2 == BALL_GROUP6) ? BALL_GROUP5 : BALL_GROUP6;
        else
            nMine = BALL_GROUP4;
    }
    if (t == BALL_GROUP6)
    {
        if (nOther1 == BALL_GROUP4 || nOther2 == BALL_GROUP4)
            nMine = (nOther1 == BALL_GROUP5 || nOther2 == BALL_GROUP5) ? BALL_GROUP6 : BALL_GROUP5;
        else
            nMine = BALL_GROUP4;
    }
}

// 0x00014910
// Pockets and the rules. A ball within g_PocketRadius of a pocket starts
// falling (gravity, curving down); once it is below the table it is off,
// and the game's rules decide fouls, groups, turns and the winner.
void UpdateBallInPocket(int ball)
{
    SBall& B = g_Balls[ball];

    if (B.inPocket == 0)
    {
        for (int p = 0; p < g_NumPockets; p++)
        {
            float dx = g_Pockets[p].pos.x - B.pos.x;
            float dz = g_Pockets[p].pos.z - B.pos.z;
            if (sqrtf(dx * dx + dz * dz) <= g_PocketRadius)
            {
                B.inPocket = 1;
                B.pocket = p;
            }
        }
        if (B.inPocket == 0)
            return;
    }

    // Falling: turn the direction downward, accelerate.
    float fUp = 1.0f;
    if (g_unknownBA298 != 0)
        fUp = -1.0f;
    if (fabsf(B.dir.y) < 0.95f)
    {
        D3DXVECTOR3 vAxis(B.dir.z * fUp - B.dir.y * 0.0f, B.dir.x * 0.0f - B.dir.z * 0.0f, B.dir.y * 0.0f - B.dir.x * fUp);
        D3DXMATRIX matRot;
        D3DXMatrixRotationAxis(&matRot, &vAxis, g_Game.frameTime * D3DX_PI);
        D3DXVec3TransformCoord(&B.dir, &B.dir, &matRot);
    }
    B.speed = g_Game.frameTime * 9.8f + B.speed;
    if (B.pos.y > 10.0f && g_unknownBA298 != 0)
    {
        B.speed = 0.0f;
        B.pos.y = 0.0f;
    }

    // Outside a game (the intro, the editor): no rules.
    if (g_Game.state != STATE_PLAYING)
    {
        if (BelowTable(B))
            RemoveBall(B);
        return;
    }

    if (g_Game.gameType == GAME_8BALL)
    {
        if (B.type == BALL_CUE && BelowTable(B))
        {
            B.onTable = false;
            B.needsReplace = true;
            g_Sound.Play(SOUND_POCKET);
        }
        if (B.type == BALL_EIGHT)
        {
            if (BelowTable(B))
                EightBallSunk();
            return;
        }
        // Quirk kept: this runs for the cue ball too, so a scratch on an
        // open table gives the shooter the stripes.
        if (g_Game.players[0].group == g_Game.players[1].group)
        {
            AssignGroups(B.type);
        }
        else
        {
            g_Game.bPottedOwnBall = B.type == g_Game.players[g_Game.curPlayer].group;
            if (!g_Game.bHitOwnGroupFirst)
            {
                g_Game.bPottedOwnBall = false;
                g_Balls[0].needsReplace = true;
            }
        }
        if (BelowTable(B))
            RemoveBall(B);
        return;
    }

    if (g_Game.gameType == GAME_GOLF)
    {
        if (!BelowTable(B))
            return;
        B.onTable = false;      // (its position is left as it is)
        g_Sound.Play(SOUND_POCKET);
    }

    if (g_Game.gameType == GAME_9BALL)
    {
        if (B.type == BALL_CUE && BelowTable(B))
        {
            B.onTable = false;
            B.needsReplace = true;
            g_Sound.Play(SOUND_POCKET);
        }
        if (B.number == 9)
        {
            // Sinking the 9 wins if 1-8 are gone, and loses otherwise.
            if (!BelowTable(B))
                return;
            int nCur = g_Game.curPlayer;
            if (!g_Balls[1].onTable && !g_Balls[2].onTable && !g_Balls[3].onTable && !g_Balls[4].onTable &&
                !g_Balls[5].onTable && !g_Balls[6].onTable && !g_Balls[7].onTable && !g_Balls[8].onTable)
            {
                int nOther = nCur + 1;
                if (nOther > 1)
                    nOther = 0;
                g_Game.profiles[g_Game.players[nCur].profile].wins++;
                g_Game.profiles[g_Game.players[nOther].profile].losses++;
                g_Game.nextPlayer = nCur;
            }
            else
            {
                g_Game.nextPlayer = nCur + 1;
                if (g_Game.nextPlayer > 1)
                    g_Game.nextPlayer = 0;
                g_Game.profiles[g_Game.players[nCur].profile].losses++;
                g_Game.profiles[g_Game.players[g_Game.nextPlayer].profile].wins++;
            }
            g_Game.state = STATE_GAMEOVER;
            g_Sound.Play(SOUND_POCKET);
            return;
        }
        if (!g_Game.bHitLowestFirst)
            g_Balls[0].needsReplace = true;
        g_Game.bPottedOwnBall = g_Game.bHitLowestFirst;
        if (BelowTable(B))
            RemoveBall(B);
    }

    if (g_Game.gameType == GAME_REDVSBLUE)
    {
        if (B.type == BALL_CUE && BelowTable(B))
        {
            B.needsReplace = true;
            B.onTable = false;
            g_Sound.Play(SOUND_POCKET);
        }
        int t = B.type;
        if (t == BALL_EIGHT)
        {
            if (BelowTable(B))
                EightBallSunk();
            return;
        }
        if (g_Game.players[0].group == g_Game.players[1].group)
        {
            AssignGroups(t);
        }
        else
        {
            // Colored pockets only take their own color: an own ball sunk in
            // the other color's pocket is put back.
            int nMine = g_Game.players[g_Game.curPlayer].group;
            int nColor = g_Pockets[B.pocket].color;
            g_Game.bPottedOwnBall = t == nMine && (t == nColor || nColor == 0);
            if (t == nMine && t != nColor && nColor != 0)
                B.needsReplace = true;
            if (!g_Game.bHitOwnGroupFirst)
            {
                g_Game.bPottedOwnBall = false;
                g_Balls[0].needsReplace = true;
            }
        }
        if (BelowTable(B))
            RemoveBall(B);
    }

    if (g_Game.gameType != GAME_CUTTHROAT)
        return;

    // Cut throat: sink the other players' balls; the last group standing wins.
    if (B.type == BALL_CUE && BelowTable(B))
    {
        B.needsReplace = true;
        B.onTable = false;
        g_Sound.Play(SOUND_POCKET);
    }
    int* pGroup = NULL;
    int nOther1, nOther2;
    switch (g_Game.curPlayer)
    {
    case 0: pGroup = &g_Game.players[0].group; nOther1 = g_Game.players[1].group; nOther2 = g_Game.players[2].group; break;
    case 1: pGroup = &g_Game.players[1].group; nOther1 = g_Game.players[0].group; nOther2 = g_Game.players[2].group; break;
    case 2: pGroup = &g_Game.players[2].group; nOther1 = g_Game.players[0].group; nOther2 = g_Game.players[1].group; break;
    }
    if (pGroup != NULL)
    {
        if (*pGroup == 0)
            AssignCutThroatGroup(*pGroup, nOther1, nOther2, B.type);
        else if (*pGroup != B.type)
            g_Game.bPottedOwnBall = true;   // sank someone else's: shoot again
    }

    if (!BelowTable(B))
        return;
    RemoveBall(B);

    // Which groups still have balls (1-5, 6-10, 11-15)?
    bool bLeft[3] = { false, false, false };
    int nLast = 0;
    for (int i = 1; i < 16; i++)
    {
        if (g_Balls[i].onTable)
        {
            bLeft[(i - 1) / 5] = true;
            nLast = i;
        }
    }
    int nGone = !bLeft[0] + !bLeft[1] + !bLeft[2];
    int nAssigned = (g_Game.players[0].group != 0) + (g_Game.players[1].group != 0) + (g_Game.players[2].group != 0);

    // Two groups given out: the third player gets the last one.
    if (nAssigned == 2)
    {
        bool bTaken[3] = { false, false, false };
        for (int p = 0; p < 3; p++)
        {
            int g = g_Game.players[p].group;
            if (g >= BALL_GROUP4 && g <= BALL_GROUP6)
                bTaken[g - BALL_GROUP4] = true;
        }
        for (int p = 0; p < 3; p++)
        {
            if (g_Game.players[p].group == 0)
            {
                if (!bTaken[0]) g_Game.players[p].group = BALL_GROUP4;
                if (!bTaken[1]) g_Game.players[p].group = BALL_GROUP5;
                if (!bTaken[2]) g_Game.players[p].group = BALL_GROUP6;
            }
        }
    }

    // Two groups cleared: the owner of the last ball standing wins. (With no
    // balls left that is ball 0's type, and everyone loses.)
    if (nGone > 1)
    {
        int nWinType = g_Balls[nLast].type;
        for (int p = 0; p < 3; p++)
        {
            if (g_Game.players[p].group == nWinType)
            {
                g_Game.profiles[g_Game.players[p].profile].wins++;
                g_Game.nextPlayer = p;
            }
            else
            {
                g_Game.profiles[g_Game.players[p].profile].losses++;
            }
        }
        g_Game.state = STATE_GAMEOVER;
    }
}
