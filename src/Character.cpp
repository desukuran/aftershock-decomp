// Characters: spawning, the player's controls, the enemies' AI, guns and
// melee, and the physics they share.
#include "Game.h"

// 0x00014920
SCharacter::SCharacter()
{
    pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    feetOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    unknown054 = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    headOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    handOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    lastHandOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    lastPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    health = 0;
    team = 0;
    bFacingRight = false;
    fallAngle = 0.0f;
    bActive = false;
    unknown09C = 2.0f;
    unknown0A0 = 2.0f;
    burstCount = 0;
    bRespawning = false;
    respawnFade = 0.0f;
    mask = 0;
    hat = 0;
    glasses = 0;
    bHolding = false;
    pHeld = NULL;
    pBack = NULL;
    pJetpack = NULL;
    pStuck = NULL;
    suit = 0;
    bodyGib = 0;
    head = 0;
    headGib = 0;
    bJetpackOn = false;
    bTriggerHeld = false;
    bDummy = false;
    bHeadless = false;
    bCutscene = false;
    muzzleFlashAngle = 0;
    swingDelay = 0.0f;
    bJumpHeld = false;
}

// Takes the first free item and puts it in the character's hand (or on
// its back, for the jetpack).
static SItem* GiveItem(float x, float y, float z)
{
    for (int i = 0; i < g_Game.maxItems; i++)
    {
        SItem* pItem = &g_Game.pItems[i];
        if (!pItem->bActive)
        {
            pItem->vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pItem->pos = D3DXVECTOR3(x, y, z);
            pItem->lastPos = pItem->pos;
            pItem->bActive = true;
            pItem->bHeld = true;
            pItem->bStuck = false;
            return pItem;
        }
    }
    return NULL;
}

// 0x0001FCE0
// team 0 is the player (or a dummy); the others get a random look (unless
// it's a cutscene's character) and a weapon: with bJetpack a shotgun and a
// jetpack, whatever weapon says. A dummy stands still facing the player.
void SCharacter::Spawn(float x, float y, float z, int team, int weapon, bool bJetpack, bool bDummy, bool bCutscene)
{
    bRespawning = false;
    respawnFade = 0.0f;
    respawnTime = 0;
    swingDelay = 0.0f;
    this->bCutscene = bCutscene;
    this->bDummy = bDummy;
    headVel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    headPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    groundY = bCutscene ? -16.0f : -24.5f;
    pJetpack = NULL;
    pBack = NULL;
    pHeld = NULL;
    bJetpackOn = false;
    bActive = true;
    bTriggerHeld = false;
    bHeadless = false;
    lastHitTime = GetTickCount();
    burstCount = 0;
    bHolding = false;
    reaction = (float)((rand() % 10) / 10 + 1);    // (integer division: always 1)
    burstPause = 4.0f;
    downTime = 0.0f;
    for (int w = 0; w < MAX_WOUNDS; w++)
        wounds[w].bUsed = false;
    health = 3;
    bImpaled = false;
    this->team = team;
    headGib = 0;

    if (team == 0)
    {
        g_Game.bulletTimeLeft = 8.0f;
        health = 10;
    }
    else
    {
        if (!bCutscene)
        {
            // Extra Heads: heads 1..29; Nerd Heads: 30..34
            head = 0;
            if (g_Game.bEnabled[UNLOCK_EXTRAHEADS])
            {
                if (g_Game.bEnabled[UNLOCK_NERDHEADS])
                    head = rand() % g_Game.numHeads;
                else
                    head = rand() % (g_Game.numHeads - 5);
            }
            else if (g_Game.bEnabled[UNLOCK_NERDHEADS])
            {
                head = rand() % (g_Game.numHeads - 30);
                if (head != 0)
                    head += 30;
            }
            int s = rand() % g_Game.numBodies;
            swingTimer = 0.2f;
            suit = s;
            if (head == 0)
            {
                hat = rand() % 3;
                glasses = rand() % 4;
                mask = rand() % 2;
            }
            else
            {
                hat = 0;
                glasses = 0;
                mask = 0;
            }
        }

        headGib = 0;
        SItem* pItem = GiveItem(x, y, z);
        if (pItem != NULL)
        {
            bHolding = true;
            pHeld = pItem;
            pItem->type = bJetpack ? WEAPON_SHOTGUN : weapon;
            pHeld->Reset();
        }
        if (pItem == NULL || bJetpack)
        {
            if (bJetpack)
            {
                pItem = GiveItem(x, y, z);
                if (pItem != NULL)
                {
                    pJetpack = pItem;
                    pItem->type = WEAPON_JETPACK;
                    pJetpack->Reset();
                    pJetpack->bJetpack = TRUE;
                    bJetpackOn = true;
                }
            }
        }
    }

    vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    pos = D3DXVECTOR3(x, y, z);
    headOffset = D3DXVECTOR3(0.0f, 6.0f, 0.0f);
    unknown054 = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    feetOffset = D3DXVECTOR3(0.0f, -4.5f, 0.0f);
    bodyGib = 0;
    headGib = 0;
    handOffset.x = 0.0f;
    bFacingRight = false;
    bMuzzleFlash = false;
    fallAngle = 0.0f;
    handOffset.y = 0.0f;
    handOffset.z = 0.0f;
}

// A gunshot wound at height y on the body (clamped between the hips and
// the neck); with all five used, the first is taken again.
static void AddWound(SCharacter* pChar, float y)
{
    int w;
    for (w = 0; w < MAX_WOUNDS; w++)
    {
        if (!pChar->wounds[w].bUsed)
            break;
    }
    if (w == MAX_WOUNDS)
        w = 0;
    pChar->wounds[w].bUsed = true;
    pChar->wounds[w].x = 0.0f;
    pChar->wounds[w].y = y - pChar->pos.y;
    if (pChar->wounds[w].y > 6.0f)
        pChar->wounds[w].y = 6.0f;
    if (pChar->wounds[w].y < -4.5)
        pChar->wounds[w].y = -4.5f;
}

// The side the gib flies off to: 1 or 2 (bodygib/headgib, or ..2.tga).
static int GibSide(const SCharacter* pTarget, const SCharacter* pShooter, bool bHead)
{
    bool bFromRight = pTarget->pos.x <= pShooter->pos.x;
    if (bHead)
        return (bFromRight ? pTarget->bFacingRight != false : pTarget->bFacingRight == false) + 1;
    return (bFromRight ? pTarget->bFacingRight == false : pTarget->bFacingRight != false) + 1;
}

// Blood at height y on the body.
static void Bleed(int target, const SCharacter* pShooter, float y, int count)
{
    for (int n = 0; n < count; n++)
    {
        SCharacter& t = g_Game.pCharacters[target];
        float dy = y - t.pos.y;
        float top = pShooter->headOffset.y + pShooter->pos.y;
        if (t.pos.y + dy > top)
            dy = t.pos.y - top;
        int vy = rand();
        int vx = rand();
        g_Game.particles.Add(t.pos.x, t.pos.y + dy, t.pos.z, (float)(vx % 10), (float)(vy % 10), 0.0f, PARTICLE_BLOOD);
    }
}

// 0x00020280
// The player shoots. Out of bullet time it's instant: the ray from the
// hand hits the nearest enemy's body (a circle of 4) or head (2), both
// turning as it falls; in bullet time it fires a real bullet. A shotgun
// closer than 20 blows the head or body off.
void SCharacter::Fire()
{
    bool bNoAim = false;
    if (handOffset.x == 0.0f && handOffset.y == 0.0f)
    {
        handOffset.x = bFacingRight == true ? 1.0f : -1.0f;
        bNoAim = true;
    }
    if (pHeld == NULL)
        goto done;

    {
        double a = atan2(handOffset.y, handOffset.x);
        bool bHit = false;
        int target = 0;
        float best = 0.0f;
        bool bHead = false, bBody = false;
        D3DXVECTOR2 shot;
        float shotT, shotOx, shotOy;
        double c = cos(a);

        if (g_Game.bBulletTime)
        {
            float mx = (float)(c * 8.0f + handOffset.x);
            float my = (float)(sin(a) * 8.0f + handOffset.y);
            int r = rand();
            float bx = pos.x + unknown054.x + mx;
            float by = my + unknown054.y + pos.y;
            bMuzzleFlash = true;
            muzzleFlashAngle = (int)((float)(r % 314) * 0.01f);
            muzzleFlashTime = 0.1f;
            g_Game.bullets.Add(bx, by, handOffset.x, handOffset.y, TRUE, bFacingRight);
            pHeld->Use(D3DXVECTOR3(pos.x + unknown054.x + mx, unknown054.y + my + pos.y, pos.z),
                       D3DXVECTOR3(0.0f - handOffset.x, handOffset.y, 0.0f), 0.0f);
            return;     // (without undoing bNoAim's aim)
        }

        float hx = handOffset.x;
        double s = sin(a);
        float hy = handOffset.y;
        if (pHeld->ammo == 0)
            goto done;

        int r = rand();
        bMuzzleFlash = true;
        muzzleFlashAngle = (int)((float)(r % 314) * 0.01f);
        muzzleFlashTime = 0.1f;
        pHeld->Use(D3DXVECTOR3(pos.x + unknown054.x + (float)(c * 3.0f + hx),
                               unknown054.y + (float)(s * 3.0f + hy) + pos.y, pos.z),
                   D3DXVECTOR3(0.0f - handOffset.x, handOffset.y, 0.0f), 0.0f);

        for (int i = 1; i < g_Game.maxCharacters; i++)
        {
            SCharacter& t = g_Game.pCharacters[i];
            if (t.health <= 0 || !t.bActive || !(t.pos.x >= -43.0f && t.pos.x <= 43.0f))
                continue;

            float dx = handOffset.x;
            double ts = sin(t.fallAngle);
            float ox = pos.x + unknown054.x + handOffset.x;
            float oy = unknown054.y + handOffset.y + pos.y;
            float dy = handOffset.y;
            float cx = (float)(ts * -6.0f + t.pos.x);
            double tc = cos(t.fallAngle);
            float cy = (float)((0.0f * tc + t.pos.y) - (tc * -6.0f + 6.0f));
            float tt[2];
            if (((dx < 0.0f && ox > cx) || (dx > 0.0f && cx > ox)) &&
                g_Game.RayCircle(ox, oy, dx, dy, cx, cy, 4.0f, tt) != -1.0f)
            {
                D3DXVECTOR3 d = pos - t.pos;
                float dist = D3DXVec3Length(&d);
                bHit = true;
                if (target == 0 || dist < best)
                {
                    target = i;
                    shot = D3DXVECTOR2(dx, dy);
                    shotT = tt[1];
                    bHead = false;
                    bBody = true;
                    shotOx = ox;
                    shotOy = oy;
                    best = dist;
                }
                if (pHeld->type != WEAPON_SHOTGUN)
                    continue;
            }
            else if (bHit && pHeld->type != WEAPON_SHOTGUN)
                continue;

            // the head (placed with the shooter's own head offset)
            double hs = sin(t.fallAngle);
            double hc = cos(t.fallAngle);
            if (g_Game.RayCircle(ox, oy, handOffset.x, handOffset.y, (float)(hs * -11.0f + t.pos.x),
                                 (float)((hc * headOffset.y + t.pos.y) - (hc * -6.0f + 6.0f)), 2.0f, tt) != -1.0f)
            {
                D3DXVECTOR3 d = pos - t.pos;
                float dist = D3DXVec3Length(&d);
                bHit = true;
                if (target == 0 || dist < best)
                {
                    target = i;
                    shot = D3DXVECTOR2(handOffset.x, handOffset.y);
                    bHead = true;
                    shotT = tt[1];
                    shotOx = ox;
                    shotOy = oy;
                    best = dist;
                }
            }
        }

        if (!bHit)
            goto done;

        SCharacter* pTarget = &g_Game.pCharacters[target];
        if (bHead)
        {
            D3DXVec2Normalize(&shot, &shot);
            pTarget->vel = D3DXVECTOR3(shot.x, shot.y, 0.0f);
            D3DXVECTOR3 d(shotOx - pTarget->pos.x, shotOy - pTarget->pos.y, 0.0f - pTarget->pos.z);
            if (pHeld->type != WEAPON_SHOTGUN || fabsf(D3DXVec3Length(&d)) >= 20.0f)
                AddWound(pTarget, shot.y * shotT + shotOy);
            else if (pHeld != NULL)
            {
                pTarget->headGib = GibSide(pTarget, this, true);
                pTarget->health -= 5;
            }
            pTarget->health -= 3;
            Bleed(target, this, shot.y * shotT + shotOy, 10);
        }
        if (bBody && (!bHead || pHeld->type == WEAPON_SHOTGUN))
        {
            if (shot.x > 0.0f || shot.x < 0.0f)
                rand();     // (unused)
            D3DXVECTOR3 d(shotOx - pTarget->pos.x, shotOy - pTarget->pos.y, 0.0f - pTarget->pos.z);
            if (pHeld->type != WEAPON_SHOTGUN || fabsf(D3DXVec3Length(&d)) >= 20.0f)
                AddWound(pTarget, shot.y * shotT + shotOy);
            else if (pHeld != NULL)
            {
                pTarget->bodyGib = GibSide(pTarget, this, false);
                pTarget->health -= 5;
            }
            pTarget->health -= 1;
            if (pHeld->type == WEAPON_SHOTGUN)
                pTarget->health -= 2;
            Bleed(target, this, shot.y * shotT + shotOy, 5);
            D3DXVec2Normalize(&shot, &shot);
            pTarget->vel = D3DXVECTOR3(shot.x, shot.y, 0.0f);
        }
    }

done:
    if (bNoAim)
        handOffset.x = 0.0f;
}

// Walking (or flying) one way, if standing on the ground or flying.
static void Walk(SCharacter* pChar, bool bRight, float speed)
{
    pChar->bFacingRight = bRight;
    if (pChar->feetOffset.y + pChar->pos.y > pChar->groundY && !pChar->bJetpackOn)
        return;
    if (bRight)
        pChar->vel.x = g_Game.frameTime * (pChar->bJetpackOn ? 10.0f : speed) + pChar->vel.x;
    else if (pChar->bJetpackOn)
        pChar->vel.x = pChar->vel.x - g_Game.frameTime * 10.0f;
    else
        pChar->vel.x = pChar->vel.x - g_Game.frameTime * speed;
}

// Flying in place: any horizontal speed becomes 1 (vel.x / vel.x, so it
// always drifts right), then the check below 1 never stops it.
static void Hover(SCharacter* pChar)
{
    if (pChar->vel.x != 0.0f)
        pChar->vel.x = pChar->vel.x / pChar->vel.x;
    if (pChar->vel.x < 1.0f && pChar->vel.x > -1.0f)
        pChar->vel.x = 0.0f;
}

// 0x00021190
// An enemy's turn. Unarmed it walks up to the player; with a melee weapon
// or an empty gun it closes in, swinging, and throws what it holds; with
// a gun it keeps within 40 and shoots in bursts of five. Its shots are
// rays at the player, like the player's own (bullets in bullet time). A
// flying head bounces off the others, hurting them. In the ending the
// characters are scripted instead.
void SCharacter::Think()
{
    SCharacter* chars = g_Game.pCharacters;

    if (bCutscene)
    {
        if (g_Game.endingStep == 0)
            vel.x = g_Game.frameTime * 1200.0f + vel.x;
        if (g_Game.endingStep == 1 && g_Game.endingTalk > 8)
        {
            vel.x = g_Game.frameTime * 1200.0f + vel.x;
            if (chars[0].pos.x > 14.0f && !chars[1].bHeadless)
            {
                // Lantus cuts Bubba's head off
                chars[0].team = 0;
                chars[1].bHeadless = true;
                g_Sound.Play(SOUND_SCREAM);
                chars[1].health = 0;
                chars[1].headPos = chars[1].pos + chars[1].headOffset;
                chars[1].headVel = D3DXVECTOR3(-10.0f, 30.0f, 0.0f);
                if (chars[1].pHeld != NULL)
                {
                    chars[1].pHeld->bounces = 1;
                    chars[1].pHeld->bSpawned = true;
                    chars[1].pHeld->bHeld = false;
                    chars[1].pHeld = NULL;
                }
            }
            if (pos.x > 20.0f)
                pos.x = 20.0f;
            if (pHeld != NULL && pHeld->type == WEAPON_SWORD && !chars[1].bHeadless)
            {
                D3DXVECTOR3 dir = chars[1].pos - pos;
                D3DXVec3Normalize(&dir, &dir);
                handOffset.x = dir.x * 5.0f;
                float hy = dir.y * 4.0f;
                handOffset.y = hy;
                swingTimer -= g_Game.frameTime;
                if (swingTimer < 0.1f)
                    hy += 1.0f;
                else
                    hy -= 1.0f;
                handOffset.y = hy;
                if (swingTimer <= 0.0f)
                    swingTimer = 0.2f;
                D3DXVECTOR3 d = pos - chars[1].pos;
                if (D3DXVec3Length(&d) <= 10.0)
                {
                    d = handOffset - lastHandOffset;
                    if (D3DXVec3Length(&d) >= 0.1)
                    {
                        d = handOffset - lastHandOffset;
                        D3DXVec3Normalize(&handOffset, &d);
                        handOffset.x = (float)(handOffset.x * 0.5 + lastHandOffset.x);
                        handOffset.y = (float)(handOffset.y * 0.5 + lastHandOffset.y);
                    }
                }
            }
        }
        if (!bHeadless)
            return;
        if (headPos.x < -34.0f)
        {
            headVel.x *= -0.5f;
            headPos.x = -33.9f;
        }
        if (headPos.x <= 34.0f)
            return;
        headVel.x *= -0.5f;
        headPos.x = 33.9f;
        return;
    }

    if (bHeadless && D3DXVec3Length(&headVel) > 5.0f)
    {
        for (int i = 1; i < g_Game.maxCharacters; i++)
        {
            if (!g_Game.pCharacters[i].bActive)
                continue;
            if (headPos.x < -50.0f || headPos.x > 50.0f)
                break;
            if (g_Game.pCharacters[i].health <= 0)
                continue;
            if (!CornerInBox(headPos.x, headPos.y, headPos.z, 2.0f, 2.0f, g_Game.pCharacters[i].pos, 5.0f, 12.0f))
                continue;
            if (GetTickCount() - g_Game.pCharacters[i].lastHitTime >= 400)
            {
                g_Game.pCharacters[i].lastHitTime = GetTickCount();
                headVel.x = 0.0f - headVel.x;
                g_Game.pCharacters[i].health--;
                g_Sound.Play(g_Game.bBulletTime ? SOUND_HIT : (rand() % 2 == 0 ? SOUND_PUNCH : SOUND_PUNCH3));
            }
        }
    }

    if (health < 1)
        return;
    lastPos = pos;
    lastHandOffset = handOffset;
    if (bDummy)
    {
        if (pos.x < -34.0f)
            pos.x = -34.0f;
        if (pos.x > 34.0f)
            pos.x = 34.0f;
        float px = g_Game.pCharacters[0].pos.x;
        if (px >= pos.x)
        {
            if (g_Game.pCharacters[0].pos.x > pos.x)
                bFacingRight = true;
        }
        else
            bFacingRight = false;
        return;
    }
    if (fallAngle != 0.0f)
        return;

    if (bJetpackOn)
        vel.y = g_Game.frameTime * (pos.y < 10.0f ? 90.0f : 70.0f) + vel.y;

    // aim at the player, swinging up and down
    SCharacter* pPlayer = g_Game.pCharacters;
    D3DXVECTOR3 dir = pPlayer->pos - pos;
    D3DXVec3Normalize(&dir, &dir);
    handOffset.x = dir.x * 5.0f;
    float hy = dir.y * 4.0f;
    handOffset.y = hy;
    swingTimer -= g_Game.frameTime;
    if (swingTimer < 0.1f)
        hy += 1.0f;
    else
        hy -= 1.0f;
    handOffset.y = hy;
    if (swingTimer <= 0.0f)
        swingTimer = 0.2f;

    swingDelay -= g_Game.frameTime;
    if (pHeld != NULL && pHeld->ammo == 0)
    {
        // swinging the weapon (fast when close)
        D3DXVECTOR3 d = pos - pPlayer->pos;
        float dist = D3DXVec3Length(&d);
        D3DXVECTOR3 swing = handOffset - lastHandOffset;
        if (dist > 10.0)
        {
            if (D3DXVec3Length(&swing) >= 0.1)
            {
                swingDelay = 0.5f;
                swing = handOffset - lastHandOffset;
                D3DXVec3Normalize(&handOffset, &swing);
                SItem* pItem = pHeld;
                handOffset.x = (float)(g_Game.frameTime * handOffset.x * 5.03 + lastHandOffset.x);
                handOffset.y = (float)(handOffset.y * g_Game.frameTime * 5.03 + lastHandOffset.y);
                if (pItem != NULL && pItem->type == WEAPON_CHAINSAW && pItem->fireTimer <= 0.0f)
                {
                    g_Game.particles.Add(pItem->pos.x, pItem->pos.y, pItem->pos.z, -3.55f, 0.0f, 0.0f, PARTICLE_SMOKE);
                    pHeld->fireTimer = pHeld->fireDelay + reaction;
                }
            }
        }
        else
        {
            if (D3DXVec3Length(&swing) >= 0.1)
            {
                swing = handOffset - lastHandOffset;
                D3DXVec3Normalize(&handOffset, &swing);
                handOffset.x = (float)(g_Game.frameTime * handOffset.x * 25.5 + lastHandOffset.x);
                handOffset.y = (float)(handOffset.y * g_Game.frameTime * 25.5 + lastHandOffset.y);
            }
            SItem* pItem = pHeld;
            if (pItem != NULL && pItem->type == WEAPON_CHAINSAW && pItem->fireTimer <= 0.0f)
            {
                g_Game.particles.Add(pItem->pos.x, pItem->pos.y, pItem->pos.z, -3.55f, 0.0f, 0.0f, PARTICLE_SMOKE);
                pHeld->fireTimer = pHeld->fireDelay + reaction;
            }
        }
    }

    SItem* pItem = pHeld;
    if (pItem == NULL)
    {
        // unarmed: walk up to the player
        if (pos.x < pPlayer->pos.x - 41.0f)
            Walk(this, true, 1200.0f);
        else if (pos.x > pPlayer->pos.x + 41.0f)
            Walk(this, false, 1200.0f);
        else if (bJetpackOn)
            Hover(this);
        return;
    }

    if (pItem->ammo == 0 || pItem->type == WEAPON_CHAINSAW)
    {
        // a melee weapon (or an empty gun): close in
        if (pos.x < -40.0f || pos.x < pPlayer->pos.x - 10.0f)
        {
            bFacingRight = true;
            if (feetOffset.y + pos.y <= groundY || bJetpackOn)
            {
                if (!bJetpackOn)
                    vel.x = g_Game.frameTime * 1200.0f + vel.x;
                else
                    vel.x = g_Game.frameTime * 10.0f + vel.x;
                pItem->fireTimer = pItem->fireDelay + reaction;
            }
        }
        else if (pos.x <= 40.0f && pos.x <= pPlayer->pos.x + 10.0f)
        {
            if (bJetpackOn)
                Hover(this);
        }
        else
        {
            bFacingRight = false;
            if (feetOffset.y + pos.y <= groundY || bJetpackOn)
            {
                if (bJetpackOn)
                    vel.x = vel.x - g_Game.frameTime * 10.0f;
                else
                    vel.x = vel.x - g_Game.frameTime * 1200.0f;
                pItem->fireTimer = pItem->fireDelay + reaction;
            }
        }

        // throw it at the player
        pItem = pHeld;
        if (pItem->type != WEAPON_CHAINSAW && !pItem->bSticks && pItem->fireTimer + reaction < 0.0f)
        {
            D3DXVECTOR3 aim = pPlayer->pos - pos;
            D3DXVec3Normalize(&aim, &aim);
            pHeld->vel = aim * 200.0f;
            float speed = D3DXVec3Length(&pHeld->vel);
            if (speed >= 300.0f)
            {
                D3DXVec3Normalize(&pHeld->vel, &pHeld->vel);
                pHeld->vel *= 300.0f;
            }
            pThrown = pHeld;
            bHolding = false;
            pHeld->angle = (float)atan2(handOffset.y, handOffset.x);
            pHeld->bFacingRight = bFacingRight;
            pHeld->bHeld = false;
            pHeld->lifeTime = 8.0f;
            pHeld = NULL;
        }
        return;
    }

    // a gun: keep within 40 of the player
    dir = pPlayer->pos - pos;
    D3DXVec3Normalize(&dir, &dir);
    handOffset.x = dir.x * 5.0f;
    handOffset.y = dir.y * 4.0f;
    if (pos.x < -40.0f || pos.x < pPlayer->pos.x - 40.0f)
    {
        bFacingRight = true;
        if (feetOffset.y + pos.y <= groundY || bJetpackOn)
        {
            vel.x = g_Game.frameTime * (bJetpackOn ? 10.0f : 1200.0f) + vel.x;
            pHeld->fireTimer = pHeld->fireDelay + reaction;
        }
    }
    else if (pos.x > 40.0f || pos.x > pPlayer->pos.x + 40.0f)
    {
        bFacingRight = false;
        if (feetOffset.y + pos.y <= groundY || bJetpackOn)
        {
            if (!bJetpackOn)
                vel.x = vel.x - g_Game.frameTime * 1200.0f;
            else
                vel.x = vel.x - g_Game.frameTime * 10.0f;
            pHeld->fireTimer = pHeld->fireDelay + reaction;
        }
    }
    else if (bJetpackOn)
        Hover(this);

    if (pos.x >= 40.0f || pos.x <= -40.0f)
        return;
    if (pos.x < pPlayer->pos.x - 40.0f || pos.x > pPlayer->pos.x + 40.0f)
        return;
    if (pPlayer->health < 1)
        return;
    if (burstPause > 0.0f && pHeld->bAutomatic)
        return;
    if (pHeld->fireTimer > 0.0f)
        return;
    if (++burstCount > 4)
    {
        burstPause = 5.0f;
        burstCount = 0;
    }

    if (g_Game.bBulletTime)
    {
        double a = atan2(handOffset.y, handOffset.x);
        float mx = (float)(cos(a) * 8.0f + handOffset.x);
        float my = (float)(sin(a) * 8.0f + handOffset.y);
        int r = rand();
        bMuzzleFlash = true;
        muzzleFlashAngle = (int)((float)(r % 314) * 0.01f);
        muzzleFlashTime = 0.1f;
        g_Game.bullets.Add(mx + pos.x + unknown054.x, my + pos.y + unknown054.y, handOffset.x, handOffset.y,
                           TRUE, bFacingRight);
        pHeld->Use(D3DXVECTOR3(mx + pos.x + unknown054.x, pos.y + my + unknown054.y, pos.z),
                   D3DXVECTOR3(0.0f - handOffset.x, handOffset.y, 0.0f), reaction);
        return;
    }
    if (pHeld->type == WEAPON_CHAINSAW)
        return;

    int r = rand();
    double a = atan2(handOffset.y, handOffset.x);
    bMuzzleFlash = true;
    muzzleFlashAngle = (int)((float)(r % 314) * 0.01f);
    muzzleFlashTime = 0.1f;
    pHeld->Use(D3DXVECTOR3((float)(cos(a) * 3.0f + handOffset.x + pos.x + unknown054.x),
                           (float)(sin(a) * 3.0f + handOffset.y + pos.y + unknown054.y), pos.z),
               D3DXVECTOR3(0.0f - handOffset.x, handOffset.y, 0.0f), 0.0f);

    // the shot: a ray at the player's body, then head
    SCharacter* pTarget = g_Game.pCharacters;
    float ox = pos.x + handOffset.x + unknown054.x;
    float oy = handOffset.y + pos.y + unknown054.y;
    float dx = handOffset.x;
    float dy = handOffset.y;
    double ts = sin(pTarget->fallAngle);
    float tt[2];
    tt[0] = (float)(ts * -6.0f + pTarget->pos.x);
    float tc = (float)cos(pTarget->fallAngle);
    float k = tc * -6.0f + 6.0f;
    tt[1] = (tc * 0.0f + pTarget->pos.y) - k;
    if (!((dx < 0.0f && ox > tt[0]) || (dx > 0.0f && tt[0] > ox)))
        return;

    if (g_Game.RayCircle(ox, oy, dx, dy, tt[0], tt[1], 4.0f, tt) == -1.0f)
    {
        if (g_Game.RayCircle(ox, oy, handOffset.x, handOffset.y, (float)ts * -11.0f + pTarget->pos.x,
                             (headOffset.y * tc + pTarget->pos.y) - k, 2.0f, tt) == -1.0f)
            return;
        D3DXVECTOR3 d(ox - pTarget->pos.x, oy - pTarget->pos.y, 0.0f - pTarget->pos.z);
        if (fabsf(D3DXVec3Length(&d)) >= 20.0f)
        {
            AddWound(pTarget, tt[1] * dy + oy);
            pTarget = g_Game.pCharacters;
        }
        else if (pHeld->type == WEAPON_SHOTGUN)
        {
            pTarget->headGib = GibSide(pTarget, this, true);
            g_Game.pCharacters->health -= 5;
            pTarget = g_Game.pCharacters;
        }
        pTarget->health -= 1;
        return;
    }

    float bloodVx;
    if (dx > 0.0f)
        bloodVx = (float)(rand() % 5) * 0.1f;
    else if (dx < 0.0f)
        bloodVx = (float)-(rand() % 5) * 0.1f;
    else
        bloodVx = 0.0f;
    D3DXVECTOR3 d(ox - pTarget->pos.x, oy - pTarget->pos.y, 0.0f - pTarget->pos.z);
    if (pHeld->type == WEAPON_SHOTGUN)
    {
        if (fabsf(D3DXVec3Length(&d)) < 20.0f)
        {
            pTarget->bodyGib = GibSide(pTarget, this, false);
            g_Game.pCharacters->health -= 5;
            pTarget = g_Game.pCharacters;
        }
    }
    else
    {
        AddWound(pTarget, tt[1] * dy + oy);
        pTarget = g_Game.pCharacters;
    }
    pTarget->health -= 1;
    g_Game.particles.Add(g_Game.pCharacters->pos.x, g_Game.pCharacters->pos.y, g_Game.pCharacters->pos.z,
                         bloodVx, dy, 0.0f, PARTICLE_BLOOD);
}

// Drops an item where it is, after its owner died.
static void Drop(SItem* pItem)
{
    pItem->bHeld = false;
    pItem->lifeTime = 8.0f;
    pItem->vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
}

// 0x00022F20
// A frame of a character: the flying head, the held items, the player's
// controls (or the enemy's Think), then movement and death.
void SCharacter::Update()
{
    if (!bActive)
        return;
    if (team == 0 && bRespawning)
        health = 10;
    if (g_Game.state == STATE_ENDING)
    {
        if (pos.x < -34.0f)
            pos.x = -34.0f;
        if (pos.x > 34.0f)
            pos.x = 34.0f;
    }

    float dt = g_Game.frameTime;
    if (bHeadless && g_Game.highestHeadHit < headPos.y)
        g_Game.highestHeadHit = headPos.y;
    if (!g_Game.bEnabled[UNLOCK_INFBULLETTIME] && team == 0)
    {
        if (!g_Game.bBulletTime)
        {
            if (g_Game.bulletTimeLeft < 8.0f)
                g_Game.bulletTimeLeft = g_Game.frameTime + g_Game.bulletTimeLeft;
        }
        else
        {
            g_Game.bulletTimeLeft -= g_Game.bulletTimeDrain;
            if (g_Game.bulletTimeLeft <= 0.0f)
                g_Game.bBulletTime = false;
        }
    }

    if (bHeadless)
    {
        // the head, flying and bouncing, dripping blood
        headPos.z = headVel.z * g_Game.frameTime + headPos.z;
        headPos.y = headVel.y * g_Game.frameTime + headPos.y;
        headPos.x = headPos.x + headVel.x * dt;
        float vy = headVel.y - g_Game.frameTime * 80.0f;
        float floor = groundY - 1.5f;
        headVel.y = vy;
        if (headPos.y < floor)
        {
            headVel.y = vy * -0.5f;
            headPos.y = groundY - 1.4f;
            if (headVel.y < 4.0f)
            {
                headVel.y = 0.0f;
                headVel.x = 0.0f;
                headVel.y = 0.0f;
                headPos.y = floor;
                headVel.z = 0.0f;
            }
        }
        if (headBloodTimer <= 0.0f)
        {
            g_Game.particles.Add(headPos.x, headPos.y, headPos.z, 0.0f - headVel.x, 0.0f - headVel.y, 0.0f - headVel.z,
                                 PARTICLE_BLOOD);
            headBloodTimer = 0.2f;
        }
        headBloodTimer -= g_Game.frameTime;
    }

    if (pHeld == NULL)
        bHolding = false;
    else
        pHeld->unknown5E = false;

    if (!bHolding)
    {
        if (team == 0)
            g_Game.input.SetRumble(0, 0.0f, 0.0f);
    }
    else if (team == 0)
    {
        if (pHeld != NULL)
        {
            pHeld->Update(1, 0.0f);
            if (pHeld->fireTimer <= 0.0f)
                g_Game.input.SetRumble(0, 0.0f, 0.0f);
        }
    }
    else if (g_Game.state != STATE_ENDING && pHeld != NULL)
    {
        if (pHeld->ammo == 0)
            pHeld->Update(2, swingDelay);
        else
            pHeld->Update(2, 0.0f);
    }

    if (team != 0)
        burstPause -= g_Game.frameTime;
    if (muzzleFlashTime > 0.0f)
        muzzleFlashTime -= g_Game.frameTime;
    else
        bMuzzleFlash = false;

    if (!bHolding)
    {
        // punching: a fast hand hits whoever it's in (the player hits
        // anyone, an enemy only the player)
        if (g_Game.state != STATE_ENDING)
        {
            D3DXVECTOR3 d = lastHandOffset - handOffset;
            if (D3DXVec3Length(&d) > 2.0f)
            {
                for (int i = 0; i < g_Game.maxCharacters; i++)
                {
                    SCharacter* pOther = &g_Game.pCharacters[i];
                    if (pOther == this || !pOther->bActive)
                        continue;
                    if (team == 0)
                    {
                        if (i == 0)
                            continue;
                    }
                    else if (i > 0)
                        break;
                    if (GetTickCount() - pOther->lastHitTime >= 200 && pOther->health > 0 &&
                        g_Game.PointInBox(pos + handOffset, pOther->pos, 5.0f, 5.0f))
                    {
                        g_Sound.Play(g_Game.bBulletTime ? SOUND_HIT : (rand() % 2 == 0 ? SOUND_PUNCH : SOUND_PUNCH3));
                        g_Game.pCharacters[i].health--;
                        g_Game.pCharacters[i].lastHitTime = GetTickCount();
                    }
                }
            }
        }
    }
    else
    {
        // the held item follows the hand
        SItem* pItem = pHeld;
        pItem->vel.x = (pItem->lastPos.x - pos.x) + handOffset.x;
        pItem->vel.y = handOffset.y + (pItem->lastPos.y - pos.y);
        pItem->vel.z = handOffset.z + (pItem->lastPos.z - pos.z);
        pHeld->lastPos = pHeld->pos;
        pHeld->pos.x = handOffset.x + pos.x;
        pHeld->pos.y = handOffset.y + pos.y;
        pHeld->pos.z = handOffset.z + pos.z;
    }
    if (pBack != NULL)
    {
        pBack->vel = pBack->lastPos - pos;
        pBack->lastPos = pBack->pos;
        pBack->pos = pos;
    }
    if (pJetpack != NULL)
        pJetpack->pos = pos;
    if (pBack != NULL)
        pBack->pos = pos;

    if (team != 0)
    {
        Think();
        if (team != 0)      // (the ending hands Lantus to the player)
            goto move;
    }

    // the player
    g_Game.pPad = g_Game.input.GetPad(0);
    if (g_Game.input.BlackPressed(0) && pJetpack != NULL && pJetpack->bJetpack == TRUE)
        bJetpackOn = !bJetpackOn;
    lastPos = pos;
    if (g_Game.pPad->fX2 != 1.0f && g_Game.pPad->fX2 != -1.0f && g_Game.pPad->fY2 != 1.0f && g_Game.pPad->fY2 != -1.0f)
        lastHandOffset = handOffset;
    if (g_Game.bPaused)
        goto move;

    if (health > 0)
    {
        if (pHeld != NULL)
            pHeld->angle = (float)atan2(handOffset.y, handOffset.x);
        if (fallAngle <= STANDING_ANGLE)
        {
            handOffset.x = g_Game.pPad->fX2 * 5.0f;
            handOffset.y = g_Game.pPad->fY2 * 4.0f;
        }
        if (!bJetpackOn)
        {
            if (g_Game.input.LeftThumbLeft(0) != 0.0f)
            {
                if (feetOffset.y + pos.y <= groundY)
                    vel.x = vel.x - g_Game.frameTime * 1500.0f;
                bFacingRight = false;
            }
            if (g_Game.input.LeftThumbUp(0) == 0.0f)
                bJumpHeld = false;
            else
            {
                if (!bJumpHeld && !bDoubleJumped)
                {
                    float feet = feetOffset.y + pos.y;
                    if (feet <= groundY)
                    {
                        vel.y = vel.y + 40.0f;
                        bJumpHeld = true;
                        goto right;
                    }
                    // the double jump, steering in the air
                    vel.y = vel.y + 40.0f;
                    if (groundY - 20.0f < feet)
                        bDoubleJumped = true;
                    if (g_Game.input.LeftThumbRight(0) != 0.0f)
                    {
                        vel.x = g_Game.frameTime * 1000.0f + vel.x;
                        bFacingRight = true;
                    }
                    if (g_Game.input.LeftThumbLeft(0) != 0.0f)
                    {
                        vel.x = vel.x - g_Game.frameTime * 1000.0f;
                        bFacingRight = false;
                    }
                }
                bJumpHeld = true;
            }
right:
            if (g_Game.input.LeftThumbRight(0) != 0.0f)
            {
                if (feetOffset.y + pos.y <= groundY)
                    vel.x = g_Game.frameTime * 1500.0f + vel.x;
                bFacingRight = true;
            }
        }
        else
        {
            if (g_Game.input.LeftThumbLeft(0) != 0.0f)
            {
                vel.x = vel.x - g_Game.frameTime * 150.0f;
                bFacingRight = false;
            }
            if (g_Game.input.LeftThumbRight(0) != 0.0f)
            {
                vel.x = g_Game.frameTime * 150.0f + vel.x;
                bFacingRight = true;
            }
            if (g_Game.input.LeftThumbUp(0) != 0.0f)
                vel.y = (float)(g_Game.frameTime * 150.0 + vel.y);
        }
    }

    // right stick click: swap the hand and back items
    if (g_Game.input.RightThumbClick(0))
    {
        if (!g_Game.bSwapHeld)
        {
            g_Game.bSwapHeld = true;
            g_Game.input.SetRumble(0, 0.0f, 0.0f);
            SItem* pItem = pHeld;
            pHeld = pBack;
            pBack = pItem;
            if (pItem != NULL)
                pItem->unknown5E = true;
            if (pHeld != NULL)
                bHolding = true;
        }
    }
    else
        g_Game.bSwapHeld = false;

    // right trigger: fire (or rev the chainsaw)
    if (!g_Game.pPad->bPressedAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
        bTriggerHeld = false;
    else
    {
        SItem* pItem = pHeld;
        if (pItem != NULL && !bTriggerHeld && pItem->type == WEAPON_CHAINSAW)
            pItem->Use(pItem->pos, pItem->vel, 0.0f);
        if (bHolding && pHeld != NULL && pHeld->bGun && (!bTriggerHeld || pHeld->bAutomatic) &&
            pHeld->fireTimer <= 0.0f)
        {
            if (pHeld->ammo < 1)
            {
                if (!bTriggerHeld)
                    g_Sound.Play(SOUND_EMPTYGUN);
            }
            else
                Fire();
        }
        pItem = pHeld;
        if (pItem != NULL && pItem->type == WEAPON_CHAINSAW && pItem->fireTimer <= 0.0f)
        {
            g_Game.particles.Add(pItem->pos.x, pItem->pos.y, pItem->pos.z, -3.55f, 0.0f, 0.0f, PARTICLE_SMOKE);
            pHeld->fireTimer = 0.2f;
        }
        bTriggerHeld = true;
    }

    // right trigger, empty-handed: pick up what the hand is on
    if (g_Game.pPad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] && !bHolding && pHeld == NULL)
    {
        for (int i = 0; i < g_Game.maxItems; i++)
        {
            SItem* pItem = &g_Game.pItems[i];
            if (!pItem->bActive || pItem->bStuck || pItem->bHeld)
                continue;
            D3DXVECTOR3 hand = pos + handOffset;
            if (!g_Game.PointInBox(hand, pItem->pos, 7.0f, 4.0f))
                continue;
            if (pItem->bJetpack == TRUE)
            {
                if (pJetpack == NULL)
                {
                    pItem->bHeld = true;
                    pJetpack = pItem;
                    bJetpackOn = false;
                }
                continue;
            }
            bHolding = true;
            pHeld = pItem;
            pItem->bounces = 0;
            g_Game.pItems[i].bHeld = true;
            g_Game.pItems[i].fireTimer = g_Game.pItems[i].fireDelay * 0.5f;
            for (int c = 0; c < g_Game.maxCharacters; c++)
            {
                if (g_Game.pCharacters[c].pThrown == pHeld)
                    g_Game.pCharacters[c].pThrown = NULL;
            }
            g_Game.pItems[i].lifeTime = 8.0f;
            break;
        }
    }

    // left trigger: throw, with the hand's swing (and the body's motion)
    if (g_Game.pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && !bTriggerHeld && bHolding && pHeld != NULL)
    {
        SItem* pItem = pHeld;
        D3DXVECTOR3 v;
        v.y = (handOffset.y - lastHandOffset.y) * 40.0f;
        v.z = (handOffset.z - lastHandOffset.z) * 40.0f;
        v.x = (pos.x - lastPos.x) + (handOffset.x - lastHandOffset.x) * 40.0f;
        pItem->vel.x = v.x;
        v.y = (pos.y - lastPos.y) + v.y;
        pItem->vel.y = v.y;
        v.z = (pos.z - lastPos.z) + v.z;
        pItem->vel.z = v.z;
        pHeld->bounces = 0;
        if (D3DXVec3Length(&pHeld->vel) >= 300.0f)
        {
            D3DXVec3Normalize(&pHeld->vel, &pHeld->vel);
            pHeld->vel *= 300.0f;
        }
        if (pHeld->blade == BLADE_SPIN)
        {
            D3DXVECTOR3 m = (pos - lastPos) + (handOffset - lastHandOffset);
            pHeld->spin = D3DXVec3Length(&m) * 50.0f;
        }
        pThrown = pHeld;
        bHolding = false;
        pHeld->angle = (float)atan2(handOffset.y, handOffset.x);
        pHeld->bFacingRight = bFacingRight;
        pHeld->bHeld = false;
        pHeld->lifeTime = 8.0f;
        pHeld->unknown5E = false;
        pHeld = NULL;
    }

move:
    if (vel.x > 250.0f)
        vel.x = 250.0f;
    if (vel.x < -250.0f)
        vel.x = -250.0f;
    pos += vel * g_Game.frameTime;
    if (bCutscene)
        vel.x = 0.0f;

    if (feetOffset.y + pos.y > groundY)
        vel.y = vel.y - g_Game.frameTime * 80.0f;
    else if (!g_Game.bEnabled[UNLOCK_HYPERMODE])
    {
        if (health < 1)
        {
            // a body bounces (then lies at y = -20, whatever the ground)
            float vy = vel.y * (-1.0f / 3.0f);
            pos.y = (groundY - feetOffset.y) - 0.2f;
            vel.y = vy;
            if (vy < 4.0f)
            {
                vel.y = 0.0f;
                vel.x = 0.0f;
                pos.y = -20.0f;
            }
            bDoubleJumped = false;
        }
        else
        {
            vel.x = 0.0f;
            vel.y = 0.0f;
            vel.z = 0.0f;
            bDoubleJumped = false;
        }
    }
    else
    {
        // Hyper Mode: sliding
        vel.x *= 0.5f;
        vel.y *= 0.5f;
        vel.z *= 0.5f;
        bDoubleJumped = false;
    }

    if (this == g_Game.pCharacters)
    {
        if (pos.x < -40.0f)
            pos.x = -40.0f;
        if (pos.x > 40.0f)
            pos.x = 40.0f;
    }
    if (pos.y < -20.0f)
        pos.y = -20.0f;
    if (pos.y > 60.0f)
    {
        vel.x = 0.0f;
        vel.y = 0.0f;
        pos.y = 60.0f;
        vel.z = 0.0f;
    }
    if (pos.z > 0.0f)
    {
        vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        pos.z = 0.0f;
    }
    if (pos.z < 0.0f)
    {
        vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        pos.z = 0.0f;
    }

    if (health < 1)
    {
        // dead: counted, then removed 5 seconds later; the items drop
        if (g_Game.state != STATE_ENDING)
        {
            if (downTime == 0.0f)
            {
                if (this != g_Game.pCharacters)
                {
                    g_Game.levelKills++;
                    g_Game.totalKills++;
                }
                downTime = 5.0f;
            }
            if (downTime > 0.0f)
                downTime -= g_Game.frameTime;
            else
            {
                bActive = false;
                if (bImpaled)
                {
                    pStuck->bStuck = false;
                    pStuck->bActive = false;
                }
            }
        }
        if (pHeld != NULL)
        {
            bHolding = false;
            Drop(pHeld);
            pHeld->bounces = 1;
            pHeld = NULL;
        }
        if (pBack != NULL)
        {
            Drop(pBack);
            pBack->bounces = 1;
            pBack->unknown5E = false;
            pBack = NULL;
        }
        if (pJetpack != NULL)
        {
            Drop(pJetpack);
            pJetpack->unknown5E = false;
            pJetpack->bounces = 1;
            pJetpack = NULL;
        }
        if (fallAngle <= STANDING_ANGLE)
            fallAngle = g_Game.frameTime * 3.0f + fallAngle;
    }
}
