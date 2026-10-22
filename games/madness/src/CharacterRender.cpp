// Drawing a character: the HUD (for the player), then the body from its
// parts, each a sprite turned with the fall: shadow, foot, body, head and
// what's on it, jetpack, wounds, the item on the back, the held item and
// its muzzle flash, a blade stuck in it, and the hand.
#include "Game.h"

// A sprite turned by angle: scaled, rotated, then moved to (x, y, z).
static void DrawPart(LPDIRECT3DTEXTURE8 pTexture, float x, float y, float z, float w, float h, float angle,
                     LPDIRECT3DVERTEXBUFFER8 pVB)
{
    D3DXMATRIX matRot, matTrans, matScale;
    D3DXMatrixRotationZ(&matRot, angle);
    D3DXMatrixTranslation(&matTrans, x, y, z);
    D3DXMatrixScaling(&matScale, w, h, 0.0f);
    matTrans = matScale * matRot * matTrans;
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
    g_Game.pDevice->SetTexture(0, pTexture);
    g_Game.pDevice->SetStreamSource(0, pVB, sizeof(SPRITEVERTEX));
    g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
    g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
}

// The player's HUD, top right: bullet time and the health ring; then the
// held weapon's rounds (in every state).
static void DrawHud(SCharacter* pChar)
{
    int i;
    if (g_Game.state != STATE_TUTORIAL)
    {
        if (pChar->team == 0 && !g_Game.bEnabled[UNLOCK_INFBULLETTIME])
        {
            DrawPart(g_Game.texBulletCountHolder, 25.0f, 17.0f, -2.0f, 5.0f, 20.0f, 0.0f, g_Game.pQuadVB);
            i = 0;
            if (g_Game.bulletTimeLeft > 0.0f)
            {
                do
                {
                    if (i < 8)
                        DrawPart(g_Game.texBulletTime, 25.0f, (float)(25.15 - i * 2.3), -2.0f, 4.0f, 2.2f, 0.0f,
                                 g_Game.pQuadVB);
                    i++;
                } while ((float)i < g_Game.bulletTimeLeft);
            }
        }
        if (pChar->team == 0)
        {
            // four quarters of a ring, one per 2 or 3 health: the
            // texture darkens as it empties
            int level;
            if (pChar->health >= 8)
                level = 4;
            else if (pChar->health >= 5)
                level = 3;
            else if (pChar->health >= 3)
                level = 2;
            else if (pChar->health >= 1)
                level = 1;
            else
                level = 0;
            static const struct { int minLevel; float angle, x, y; } s_aQuarters[4] =
            {
                { 4, D3DX_PI / 2,      15.2f, 22.0f },
                { 3, D3DX_PI * 2,      15.2f, 26.0f },
                { 2, D3DX_PI * 5.5f,   19.0f, 26.0f },
                { 1, D3DX_PI * 5,      19.0f, 22.0f },
            };
            for (i = 0; level > 0 && i < 4; i++)
            {
                if (level < s_aQuarters[i].minLevel)
                    continue;
                LPDIRECT3DTEXTURE8 pTexture = g_Game.texHealthBar;
                if (level < 4)
                    pTexture = pChar->health < 3 ? g_Game.texHealthBar3 : g_Game.texHealthBar2;
                DrawPart(pTexture, s_aQuarters[i].x, s_aQuarters[i].y, 1.0f, 4.0f, 4.0f, s_aQuarters[i].angle,
                         g_Game.pQuadVB);
            }
        }
    }

    if (pChar->team == 0 && pChar->pHeld != NULL)
    {
        DrawPart(g_Game.texBulletCountHolder, 33.0f, 17.0f, -2.0f, 5.0f, 20.0f, 0.0f, g_Game.pQuadVB);
        if (pChar->pHeld->ammo > 0)
        {
            i = 0;
            do
            {
                if (i > 7)
                    break;
                DrawPart(g_Game.texBullet, 33.0f, (float)(25.0 - i * 2.3), -2.0f, 4.0f, 2.0f, 0.0f, g_Game.pQuadVB);
                i++;
            } while (i < pChar->pHeld->ammo);
        }
    }
}

// The head's texture: its own, or a gib (headgib.tga, headgib2.tga).
static LPDIRECT3DTEXTURE8 HeadTexture(const SCharacter* pChar)
{
    if (pChar->headGib == 0)
        return g_Game.texHeads[pChar->head];
    return g_Game.texHeads[g_Game.numHeads + pChar->headGib - 1];
}

// Big Heads doesn't enlarge the default head.
static float HeadSize(const SCharacter* pChar)
{
    if (pChar->head == 0 || !g_Game.bEnabled[UNLOCK_BIGHEADS])
        return 5.0f;
    return 7.0f;
}

// 0x0001BA50
void SCharacter::Render()
{
    if (!bActive)
        return;

    DrawHud(this);

    // just respawned: blinking for 3 seconds
    if (bRespawning)
    {
        respawnFade -= g_Game.bulletTimeDrain;
        if (GetTickCount() - respawnTime > 2999)
        {
            respawnTime = 0;
            bRespawning = false;
            respawnFade = 0.0f;
        }
        if (respawnFade <= 0.0f)
            respawnFade = 0.5f;
        if (respawnFade <= 0.25f)
            return;
    }

    if (handOffset.x == 0.0f && handOffset.y == 0.0f)
        handOffset.x = bFacingRight == true ? 0.0001f : -0.0001f;

    LPDIRECT3DVERTEXBUFFER8 pFacing = bFacingRight ? g_Game.pMirroredVB : g_Game.pQuadVB;
    LPDIRECT3DVERTEXBUFFER8 pFacingFlipped = bFacingRight ? g_Game.pQuadVB : g_Game.pMirroredVB;
    LPDIRECT3DVERTEXBUFFER8 pItemVB = bFacingRight ? g_Game.pQuadVB : g_Game.pVB460;

    if (fallAngle == 0.0f)
    {
        // the shadow, smaller the higher it is (pos.y over 1 doesn't count)
        float height = pos.y;
        if (height > 1.0f)
            height = 1.0f;
        DrawPart(g_Game.texShadow, pos.x, groundY - 1.1f, unknown054.z, height * -0.05f * 5.0f, 1.5f, 0.0f,
                 g_Game.pQuadVB);
    }

    // Everything turns about the feet as the body falls over; feetDrop
    // keeps the feet where they stand (the parts' heights are measured
    // from the body's middle).
    double s = sin(fallAngle);
    double c = cos(fallAngle);
    float feetDrop = (float)(c * -4.5 + 4.5);

    DrawPart(g_Game.texFoot, pos.x + (float)(s * feetOffset.x), pos.y + ((float)(c * -4.5) - feetDrop), feetOffset.z,
             3.5f, 2.0f, fallAngle, pFacing);

    // the body: suit 0's texture faces the other way from the others'
    LPDIRECT3DTEXTURE8 pBody;
    LPDIRECT3DVERTEXBUFFER8 pBodyVB;
    if (bodyGib == 0)
    {
        pBody = g_Game.texBodies[suit];
        pBodyVB = suit == 0 ? pFacing : pFacingFlipped;
    }
    else
    {
        pBody = g_Game.texBodies[g_Game.numBodies + bodyGib - 1];
        pBodyVB = pFacing;
    }
    DrawPart(pBody, (float)(s * -4.5) + pos.x, (float)(c * unknown054.y - feetDrop) + pos.y, unknown054.z,
             5.0f, 8.7f, fallAngle, pBodyVB);

    if (!bHeadless)
    {
        float hx = (float)(s * -11.0f);
        float hy = (float)(c * headOffset.y - feetDrop);

        // standing enemies with a bare face look at the player (up to 45
        // degrees up or down)
        float angle = fallAngle;
        if (fallAngle == 0.0f && mask == 0 && glasses == 0 && team != 0 && !bCutscene)
        {
            SCharacter* pPlayer = g_Game.pCharacters;
            D3DXVECTOR3 dir = bFacingRight == true ? pPlayer->pos - pos : pos - pPlayer->pos;
            D3DXVec3Normalize(&dir, &dir);
            double look = atan2(dir.y, dir.x);
            if (bFacingRight == true)
            {
                float a = (float)look;
                angle = D3DX_PI / 4;
                if (a < D3DX_PI / 4)
                {
                    angle = a;
                    if (a <= -D3DX_PI / 4)
                        angle = -D3DX_PI / 4;
                }
            }
            else
            {
                if (look <= -D3DX_PI / 4)
                    angle = -D3DX_PI / 4;
                else if ((float)look >= D3DX_PI / 4)
                    angle = D3DX_PI / 4;
                else
                    angle = (float)look;
            }
        }
        DrawPart(HeadTexture(this), pos.x + hx, pos.y + hy, 0.1f, HeadSize(this), HeadSize(this), angle, pFacing);

        if (mask != 0 && headGib == 0)
            DrawPart(g_Game.texNinja, (float)(s * -11.0f) + pos.x, (float)(c * headOffset.y - feetDrop) + pos.y,
                     headOffset.z, 5.0f, 5.0f, fallAngle, pFacing);
        if (hat != 0)
        {
            float hatX = (float)(s * -12.4);
            float hatY = (float)((headOffset.y + 1.4) * c - feetDrop);
            float x;
            if (fallAngle == 0.0f)
                x = bFacingRight == true ? pos.x + hatX + 1.0f : (pos.x + hatX) - 1.0f;
            else
                x = pos.x + hatX;
            DrawPart(g_Game.texHat, x, pos.y + hatY, headOffset.z, 7.0f, 3.0f, fallAngle, pFacing);
        }
        if (glasses != 0)
            DrawPart(g_Game.texGlasses[glasses], pos.x + (float)(s * -11.0f), pos.y + (float)(c * headOffset.y - feetDrop),
                     headOffset.z, 7.9f, 5.0f, fallAngle, pFacingFlipped);
    }
    else
    {
        // the head, flying off on its own
        DrawPart(HeadTexture(this), headPos.x, headPos.y, 0.1f, HeadSize(this), HeadSize(this), fallAngle, pFacing);
        if (mask != 0 && headGib == 0)
            DrawPart(g_Game.texNinja, headPos.x, headPos.y, headOffset.z, 5.0f, 5.0f, fallAngle, pFacing);
        if (hat != 0)
        {
            float x;
            if (fallAngle == 0.0f)
                x = bFacingRight == true ? headPos.x + 1.0f : headPos.x - 1.0f;
            else
                x = (float)(headPos.x - 1.4);
            DrawPart(g_Game.texHat, x, headPos.y, headOffset.z, 7.0f, 3.0f, fallAngle, pFacing);
        }
        if (glasses != 0)
            DrawPart(g_Game.texGlasses[glasses], headPos.x, headPos.y, headOffset.z, 7.9f, 5.0f, fallAngle, pFacingFlipped);
    }

    if (pJetpack != NULL && pJetpack->bJetpack == TRUE)
    {
        if (bJetpackOn)
        {
            // the exhaust, flickering (and the flight kept in bounds)
            float ex = (float)(s * -4.5);
            float ey = (float)(c * 0.8f - feetDrop);
            float angle = (float)(rand() % 5) + fallAngle;
            if (pos.x > 60.0f)
                pos.x = 60.0f;
            if (pos.x < -60.0f)
                pos.x = -60.0f;
            if (pos.y > 40.0f)
                pos.y = 40.0f;
            float x, y;
            if (fallAngle == 0.0f)
            {
                y = (float)((ey + pos.y) - 0.9);
                x = bFacingRight == true ? (float)((pos.x + ex) - 4.9) : (float)(pos.x + ex + 4.9);
            }
            else
            {
                y = pos.y + ey;
                x = pos.x + ex;
            }
            DrawPart(g_Game.pWeaponTypes[WEAPON_JETPACK].pFlashTexture, x, y, headOffset.z, 2.0f, 2.0f, angle,
                     g_Game.pQuadVB);
        }
        float jx = (float)(s * -4.5);
        float jy = (float)((c + c) - feetDrop);
        float x;
        if (fallAngle == 0.0f)
            x = bFacingRight == true ? (pos.x + jx) - 3.0f : pos.x + jx + 3.0f;
        else
            x = pos.x + jx;
        DrawPart(g_Game.pWeaponTypes[WEAPON_JETPACK].pTexture, x, pos.y + jy, headOffset.z, 4.0f, 4.0f, fallAngle,
                 pFacingFlipped);
    }

    for (int w = 0; w < MAX_WOUNDS; w++)
    {
        if (!wounds[w].bUsed)
            continue;
        if (wounds[w].y > 7.0f)
            wounds[w].y = 7.0f;
        DrawPart(g_Game.texBloodSpot, (float)((-6.0 - wounds[w].y) * sin(fallAngle)) + pos.x,
                 (float)(cos(fallAngle) * wounds[w].y - feetDrop) + pos.y, unknown054.z, 2.0f, 2.0f, fallAngle, pFacing);
    }

    if (pBack != NULL)
    {
        // across the back
        float bx = (float)(c * -3.0f);
        float by = (float)(s * -3.0f);
        float x = bFacingRight ? unknown054.x + bx + pos.x : (pos.x - bx) + unknown054.x;
        SWeaponType& t = g_Game.pWeaponTypes[pBack->type];
        DrawPart(t.pTexture, x, by + pos.y + unknown054.y, handOffset.z, t.width, t.height,
                 fallAngle - STANDING_ANGLE, pItemVB);
    }

    if (bHolding)
    {
        float aim = (float)atan2(handOffset.y, handOffset.x);
        if (pHeld != NULL)
        {
            SWeaponType& t = g_Game.pWeaponTypes[pHeld->type];
            float gx = (float)(cos(aim) * t.holdOffsetX + handOffset.x);
            float gy = (float)(sin(aim) * t.holdOffsetY + handOffset.y);
            DrawPart(t.pTexture, gx + unknown054.x + pos.x, gy + pos.y + unknown054.y, handOffset.z, t.width, t.height,
                     aim, pItemVB);
        }
        if (bMuzzleFlash && pHeld != NULL)
        {
            // at the muzzle (a weapon's length along the aim), turned at random
            SWeaponType& t = g_Game.pWeaponTypes[pHeld->type];
            float fx = (float)(cos(aim) * t.width + handOffset.x);
            float fy = (float)(sin(aim) * t.width + handOffset.y);
            DrawPart(t.pFlashTexture, fx + unknown054.x + pos.x, fy + pos.y + unknown054.y, handOffset.z, 4.0f, 4.0f,
                     (float)muzzleFlashAngle, pItemVB);
        }
    }

    if (bImpaled && pStuck != NULL)
    {
        // the blade in the body, twice its size
        SWeaponType& t = g_Game.pWeaponTypes[pStuck->type];
        DrawPart(t.pStuckTexture, (float)((-4.5 - pStuck->pos.y) * sin(fallAngle) + pos.x),
                 (float)((cos(fallAngle) * pStuck->pos.y - feetDrop) + pos.y), pStuck->pos.z,
                 t.width * 2.0f, t.height * 2.0f, pStuck->angle + fallAngle,
                 pStuck->bFacingRight ? g_Game.pQuadVB : g_Game.pVB460);
    }

    // the hand
    DrawPart(g_Game.texHand, (float)(s * -6.0f) + pos.x + handOffset.x, (float)(c * 0.0f - feetDrop) + pos.y + handOffset.y,
             -1.0f, 2.0f, 2.0f, (float)(atan2(handOffset.y, handOffset.x) + fallAngle), g_Game.pQuadVB);
}
