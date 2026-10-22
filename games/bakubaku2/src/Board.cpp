// A board: the falling pair, the queue, the four animals that eat, the
// crumbs, and the lion that eats a lost board. Row 0 is the bottom; a cell
// is grid[row * 6 + col].
#include "Game.h"

// 0x000FF6E0 the animals' frame (all four share it)
static SFrameUV s_eaterUV;
// 0x000FF700 the pieces' sparkle: 0-13 still, 14-20 the spin's frames
int g_nPieceFrame;
// 0x000FF704
static DWORD s_dwEaterTime;
// 0x000FF708
static SFrameUV s_pieceUV;
// 0x000FF728
static LPDIRECT3DVERTEXBUFFER8 s_pEaterVB;
// 0x000FF72C
static DWORD s_dwPieceTime;
// 0x000FF730
static LPDIRECT3DVERTEXBUFFER8 s_pPieceVB;
// 0x000FF734
static SFrame s_pieceFrame;
// 0x000FF744
static SFrame s_eaterFrame;

// ---- crumbs ----------------------------------------------------------------------------

// 0x00011000
// Four bits, each flying at its own speed.
void CCrumbs::Start(float x, float y, int type)
{
    m_type = type;
    for (int i = 0; i < 4; i++)
    {
        m_crumbs[i].x = x;
        m_crumbs[i].y = y;
        m_crumbs[i].dy = 4.0f;
        m_crumbs[i].dx = (float)(rand() % 4 + 1);
    }
    m_dwStart = GetTickCount();
}

// 0x00011070
// Up and apart for half a second, then down; gone after 2 s. Quirk: the
// frame they stop on, they still fall (the start is 0 by then).
void CCrumbs::Update()
{
    if (m_dwStart == 0)
        return;
    if (GetTickCount() - m_dwStart >= 2000)
        m_dwStart = 0;
    if (GetTickCount() - m_dwStart <= 500)
    {
        m_crumbs[0].y += m_crumbs[0].dy;
        m_crumbs[1].y += m_crumbs[1].dy * 0.5f;
        m_crumbs[0].x -= m_crumbs[0].dx;
        m_crumbs[1].x -= m_crumbs[1].dx;
        m_crumbs[2].y += m_crumbs[2].dy;
        m_crumbs[3].y += m_crumbs[3].dy * 0.5f;
        m_crumbs[2].x += m_crumbs[2].dx;
        m_crumbs[3].x += m_crumbs[3].dx;
    }
    if (GetTickCount() - m_dwStart > 500)
    {
        m_crumbs[0].y -= m_crumbs[0].dy;
        m_crumbs[1].y -= m_crumbs[1].dy;
        m_crumbs[0].x -= m_crumbs[0].dx;
        m_crumbs[1].x -= m_crumbs[1].dx;
        m_crumbs[2].y -= m_crumbs[2].dy;
        m_crumbs[3].y -= m_crumbs[3].dy;
        m_crumbs[2].x += m_crumbs[2].dx;
        m_crumbs[3].x += m_crumbs[3].dx;
    }
}

// 0x000111C0
// The crumbs of a food or of its animal's head look the same.
void CCrumbs::Render()
{
    D3DXMATRIX matScale, matTrans, matWorld;
    for (int i = 0; i < 4; i++)
    {
        if (m_dwStart == 0)
            continue;
        D3DXMatrixScaling(&matScale, m_crumbs[i].fScale, m_crumbs[i].fScale, 1.0f);
        D3DXMatrixTranslation(&matTrans, m_crumbs[i].x, m_crumbs[i].y, m_crumbs[i].z);
        D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        if (m_type == 1 || m_type == 5)
            m_pTextures->Set(98);
        if (m_type == 2 || m_type == 6)
            m_pTextures->Set(100);
        if (m_type == 3 || m_type == 7)
            m_pTextures->Set(99);
        if (m_type == 4 || m_type == 8)
            m_pTextures->Set(101);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);
    }
}

// ---- the falling pair ------------------------------------------------------------------

// Puts a piece that has stopped into the grid (on top of the one in its
// cell, if there is one).
static __forceinline void Place(CBoard* pBoard, CPiece& piece)
{
    if (pBoard->Get(piece.m_col, piece.m_row))
        piece.m_row++;
    pBoard->Set(piece.m_col, piece.m_row, piece.m_type);
    piece.m_bLanded = false;
    piece.m_type = 0;
}

// 0x000112D0
// Moves the pair down a pixel at a time; a piece lands on the bottom or on
// a full cell under it. Below each other, the lower one lands first.
void CBoard::Drop(int nSteps)
{
    for (int i = 0; i < nSteps; i++)
    {
        if (m_pieces[0].m_fy <= m_y && m_pieces[0].m_type)
        {
            m_pieces[0].m_fy = (float)m_y;
            m_pieces[0].m_bLanded = true;
            m_pieces[0].m_row = 0;
            if (Get(m_pieces[0].m_col, 0))
                m_pieces[0].m_row++;
            Set(m_pieces[0].m_col, m_pieces[0].m_row, m_pieces[0].m_type);
            m_pieces[0].m_bLanded = false;
            m_pieces[0].m_type = 0;
        }
        if (m_pieces[1].m_fy <= m_y && m_pieces[1].m_type)
        {
            m_pieces[1].m_fy = (float)m_y;
            m_pieces[1].m_bLanded = true;
            m_pieces[1].m_row = 0;
            if (Get(m_pieces[1].m_col, 0))
                m_pieces[1].m_row++;
            Set(m_pieces[1].m_col, m_pieces[1].m_row, m_pieces[1].m_type);
            m_pieces[1].m_bLanded = false;
            m_pieces[1].m_type = 0;
        }

        if ((m_pieces[0].m_fy > m_y && m_pieces[0].m_type) || (m_pieces[1].m_fy > m_y && m_pieces[1].m_type))
        {
            // The row each one is in.
            if (m_pieces[0].m_type)
            {
                for (int row = 0; row < 10; row++)
                {
                    if (m_y + row * m_nCell > m_pieces[0].m_fy)
                    {
                        m_pieces[0].m_row = row - 1;
                        break;
                    }
                }
            }
            if (m_pieces[1].m_type)
            {
                for (int row = 0; row < 10; row++)
                {
                    if (m_y + row * m_nCell > m_pieces[1].m_fy)
                    {
                        m_pieces[1].m_row = row - 1;
                        break;
                    }
                }
            }

            // Exactly on a row: is there something under it?
            if (m_pieces[0].m_fy == m_pieces[0].m_row * m_nCell + m_y ||
                m_pieces[1].m_fy == m_pieces[1].m_row * m_nCell + m_y)
            {
                if (m_rotation == 1)
                {
                    if (Get(m_pieces[1].m_col, m_pieces[1].m_row - 1) || m_pieces[1].m_fy <= m_y)
                    {
                        if (m_pieces[1].m_type)
                        {
                            m_pieces[1].m_bLanded = true;
                            if (Get(m_pieces[1].m_col, m_pieces[1].m_row))
                                m_pieces[1].m_row++;
                            Set(m_pieces[1].m_col, m_pieces[1].m_row, m_pieces[1].m_type);
                            m_pieces[1].m_bLanded = false;
                            m_pieces[1].m_type = 0;
                        }
                        m_pieces[0].m_bLanded = true;
                        if (m_pieces[0].m_type)
                            Place(this, m_pieces[0]);
                    }
                }
                if (m_rotation != 1)
                {
                    if (Get(m_pieces[0].m_col, m_pieces[0].m_row - 1) || m_pieces[0].m_row == 0)
                    {
                        m_pieces[0].m_bLanded = true;
                        if (m_pieces[0].m_type)
                            Place(this, m_pieces[0]);
                    }
                    if (Get(m_pieces[1].m_col, m_pieces[1].m_row - 1) || m_pieces[1].m_row == 0)
                    {
                        m_pieces[1].m_bLanded = true;
                        if (m_pieces[1].m_type)
                            Place(this, m_pieces[1]);
                    }
                }
            }

            if (m_pieces[0].m_type)
                m_pieces[0].m_fy -= 1.0f;
            if (m_pieces[1].m_type)
                m_pieces[1].m_fy -= 1.0f;
        }
    }
}

// 0x000116F0
// Slides the pieces across to their columns, a pixel a step.
void CBoard::SlideLeft(int nSteps)
{
    for (int i = 0; i < nSteps; i++)
    {
        if (m_pieces[0].m_col * m_nCell + m_x < m_pieces[0].m_fx)
            m_pieces[0].m_fx -= 1.0f;
        if (m_pieces[1].m_col * m_nCell + m_x < m_pieces[1].m_fx)
            m_pieces[1].m_fx -= 1.0f;
    }
}

// 0x00011820
void CBoard::SlideRight(int nSteps)
{
    for (int i = 0; i < nSteps; i++)
    {
        if (m_pieces[0].m_fx < m_pieces[0].m_col * m_nCell + m_x)
            m_pieces[0].m_fx += 1.0f;
        if (m_pieces[1].m_fx < m_pieces[1].m_col * m_nCell + m_x)
            m_pieces[1].m_fx += 1.0f;
    }
}

// 0x00011950
// The pair, the queue (right to left above the board) and the portrait
// (left of it). Quirk: each piece's type is drawn twice, and the
// portrait's once before it is overwritten.
void CBoard::SetupPieces(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nCell, int nScale, int x, int y,
                         int z, int portrait)
{
    m_pDevice = pDevice;
    m_pTextures = pTextures;

    m_pieces[0].m_pDevice = pDevice;
    m_pieces[0].m_pTextures = pTextures;
    m_pieces[0].m_nCell = nCell;
    m_pieces[0].m_nScale = nScale;
    m_pieces[0].m_x = x;
    m_pieces[0].m_y = y;
    m_pieces[0].m_z = z;
    m_pieces[0].m_type = 0;
    m_pieces[1].m_pDevice = pDevice;
    m_pieces[1].m_pTextures = pTextures;
    m_pieces[1].m_nCell = nCell;
    m_pieces[1].m_nScale = nScale;
    m_pieces[1].m_x = x;
    m_pieces[1].m_y = y;
    m_pieces[1].m_z = z;
    m_pieces[1].m_type = 0;
    g_nPieceFrame = 0;

    m_nCell = nCell;
    m_nScale = nScale;
    m_x = x;
    m_y = y;
    m_z = z;

    for (int i = 0; i < 5; i++)
    {
        float fy = (float)(m_y + m_nCell * 10);
        float fx = nCell + (float)((4 - i) * m_nCell + m_x);
        m_preview[i].pDevice = pDevice;
        m_preview[i].pTextures = pTextures;
        m_preview[i].nScale = nScale;
        m_preview[i].nCell = nCell;
        m_preview[i].x = x;
        m_preview[i].y = y;
        m_preview[i].z = z;
        m_preview[i].fx = fx;
        m_preview[i].fy = fy;
        m_preview[i].type = rand() % 8 + 1;
        m_preview[i].type = rand() % 8 + 1;
    }

    m_portrait.pDevice = pDevice;
    m_portrait.pTextures = pTextures;
    m_portrait.nScale = nScale;
    m_portrait.nCell = nCell;
    m_portrait.x = x;
    m_portrait.y = y;
    m_portrait.z = z;
    m_portrait.fx = nCell + (float)(m_x - m_nCell);
    m_portrait.fy = (float)(m_y + m_nCell * 10);
    m_portrait.type = rand() % 8 + 1;
    m_portrait.type = portrait;
}

// 0x00011B60
// The next pair from the queue, two new pieces at its end. The piece set:
// 0 anything, 1 a food in each pair, 2 a head in each pair.
void CBoard::NextPair()
{
    m_nAiMove++;
    if (m_nAiMove > 2)
        m_nAiMove = 0;

    m_pieces[0].m_type = m_preview[1].type;
    m_pieces[1].m_type = m_preview[0].type;
    m_bAimed = false;
    m_preview[0].type = m_preview[2].type;
    m_preview[1].type = m_preview[3].type;
    m_preview[2].type = m_preview[4].type;

    if (g_nPieceSet == 0)
    {
        m_preview[3].type = rand() % 8 + 1;
        m_preview[4].type = rand() % 8 + 1;
    }
    if (g_nPieceSet == 1)
    {
        switch (rand() % 2)
        {
        case 0:
            m_preview[3].type = rand() % 4 + 1;
            m_preview[4].type = rand() % 8 + 1;
            break;
        case 1:
            m_preview[3].type = rand() % 8 + 1;
            m_preview[4].type = rand() % 4 + 1;
            break;
        }
    }
    if (g_nPieceSet == 2)
    {
        switch (rand() % 2)
        {
        case 0:
            m_preview[3].type = rand() % 4 + FIRST_HEAD;
            m_preview[4].type = rand() % 8 + 1;
            break;
        case 1:
            m_preview[3].type = rand() % 8 + 1;
            m_preview[4].type = rand() % 4 + FIRST_HEAD;
            break;
        }
    }

    // Side by side at the top, in columns 2 and 3.
    m_pieces[0].m_fx = (float)(m_x + m_nCell * 2);
    m_pieces[1].m_fx = (float)(m_x + m_nCell * 2 + m_nCell);
    m_pieces[0].m_fy = (float)(m_nCell * 9 + m_y);
    m_pieces[1].m_fy = (float)(m_nCell * 9 + m_y);
    m_pieces[0].m_bLanded = false;
    m_pieces[1].m_bLanded = false;
    m_rotation = 0;
    m_pieces[0].m_row = 9;
    m_pieces[1].m_row = 9;
    m_pieces[0].m_col = 2;
    m_pieces[1].m_col = 3;
}

// 0x00011D70
// Turns the second piece a quarter round the first, if there is room.
// The column checks are made twice, as in the original.
void CBoard::Rotate()
{
    if (!m_pieces[0].m_type || !m_pieces[1].m_type)
        return;
    switch (m_rotation)
    {
    case 0:
        if (m_pieces[0].m_row > 0 && !Get(m_pieces[0].m_col, m_pieces[0].m_row - 1))
        {
            m_pieces[1].m_row--;
            m_pieces[1].m_col--;
            m_pieces[1].m_fx = m_pieces[0].m_fx;
            m_pieces[1].m_fy = m_pieces[0].m_fy - m_nScale * 2;
            m_rotation++;
        }
        break;
    case 1:
        if (m_pieces[0].m_col > 0 && !Get(m_pieces[0].m_col - 1, m_pieces[0].m_row) && m_pieces[0].m_col > 0)
        {
            m_pieces[1].m_row++;
            m_pieces[1].m_col--;
            m_pieces[1].m_fx = m_pieces[0].m_fx - m_nScale * 2;
            m_pieces[1].m_fy = m_pieces[0].m_fy;
            m_rotation++;
        }
        break;
    case 2:
        if (!Get(m_pieces[0].m_col, m_pieces[0].m_row + 1))
        {
            m_pieces[1].m_row++;
            m_pieces[1].m_col++;
            m_pieces[1].m_fx = m_pieces[0].m_fx;
            m_pieces[1].m_fy = m_nScale * 2 + m_pieces[0].m_fy;
            m_rotation++;
        }
        break;
    case 3:
        if (m_pieces[0].m_col < 5 && !Get(m_pieces[0].m_col + 1, m_pieces[0].m_row) && m_pieces[0].m_col < 5)
        {
            m_pieces[1].m_row--;
            m_pieces[1].m_col++;
            m_pieces[1].m_fx = m_nScale * 2 + m_pieces[0].m_fx;
            m_pieces[1].m_fy = m_pieces[0].m_fy;
            m_rotation = 0;
        }
        break;
    }
}

// 0x00011FA0
void CBoard::Begin()
{
    m_bAlive = true;
    m_bLost = false;
}

// 0x00011FB0
// Drops the garbage owed into the free cells of the top row; done when
// it is all in and nothing is falling.
void CBoard::DropGarbage()
{
    if (m_nGarbage)
    {
        for (int col = 0; col < BOARD_COLS; col++)
        {
            if (m_nGarbage && m_grid[9 * BOARD_COLS + col] == 0)
            {
                m_grid[9 * BOARD_COLS + col] = rand() % 8 + 1;
                m_nGarbage--;
            }
        }
        if (m_nGarbage)
            return;
    }
    if (!m_bFalling)
        m_bGarbage = false;
}

// 0x00012020
// Lets every piece with a hole under it fall one row. No hole in the bottom
// nine rows: the board is full, and the lion comes for it.
void CBoard::Settle()
{
    BYTE bMoved = FALSE;
    bool bFull = true;
    for (int row = 0; row < 9; row++)
    {
        for (int col = 0; col < BOARD_COLS; col++)
        {
            if (m_grid[row * BOARD_COLS + col] == 0)
            {
                int above = m_grid[(row + 1) * BOARD_COLS + col];
                bFull = false;
                if (above)
                {
                    bMoved = true;
                    m_grid[row * BOARD_COLS + col] = above;
                    m_grid[(row + 1) * BOARD_COLS + col] = 0;
                }
            }
        }
    }
    m_bFalling = bMoved != 0;
    if (bFull)
    {
        if (m_bAlive)
        {
            m_botX = (float)(m_nCellSize * 3 + m_bx);
            m_botY = (float)m_by;
            m_topX = m_botX;
            m_topY = (float)(m_by + m_nCellSize * 9);
            m_bLost = true;
        }
        m_bAlive = false;
    }
}

// 0x000120F0
void CBoard::Set(int col, int row, int value)
{
    m_grid[row * BOARD_COLS + col] = value;
}

// 0x00012110
// No bounds check: the moves ask for column -1 and 6 (the row's
// neighbours) and row 10.
int CBoard::Get(int col, int row)
{
    return m_grid[row * BOARD_COLS + col];
}

// 0x00012130
void CBoard::MoveLeft()
{
    if (m_pieces[0].m_type && m_pieces[1].m_type)
    {
        if (!Get(m_pieces[0].m_col - 1, m_pieces[0].m_row) && !Get(m_pieces[1].m_col - 1, m_pieces[1].m_row) &&
            m_pieces[0].m_col > 0 && m_pieces[1].m_col > 0)
        {
            m_pieces[0].m_col--;
            m_pieces[1].m_col--;
        }
    }
}

// 0x000121A0
void CBoard::MoveRight()
{
    if (m_pieces[0].m_type && m_pieces[1].m_type)
    {
        if (!Get(m_pieces[0].m_col + 1, m_pieces[0].m_row) && !Get(m_pieces[1].m_col + 1, m_pieces[1].m_row) &&
            m_pieces[0].m_col < 5 && m_pieces[1].m_col < 5)
        {
            m_pieces[0].m_col++;
            m_pieces[1].m_col++;
        }
    }
}

// 0x00012220
// The computer. Hard: steers the first piece to the topmost, rightmost
// piece of its kind (its food, for a head). Otherwise (and once there) it
// takes turns by pair: drop, left, right.
void CBoard::Think(bool bHard)
{
    if (bHard)
    {
        if (m_bAimed)
            return;
        bool bFound = false;
        for (int row = 9; row >= 0; row--)
        {
            for (int col = 5; col >= 0; col--)
            {
                if (bFound)
                    break;
                if (!m_bAimed && (m_pieces[0].m_type == Get(col, row) || m_pieces[0].m_type - 4 == Get(col, row)))
                {
                    if (m_pieces[0].m_col < col)
                    {
                        if (col == 5 && m_rotation == 0)
                            Rotate();
                        MoveRight();
                    }
                    if (m_pieces[0].m_col > col)
                        MoveLeft();
                    if (m_pieces[0].m_col == col)
                        m_bAimed = true;
                    bFound = true;
                    break;
                }
            }
        }
        if (bFound)
            return;
    }

    switch (m_nAiMove)
    {
    case 0:
        if (m_pieces[0].m_type && m_pieces[1].m_type)
            Drop(8);
        break;
    case 1:
        MoveLeft();
        break;
    case 2:
        MoveRight();
        break;
    }
}

// 0x00012330
bool CBoard::IsListed(int eater, int cell)
{
    for (int i = 0; i < m_eaters[eater].m_nEat; i++)
    {
        if (m_eaters[eater].m_eat[i] == cell)
            return true;
    }
    return false;
}

// 0x00012370
// Every piece eaten sends one piece of garbage.
void CBoard::List(int eater, int cell)
{
    m_eaters[eater].m_eat[m_eaters[eater].m_nEat] = cell;
    m_eaters[eater].m_nEat++;
    m_nSend++;
}

// ---- the animals ----------------------------------------------------------------------

// 0x000123B0
// Eats the first cell of the list: a chomp, crumbs, the cell emptied, and
// the list moved up.
void CEater::EatNext()
{
    if (m_nEat == 0)
        return;

    bool bDone = false;
    int cell = 0;
    for (int row = 0; row < BOARD_ROWS; row++)
    {
        if (bDone || cell > m_eat[0])
            break;
        for (int col = 0; col < BOARD_COLS; col++)
        {
            if (bDone)
                break;
            if (cell == m_eat[0])
            {
                if (g_bSoundEffects)
                    g_SoundEat.Play();
                m_nChomp = 1;
                for (int i = 0; i < 10; i++)
                {
                    if (m_pCrumbs[i].m_dwStart == 0)
                    {
                        m_pCrumbs[i].Start(m_fx, m_fy, m_pGrid[row * BOARD_COLS + col]);
                        break;
                    }
                }
                m_pGrid[row * BOARD_COLS + col] = 0;
                bDone = true;
            }
            cell++;
        }
    }

    for (int i = 0; i < m_nEat; i++)
        m_eat[i] = m_eat[i + 1];
    m_eat[m_nEat - 1] = -1;
    m_nEat--;
    m_tx = m_ty = -1.0f;
}

// 0x000124E0
// Heads for the first cell of the list (the argument is not used).
void CEater::Aim(int unused)
{
    m_tx = -1.0f;
    m_ty = -1.0f;
    int i = 0;
    for (int row = 0; row < BOARD_ROWS; row++)
    {
        for (int col = 0; col < BOARD_COLS; col++)
        {
            if (i == m_eat[0])
            {
                m_tx = (float)(m_x + col * m_nCell);
                m_ty = (float)(m_nCell * row + m_y);
            }
            i++;
        }
    }
}

// ---- the falling pair, drawn -------------------------------------------------------------

// A quad from -1 to 1 (a triangle strip), facing the camera or mirrored.
#define SET_QUAD(v, uv) \
    v[0].x = -1.0f; v[0].y =  1.0f; v[0].z = 0.0f; v[0].tu = uv.tu1; v[0].tv = uv.tv2; \
    v[1].x =  1.0f; v[1].y =  1.0f; v[1].z = 0.0f; v[1].tu = uv.tu2; v[1].tv = uv.tv1; \
    v[2].x = -1.0f; v[2].y = -1.0f; v[2].z = 0.0f; v[2].tu = uv.tu4; v[2].tv = uv.tv3; \
    v[3].x =  1.0f; v[3].y = -1.0f; v[3].z = 0.0f; v[3].tu = uv.tu3; v[3].tv = uv.tv4

#define SET_QUAD_MIRRORED(v, uv) \
    v[0].x = -1.0f; v[0].y =  1.0f; v[0].z = 0.0f; v[0].tu = uv.tu2; v[0].tv = uv.tv1; \
    v[1].x =  1.0f; v[1].y =  1.0f; v[1].z = 0.0f; v[1].tu = uv.tu1; v[1].tv = uv.tv2; \
    v[2].x = -1.0f; v[2].y = -1.0f; v[2].z = 0.0f; v[2].tu = uv.tu3; v[2].tv = uv.tv4; \
    v[3].x =  1.0f; v[3].y = -1.0f; v[3].z = 0.0f; v[3].tu = uv.tu4; v[3].tv = uv.tv3

// 0x00012620
// The spin, every 50 ms: frame 0 of the 4x2 sheet for 14 steps, then its
// other seven frames. All the pieces share it (and one vertex buffer); the
// first one drawn each time moves it on.
void CPiece::UpdateVertices()
{
    if (GetTickCount() - s_dwPieceTime < 50 && s_dwPieceTime != 0)
        return;
    s_dwPieceTime = GetTickCount();

    SFrameUV& uv = s_pieceUV;
    if (g_nPieceFrame > 13)
        { SET_FRAME(s_pieceFrame, uv, g_nPieceFrame - 13, 4, 2); }
    else
        { SET_FRAME(s_pieceFrame, uv, 0, 4, 2); }
    SET_QUAD(m_v, uv);


    if (s_pPieceVB == NULL)
        m_pDevice->CreateVertexBuffer(4 * sizeof(m_v), 0, 0, 0, &s_pPieceVB);
    void* pVertices;
    s_pPieceVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, m_v, sizeof(m_v));
    s_pPieceVB->Unlock();

    g_nPieceFrame++;
    if (g_nPieceFrame > 20)
        g_nPieceFrame = 0;
}

// 0x00012880
// Still: the piece's texture; spinning: its spin sheet (type + 8).
void CPiece::Render()
{
    if (m_type == 0)
        return;
    UpdateVertices();

    D3DXMATRIX matScale, matTrans, matWorld;
    D3DXMatrixScaling(&matScale, (float)m_nScale, (float)m_nScale, 1.0f);
    D3DXMatrixTranslation(&matTrans, m_fx, m_fy, (float)m_z);
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
    m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    if (g_nPieceFrame <= 13)
    {
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(m_type);
    }
    if (g_nPieceFrame > 13)
    {
        m_pDevice->SetStreamSource(0, s_pPieceVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(m_type + 8);
    }
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    m_pDevice->SetTexture(0, NULL);
}

// ---- the board -----------------------------------------------------------------------------

// 0x00012980
void CBoard::Reset()
{
    memset(m_bEating, 0, sizeof(m_bEating));
    m_bUnused928 = false;
    m_nLevel = 1;
    for (int i = 0; i < BOARD_ROWS * BOARD_COLS; i++)
        m_grid[i] = 0;

    m_pieces[0].m_type = 0;
    m_pieces[1].m_type = 0;
    m_bFalling = false;
    m_nGarbage = 0;
    m_nSend = 0;
    m_bGarbage = false;
    m_bShut = false;
}

// 0x000129E0
// From a head, lists the food of its kind touching it, and the food
// touching that, for its animal (type 5 + n: animal n). Past the first
// piece, a cell of the head's own kind next to food carries on the chain.
// The first cell listed is the head itself. List is inlined into some of
// the calls and not others, as in the original.
void CBoard::Flood(int col, int row, int type)
{
    // Below.
    if (row > 0 &&
        (Get(col, row - 1) == type - 4 ||
         (Get(col, row - 1) == type && m_eaters[type - 5].m_nEat > 0 && Get(col, row) == type - 4)))
    {
        if (!IsListed(type - 5, (row - 1) * BOARD_COLS + col))
        {
            if (m_eaters[type - 5].m_nEat == 0)
                List(type - 5, row * BOARD_COLS + col);
            List(type - 5, (row - 1) * BOARD_COLS + col);
            Flood(col, row - 1, type);
        }
    }
    // Left.
    if (col > 0 &&
        (Get(col - 1, row) == type - 4 ||
         (Get(col - 1, row) == type && m_eaters[type - 5].m_nEat > 0 && Get(col, row) == type - 4)))
    {
        if (!IsListed(type - 5, row * BOARD_COLS + col - 1))
        {
            if (m_eaters[type - 5].m_nEat == 0)
                List(type - 5, row * BOARD_COLS + col);
            List(type - 5, row * BOARD_COLS + col - 1);
            Flood(col - 1, row, type);
        }
    }
    // Above.
    if (row < 9 &&
        (Get(col, row + 1) == type - 4 ||
         (Get(col, row + 1) == type && m_eaters[type - 5].m_nEat > 0 && Get(col, row) == type - 4)))
    {
        if (!IsListed(type - 5, (row + 1) * BOARD_COLS + col))
        {
            if (m_eaters[type - 5].m_nEat == 0)
                List(type - 5, row * BOARD_COLS + col);
            List(type - 5, (row + 1) * BOARD_COLS + col);
            Flood(col, row + 1, type);
        }
    }
    // Right.
    if (col < 5 &&
        (Get(col + 1, row) == type - 4 ||
         (Get(col + 1, row) == type && m_eaters[type - 5].m_nEat > 0 && Get(col, row) == type - 4)))
    {
        if (!IsListed(type - 5, row * BOARD_COLS + col + 1))
        {
            if (m_eaters[type - 5].m_nEat == 0)
                List(type - 5, row * BOARD_COLS + col);
            List(type - 5, row * BOARD_COLS + col + 1);
            Flood(col + 1, row, type);
        }
    }
}

// ---- the animals ----------------------------------------------------------------------------

// 0x00012DB0
// At home below the board, in its column.
void CEater::Init(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int* pGrid, int index, int nCell, int nScale,
                  int x, int y, int z, CParticles* pParticles)
{
    m_pDevice = pDevice;
    m_pTextures = pTextures;
    m_pParticles = pParticles;
    m_pGrid = pGrid;
    m_index = index;
    m_nCell = nCell;
    m_nScale = nScale;
    m_x = x;
    m_y = y;
    m_z = z;
    m_homeX = (float)x;
    m_homeY = (float)(y + nCell * 10);
    m_nSpeed = 5;
    memset(m_eat, -1, sizeof(m_eat));
    m_fScaleX = m_fScaleY = m_fCell;
    m_nEat = 0;
    m_bHome = true;
    m_nChomp = 0;
    m_frame = 0;
    m_tx = m_ty = -1.0f;
}

// 0x00012E80
// With a list: jumps to its first cell (the head), then walks to each
// piece and eats it; done, walks home (twice as fast up and down).
// Puffs follow it.
void CEater::Update()
{
    if (m_nChomp)
        m_nChomp++;
    if (m_nChomp > 5)
        m_nChomp = 0;

    if (m_bHome)
    {
        if (m_nEat)
        {
            int first = m_eat[0];
            m_bHome = false;
            int i = 0;
            for (int row = 0; row < BOARD_ROWS; row++)
            {
                for (int col = 0; col < BOARD_COLS; col++)
                {
                    if (i == first)
                    {
                        m_fx = (float)(m_x + col * m_nCell);
                        m_fy = (float)(m_nCell * row + m_y);
                    }
                    i++;
                }
            }
        }
        if (m_bHome)
            return;
    }

    if (m_pParticles)
        m_pParticles->Add(m_fx, (float)(rand() % (int)m_fCell) + (m_fy - m_fCell), (float)m_z);

    if (!m_bHome && m_nEat == 0)
    {
        for (int i = 0; i < m_nSpeed; i++)
        {
            if (m_fx == m_homeX && m_fy == m_homeY)
            {
                m_bHome = true;
                break;
            }
            if (m_fx < m_homeX)
                m_fx += 1.0f;
            if (m_fx > m_homeX)
                m_fx -= 1.0f;
            if (m_fy < m_homeY)
                m_fy += 1.0f;
            if (m_fy > m_homeY)
                m_fy -= 1.0f;
            if (m_fy < m_homeY)
                m_fy += 1.0f;
            if (m_fy > m_homeY)
                m_fy -= 1.0f;
        }
    }
    if (!m_bHome && m_nEat)
    {
        for (int i = 0; i < m_nSpeed; i++)
        {
            if (m_fx == m_tx && m_fy == m_ty)
                EatNext();
            if (m_tx == -1.0f || m_tx == 0.0f)
                Aim(m_eat[0]);
            if (m_fx < m_tx)
                m_fx += 1.0f;
            if (m_fx > m_tx)
                m_fx -= 1.0f;
            if (m_fy < m_ty)
                m_fy += 1.0f;
            if (m_fy > m_ty)
                m_fy -= 1.0f;
        }
    }
}

// 0x000132B0
// Walking: frame m_frame of the 4x2 sheet (it counts to 17, past the
// sheet's 8: texture wrapping shows the rest); chomping: the whole
// texture. It faces the way it is going. All four share the frame and one
// vertex buffer.
void CEater::UpdateVertices()
{
    SFrameUV& uv = s_eaterUV;
    if (m_nChomp == 0)
        { SET_FRAME(s_eaterFrame, uv, m_frame, 4, 2); }
    if (m_nChomp != 0)
        { SET_FRAME(s_eaterFrame, uv, 1, 1, 1); }

    { SET_QUAD(m_v, uv); }

    if (!m_bHome && m_nEat == 0)
    {
        if (m_fx <= m_homeX)
            { SET_QUAD(m_v, uv); }
        if (m_homeX <= m_fx)
            { SET_QUAD_MIRRORED(m_v, uv); }
    }
    if (!m_bHome && m_nEat != 0)
    {
        if (m_fx <= m_tx)
            { SET_QUAD(m_v, uv); }
        if (m_tx <= m_fx)
            { SET_QUAD_MIRRORED(m_v, uv); }
    }

    if (s_pEaterVB == NULL)
        m_pDevice->CreateVertexBuffer(4 * sizeof(m_v), 0, 0, 0, &s_pEaterVB);
    void* pVertices;
    s_pEaterVB->Lock(0, 0, (BYTE**)&pVertices, 0);
    memcpy(pVertices, m_v, sizeof(m_v));
    s_pEaterVB->Unlock();

    if (GetTickCount() - s_dwEaterTime >= 50 || s_dwEaterTime == 0)
    {
        s_dwEaterTime = GetTickCount();
        m_frame++;
        if (m_frame > 17)
            m_frame = 0;
    }
}

// ---- the board ---------------------------------------------------------------------------------

// 0x00013850
// The board's corner, cell size and piece size; the pieces, the animals
// (one per column from the left) and the crumbs.
void CBoard::Init(LPDIRECT3DDEVICE8 pDevice, CTextures* pTextures, int nCell, int nScale, int x, int y, int z,
                  CParticles* pParticles, int portrait)
{
    m_bz = z;
    m_pParticles = pParticles;
    m_pTextures2 = pTextures;
    m_bx = x;
    m_nPieceScale = nScale;
    m_pDevice2 = pDevice;
    m_nCellSize = nCell;
    m_bUnused928 = false;
    m_by = y;
    m_bLost = false;
    SetupPieces(pDevice, pTextures, nCell, nScale, x, y, z, portrait);

    for (int i = 0; i < 4; i++)
    {
        m_eaters[i].m_fCell = (float)m_nCellSize;
        m_eaters[i].Init(pDevice, pTextures, m_grid, i + 1, m_nCellSize, m_nPieceScale, m_bx, m_by, m_bz,
                         pParticles);
        m_eaters[i].m_pCrumbs = m_crumbs;
    }
    for (int i = 0; i < 10; i++)
    {
        m_crumbs[i].m_pTextures = pTextures;
        m_crumbs[i].m_pDevice = pDevice;
        float z = (float)m_bz;
        for (int j = 0; j < 4; j++)
        {
            m_crumbs[i].m_crumbs[j].z = z;
            m_crumbs[i].m_crumbs[j].fScale = 5.0f;
        }
    }
    m_nUnused23C = 0;
}

// 0x000139B0
// Every head on the board lists what its animal can eat.
void CBoard::FindMatches()
{
    for (int row = 0; row < BOARD_ROWS; row++)
    {
        for (int col = 0; col < BOARD_COLS; col++)
        {
            int type = Get(col, row);
            if (type > 4)
                Flood(col, row, type);
        }
    }
}

// ---- the animals, drawn -----------------------------------------------------------------------

// 0x000139F0
void CEater::Render()
{
    if (m_bHome)
        return;
    UpdateVertices();

    D3DXMATRIX matScale, matTrans, matWorld;
    D3DXMatrixScaling(&matScale, m_fScaleX, m_fScaleY, 1.0f);
    D3DXMatrixTranslation(&matTrans, m_fx, m_fy, (float)m_z);
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
    m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    if (m_nChomp == 0)
    {
        m_pDevice->SetStreamSource(0, s_pEaterVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(m_index + 16);
    }
    if (m_nChomp != 0)
    {
        m_pDevice->SetStreamSource(0, s_pEaterVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures->Set(m_index + 120);
    }
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    m_pDevice->SetTexture(0, NULL);
}

// ---- the board ------------------------------------------------------------------------------------

// 0x00013B00
// A frame of play: the garbage an animal's meal cancels, the pair falling
// (fast once one has landed), matches when everything is still, gravity,
// the next pair, the animals; and the lion's jaws for a lost board (they
// close, then chew for 3 s).
void CBoard::Update()
{
    for (int i = 0; i < 4; i++)
    {
        if (m_bEating[i])
            continue;
        if (m_eaters[i].m_nEat == 0)
            m_bEating[i] = false;
        if (m_eaters[i].m_nEat >= 2)
        {
            m_bEating[i] = true;
            if (m_nGarbage)
            {
                m_nGarbage -= m_eaters[i].m_nEat;
                if (m_nGarbage < 0)
                    m_nGarbage = 0;
            }
        }
    }
    if (m_nSpeed != m_nLevel)
        m_nSpeed = m_nLevel;
    for (int i = 0; i < 10; i++)
        m_crumbs[i].Update();

    if (m_bAlive)
    {
        if (m_bGarbage && !m_pieces[0].m_type && !m_pieces[1].m_type)
            DropGarbage();
        SlideRight(5);
        SlideLeft(5);
        if (m_pieces[0].m_type && m_pieces[1].m_type)
            Drop(m_nBoost + m_nSpeed);
        m_nBoost = 0;
        if ((m_pieces[0].m_type ? true : false) != (m_pieces[1].m_type ? true : false))
            Drop(m_nSpeed * 5);
        if (!m_bFalling && !m_pieces[0].m_type && !m_pieces[1].m_type)
            FindMatches();
        if (m_eaters[0].m_nEat == 0 && m_eaters[1].m_nEat == 0 && m_eaters[2].m_nEat == 0 &&
            m_eaters[3].m_nEat == 0 && !m_pieces[0].m_type && !m_pieces[1].m_type)
            Settle();

        // Columns 2 and 3 of the top row are where a pair comes in: full,
        // the board has lost.
        if ((m_grid[9 * BOARD_COLS + 2] || m_grid[9 * BOARD_COLS + 3]) && !m_bGarbage)
        {
            if (!m_bFalling)
            {
                if (m_bAlive)
                {
                    m_botX = (float)(m_nCellSize * 3 - m_nCellSize / 2 + m_bx);
                    m_botY = (float)m_by;
                    m_topX = m_botX;
                    m_topY = (float)(m_by + m_nCellSize * 9);
                    m_bLost = true;
                }
                m_bAlive = false;
            }
        }
        if (!m_bGarbage && !m_pieces[0].m_type && !m_pieces[1].m_type && m_eaters[0].m_nEat == 0 &&
            m_eaters[1].m_nEat == 0 && m_eaters[2].m_nEat == 0 && m_eaters[3].m_nEat == 0 && !m_bFalling)
        {
            FindMatches();
            if (m_eaters[0].m_nEat == 0 && m_eaters[1].m_nEat == 0 && m_eaters[2].m_nEat == 0 &&
                m_eaters[3].m_nEat == 0)
                NextPair();
        }
        for (int i = 0; i < 4; i++)
            m_eaters[i].Update();
    }

    if (!m_bLost)
        return;
    if (!m_bShut)
    {
        // The jaws close on the board.
        if (m_botY < m_nCellSize / 2 + m_by + m_nCellSize)
            m_botY += 1.0f;
        if (m_by + m_nCellSize * 6 < m_topY)
            m_topY -= 1.0f;
        if (m_botY < m_nCellSize / 2 + m_by + m_nCellSize)
            m_botY += 1.0f;
        if (m_by + m_nCellSize * 6 < m_topY)
            m_topY -= 1.0f;
    }
    if (!m_bShut)
    {
        if (m_botY != m_nCellSize / 2 + m_by + m_nCellSize)
            return;
        if (m_topY != m_by + m_nCellSize * 6)
            return;
    }
    if (!m_bShut)
    {
        m_dwShutTime = GetTickCount();
        m_bShut = true;
    }
    if (GetTickCount() - m_dwShutTime < 3000)
    {
        // Chewing: the bottom jaw goes side to side, and up and down at
        // three times the speed.
        if (m_botX < m_bx + m_nCellSize * 2 + m_nCellSize && m_bChewRight)
            m_botX += 1.0f;
        if (m_botX == m_bx + m_nCellSize * 2 + m_nCellSize && m_bChewRight)
            m_bChewRight = false;
        if (!m_bChewRight && m_nCellSize / 2 + m_bx + m_nCellSize * 2 < m_botX)
            m_botX -= 1.0f;
        if (m_botX == m_bx + m_nCellSize * 2 + m_nCellSize / 2 && !m_bChewRight)
            m_bChewRight = true;

        if (m_botY < m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_botY += 1.0f;
        if (m_botY == m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_bChewUp = false;
        if (!m_bChewUp && m_by - m_nCellSize / 2 + m_nCellSize < m_botY)
            m_botY -= 1.0f;
        if (m_botY == m_by - m_nCellSize / 2 + m_nCellSize && !m_bChewUp)
            m_bChewUp = true;
        if (m_botY < m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_botY += 1.0f;
        if (m_botY == m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_bChewUp = false;
        if (!m_bChewUp && m_by - m_nCellSize / 2 + m_nCellSize < m_botY)
            m_botY -= 1.0f;
        if (m_botY == m_by - m_nCellSize / 2 + m_nCellSize && !m_bChewUp)
            m_bChewUp = true;
        if (m_botY < m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_botY += 1.0f;
        if (m_botY == m_by + m_nCellSize / 2 + m_nCellSize && m_bChewUp)
            m_bChewUp = false;
        if (!m_bChewUp && m_by - m_nCellSize / 2 + m_nCellSize < m_botY)
            m_botY -= 1.0f;
        if (m_botY == m_by - m_nCellSize / 2 + m_nCellSize && !m_bChewUp)
            m_bChewUp = true;
    }
}

// 0x00014160
// The cells (an empty one is the plain colored quad), the garbage bar
// right of the board (its height the garbage owed, at most 42), the pair
// and the animals, the queue, the portrait, the lion and the crumbs.
void CBoard::Render()
{
    D3DXMATRIX matScale, matTrans, matWorld;
    for (int row = 0; row < BOARD_ROWS; row++)
    {
        for (int col = 0; col < BOARD_COLS; col++)
        {
            D3DXMatrixScaling(&matScale, (float)m_nPieceScale, (float)m_nPieceScale, 1.0f);
            D3DXMatrixTranslation(&matTrans, (float)(m_nCellSize * col + m_bx), (float)(m_nCellSize * row + m_by),
                                  (float)m_bz);
            D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
            m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            if (m_grid[row * BOARD_COLS + col] == 0)
            {
                m_pDevice->SetStreamSource(0, g_pColorQuadVB, sizeof(COLORVERTEX));
                m_pDevice->SetVertexShader(D3DFVF_COLORVERTEX);
            }
            if (m_grid[row * BOARD_COLS + col] != 0)
            {
                m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
                m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
                m_pTextures2->Set(m_grid[row * BOARD_COLS + col]);
            }
            m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            m_pDevice->SetTexture(0, NULL);
        }
    }

    int nGarbage = m_nGarbage;
    if (nGarbage > 42)
        nGarbage = 42;
    D3DXMATRIX matBarScale, matBarTrans, matBar;
    D3DXMatrixScaling(&matBarScale, (float)(m_nPieceScale / 4), (float)(m_nPieceScale * nGarbage / 4), 1.0f);
    D3DXMatrixTranslation(&matBarTrans, (float)(m_bx + m_nCellSize * 6 - m_nCellSize * 0.4 + m_nPieceScale / 5),
                          (float)(m_nPieceScale * nGarbage / 4 - m_nCellSize / 2 + m_by), (float)m_bz);
    D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
    m_pDevice->SetTransform(D3DTS_WORLD, &matBar);
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetStreamSource(0, g_pBarVB, sizeof(COLORVERTEX));
    m_pDevice->SetVertexShader(D3DFVF_COLORVERTEX);
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    if (m_bAlive)
    {
        m_pieces[0].Render();
        m_pieces[1].Render();
        if (m_bShowEaters)
        {
            for (int i = 0; i < 4; i++)
                m_eaters[i].Render();
        }
    }

    for (int i = 0; i < 5; i++)
    {
        if (m_preview[i].type == 0)
            continue;
        D3DXMatrixScaling(&matBarScale, (float)m_preview[i].nScale, (float)m_preview[i].nScale, 1.0f);
        D3DXMatrixTranslation(&matBarTrans, m_preview[i].fx, m_preview[i].fy, (float)m_preview[i].z);
        D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matBar);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_preview[i].pTextures->Set(m_preview[i].type);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);
    }

    D3DXMatrixScaling(&matBarScale, (float)m_portrait.nScale, (float)m_portrait.nScale, 1.0f);
    D3DXMatrixTranslation(&matBarTrans, m_portrait.fx, m_portrait.fy, (float)m_portrait.z);
    D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
    m_pDevice->SetTransform(D3DTS_WORLD, &matBar);
    m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
    m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
    m_portrait.pTextures->Set(m_portrait.type);
    m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    m_pDevice->SetTexture(0, NULL);

    if (m_bLost)
    {
        D3DXMatrixScaling(&matBarScale, (float)(m_nPieceScale * 6), (float)(m_nPieceScale * 4), 1.0f);
        D3DXMatrixTranslation(&matBarTrans, m_botX, m_botY, (float)m_bz);
        D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matBar);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures2->Set(46);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);

        D3DXMatrixScaling(&matBarScale, (float)(m_nPieceScale * 8), (float)(m_nPieceScale * 11), 1.0f);
        D3DXMatrixTranslation(&matBarTrans, m_topX, m_topY, (float)m_bz);
        D3DXMatrixMultiply(&matBar, &matBarScale, &matBarTrans);
        m_pDevice->SetTransform(D3DTS_WORLD, &matBar);
        m_pDevice->SetStreamSource(0, g_pQuadVB, sizeof(QUADVERTEX));
        m_pDevice->SetVertexShader(D3DFVF_QUADVERTEX);
        m_pTextures2->Set(45);
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        m_pDevice->SetTexture(0, NULL);
    }

    for (int i = 0; i < 10; i++)
        m_crumbs[i].Render();
}
