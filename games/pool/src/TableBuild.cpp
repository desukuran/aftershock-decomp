// The editor's "auto calc": a table's cushion lines (g_Barriers) and pockets
// built from its pieces. Each cushion and big piece mesh has a fixed outline
// and pocket spots, given here as offsets from the piece in its unrotated
// orientation; they're turned with the piece (in quarter turns) and placed
// at its position.
//
// The original spells every piece out as straight-line code; the tables
// below were extracted from it (by running it on one piece at a time) and
// checked for all four rotations.
#include "Game.h"

struct PIECEPOINT
{
    float x, z;
    BOOL  bBreak;           // ends a cushion line: a (0,0,0) separator in g_Barriers
};
#define BREAK { 0.0f, 0.0f, TRUE }

struct PIECESHAPE
{
    int type;               // PIECE_*
    int mesh;
    const PIECEPOINT* points;
    int count;
};

static const PIECEPOINT kBarriers1_0[] =
{
    { 10.0f, -9.652f }, { -10.0f, -9.652f }, BREAK,
};
static const PIECEPOINT kBarriers1_1[] =
{
    { 10.0f, -9.652f }, { 2.117f, -9.652f }, { 1.905f, -11.271f }, { 0.0f, -12.371f },
    { -1.905f, -11.271f }, { -2.117f, -9.651f }, { -10.0f, -9.652f }, BREAK,
};
static const PIECEPOINT kBarriers1_2[] =
{
    { 10.0f, -9.652f }, { -9.652f, -9.652f }, { -9.652f, 10.0f }, BREAK,
};
static const PIECEPOINT kBarriers1_3[] =
{
    { 10.0f, -9.652f }, { -7.821f, -9.652f }, { -8.083f, -11.043f }, { -11.503f, -11.509f },
    { -11.893f, -8.843f }, { -9.652f, -7.844f }, { -9.652f, 10.0f }, BREAK,
};
static const PIECEPOINT kBarriers1_4[] =
{
    { -26.482f, -10.348f }, { -9.652f, -10.458f }, { -9.652f, 10.0f }, BREAK,
};
static const PIECEPOINT kBarriers1_5[] =
{
    { -30.0f, -10.348f }, { -26.482f, -10.348f }, BREAK,
};
static const PIECEPOINT kBarriers1_6[] =
{
    { 9.652f, 10.0f }, { 9.652f, -10.458f }, { 26.482f, -10.348f }, BREAK,
};
static const PIECEPOINT kBarriers1_7[] =
{
    { 26.482f, -10.348f }, { 30.0f, -10.348f }, BREAK,
};
static const PIECEPOINT kBarriers1_8[] =
{
    { 9.652f, 10.0f }, { 9.652f, -9.652f }, { -9.652f, -9.652f }, { -9.652f, 10.0f }, BREAK,
};
static const PIECEPOINT kBarriers1_9[] =
{
    { 9.652f, 10.0f }, { 9.652f, -9.652f }, { 2.117f, -9.651f }, { 1.905f, -11.271f },
    { 0.0f, -12.371f }, { -1.905f, -11.271f }, { -2.117f, -9.651f }, { -9.652f, -9.652f },
    { -9.652f, 10.0f }, BREAK,
};
static const PIECEPOINT kBarriers1_10[] =
{
    { -30.0f, -10.348f }, { -9.652f, -10.348f }, { -9.652f, -0.348f }, BREAK,
};
static const PIECEPOINT kBarriers1_11[] =
{
    { 6.482f, -9.652f }, { -6.482f, -9.652f }, BREAK,
};
static const PIECEPOINT kBarriers1_12[] =
{
    { 6.482f, -9.651f }, { 2.117f, -9.651f }, { 1.905f, -11.271f }, { 0.0f, -12.371f },
    { -1.905f, -11.271f }, { -2.117f, -9.651f }, { -6.482f, -9.651f }, BREAK,
};
static const PIECEPOINT kBarriers1_14[] =
{
    { -6.482f, -9.652f }, { -9.652f, -9.652f }, { -9.652f, -6.482f }, BREAK, { 9.5f, -9.5f },
    { 9.5f, 9.5f }, { 2.142f, 9.5f }, { 2.142f, -9.5f }, { 9.5f, -9.5f }, BREAK,
    { -2.142f, -9.5f }, { -2.142f, 9.5f }, { -9.5f, 9.5f }, { -9.5f, -9.5f }, { -2.142f, -9.5f },
    BREAK,
};
static const PIECEPOINT kBarriers1_15[] =
{
    { -1.215f, -5.18f }, { 1.215f, -5.18f }, { 1.215f, 5.18f }, { -1.215f, 5.18f },
    { -1.215f, -5.18f }, BREAK,
};
static const PIECEPOINT kBarriers1_16[] =
{
    { 4.522f, -2.804f }, { -2.804f, 4.522f }, { -4.522f, 2.804f }, { 2.804f, -4.522f },
    { 4.522f, -2.804f }, BREAK,
};
static const PIECEPOINT kBarriers1_17[] =
{
    { 0.912f, -2.292f }, { 0.912f, 2.292f }, { -0.912f, 2.292f }, { -0.912f, -2.292f },
    { 0.912f, -2.292f }, BREAK,
};
static const PIECEPOINT kBarriers1_18[] =
{
    { 2.266f, -0.976f }, { -0.976f, 2.265f }, { -2.266f, 0.976f }, { 0.976f, -2.266f },
    { 2.266f, -0.976f }, BREAK,
};
static const PIECEPOINT kBarriers1_19[] =
{
    { -5.0f, -1.837f }, { 5.0f, -1.837f }, BREAK,
};
static const PIECEPOINT kBarriers2_0[] =
{
    { -43.767f, 20.767f }, { -40.806f, 20.925f }, { -39.621f, 18.671f }, { -1.772f, 18.671f },
    { -2.166f, 20.312f }, { 0.0f, 22.13f }, { 2.167f, 20.312f }, { 2.272f, 18.671f },
    { 39.663f, 18.671f }, { 40.356f, 20.34f }, { 43.789f, 20.789f }, { 43.676f, 17.555f },
    { 41.671f, 16.644f }, { 41.671f, -16.591f }, { 43.655f, -17.505f }, { 43.756f, -20.756f },
    { 40.335f, -20.29f }, { 39.624f, -18.671f }, { 39.624f, -18.671f }, { 2.117f, -18.671f },
    { 0.0f, -21.39f }, { -2.117f, -18.671f }, { -39.624f, -18.671f }, { -40.335f, -20.29f },
    { -43.756f, -20.756f }, { -43.655f, -17.505f }, { -41.671f, -16.591f }, { -41.671f, 16.637f },
    { -43.634f, 17.555f }, { -43.767f, 20.767f }, { -40.806f, 20.925f }, BREAK,
};
static const PIECEPOINT kBarriers2_1[] =
{
    { -43.634f, 20.925f }, { -40.315f, 20.34f }, { -39.621f, 18.671f }, { -1.772f, 18.671f },
    { 0.0f, 22.13f }, { 1.772f, 18.671f }, { 39.663f, 18.671f }, { 40.847f, 20.925f },
    { 43.789f, 20.789f }, { 43.676f, 17.555f }, { 41.671f, 16.644f }, { 41.671f, -21.228f },
    { 45.13f, -23.0f }, { 41.671f, -24.772f }, { 41.671f, -62.663f }, { 43.925f, -63.847f },
    { 43.789f, -66.789f }, { 40.555f, -66.676f }, { 39.644f, -64.671f }, { 6.409f, -64.671f },
    { 5.495f, -66.655f }, { 2.71f, -67.146f }, { 2.124f, -63.827f }, { 4.329f, -62.624f },
    { 4.329f, -18.671f }, { -39.624f, -18.671f }, { -40.827f, -20.876f }, { -43.756f, -20.756f },
    { -43.655f, -17.505f }, { -41.671f, -16.591f }, { -41.671f, 16.637f }, { -44.125f, 18.14f },
    { -43.767f, 20.767f }, BREAK,
};

static const PIECESHAPE kBarriers[] =
{
    { 1,  0, kBarriers1_0, sizeof(kBarriers1_0) / sizeof(PIECEPOINT) },
    { 1,  1, kBarriers1_1, sizeof(kBarriers1_1) / sizeof(PIECEPOINT) },
    { 1,  2, kBarriers1_2, sizeof(kBarriers1_2) / sizeof(PIECEPOINT) },
    { 1,  3, kBarriers1_3, sizeof(kBarriers1_3) / sizeof(PIECEPOINT) },
    { 1,  4, kBarriers1_4, sizeof(kBarriers1_4) / sizeof(PIECEPOINT) },
    { 1,  5, kBarriers1_5, sizeof(kBarriers1_5) / sizeof(PIECEPOINT) },
    { 1,  6, kBarriers1_6, sizeof(kBarriers1_6) / sizeof(PIECEPOINT) },
    { 1,  7, kBarriers1_7, sizeof(kBarriers1_7) / sizeof(PIECEPOINT) },
    { 1,  8, kBarriers1_8, sizeof(kBarriers1_8) / sizeof(PIECEPOINT) },
    { 1,  9, kBarriers1_9, sizeof(kBarriers1_9) / sizeof(PIECEPOINT) },
    { 1, 10, kBarriers1_10, sizeof(kBarriers1_10) / sizeof(PIECEPOINT) },
    { 1, 11, kBarriers1_11, sizeof(kBarriers1_11) / sizeof(PIECEPOINT) },
    { 1, 12, kBarriers1_12, sizeof(kBarriers1_12) / sizeof(PIECEPOINT) },
    { 1, 14, kBarriers1_14, sizeof(kBarriers1_14) / sizeof(PIECEPOINT) },
    { 1, 15, kBarriers1_15, sizeof(kBarriers1_15) / sizeof(PIECEPOINT) },
    { 1, 16, kBarriers1_16, sizeof(kBarriers1_16) / sizeof(PIECEPOINT) },
    { 1, 17, kBarriers1_17, sizeof(kBarriers1_17) / sizeof(PIECEPOINT) },
    { 1, 18, kBarriers1_18, sizeof(kBarriers1_18) / sizeof(PIECEPOINT) },
    { 1, 19, kBarriers1_19, sizeof(kBarriers1_19) / sizeof(PIECEPOINT) },
    { 2,  0, kBarriers2_0, sizeof(kBarriers2_0) / sizeof(PIECEPOINT) },
    { 2,  1, kBarriers2_1, sizeof(kBarriers2_1) / sizeof(PIECEPOINT) },
};

static const PIECEPOINT kPockets0_2[] =
{
    { 0.28f, 0.01f },
};
static const PIECEPOINT kPockets0_6[] =
{
    { 0.28f, 0.01f },
};
static const PIECEPOINT kPockets0_10[] =
{
    { 0.28f, 0.01f },
};
static const PIECEPOINT kPockets1_1[] =
{
    { 0.0f, -10.766f },
};
static const PIECEPOINT kPockets1_3[] =
{
    { -9.918f, -9.92f },
};
static const PIECEPOINT kPockets1_9[] =
{
    { 0.0f, -10.766f },
};
static const PIECEPOINT kPockets1_12[] =
{
    { 0.0f, -10.766f },
};
static const PIECEPOINT kPockets2_0[] =
{
    { 42.171f, -19.167f }, { 0.0f, -19.785f }, { -42.171f, -19.167f }, { -42.159f, 19.171f },
    { 0.0f, 19.675f }, { 42.157f, 19.16f },
};
static const PIECEPOINT kPockets2_1[] =
{
    { -42.159f, 19.171f }, { 0.0f, 19.675f }, { 42.157f, 19.16f }, { 42.675f, -23.0f },
    { 42.16f, -65.157f }, { 3.833f, -65.171f }, { -42.171f, -19.167f },
};

static const PIECESHAPE kPockets[] =
{
    { 0,  2, kPockets0_2, sizeof(kPockets0_2) / sizeof(PIECEPOINT) },
    { 0,  6, kPockets0_6, sizeof(kPockets0_6) / sizeof(PIECEPOINT) },
    { 0, 10, kPockets0_10, sizeof(kPockets0_10) / sizeof(PIECEPOINT) },
    { 1,  1, kPockets1_1, sizeof(kPockets1_1) / sizeof(PIECEPOINT) },
    { 1,  3, kPockets1_3, sizeof(kPockets1_3) / sizeof(PIECEPOINT) },
    { 1,  9, kPockets1_9, sizeof(kPockets1_9) / sizeof(PIECEPOINT) },
    { 1, 12, kPockets1_12, sizeof(kPockets1_12) / sizeof(PIECEPOINT) },
    { 2,  0, kPockets2_0, sizeof(kPockets2_0) / sizeof(PIECEPOINT) },
    { 2,  1, kPockets2_1, sizeof(kPockets2_1) / sizeof(PIECEPOINT) },
};

// The direction a piece faces: rotation 0-3 quarter turns as cos, sin.
static void PieceRotation(float rotation, float* pCos, float* pSin)
{
    *pSin = 0.0f;
    *pCos = 1.0f;
    if (rotation == 1.0f)
    {
        *pCos = 0.0f;
        *pSin = -1.0f;
    }
    if (rotation == 2.0f)
    {
        *pSin = 0.0f;
        *pCos = -1.0f;
    }
    if (rotation == 3.0f)
    {
        *pCos = 0.0f;
        *pSin = 1.0f;
    }
}

static const PIECESHAPE* FindShape(const PIECESHAPE* pShapes, int nShapes, int type, int mesh)
{
    for (int i = 0; i < nShapes; i++)
    {
        if (pShapes[i].type == type && pShapes[i].mesh == mesh)
            return &pShapes[i];
    }
    return NULL;
}

// Places a shape's points from a piece; an index past nMax is counted but
// not written.
static int PlacePoints(const PIECESHAPE* pShape, const STablePiece* pPiece, D3DXVECTOR3* pOut, int n, int nMax,
                       int nStride)
{
    float c, s;
    PieceRotation(pPiece->rotation, &c, &s);
    for (int i = 0; i < pShape->count; i++, n++)
    {
        const PIECEPOINT& pt = pShape->points[i];
        D3DXVECTOR3* pDst = (D3DXVECTOR3*)((BYTE*)pOut + n * nStride);
        if (pt.bBreak)
        {
            if (n < nMax)
                *pDst = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            continue;
        }
        g_Game.pieceOffset = D3DXVECTOR3(pt.x * c - pt.z * s, 0.0f, pt.z * c + pt.x * s);
        if (n < nMax)
        {
            pDst->x = pPiece->pos.x + g_Game.pieceOffset.x;
            pDst->y = pPiece->pos.y + 0.0f;
            pDst->z = pPiece->pos.z + g_Game.pieceOffset.z;
        }
    }
    return n;
}

// 0x00017800
void BuildTableBarriers()
{
    int n = 0;
    g_Game.pCurPiece = &g_Game.table;
    do
    {
        const PIECESHAPE* pShape = FindShape(kBarriers, sizeof(kBarriers) / sizeof(kBarriers[0]),
                                             g_Game.pCurPiece->type, g_Game.pCurPiece->mesh);
        if (pShape != NULL)
            n = PlacePoints(pShape, g_Game.pCurPiece, g_Barriers, n, MAX_BARRIERS, sizeof(D3DXVECTOR3));
        g_Game.pCurPiece = g_Game.pCurPiece->next;
    } while (g_Game.pCurPiece != NULL);

    if (n > MAX_BARRIERS - 1)
        n = MAX_BARRIERS - 1;
    g_NumBarriers = n - 1;      // the last separator isn't counted
    if (g_NumBarriers == -1)
        g_NumBarriers = 0;
}

// 0x00023650
// Leaves the pockets' colors as they were.
void BuildTablePockets()
{
    g_Game.pCurPiece = &g_Game.table;
    g_NumPockets = 0;
    do
    {
        const PIECESHAPE* pShape = FindShape(kPockets, sizeof(kPockets) / sizeof(kPockets[0]),
                                             g_Game.pCurPiece->type, g_Game.pCurPiece->mesh);
        if (pShape != NULL)
            g_NumPockets = PlacePoints(pShape, g_Game.pCurPiece, &g_Pockets[0].pos, g_NumPockets, MAX_POCKETS,
                                       sizeof(SPocket));
        g_Game.pCurPiece = g_Game.pCurPiece->next;
    } while (g_Game.pCurPiece != NULL);

    if (g_NumPockets > MAX_POCKETS - 1)
        g_NumPockets = MAX_POCKETS - 1;
    if (g_NumPockets == -1)
        g_NumPockets = 1;
}
