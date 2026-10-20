// The table editor ("Create a Table"): its menu, the info screen, placing
// pieces, cushion lines, pockets and balls, test play, saving and loading.
// Pad 1 drives it.
#include "Game.h"

static bool InputReady(int ms)
{
    return (int)(GetTickCount() - g_Game.lastInputTime) >= ms;
}

// The balls (types, textures, count) and player count for the game type.
static void SetupBallsForGameType()
{
    switch (g_Game.gameType)
    {
    case GAME_8BALL:
        g_NumBalls = 16;
        for (int i = 0; i < 16; i++)
        {
            g_Balls[i].number = i;
            if (i == 0)
                g_Balls[0].type = BALL_CUE;
            else if (i < 8)
                g_Balls[i].type = BALL_SOLID;
            else if (i == 8)
                g_Balls[8].type = BALL_EIGHT;
            else
                g_Balls[i].type = BALL_STRIPE;
        }
        g_Game.numPlayers = 2;
        break;
    case GAME_9BALL:
        g_NumBalls = 10;
        for (int i = 0; i < 10; i++)
        {
            g_Balls[i].number = i;
            if (i == 0)
                g_Balls[0].type = BALL_CUE;
        }
        g_Game.numPlayers = 2;
        break;
    case GAME_REDVSBLUE:
        g_NumBalls = 16;
        for (int i = 1; i < 16; i++)
        {
            if (i < 8)
            {
                g_Balls[i].number = BALLTEX_RED;
                g_Balls[i].type = BALL_SOLID;
            }
            if (i == 8)
            {
                g_Balls[8].number = BALLTEX_BLACK;
                g_Balls[8].type = BALL_EIGHT;
            }
            else if (i > 8)
            {
                g_Balls[i].number = BALLTEX_BLUE;
                g_Balls[i].type = BALL_STRIPE;
            }
        }
        g_Game.numPlayers = 2;
        break;
    case GAME_GOLF:
        for (int i = 1; i < 16; i++)
            g_Balls[i].onTable = false;
        g_NumBalls = 2;
        g_Balls[0].onTable = true;
        g_Balls[0].number = 0;
        g_Balls[1].onTable = true;
        g_Balls[1].number = 1;
        g_Game.numPlayers = 1;
        g_Balls[0].type = BALL_CUE;
        g_Balls[1].type = BALL_CUE;
        break;
    case GAME_CUTTHROAT:
        g_NumBalls = 16;
        for (int i = 0; i < 16; i++)
        {
            g_Balls[i].onTable = true;
            if (i == 0)
            {
                g_Balls[0].number = 0;
                continue;
            }
            if (i < 6)
            {
                g_Balls[i].type = BALL_GROUP4;
                g_Balls[i].number = BALLTEX_RED;
            }
            if (i > 5 && i < 11)
            {
                g_Balls[i].type = BALL_GROUP5;
                g_Balls[i].number = BALLTEX_BLUE;
            }
            if (i > 10)
            {
                g_Balls[i].type = BALL_GROUP6;
                g_Balls[i].number = BALLTEX_BLACK;
            }
        }
        g_Game.numPlayers = 3;
        break;
    }
}

static void NextGameType()
{
    if (++g_Game.gameType >= 5)
        g_Game.gameType = GAME_8BALL;
    SetupBallsForGameType();
}

// Racks the balls around the cursor, turned by (c, s): the cue ball stays
// where it is. Every button gives another rotation. The balls sit at 5 + a
// radius whatever the table's height.
static void RackAtCursor(float c, float s)
{
    D3DXVECTOR3 vCue = g_Balls[0].pos;
    RackBalls();
    g_Balls[0].pos = vCue;
    float x = g_Game.editCursor.x;
    float z = g_Game.editCursor.z;

    if (g_Game.gameType == GAME_8BALL || g_Game.gameType == GAME_REDVSBLUE || g_Game.gameType == GAME_CUTTHROAT)
    {
        for (int i = 1; i < g_NumBalls; i++)
        {
            float ox = kRackShape16X[i] * 34.0f;
            float oz = kRackShape16Z[i] * 46.0f;
            g_Game.pieceOffset.x = ox * c - oz * s;
            g_Game.pieceOffset.z = oz * c + ox * s;
            g_Balls[i].pos.x = x + g_Game.pieceOffset.x;
            g_Balls[i].number = i;
            g_Balls[i].pos.y = g_BallRadius + 5.0f;
            g_Balls[i].pos.z = z + g_Game.pieceOffset.z;
            g_Balls[i].onTable = true;
            g_Game.pieceOffset.y = 0.0f;
        }
    }
    else if (g_Game.gameType == GAME_9BALL)
    {
        // The diamond doesn't turn.
        for (int i = 1; i < g_NumBalls; i++)
        {
            g_Balls[i].pos.x = kRackShape10X[i] * 30.0f + x;
            g_Balls[i].number = i;
            g_Balls[i].pos.y = g_BallRadius + 5.0f;
            g_Balls[i].pos.z = kRackShape10Z[i] * 42.0f + z;
            g_Balls[i].onTable = true;
        }
    }
    else if (g_Game.gameType == GAME_GOLF)
    {
        g_Balls[1].pos.y = g_BallRadius + 5.0f;
        g_Balls[1].pos.x = g_Game.editCursor.x;
        g_NumBalls = 2;
        g_Balls[1].pos.z = g_Game.editCursor.z;
        g_Balls[1].onTable = true;
        g_Balls[1].number = 1;
    }
}

// Right stick up/down: the editor camera's height.
static void UpdateEditorHeight(XBGAMEPAD* pPad)
{
    if (g_Game.input.RightThumbUp(0) != 0.0f)
        g_Game.editCursor.y = pPad->fY2 * g_Game.frameTime * 20.0f + g_Game.editCursor.y;
    if (g_Game.input.RightThumbDown(0) != 0.0f)
        g_Game.editCursor.y = pPad->fY2 * g_Game.frameTime * 20.0f + g_Game.editCursor.y;
}

// The stick moves the cursor, or the grabbed point with it. Quirk kept:
// moving right ignores how far the stick is pushed (squared, for the cushion
// lines).
static void MovePoint(XBGAMEPAD* pPad, D3DXVECTOR3* pPoint, bool bSquaredRight)
{
    float d;
    if (g_Game.input.LeftThumbLeft(0) != 0.0f)
    {
        d = pPad->fX1 * g_Game.frameTime * 20.0f;
        if (!g_Game.bEditGrab)
            g_Game.editCursor.x = d + g_Game.editCursor.x;
        else
            pPoint->x = g_Game.editCursor.x = d + pPoint->x;
    }
    if (g_Game.input.LeftThumbRight(0) != 0.0f)
    {
        if (!g_Game.bEditGrab)
        {
            if (bSquaredRight)
                g_Game.editCursor.x = pPad->fX1 * pPad->fX1 * g_Game.frameTime * 20.0f + g_Game.editCursor.x;
            else
                g_Game.editCursor.x = g_Game.frameTime * 20.0f + g_Game.editCursor.x;
        }
        else
        {
            pPoint->x = g_Game.editCursor.x = pPad->fX1 * g_Game.frameTime * 20.0f + pPoint->x;
        }
    }
    if (g_Game.input.LeftThumbUp(0) != 0.0f)
    {
        d = pPad->fY1 * g_Game.frameTime * 20.0f;
        if (!g_Game.bEditGrab)
            g_Game.editCursor.z = d + g_Game.editCursor.z;
        else
            pPoint->z = g_Game.editCursor.z = d + pPoint->z;
    }
    if (g_Game.input.LeftThumbDown(0) != 0.0f)
    {
        d = pPad->fY1 * g_Game.frameTime * 20.0f;
        if (!g_Game.bEditGrab)
            g_Game.editCursor.z = d + g_Game.editCursor.z;
        else
            pPoint->z = g_Game.editCursor.z = d + pPoint->z;
    }
    UpdateEditorHeight(pPad);
}

// Steps the selected piece's mesh within its set.
static void NextPieceMesh()
{
    STablePiece* p = g_Game.pSelPiece;
    p->mesh++;
    if (p->type >= PIECE_MAT && p->type <= PIECE_BIG && p->mesh > g_Game.pieceSets[p->type].count - 1)
        p->mesh = 0;
}

static void PrevPieceMesh()
{
    STablePiece* p = g_Game.pSelPiece;
    p->mesh--;
    if (p->type >= PIECE_MAT && p->type <= PIECE_BIG && p->mesh < 0)
        p->mesh = g_Game.pieceSets[p->type].count - 1;
}

// Adds a piece at the cursor, with the selected piece's mesh if it's the
// same kind.
static void AddPiece(int nType)
{
    g_Game.table.Append(g_Game.editCursor.x, 0.0f, g_Game.editCursor.z, nType);
    if (g_Game.pSelPiece == NULL)
        return;
    g_Game.pCurPiece = &g_Game.table;
    for (STablePiece* p = g_Game.table.next; p != NULL; p = p->next)
        g_Game.pCurPiece = p;
    if (g_Game.pCurPiece->type == g_Game.pSelPiece->type)
        g_Game.pCurPiece->mesh = g_Game.pSelPiece->mesh;
}

// The same stick handling as the game's, for test play.
static void UpdateTestShot(XBGAMEPAD* pPad)
{
    g_CamDistanceMin = 20.0f;
    g_CamDistanceMax = 70.0f;
    g_CamAngleSpeed = 0.0f;
    if (g_Game.input.LeftThumbLeft(0) != 0.0f || g_Game.input.LeftThumbRight(0) != 0.0f)
    {
        // (both tested separately in the original, with the same effect)
        g_CamAngleSpeed = pPad->fX1;
        g_CueDistance = g_CueMinPull + g_BallRadius;
        g_CamHeightSpeed = 0.0f;
        g_ShotPower = 0.0f;
        g_Game.aimBall = 0;
    }
    g_CueMaxPull = 10.0f;
    if (!g_bCueMode)
    {
        if (g_Game.input.LeftThumbUp(0) != 0.0f)
        {
            g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
            if (g_CamDistance < g_CamDistanceMin) g_CamDistance = g_CamDistanceMin;
            if (g_CamDistance > g_CamDistanceMax) g_CamDistance = g_CamDistanceMax;
        }
        if (g_Game.input.LeftThumbDown(0) != 0.0f)
        {
            g_CamDistance = pPad->fY1 * g_Game.frameTime * 20.0f + g_CamDistance;
            if (g_CamDistance < g_CamDistanceMin) g_CamDistance = g_CamDistanceMin;
            if (g_CamDistance > g_CamDistanceMax) g_CamDistance = g_CamDistanceMax;
        }
    }
    else
    {
        // Unlike the game, the power drains while the stick is released.
        if (g_Game.input.LeftThumbUp(0) == 0.0f)
        {
            if (g_ShotPower > 0.0f)
                g_ShotPower -= 1.0f;
        }
        else
        {
            g_CamHeightSpeed = 0.0f;
            g_CamAngleSpeed = 0.0f;
            g_CueDistance = g_CueDistance - pPad->fY1 * g_Game.frameTime * 140.0f;
            if (g_CueDistance < g_BallRadius * 0.8f) g_CueDistance = g_BallRadius * 0.8f;
            if (g_CueDistance > g_CueMaxPull + g_BallRadius) g_CueDistance = g_CueMaxPull + g_BallRadius;
            g_ShotPower = pPad->fY1 * 100.0f + g_ShotPower;
        }
        if (g_Game.input.LeftThumbDown(0) != 0.0f)
        {
            g_CamHeightSpeed = 0.0f;
            g_CamAngleSpeed = 0.0f;
            g_CueDistance = g_CueDistance - pPad->fY1 * g_Game.frameTime * 140.0f;
            if (g_CueDistance < g_BallRadius * 0.8f) g_CueDistance = g_BallRadius * 0.8f;
            if (g_CueDistance > g_CueMaxPull + g_BallRadius) g_CueDistance = g_CueMaxPull + g_BallRadius;
            g_ShotPower = 0.0f;
        }
    }
    if (g_Game.input.APressed(0))
        g_bCueMode = !g_bCueMode;
}

// Up/down through the slots, previewing each in g_Game.editTable.
static void StepSaveSlot()
{
    g_Game.lastInputTime = GetTickCount();
    g_Game.lastInputTime = GetTickCount();
    if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
    {
        if (--g_Game.tableSlot < 0)
            g_Game.tableSlot = 9;
        LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
    }
    if (g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f)
    {
        if (++g_Game.tableSlot > 9)
            g_Game.tableSlot = 0;
        LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
    }
}

// 0x00031AD0
void UpdateEditor()
{
    g_Game.bEditorHelp = false;
    XBGAMEPAD* pPad = g_Game.input.GetPad(0);
    if (g_Game.input.StartDown(0))
        g_Game.bEditorHelp = true;

    // As in the menus: the stick tumbles the cursor ball.
    if (g_Game.input.LeftThumbDown(0) != 0.0f)
    {
        g_Game.menuVelX -= g_Game.frameTime * 40.0f;
        if (g_Game.menuVelX < -25.0f)
            g_Game.menuVelX = -25.0f;
    }
    if (g_Game.input.LeftThumbUp(0) != 0.0f)
    {
        g_Game.menuVelX = g_Game.frameTime * 40.0f + g_Game.menuVelX;
        if (g_Game.menuVelX > 25.0f)
            g_Game.menuVelX = 25.0f;
    }
    if (g_Game.input.LeftThumbLeft(0) != 0.0f)
    {
        g_Game.menuVelY = g_Game.frameTime * 40.0f + g_Game.menuVelY;
        if (g_Game.menuVelY > 25.0f)
            g_Game.menuVelY = 25.0f;
    }
    if (g_Game.input.LeftThumbRight(0) != 0.0f)
    {
        g_Game.menuVelY -= g_Game.frameTime * 40.0f;
        if (g_Game.menuVelY < -25.0f)
            g_Game.menuVelY = -25.0f;
    }

    if (g_EditorScreen == EDIT_MENU)
    {
        if (g_Game.input.BackDown(0) && InputReady(250))
        {
            g_Game.lastInputTime = GetTickCount();
            g_Game.state = STATE_MENU;
        }
        if (g_Game.input.StartDown(0))
            g_EditorScreen = EDIT_TEST;
        if (g_Game.input.LeftThumbUp(0) != 0.0f && InputReady(200))
        {
            if (--g_Game.editorMode < 0)
                g_Game.editorMode = 6;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.LeftThumbDown(0) != 0.0f && InputReady(200))
        {
            if (++g_Game.editorMode > 6)
                g_Game.editorMode = 0;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.APressed(0))
        {
            switch (g_Game.editorMode)
            {
            case 0: g_EditorScreen = EDIT_INFO; return;
            case 1: g_EditorScreen = EDIT_PIECES; return;
            case 2: g_EditorScreen = EDIT_BARRIERS; return;
            case 3: g_EditorScreen = EDIT_POCKETS; return;
            case 4: g_EditorScreen = EDIT_BALLS; return;
            case 5:
                g_EditorScreen = EDIT_SAVE;
                LoadSavedTableNames();
                LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
                return;
            case 6:
                g_EditorScreen = EDIT_LOAD;
                LoadSavedTableNames();
                LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
                return;
            }
            return;
        }
    }

    // The auto-repeat for the D-pad and the name entry: every 250 ms.
    bool bRepeat = InputReady(250);
    if (bRepeat)
        g_Game.lastInputTime = GetTickCount();

    if (g_EditorScreen == EDIT_INFO)
    {
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.LeftThumbUp(0) != 0.0f && InputReady(200))
        {
            if (--g_Game.editorMode < 0)
                g_Game.editorMode = 2;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.LeftThumbDown(0) != 0.0f && InputReady(200))
        {
            if (++g_Game.editorMode > 2)
                g_Game.editorMode = 0;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.LeftThumbLeft(0) != 0.0f && InputReady(200))
        {
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.editorMode == 0)
            {
                if (--g_Game.gameType < 0)
                    g_Game.gameType = GAME_CUTTHROAT;
                SetupBallsForGameType();
            }
            else if (g_Game.editorMode == 1 && --g_Game.roomMode < 0)
            {
                g_Game.roomMode = ROOM_ENTIRE;
            }
        }
        if (g_Game.input.LeftThumbRight(0) != 0.0f && InputReady(200))
        {
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.editorMode == 0)
                NextGameType();
            else if (g_Game.editorMode == 1 && ++g_Game.roomMode > ROOM_ENTIRE)
                g_Game.roomMode = ROOM_EXTERIOR;
        }
        if (g_Game.input.APressed(0))
        {
            if (g_Game.editorMode == 0)
            {
                NextGameType();
            }
            else if (g_Game.editorMode == 1)
            {
                if (++g_Game.roomMode > ROOM_ENTIRE)
                    g_Game.roomMode = ROOM_EXTERIOR;
            }
            else if (g_Game.editorMode == 2)
            {
                // "Back"
                g_EditorScreen = EDIT_MENU;
                g_Game.lastInputTime = GetTickCount();
                g_Game.editorMode = 0;
            }
        }
    }

    if (g_EditorScreen == EDIT_LOAD)
    {
        // A loads the table into the editor; Start loads it and plays it.
        if (g_Game.input.StartDown(0))
        {
            LoadSavedTable(g_Game.tableSlot, &g_Game.table);
            g_Game.state = STATE_PLAYING;
            return;
        }
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
            return;
        }
        if (g_Game.input.APressed(0))
        {
            LoadSavedTable(g_Game.tableSlot, &g_Game.table);
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
            return;
        }
        if (InputReady(200))
            StepSaveSlot();
    }

    if (g_EditorScreen == EDIT_SAVE)
    {
        if (g_Game.input.StartDown(0))
        {
            g_EditorScreen = EDIT_SAVENAME;
            g_Game.lastInputTime = GetTickCount();
            return;
        }
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.APressed(0))
        {
            g_EditorScreen = EDIT_SAVENAME;
            g_Game.lastInputTime = GetTickCount();
            return;
        }
        if (InputReady(200) || bRepeat)
            StepSaveSlot();
    }

    if (g_EditorScreen == EDIT_SAVENAME)
    {
        // (A saves the table twice.)
        if (g_Game.input.StartDown(0) || g_Game.input.APressed(0))
        {
            SaveTable(g_Game.tableSlot);
            g_EditorScreen = EDIT_MENU;
        }
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.APressed(0))
        {
            SaveTable(g_Game.tableSlot);
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.XPressed(0))
        {
            // Backspace
            int c = g_Game.tableNameCursor;
            if (c < 14)
                memmove(&g_Game.tableName[c], &g_Game.tableName[c + 1], (14 - c) * sizeof(WCHAR));
            g_Game.tableName[14] = 0;
            if ((c == 14 || g_Game.tableName[c + 1] == 0) && --g_Game.tableNameCursor < 0)
                g_Game.tableNameCursor = 0;
        }
        if (InputReady(200) || bRepeat)
        {
            g_Game.lastInputTime = GetTickCount();
            if ((g_Game.input.DPadLeft(0) || g_Game.input.LeftThumbLeft(0) != 0.0f) && --g_Game.tableNameCursor < 0)
                g_Game.tableNameCursor = 14;
            if ((g_Game.input.DPadRight(0) || g_Game.input.LeftThumbRight(0) != 0.0f) && ++g_Game.tableNameCursor > 14)
                g_Game.tableNameCursor = 0;
            WCHAR& ch = g_Game.tableName[g_Game.tableNameCursor];
            if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
                ch++;
            if (g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f)
                ch--;
            if (ch > L'z')
                ch = L'a';
            if (ch < L'a')
                ch = L'z';
        }
    }

    if (g_EditorScreen == EDIT_TEST)
    {
        // As in the game, without the turns' rules for the buttons.
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
                    g_bBallsStopped = false;
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
                if (!g_Game.bPlacingBall)
                    UpdateTestShot(pPad);
                else if (!g_Game.bBallsMoving)
                    bBallInHand = true;
            }
            else
            {
                g_Game.bPlacingBall = false;
                UpdateTestShot(pPad);
            }
        }
        if (bBallInHand)
        {
            SBall& B = g_Balls[nReplace];
            if (g_Game.input.LeftThumbUp(0) != 0.0f)
                B.pos.z = pPad->fY1 * g_Game.frameTime * 10.0f + B.pos.z;
            if (g_Game.input.LeftThumbDown(0) != 0.0f)
                B.pos.z = pPad->fY1 * g_Game.frameTime * 10.0f + B.pos.z;
            if (g_Game.input.LeftThumbLeft(0) != 0.0f)
                B.pos.x = pPad->fX1 * g_Game.frameTime * 10.0f + B.pos.x;
            if (g_Game.input.LeftThumbRight(0) != 0.0f)
                B.pos.x = pPad->fX1 * g_Game.frameTime * 10.0f + B.pos.x;
            if (g_Game.input.APressed(0))
            {
                B.needsReplace = false;
                B.pocket = 0;
                B.inPocket = 0;
                g_Game.bPlacingBall = false;
            }
        }
        UpdatePhysics();
        if (g_Game.input.BackDown(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
    }

    if (g_EditorScreen == EDIT_BALLS)
    {
        if (g_Game.input.BackDown(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        // The four rack buttons, each a quarter turn.
        if (g_Game.input.XPressed(0))
            RackAtCursor(-1.0f, 0.0f);
        if (g_Game.input.BPressed(0))
            RackAtCursor(1.0f, 0.0f);
        if (g_Game.input.YPressed(0))
            RackAtCursor(0.0f, 1.0f);
        if (g_Game.input.APressed(0))
            RackAtCursor(0.0f, -1.0f);
        if (g_Game.input.WhitePressed(0))
        {
            g_Balls[0].pos.y = g_BallRadius + 5.0f;
            g_Balls[0].pos.x = g_Game.editCursor.x;
            g_Balls[0].pos.z = g_Game.editCursor.z;
            g_Balls[0].type = BALL_CUE;
        }
        if (g_Game.input.LeftThumbLeft(0) != 0.0f)
            g_Game.editCursor.x = pPad->fX1 * g_Game.frameTime * 20.0f + g_Game.editCursor.x;
        if (g_Game.input.LeftThumbRight(0) != 0.0f)
            g_Game.editCursor.x = g_Game.frameTime * 20.0f + g_Game.editCursor.x;
        if (g_Game.input.LeftThumbUp(0) != 0.0f)
            g_Game.editCursor.z = pPad->fY1 * g_Game.frameTime * 20.0f + g_Game.editCursor.z;
        if (g_Game.input.LeftThumbDown(0) != 0.0f)
            g_Game.editCursor.z = pPad->fY1 * g_Game.frameTime * 20.0f + g_Game.editCursor.z;
        UpdateEditorHeight(pPad);
        if (g_Game.gameType == GAME_REDVSBLUE)
        {
            // Red vs Blue's colors again (RackBalls numbers them).
            g_NumBalls = 16;
            for (int i = 1; i < 16; i++)
            {
                if (i < 8)
                {
                    g_Balls[i].number = BALLTEX_RED;
                    g_Balls[i].type = BALL_SOLID;
                }
                if (i == 8)
                {
                    g_Balls[8].number = BALLTEX_BLACK;
                    g_Balls[8].type = BALL_EIGHT;
                }
                else if (i > 8)
                {
                    g_Balls[i].number = BALLTEX_BLUE;
                    g_Balls[i].type = BALL_STRIPE;
                }
            }
            g_Game.numPlayers = 2;
        }
    }

    if (g_EditorScreen == EDIT_POCKETS)
    {
        if (g_Game.input.BackDown(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.APressed(0))
        {
            g_Game.bEditGrab = !g_Game.bEditGrab;
            g_Game.editCursor.x = g_Pockets[g_Game.editSel].pos.x;
            g_Game.editCursor.z = g_Pockets[g_Game.editSel].pos.z;
        }
        if (g_Game.input.XPressed(0))
        {
            // The pocket's color: neutral, red, blue.
            int sel = g_Game.editSel;
            if (g_Game.bEditGrab)
                g_Pockets[sel].color++;
            if (g_Pockets[sel].color > 2)
                g_Pockets[sel].color = 0;
        }
        if (g_Game.input.YPressed(0))
            BuildTablePockets();
        if (bRepeat)
        {
            if (g_Game.input.DPadLeft(0))
                g_Game.editSel--;
            if (g_Game.input.DPadRight(0))
                g_Game.editSel++;
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.input.DPadDown(0))
                g_NumPockets--;
            if (g_NumPockets < 1)
                g_NumPockets = 1;
            if (g_Game.input.DPadUp(0))
                g_NumPockets++;
            if (g_NumPockets > MAX_POCKETS - 1)
                g_NumPockets = MAX_POCKETS - 1;
        }
        if (g_Game.editSel > g_NumPockets - 1)
            g_Game.editSel = 0;
        else if (g_Game.editSel < 0)
            g_Game.editSel = g_NumPockets - 1;
        MovePoint(pPad, &g_Pockets[g_Game.editSel].pos, false);
    }

    if (g_EditorScreen == EDIT_BARRIERS)
    {
        if (g_Game.input.BackDown(0))
        {
            g_EditorScreen = EDIT_MENU;
            g_Game.lastInputTime = GetTickCount();
        }
        if (g_Game.input.YPressed(0))
            BuildTableBarriers();
        if (g_Game.input.APressed(0))
        {
            g_Game.bEditGrab = !g_Game.bEditGrab;
            g_Game.editCursor.x = g_Barriers[g_Game.editSel].x;
            g_Game.editCursor.z = g_Barriers[g_Game.editSel].z;
        }
        if (g_Game.input.XPressed(0) && g_Game.bEditGrab)
        {
            // Turns the point into a break between lines.
            int sel = g_Game.editSel;
            g_Barriers[sel].x = 0.0f;
            g_Game.editCursor.x = 0.0f;
            g_Barriers[sel].z = 0.0f;
            g_Game.editCursor.z = 0.0f;
        }
        if (bRepeat)
        {
            if (g_Game.input.DPadLeft(0))
                g_Game.editSel--;
            if (g_Game.input.DPadRight(0))
                g_Game.editSel++;
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.input.DPadDown(0))
                g_NumBarriers--;
            if (g_NumBarriers < 2)
                g_NumBarriers = 2;
            if (g_Game.input.DPadUp(0))
                g_NumBarriers++;
            if (g_NumBarriers > MAX_BARRIERS - 1)
                g_NumBarriers = MAX_BARRIERS - 1;
        }
        if (g_Game.editSel > g_NumBarriers - 1)
            g_Game.editSel = 0;
        else if (g_Game.editSel < 0)
            g_Game.editSel = g_NumBarriers - 1;
        MovePoint(pPad, &g_Barriers[g_Game.editSel], true);
    }

    if (g_EditorScreen != EDIT_PIECES)
        return;

    // The pieces. White/Black/Y add a mat, cushion or big piece at the
    // cursor; the D-pad picks one (left/right), A grabs it, then the stick
    // moves it a unit at a time, left/right turns it and up/down or X
    // cycles its mesh. B deletes it (the first piece can't be: its type
    // changes instead).
    if (g_Game.input.BackDown(0))
    {
        g_EditorScreen = EDIT_MENU;
        g_Game.lastInputTime = GetTickCount();
    }
    if (g_Game.input.APressed(0) && g_Game.pSelPiece != NULL)
        g_Game.bEditGrab = !g_Game.bEditGrab;
    if (g_Game.input.XPressed(0) && g_Game.pSelPiece != NULL && g_Game.bEditGrab)
        NextPieceMesh();
    if (g_Game.input.WhitePressed(0))
        AddPiece(PIECE_MAT);
    if (g_Game.input.BlackPressed(0))
        AddPiece(PIECE_CUSHION);
    if (g_Game.input.YPressed(0))
        AddPiece(PIECE_BIG);
    if (g_Game.input.BPressed(0))
    {
        if (g_Game.bEditGrab)
            g_Game.bEditGrab = false;
        g_Game.pCurPiece = &g_Game.table;
        // (Loops forever if the selected piece isn't in the list.)
        for (;;)
        {
            if (g_Game.pSelPiece == &g_Game.table)
            {
                g_Game.table.type++;
                if (g_Game.pSelPiece->type > PIECE_BIG)
                    g_Game.pSelPiece->type = PIECE_MAT;
                g_Game.pSelPiece->mesh = 0;
                break;
            }
            STablePiece* pNext = g_Game.pCurPiece->next;
            if (pNext != NULL && pNext == g_Game.pSelPiece)
            {
                g_Game.pCurPiece->next = g_Game.pSelPiece->next;
                delete[] g_Game.pSelPiece;
                g_Game.pSelPiece = NULL;
                break;
            }
            if (pNext != NULL)
                g_Game.pCurPiece = pNext;
            if (g_Game.pSelPiece == NULL)
                break;
        }
    }

    if (bRepeat)
    {
        if (g_Game.input.DPadUp(0) && g_Game.pSelPiece != NULL && g_Game.bEditGrab)
            NextPieceMesh();
        if (g_Game.input.DPadDown(0) && g_Game.pSelPiece != NULL && g_Game.bEditGrab)
            PrevPieceMesh();
        if (g_Game.input.DPadLeft(0))
        {
            if (!g_Game.bEditGrab)
            {
                // The previous piece (from the first: the last).
                if (g_Game.pSelPiece == NULL)
                    return;
                STablePiece* pPrev = g_Game.pSelPiece;
                if (g_Game.pSelPiece == &g_Game.table)
                {
                    for (STablePiece* p = g_Game.table.next; p != NULL; p = p->next)
                        pPrev = p;
                }
                else
                {
                    g_Game.pCurPiece = g_Game.pSelPiece;
                    for (STablePiece* p = g_Game.pSelPiece->next; p != g_Game.pSelPiece; p = p->next)
                    {
                        if (p == NULL)
                            p = &g_Game.table;
                        pPrev = p;
                    }
                }
                g_Game.pSelPiece = pPrev;
            }
            else
            {
                g_Game.pSelPiece->rotation -= 1.0f;
                if (g_Game.pSelPiece->rotation < 0.0f)
                    g_Game.pSelPiece->rotation = 3.0f;
            }
        }
        if (g_Game.input.DPadRight(0))
        {
            if (!g_Game.bEditGrab)
            {
                if (g_Game.pSelPiece == NULL)
                    g_Game.pSelPiece = &g_Game.table;
                else
                    g_Game.pSelPiece = g_Game.pSelPiece->next;
            }
            else
            {
                g_Game.pSelPiece->rotation += 1.0f;
                if (g_Game.pSelPiece->rotation > 3.0f)
                    g_Game.pSelPiece->rotation = 0.0f;
            }
        }
        g_Game.lastInputTime = GetTickCount();
    }

    // The stick moves the cursor, or the grabbed piece a unit per frame.
    STablePiece* pSel = g_Game.pSelPiece;
    if (g_Game.input.LeftThumbLeft(0) != 0.0f)
    {
        if (!g_Game.bEditGrab)
            g_Game.editCursor.x = pPad->fX1 * g_Game.frameTime * 20.0f + g_Game.editCursor.x;
        else
            g_Game.editCursor.x = pSel->pos.x = pSel->pos.x - 1.0f;
    }
    if (g_Game.input.LeftThumbRight(0) != 0.0f)
    {
        if (!g_Game.bEditGrab)
            g_Game.editCursor.x = g_Game.frameTime * 20.0f + g_Game.editCursor.x;
        else
            g_Game.editCursor.x = pSel->pos.x = pSel->pos.x + 1.0f;
    }
    if (g_Game.input.LeftThumbUp(0) != 0.0f)
    {
        if (!g_Game.bEditGrab)
            g_Game.editCursor.z = pPad->fY1 * g_Game.frameTime * 20.0f + g_Game.editCursor.z;
        else
            g_Game.editCursor.z = pSel->pos.z = pSel->pos.z + 1.0f;
    }
    if (g_Game.input.LeftThumbDown(0) != 0.0f)
    {
        if (!g_Game.bEditGrab)
            g_Game.editCursor.z = pPad->fY1 * g_Game.frameTime * 20.0f + g_Game.editCursor.z;
        else
            g_Game.editCursor.z = pSel->pos.z = pSel->pos.z - 1.0f;
    }
    UpdateEditorHeight(pPad);
    if (g_Game.bEditGrab)
    {
        // Grabbed pieces snap to whole units.
        g_Game.pSelPiece->pos.x = (float)(int)g_Game.pSelPiece->pos.x;
        g_Game.pSelPiece->pos.z = (float)(int)g_Game.pSelPiece->pos.z;
    }
}
