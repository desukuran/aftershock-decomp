// Tables: the editor's piece lists, the custom piece meshes, and loading and
// saving tables (D:\media\tables\N.cfg are the built-in ones, T:\N.cfg the
// player's).
//
// A table file is whitespace-separated text:
//   name  gametype  piececount  pieces (x y z / type / mesh / rotation)...
//   then for a played table: gametype  ballcount  (x z) per ball
//   barriercount  (x z) per barrier point  pocketcount  pocketradius
//   (x z color) per pocket  tableheight camheight camdistance camangle
//   camheightmin camheightmax
//   or for the editor's table: gametype
#include "Game.h"

// 0x00012BA0
STablePiece::STablePiece()
{
    next = NULL;
}

// 0x00011000 (empty; used by the array new/delete helpers)
STablePiece::~STablePiece()
{
}

// 0x00012BB0
// Adds a piece at the end of the list.
void STablePiece::Append(float x, float y, float z, int nType)
{
    if (next != NULL)
    {
        next->Append(x, y, z, nType);
        return;
    }
    next = new STablePiece[1];
    next->type = nType;
    next->mesh = 0;
    next->pos = D3DXVECTOR3(x, y, z);
    next->rotation = 0.0f;
}

// 0x00012CA0
// Writes this piece and the rest of the list.
void STablePiece::Save(FILE* f)
{
    for (STablePiece* p = this; p != NULL; p = p->next)
    {
        fprintf(f, "%f", p->pos.x);
        fwprintf(f, L" ");
        fprintf(f, "%f", p->pos.y);
        fwprintf(f, L" ");
        fprintf(f, "%f", p->pos.z);
        fwprintf(f, L"\n");
        fprintf(f, "%d", p->type);
        fwprintf(f, L"\n");
        fprintf(f, "%d", p->mesh);
        fwprintf(f, L"\n");
        fprintf(f, "%f", p->rotation);
        fwprintf(f, L"\n");
    }
}

// 0x00012E30
void STablePiece::Load(FILE* f)
{
    fscanf(f, "%f ", &pos.x);
    fscanf(f, "%f ", &pos.y);
    fscanf(f, "%f ", &pos.z);
    fscanf(f, "%d ", &type);
    fscanf(f, "%d ", &mesh);
    fscanf(f, "%f ", &rotation);
}

// 0x00012EA0
// Draws the piece, untextured meshes using the first mesh's texture of the
// set.
void STablePiece::Render(LPDIRECT3DDEVICE8 pDevice, SPieceSet* pSets)
{
    if (pDevice == NULL || pSets == NULL)
        return;

    D3DXMATRIX matRot, matTrans, matWorld;
    D3DXMatrixRotationY(&matRot, rotation * (D3DX_PI / 2));
    D3DXMatrixTranslation(&matTrans, pos.x, pos.y, pos.z);
    D3DXMatrixMultiply(&matWorld, &matRot, &matTrans);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    if (type == PIECE_MAT)
        pSets[PIECE_MAT].meshes[mesh].Render(pDevice, pSets[PIECE_MAT].meshes[0].m_pTexture);
    if (type == PIECE_CUSHION)
        pSets[PIECE_CUSHION].meshes[mesh].Render(pDevice, pSets[PIECE_CUSHION].meshes[0].m_pTexture);
    if (type == PIECE_BIG)
        pSets[PIECE_BIG].meshes[mesh].Render(pDevice, pSets[PIECE_BIG].meshes[0].m_pTexture);
}

// Counts the files in a folder ("D:\...\*"). Every file is looked up twice
// more by the wildcard, as the original does.
static int CountFiles(const char* strWildcard)
{
    WIN32_FIND_DATA fd, fdFile;
    char strPath[256];
    int nCount = 0;

    HANDLE hFind = FindFirstFileA(strWildcard, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("FindFirstFile failed.");
        return -1;
    }
    do
    {
        wsprintfA(strPath, strWildcard);
        HANDLE hFile = FindFirstFileA(strPath, &fdFile);
        if (hFile != INVALID_HANDLE_VALUE && (hFile = FindFirstFileA(strPath, &fdFile)) != INVALID_HANDLE_VALUE)
            nCount++;
        FindClose(hFile);
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    return nCount;
}

// Loads a folder's files in pairs (in directory order: the .bin, then its
// texture; an empty texture file means none) into pSet->meshes.
static void LoadPieceMeshes(const char* strWildcard, const char* strFormat, SPieceSet* pSet)
{
    WIN32_FIND_DATA fd, fdFile;
    char strPath[256];
    char strFiles[2][256];
    bool bNoTexture = false;

    HANDLE hFind = FindFirstFileA(strWildcard, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("FindFirstFile failed.");
        return;
    }
    int nInPair = 0;
    int nMesh = 0;
    do
    {
        wsprintfA(strPath, strWildcard);
        HANDLE hFile = FindFirstFileA(strPath, &fdFile);
        if (hFile != INVALID_HANDLE_VALUE && (hFile = FindFirstFileA(strPath, &fdFile)) != INVALID_HANDLE_VALUE)
        {
            if (nInPair < 2)
                sprintf(strFiles[nInPair], strFormat, fd.cFileName);
            bNoTexture = fd.nFileSizeLow == 0;
            nInPair++;
        }
        if (nInPair == 2)
        {
            pSet->meshes[nMesh].Load(g_Game.device, strFiles[0], strFiles[1], bNoTexture);
            nInPair = 0;
            nMesh++;
        }
        FindClose(hFile);
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

// 0x0002F090
// The table editor's building blocks from D:\media\custom. Quirk kept: the
// big pieces are only allocated and loaded if there are mats.
void LoadCustomPieces()
{
    int nMats = CountFiles("D:\\media\\custom\\mats\\*");
    if (nMats == -1)
    {
        nMats = 0;
    }
    else if (nMats != 0)
    {
        g_Game.pieceSets[PIECE_MAT].count = nMats / 2;
        g_Game.pieceSets[PIECE_MAT].meshes = new CMesh[nMats / 2];
    }
    if (g_Game.pieceSets[PIECE_MAT].count != 0)
        LoadPieceMeshes("D:\\media\\custom\\mats\\*", "D:\\media\\custom\\mats\\%s", &g_Game.pieceSets[PIECE_MAT]);

    g_Game.table.pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    g_Game.table.type = PIECE_MAT;
    g_Game.table.mesh = 0;
    g_Game.table.rotation = 0.0f;
    g_Game.pSelPiece = &g_Game.table;

    int nCushions = CountFiles("D:\\media\\custom\\cushions\\*");
    if (nCushions > 0)
    {
        g_Game.pieceSets[PIECE_CUSHION].count = nCushions / 2;
        g_Game.pieceSets[PIECE_CUSHION].meshes = new CMesh[nCushions / 2];
    }
    if (g_Game.pieceSets[PIECE_CUSHION].count != 0)
        LoadPieceMeshes("D:\\media\\custom\\cushions\\*", "D:\\media\\custom\\cushions\\%s", &g_Game.pieceSets[PIECE_CUSHION]);

    int nBig = CountFiles("D:\\media\\custom\\bigpieces\\*");
    if (nBig != -1 && nMats != 0)
    {
        g_Game.pieceSets[PIECE_BIG].count = nBig / 2;
        g_Game.pieceSets[PIECE_BIG].meshes = new CMesh[nBig / 2];
    }
    if (g_Game.pieceSets[PIECE_MAT].count != 0)
        LoadPieceMeshes("D:\\media\\custom\\bigpieces\\*", "D:\\media\\custom\\bigpieces\\%s", &g_Game.pieceSets[PIECE_BIG]);
}

// The two table loaders differ only in the folder, which name list they
// update and where the first game type goes.
static void LoadTable(const char* strFormat, int slot, STablePiece* pHead, bool bSaved)
{
    // Quirk kept: frees the list from its second piece on (the first leaks).
    if (pHead->next != NULL)
    {
        delete[] pHead->next->next;
        pHead->next = NULL;
    }

    char strPath[256];
    sprintf(strPath, strFormat, slot);
    FILE* f = fopen(strPath, "r");
    if (f == NULL)
    {
        pHead->pos.x = 0.0f;
        pHead->pos.y = 0.0f;
        pHead->mesh = 0;
        pHead->type = PIECE_MAT;
        pHead->pos.z = 0.0f;
        return;
    }

    char strName[16];
    fscanf(f, "%s ", strName);
    swprintf(g_Game.tableName, L"%S", strName);
    if (bSaved)
        g_Game.savedTableNames[slot] = strName;
    else
        g_Game.defaultTableNames[slot] = strName;

    int nTableGameType = 0;
    if (bSaved)
        fscanf(f, "%d ", &nTableGameType);
    else
        fscanf(f, "%d ", &g_Game.roomMode);

    int nPieces = 0;
    fscanf(f, "%d", &nPieces);
    g_Game.pCurPiece = pHead;
    for (int i = 0; i < nPieces; i++)
    {
        STablePiece* p = g_Game.pCurPiece;
        p->Load(f);
        if (i < nPieces - 1 && p->next == NULL)
            p->Append(0.0f, 0.0f, 0.0f, PIECE_MAT);
        if (p->pos.x < g_Game.tableMinX)
            g_Game.tableMinX = p->pos.x;
        if (p->pos.z < g_Game.tableMinZ)
            g_Game.tableMinZ = p->pos.z;
        if (p->pos.x > g_Game.tableMaxX)
            g_Game.tableMaxX = p->pos.x;
        if (p->pos.z > g_Game.tableMaxZ)
            g_Game.tableMaxZ = p->pos.z;
        g_Game.pCurPiece = p->next;
    }

    if (pHead == &g_Game.table)
    {
        if (bSaved)
            g_Game.roomMode = nTableGameType;
        fscanf(f, "%d ", &g_Game.gameType);
        switch (g_Game.gameType)
        {
        case GAME_CUTTHROAT:
            g_Game.numPlayers = 3;
            break;
        default:
            g_Game.numPlayers = 2;
            break;
        }

        fscanf(f, "%d ", &g_NumBalls);
        for (int i = 0; i < g_NumBalls; i++)
            fscanf(f, "%f %f ", &g_RackX[i], &g_RackZ[i]);
        fscanf(f, "%d ", &g_NumBarriers);
        for (int i = 0; i < g_NumBarriers; i++)
            fscanf(f, "%f %f ", &g_Barriers[i].x, &g_Barriers[i].z);
        fscanf(f, "%d ", &g_NumPockets);
        fscanf(f, "%f ", &g_PocketRadius);
        g_PocketRadius = 1.0f;
        for (int i = 0; i < g_NumPockets; i++)
        {
            fscanf(f, "%f %f ", &g_Pockets[i].pos.x, &g_Pockets[i].pos.z);
            fscanf(f, "%d ", &g_Pockets[i].color);
        }

        // Read, then mostly replaced by the defaults (the angle stays).
        fscanf(f, "%f %f %f %f %f %f ", &g_TableHeight, &g_CamHeight, &g_CamDistance, &g_CamAngle,
               &g_CamHeightMin, &g_CamHeightMax);
        g_TableHeight = 5.0f;
        g_CamHeight = 15.0f;
        g_CamDistance = 30.0f;
        g_CamHeightMin = 0.0f;
        g_CamHeightMax = 50.0f;
        RackBalls();
    }
    else
    {
        fscanf(f, "%d ", &g_Game.editGameType);
    }
    fclose(f);
}

// 0x0002FB00
void LoadDefaultTable(int slot, STablePiece* pHead)
{
    LoadTable("D:\\media\\tables\\%d.cfg", slot, pHead, false);
}

// 0x0002FF30
void LoadSavedTable(int slot, STablePiece* pHead)
{
    LoadTable("T:\\%d.cfg", slot, pHead, true);
}

// 0x00016460
// Saves g_Game.table (the editor works on it while testing) with the balls'
// current positions as the rack.
void SaveTable(int slot)
{
    char strPath[260];
    sprintf(strPath, "T:\\%d.cfg", slot);
    FILE* f = fopen(strPath, "w");
    if (f == NULL)
        return;

    fwprintf(f, L"%s", g_Game.tableName);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.roomMode);
    fwprintf(f, L" ");
    int nPieces = 0;
    g_Game.pCurPiece = &g_Game.table;
    do
    {
        g_Game.pCurPiece = g_Game.pCurPiece->next;
        nPieces++;
    } while (g_Game.pCurPiece != NULL);
    fprintf(f, "%d", nPieces);
    fwprintf(f, L" ");
    g_Game.table.Save(f);

    fprintf(f, "%d", g_Game.gameType);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_NumBalls);
    fwprintf(f, L" ");
    for (int i = 0; i < g_NumBalls; i++)
    {
        fprintf(f, "%f", g_Balls[i].pos.x);
        fwprintf(f, L" ");
        fprintf(f, "%f", g_Balls[i].pos.z);
        fwprintf(f, L" ");
    }
    fprintf(f, "%i", g_NumBarriers);
    fwprintf(f, L" ");
    for (int i = 0; i < g_NumBarriers; i++)
    {
        fprintf(f, "%f", g_Barriers[i].x);
        fwprintf(f, L" ");
        fprintf(f, "%f", g_Barriers[i].z);
        fwprintf(f, L" ");
    }
    fprintf(f, "%i", g_NumPockets);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_PocketRadius);
    fwprintf(f, L" ");
    for (int i = 0; i < g_NumPockets; i++)
    {
        fprintf(f, "%f", g_Pockets[i].pos.x);
        fwprintf(f, L" ");
        fprintf(f, "%f", g_Pockets[i].pos.z);
        fwprintf(f, L" ");
        fprintf(f, "%d", g_Pockets[i].color);
        fwprintf(f, L" ");
    }
    fprintf(f, "%f", g_TableHeight);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_CamHeight);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_CamDistance);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_CamAngle);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_CamHeightMin);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_CamHeightMax);
    fwprintf(f, L" ");
    fclose(f);
}

// 0x00030370
void LoadDefaultTableNames()
{
    char strPath[256];
    char strName[16];
    for (int i = 0; i < NUM_TABLE_SLOTS; i++)
    {
        sprintf(strPath, "D:\\media\\tables\\%d.cfg", i);
        FILE* f = fopen(strPath, "r");
        if (f != NULL)
        {
            fscanf(f, "%s ", strName);
            g_Game.defaultTableNames[i] = strName;
            fclose(f);
        }
    }
}

// 0x00030420
void LoadSavedTableName(int slot)
{
    char strPath[256];
    char strName[16];
    sprintf(strPath, "T:\\%d.cfg", slot);
    FILE* f = fopen(strPath, "r");
    if (f != NULL)
    {
        fscanf(f, "%s ", strName);
        g_Game.savedTableNames[slot] = strName;
        fclose(f);
    }
}

// 0x000304C0
void LoadSavedTableNames()
{
    char strPath[256];
    for (int i = 0; i < NUM_TABLE_SLOTS; i++)
    {
        sprintf(strPath, "T:\\%d.cfg", i);
        FILE* f = fopen(strPath, "r");
        if (f != NULL)
        {
            fclose(f);
            LoadSavedTableName(i);
        }
    }
}
