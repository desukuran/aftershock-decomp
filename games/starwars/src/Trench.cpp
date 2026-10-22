// The Death Star's trench: twelve segments scrolled past the player, who
// stays put, recycled to the front; after 32 of them, the end with the
// exhaust port.
#include "Game.h"

#define TRENCH_SEGMENTS     12
#define TRENCH_LENGTH       227         // one segment
#define TRENCH_PASSES       32          // segments recycled before the end comes

// 0x00027CF0
// From the flight to the Death Star: the Star Destroyers and the glows go,
// Vader and his wingmen line up ahead, and the dive into the trench starts
// 40 above it.
void CTrench::Start(CGame* pGame)
{
    delete[] pGame->pStarDestroyers;
    pGame->pStarDestroyers = NULL;
    pGame->glows.Clear();
    pGame->vaders[0].Reset();
    pGame->vaders[1].Reset();
    pGame->vaders[2].Reset();

    pSegments = new CProp[TRENCH_SEGMENTS];
    pEnd = new CProp;
    int i, z;
    for (i = 0, z = 0; z < TRENCH_SEGMENTS * TRENCH_LENGTH; i++, z += TRENCH_LENGTH)
    {
        pSegments[i].SetMesh(&pGame->meshTrench);
        pSegments[i].bVisible = true;
        pSegments[i].vPos = D3DXVECTOR3(60.0f, 0.0f, (float)z);
    }
    pEnd->SetMesh(&pGame->meshTrenchEnd);
    pEnd->bVisible = false;
    pEnd->vPos = D3DXVECTOR3(60.0f, 0.0f, 12712.0f);

    pGame->mode = MODE_TRENCH_ENTRY;
    CXWing* pPlayer = pGame->pXWings;
    pPlayer->vPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    pPlayer->qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
    CVader* pVader = pGame->vaders;
    for (int x = -10; x < 5; x += 5)
    {
        pVader->SetPosition((float)x, 10.0f, 10.0f);
        pVader++;
    }

    nPassed = 0;
    for (i = 0; i < TRENCH_SEGMENTS; i++)
        pSegments[i].vPos = D3DXVECTOR3(pGame->pXWings->vPos.x, pGame->pXWings->vPos.y - 10.0f,
                                        (float)(i * TRENCH_LENGTH));
    D3DXVECTOR3 v(pGame->pXWings->vPos.x, 40.0f, pGame->pXWings->vPos.z);
    pGame->pXWings->SetPosition(v);
}

// 0x00028160
// A segment 150 behind the player goes after the furthest one. The trench
// comes by 5 a frame, 1 once the port is in 300; at 300 the run ends
// (torpedoes or not) and the X-wings level out.
void CTrench::Update(CGame* pGame)
{
    int i;
    for (i = 0; i < TRENCH_SEGMENTS; i++)
    {
        if (pSegments[i].vPos.z < pGame->pXWings->vPos.z - 150.0f)
        {
            int nFurthest = 0;
            for (int k = 0; k < TRENCH_SEGMENTS; k++)
            {
                if ((float)nFurthest <= pSegments[k].vPos.z)
                    nFurthest = (int)pSegments[k].vPos.z;
            }
            if (nPassed == TRENCH_PASSES)
            {
                // Quirk: the end goes one length on, and the segment one more
                // (nFurthest itself moves), leaving a gap before the end.
                nFurthest += TRENCH_LENGTH;
                pEnd->vPos = D3DXVECTOR3(pSegments[i].vPos.x, pSegments[i].vPos.y, (float)nFurthest);
                pEnd->bVisible = true;
            }
            nPassed++;
            pSegments[i].SetPosition(pSegments[i].vPos.x, pSegments[i].vPos.y,
                                     (float)(nFurthest + TRENCH_LENGTH));
        }
    }
    for (i = 0; i < TRENCH_SEGMENTS; i++)
        pSegments[i].SetPosition(pSegments[i].vPos.x, pSegments[i].vPos.y,
                                 pSegments[i].vPos.z - (fHoleDistance >= 300.0f ? 5.0f : 1.0f));
    pEnd->SetPosition(pEnd->vPos.x, pEnd->vPos.y, pEnd->vPos.z - (fHoleDistance >= 300.0f ? 5.0f : 1.0f));

    D3DXVECTOR3 v = pEnd->vPos - pGame->pXWings->vPos;
    fHoleDistance = D3DXVec3Length(&v);
    if (pGame->mode == MODE_TRENCH && fHoleDistance < 300.0f)
    {
        pGame->mode = MODE_TRENCH_EXIT;
        pGame->dwTrenchExitTime = GetTickCount();
        for (i = 0; i < pGame->nXWings; i++)
            pGame->pXWings[i].qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

// 0x000284C0
void CTrench::Free()
{
    if (pSegments)
    {
        delete[] pSegments;
        pSegments = NULL;
    }
    if (pEnd)
    {
        delete pEnd;
        pEnd = NULL;
    }
}

// 0x00028500
void CTrench::Render(CGame* pGame)
{
    for (int i = 0; i < TRENCH_SEGMENTS; i++)
        pSegments[i].Render(pGame->pDevice);
    pEnd->Render(pGame->pDevice);
}
