// The proton torpedoes of the trench run.
#include "Game.h"

// 0x000226A0
// The four tubes are the X-wing's gun positions.
void CTorpedoes::Create(CGame* pGame)
{
    char strFile[256];
    for (int i = 0; i < 8; i++)
    {
        sprintf(strFile, "D:\\media\\proton%i.tga", i);
        D3DXCreateTextureFromFileA(pGame->pDevice, strFile, &texFrames[i]);
    }
    torpedoes[0].dwLaunch = 0;
    torpedoes[1].dwLaunch = 0;
    torpedoes[2].dwLaunch = 0;
    torpedoes[3].dwLaunch = 0;
    vTubes[0] = D3DXVECTOR3(-1.471f, 0.405f, 1.503f);
    vTubes[1] = D3DXVECTOR3(-1.482f, -0.356f, 1.504f);
    vTubes[2] = D3DXVECTOR3(1.471f, 0.405f, 1.503f);
    vTubes[3] = D3DXVECTOR3(1.482f, -0.356f, 1.504f);
}

// 0x00022850
// All four tubes that are free fire at once.
void CTorpedoes::Fire(D3DXVECTOR3 vPos)
{
    for (int i = 0; i < 4; i++)
    {
        if (torpedoes[i].dwLaunch == 0)
        {
            torpedoes[i].vPos = D3DXVECTOR3(vTubes[i].x + vPos.x, vTubes[i].y + vPos.y, vTubes[i].z + vPos.z);
            torpedoes[i].dwLaunch = GetTickCount();
        }
    }
}

// 0x00022800
// Straight on at 5 a frame for 3 s.
void CTorpedoes::Update()
{
    for (int i = 0; i < 4; i++)
    {
        DWORD dwLaunch = torpedoes[i].dwLaunch;
        if (GetTickCount() - dwLaunch < 3000)
        {
            if (dwLaunch != 0)
                torpedoes[i].vPos.z += 5.0f;
        }
        else
        {
            torpedoes[i].dwLaunch = 0;
        }
    }
}

// 0x000228D0
// A strip of the unit cube with proton0..7.tga, animated over 800 ms
// (quirk: (t / 800) * 7 in integers, so only frames 0 and 7 show).
void CTorpedoes::Render(CGame* pGame)
{
    DWORD dwNow = GetTickCount();
    for (int i = 0; i < 4; i++)
    {
        if (torpedoes[i].dwLaunch == 0)
            continue;
        int frame = ((int)(dwNow - torpedoes[i].dwLaunch) / 800) * 7;
        if (frame > 7)
            frame = 7;
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, torpedoes[i].vPos.x, torpedoes[i].vPos.y, torpedoes[i].vPos.z);
        pGame->pDevice->SetTransform(D3DTS_WORLD, &mat);
        pGame->pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        pGame->pDevice->SetStreamSource(0, pGame->pCubeVB, sizeof(MESHVERTEX));
        pGame->pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pGame->pDevice->SetTexture(0, texFrames[frame]);
        pGame->pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }
}
