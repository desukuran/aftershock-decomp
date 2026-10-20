// Playing: the shooter's controls, ball in hand, and the music buttons.
#include "Game.h"

static void ClampCamDistance()
{
    if (g_CamDistance < g_CamDistanceMin)
        g_CamDistance = g_CamDistanceMin;
    if (g_CamDistance > g_CamDistanceMax)
        g_CamDistance = g_CamDistanceMax;
}

static void ClampCue()
{
    if (g_CueDistance < g_BallRadius * 0.8f)
        g_CueDistance = g_BallRadius * 0.8f;
    if (g_CueDistance > g_CueMaxPull + g_BallRadius)
        g_CueDistance = g_CueMaxPull + g_BallRadius;
}

// 0x00027A50
// The current player's pad aims and shoots: left/right turns the camera
// around the cue ball, up/down zooms, or with A toggled moves the cue
// (pushing it into the ball shoots). A ball to be replaced (a scratch) is
// moved with the stick and put down with A. Start switches to the free
// camera, Back to the menu; the first pad's B, X, Y and Black control the
// music in any state.
void UpdateGame()
{
    int pad = g_Game.players[g_Game.curPlayer].pad;
    if (pad > 3)
        pad = 0;
    XBGAMEPAD* pPad = g_Game.input.GetPad(pad);

    if (g_Game.state == STATE_PLAYING)
    {
        int nReplace = 0;
        bool bBallInHand = false;
        for (int i = 0; i < g_NumBalls; i++)
        {
            if (g_Balls[i].needsReplace)
            {
                nReplace = i;
                if (!g_Game.bBallsMoving)
                {
                    g_Balls[i].pos.y = g_BallRadius + g_TableHeight;
                    g_Game.bPlacingBall = true;
                    g_Balls[i].onTable = true;
                    g_Balls[i].inPocket = 0;
                    g_bBallsStopped = g_Game.bBallsMoving;
                    g_Balls[i].speed = 0.0f;
                    bBallInHand = true;
                }
                break;
            }
        }

        if (!bBallInHand)
        {
            if (!g_bBallsStopped)
            {
                if (g_Game.bPlacingBall)
                {
                    if (g_Game.bBallsMoving)
                        goto Music;
                    bBallInHand = true;
                }
            }
            else
            {
                g_Game.bPlacingBall = false;
            }
        }

        if (bBallInHand)
        {
            // Ball in hand, within 10 of the table's pieces.
            SBall& B = g_Balls[nReplace];
            if (g_Game.input.LeftThumbUp(pad) != 0.0f)
                B.pos.z = pPad->fY1 * g_Game.frameTime * 10.0f + B.pos.z;
            if (g_Game.input.LeftThumbDown(pad) != 0.0f)
                B.pos.z = pPad->fY1 * g_Game.frameTime * 10.0f + B.pos.z;
            if (g_Game.input.LeftThumbLeft(pad) != 0.0f)
                B.pos.x = pPad->fX1 * g_Game.frameTime * 10.0f + B.pos.x;
            if (g_Game.input.LeftThumbRight(pad) != 0.0f)
                B.pos.x = pPad->fX1 * g_Game.frameTime * 10.0f + B.pos.x;
            if (g_Game.input.APressed(pad))
            {
                B.needsReplace = false;
                B.pocket = 0;
                B.inPocket = 0;
                g_Game.bPlacingBall = false;
            }
            if (B.pos.x < g_Game.tableMinX - 10.0f)
                B.pos.x = g_Game.tableMinX - 10.0f;
            if (B.pos.z < g_Game.tableMinZ - 10.0f)
                B.pos.z = g_Game.tableMinZ - 10.0f;
            if (B.pos.x > g_Game.tableMaxX + 10.0f)
                B.pos.x = g_Game.tableMaxX + 10.0f;
            if (B.pos.z > g_Game.tableMaxZ + 10.0f)
                B.pos.z = g_Game.tableMaxZ + 10.0f;
        }
        else
        {
            g_CamDistanceMin = 20.0f;
            g_CamDistanceMax = 70.0f;
            g_CamAngleSpeed = 0.0f;
            if (GetTickCount() - g_Game.lastInputTime >= 300)
            {
                if (g_Game.input.StartDown(pad) && ++g_ViewMode > 1)
                    g_ViewMode = 0;
                if (g_Game.input.BackDown(pad))
                    g_Game.state = STATE_MENU;
                g_Game.lastInputTime = GetTickCount();
            }
            g_Game.unknownA18 = g_TableHeight * 15.0f;

            if (g_Game.players[g_Game.curPlayer].pad < 4 && g_ViewMode == 0)
            {
                // Turning around the ball starts the aim over.
                if (g_Game.input.LeftThumbLeft(pad) != 0.0f)
                {
                    g_CamAngleSpeed = pPad->fX1;
                    g_CueDistance = g_CueMinPull + g_BallRadius;
                    g_CamHeightSpeed = 0.0f;
                    g_ShotPower = 0.0f;
                    g_Game.aimBall = 0;
                }
                if (g_Game.input.LeftThumbRight(pad) != 0.0f)
                {
                    g_CamAngleSpeed = pPad->fX1;
                    g_CueDistance = g_CueMinPull + g_BallRadius;
                    g_CamHeightSpeed = 0.0f;
                    g_ShotPower = 0.0f;
                    g_Game.aimBall = 0;
                }
                g_CueMaxPull = 10.0f;

                if (!g_bCueMode)
                {
                    if (g_Game.input.LeftThumbUp(pad) != 0.0f)
                    {
                        g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
                        ClampCamDistance();
                    }
                    if (g_Game.input.LeftThumbDown(pad) != 0.0f)
                    {
                        g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
                        ClampCamDistance();
                    }
                }
                else
                {
                    // Pushing the cue in builds up the power; pulling it back resets it.
                    if (g_Game.input.LeftThumbUp(pad) != 0.0f)
                    {
                        g_CamHeightSpeed = 0.0f;
                        g_CamAngleSpeed = 0.0f;
                        g_CueDistance = g_CueDistance - g_Game.frameTime * pPad->fY1 * 140.0f;
                        ClampCue();
                        float fPower = pPad->fY1 * 100.0f + g_ShotPower;
                        if (fPower > 0.0f)
                            fPower = fPower - (1.0f - pPad->fY1) * 100.0f;
                        if (fPower < 0.0f)
                            fPower = 0.0f;
                        g_ShotPower = fPower + 20.0f;
                    }
                    if (g_Game.input.LeftThumbDown(pad) != 0.0f)
                    {
                        g_CamHeightSpeed = 0.0f;
                        g_CamAngleSpeed = 0.0f;
                        g_CueDistance = g_CueDistance - pPad->fY1 * g_Game.frameTime * 140.0f;
                        ClampCue();
                        g_ShotPower = 0.0f;
                    }
                }
            }
            if (g_Game.input.APressed(pad))
                g_bCueMode = !g_bCueMode;

            // Pad 4 ("no pad"): the first pad just looks around.
            if (g_Game.players[g_Game.curPlayer].pad == 4)
            {
                if (g_Game.input.LeftThumbLeft(0) != 0.0f)
                    g_CamDefaultAngle = pPad->fX1 * g_Game.frameTime * 5.0f + g_CamDefaultAngle;
                if (g_Game.input.LeftThumbRight(0) != 0.0f)
                    g_CamDefaultAngle = pPad->fX1 * g_Game.frameTime * 5.0f + g_CamDefaultAngle;
                g_CueMaxPull = 10.0f;
                if (g_Game.input.LeftThumbUp(0) != 0.0f)
                {
                    g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
                    ClampCamDistance();
                }
                if (g_Game.input.LeftThumbDown(0) != 0.0f)
                {
                    g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
                    ClampCamDistance();
                }
            }
        }
    }

    if (g_Game.state == STATE_GAMEOVER)
    {
        // A: rematch, B: back to the menu.
        if (g_Game.input.APressed(0))
        {
            RackBalls();
            g_Game.state = STATE_PLAYING;
        }
        if (g_Game.input.BPressed(0))
            g_Game.state = STATE_MENU;
    }

Music:
    if (g_Game.input.BPressed(0))
    {
        g_Game.musicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
    }
    if (g_Game.input.YPressed(0))
    {
        g_Game.musicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSong();
        g_Game.music.Play(&g_Game);
    }
    if (g_Game.input.XPressed(0))
    {
        g_Game.musicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.PrevSong();
        g_Game.music.Play(&g_Game);
    }
    if (g_Game.input.BlackPressed(0))
    {
        g_Game.musicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSoundtrack();
        g_Game.music.Play(&g_Game);
        g_Game.musicTime = GetTickCount();
    }

    UpdatePhysics();
    if (g_ViewMode == 1)
        g_Game.camera.FreeLook();
}
