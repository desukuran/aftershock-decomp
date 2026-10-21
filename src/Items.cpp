// Items: the weapons and the jetpack.
#include "Game.h"

// 0x00011FB0
// (bJetpack is left uninitialized.)
SItem::SItem()
{
    lastPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    type = WEAPON_SHOTGUN;
    bSticks = false;
    bGun = false;
    blade = BLADE_NONE;
    bounces = 0;
    angle = 0.0f;
    fireTimer = 0.0f;
    unknown30 = 0.0f;
    fireDelay = 0.0f;
    spin = 0.0f;
    ammo = 0;
    bFacingRight = FALSE;
    bStuck = false;
    bHeld = false;
    lifeTime = 0.0f;
    bAutomatic = false;
    unknown5E = false;
    bSpawned = false;
    bActive = false;
}

// 0x000272D0
// Sets up the item for its type, and the type's drawn size (again: every
// reset writes the same values into g_Game.pWeaponTypes). An unknown type
// becomes a shotgun with a faster trigger.
void SItem::Reset()
{
    bSpawned = false;
    unknown5E = false;
    bounces = 0;
    angle = 0.0f;
    spin = 0.0f;
    bFacingRight = FALSE;
    ammo = 0;

    SWeaponType* pTypes = g_Game.pWeaponTypes;
    switch (type)
    {
    case WEAPON_KNIFE:
        fireDelay = 0.0f;
        ammo = 0;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        blade = BLADE_POINT;
        bSticks = true;
        pTypes[WEAPON_KNIFE].width = 5.0f;
        pTypes[WEAPON_KNIFE].height = 2.0f;
        pTypes[WEAPON_KNIFE].unknown14 = 1.5f;
        pTypes[WEAPON_KNIFE].unknown18 = 1.5f;
        break;
    case WEAPON_BAT:
        fireDelay = 0.0f;
        ammo = 0;
        blade = BLADE_NONE;
        bSticks = false;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        pTypes[WEAPON_BAT].width = 8.0f;
        pTypes[WEAPON_BAT].height = 2.7f;
        pTypes[WEAPON_BAT].unknown14 = 2.5f;
        pTypes[WEAPON_BAT].unknown18 = 2.5f;
        break;
    case WEAPON_NIGHTSTICK:
        fireDelay = 0.0f;
        ammo = 0;
        blade = BLADE_NONE;
        bSticks = false;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        pTypes[WEAPON_NIGHTSTICK].width = 7.0f;
        pTypes[WEAPON_NIGHTSTICK].height = 3.0f;
        pTypes[WEAPON_NIGHTSTICK].unknown14 = 1.5f;
        pTypes[WEAPON_NIGHTSTICK].unknown18 = 1.5f;
        break;
    case WEAPON_SWORD:
        fireDelay = 0.0f;
        ammo = 0;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        blade = BLADE_POINT;
        bSticks = true;
        pTypes[WEAPON_SWORD].width = 10.0f;
        pTypes[WEAPON_SWORD].height = 2.5f;
        pTypes[WEAPON_SWORD].unknown14 = 3.5f;
        pTypes[WEAPON_SWORD].unknown18 = 3.5f;
        break;
    case WEAPON_HANDGUN:
        fireDelay = 0.3f;
        blade = BLADE_NONE;
        bSticks = false;
        bJetpack = FALSE;
        bAutomatic = false;
        bStuck = false;
        ammo = 8;
        bGun = true;
        pTypes[WEAPON_HANDGUN].width = 5.0f;
        pTypes[WEAPON_HANDGUN].height = 3.0f;
        pTypes[WEAPON_HANDGUN].unknown14 = 1.0f;
        pTypes[WEAPON_HANDGUN].unknown18 = 1.0f;
        break;
    case WEAPON_SHOTGUN:
        fireDelay = 0.9f;
        blade = BLADE_NONE;
        bSticks = false;
        bJetpack = FALSE;
        bAutomatic = false;
        bStuck = false;
        ammo = 8;
        bGun = true;
        pTypes[WEAPON_SHOTGUN].width = 7.0f;
        pTypes[WEAPON_SHOTGUN].height = 4.0f;
        pTypes[WEAPON_SHOTGUN].unknown14 = 2.4f;
        pTypes[WEAPON_SHOTGUN].unknown18 = 2.4f;
        break;
    case WEAPON_JETPACK:
        ammo = 0;
        blade = BLADE_NONE;
        bSticks = false;
        bGun = false;
        bStuck = false;
        fireDelay = 0.0f;
        bJetpack = TRUE;
        pTypes[WEAPON_JETPACK].width = 4.0f;
        pTypes[WEAPON_JETPACK].height = 4.0f;
        pTypes[WEAPON_JETPACK].unknown14 = 0.0f;
        pTypes[WEAPON_JETPACK].unknown18 = 0.0f;
        break;
    case WEAPON_MACHINEGUN:
        fireDelay = 0.3f;
        blade = BLADE_NONE;
        bSticks = false;
        bJetpack = FALSE;
        bStuck = false;
        ammo = 50;
        bGun = true;
        bAutomatic = true;
        pTypes[WEAPON_MACHINEGUN].width = 7.0f;
        pTypes[WEAPON_MACHINEGUN].height = 4.0f;
        pTypes[WEAPON_MACHINEGUN].unknown14 = 0.0f;
        pTypes[WEAPON_MACHINEGUN].unknown18 = 0.0f;
        break;
    case WEAPON_AXE:
        fireDelay = 0.0f;
        ammo = 0;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        blade = BLADE_SPIN;
        bSticks = true;
        pTypes[WEAPON_AXE].width = 5.0f;
        pTypes[WEAPON_AXE].height = 4.0f;
        pTypes[WEAPON_AXE].unknown14 = 2.0f;
        pTypes[WEAPON_AXE].unknown18 = 2.0f;
        break;
    case WEAPON_CHAINSAW:
        fireDelay = 0.2f;
        blade = BLADE_NONE;
        bSticks = false;
        bGun = false;
        bJetpack = FALSE;
        bStuck = false;
        ammo = 400;             // fuel
        bAutomatic = true;
        pTypes[WEAPON_CHAINSAW].width = 11.0f;
        pTypes[WEAPON_CHAINSAW].height = 5.0f;
        pTypes[WEAPON_CHAINSAW].unknown14 = 2.0f;
        pTypes[WEAPON_CHAINSAW].unknown18 = 2.0f;
        break;
    case WEAPON_RIFLE:
        fireDelay = 0.2f;
        blade = BLADE_NONE;
        bSticks = false;
        bJetpack = FALSE;
        bStuck = false;
        ammo = 100;
        bGun = true;
        bAutomatic = true;
        pTypes[WEAPON_RIFLE].width = 10.0f;
        pTypes[WEAPON_RIFLE].height = 4.0f;
        pTypes[WEAPON_RIFLE].unknown14 = 2.0f;
        pTypes[WEAPON_RIFLE].unknown18 = 2.0f;
        break;
    default:
        fireDelay = 0.4f;
        blade = BLADE_NONE;
        bSticks = false;
        bJetpack = FALSE;
        bAutomatic = false;
        bStuck = false;
        type = WEAPON_SHOTGUN;
        ammo = 8;
        bGun = true;
        pTypes[WEAPON_SHOTGUN].width = 7.0f;
        pTypes[WEAPON_SHOTGUN].height = 4.0f;
        pTypes[WEAPON_SHOTGUN].unknown14 = 2.4f;
        pTypes[WEAPON_SHOTGUN].unknown18 = 2.4f;
        break;
    }
}

// 0x00025230
// A shot (or a chainsaw rev): restarts the trigger delay, uses up a round,
// plays the sound and throws out a shell. extraDelay is 0 for the player,
// who also gets the rumble. The handgun throws no shell.
void SItem::Use(float x, float y, float z, float dx, float dy, float dz, float extraDelay)
{
    fireTimer = fireDelay + extraDelay;
    ammo--;
    switch (type)
    {
    case WEAPON_HANDGUN:
        break;
    case WEAPON_SHOTGUN:
        if (extraDelay == 0.0f)
            g_Game.input.SetRumble(0, 9.0f, 9.0f);
        g_Sound.Play(g_Game.bBulletTime ? SOUND_BULLETTIMESHOT : SOUND_SHOTGUN);
        g_Game.particles.Add(x, y, z, dx, dy, dz, PARTICLE_SHELL);
        return;
    default:
        g_Sound.Play(SOUND_SHOTGUN);
        return;
    case WEAPON_MACHINEGUN:
        if (extraDelay == 0.0f)
            g_Game.input.SetRumble(0, 3.0f, 1.0f);
        g_Sound.Play(g_Game.bBulletTime ? SOUND_BULLETTIMEMACHINE : SOUND_MACHINEGUN);
        g_Game.particles.Add(x, y, z, dx * 1.5f, dy * 1.5f, dz * 1.5f, PARTICLE_BIGSHELL);
        return;
    case WEAPON_CHAINSAW:
        if (extraDelay == 0.0f)
            g_Game.input.SetRumble(0, 9.0f, 9.0f);
        g_Sound.Play(SOUND_CHAINSAW);
        return;
    case WEAPON_RIFLE:
        if (extraDelay == 0.0f)
            g_Game.input.SetRumble(0, 3.0f, 3.0f);
        g_Sound.Play(g_Game.bBulletTime ? SOUND_BULLETTIMESHOT : SOUND_RIFLE);
        g_Game.particles.Add(x, y, z, dx, dy, dz, PARTICLE_SHELL);
        return;
    }

    // the handgun
    if (extraDelay == 0.0f)
        g_Game.input.SetRumble(0, 5.0f, 3.0f);
    g_Sound.Play(g_Game.bBulletTime ? SOUND_BULLETTIMESHOT : SOUND_PISTOL);
}

// Whether any corner of the box (x +- dx, y +- dy) is in the box around (bx, by).
static bool CornerInBox(float x, float y, float z, float dx, float dy, const D3DXVECTOR3& b, float w, float h)
{
    return g_Game.PointInBox(D3DXVECTOR3(x + dx, y + dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x - dx, y + dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x + dx, y - dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x - dx, y - dy, z), b, w, h);
}

// 0x000254D0
// owner: 1 held by the player, 2 held by an enemy, 0 otherwise. delay > 0
// (an enemy's empty gun) skips the hits.
//
// Lying around it times out; flying it falls, bounces and spins down. A
// revving chainsaw cuts heads off; a flying or swung item hits bodies: it
// bounces off them, sticks in them (a fast blade), knocks them back and
// hurts them by its speed, and a sword through the head decapitates. A bat
// can also knock a flying head about.
void SItem::Update(int owner, float delay)
{
    if (!bActive)
        return;

    if (bHeld)
        bSpawned = false;

    SCharacter* pPlayer = g_Game.pCharacters;
    if (unknown5E || (pPlayer->pBack != NULL && pPlayer->pBack == this) ||
        (pPlayer->pJetpack != NULL && pPlayer->pJetpack == this))
        return;

    if (!bHeld)
    {
        if (lifeTime >= 0.0f)
            lifeTime -= g_Game.frameTime;
        else
        {
            lifeTime = 0.0f;
            bActive = false;
        }
    }
    if (fireTimer > 0.0f)
        fireTimer -= g_Game.frameTime;

    int i;
    if (type == WEAPON_CHAINSAW && bHeld)
    {
        if (g_Game.state == STATE_EXPERIMENT && bSpawned)
            fireTimer = 0.0f;
        if (fireTimer > 0.0f)
        {
            // running: the blade's ends cut off heads
            float s = (float)(sin(angle) * 2.5);
            float c = (float)cos(angle);
            for (i = 0; i < g_Game.maxCharacters; i++)
            {
                SCharacter& ch = g_Game.pCharacters[i];
                if (ch.bRespawning || (g_Game.state == STATE_EXPERIMENT && i == 0))
                    continue;
                if (owner == 1)
                {
                    if (i == 0)
                        continue;
                }
                else if (owner == 2 && i > 0)
                    break;
                if (ch.pThrown != NULL && ch.pThrown == this)
                {
                    if (ch.team > 0)
                        break;
                    continue;
                }
                if (ch.fallAngle > STANDING_ANGLE)
                    continue;
                if (!CornerInBox(pos.x, pos.y, pos.z, s, c, ch.pos, 6.0f, 12.0f))
                    continue;
                if (this == ch.pHeld || this == ch.pBack || this == ch.pJetpack ||
                    GetTickCount() - ch.lastHitTime < 400 || ch.health <= 0)
                    continue;

                ch.vel = pos - lastPos;
                g_Game.pCharacters[i].health -= 10;
                g_Game.pCharacters[i].bodyGib = 1;
                g_Game.pCharacters[i].headGib = 1;
                g_Game.pCharacters[i].bHeadless = true;
                g_Game.pCharacters[i].headVel = pos - lastPos;
                g_Game.pCharacters[i].headVel.y *= 3.0f;
                g_Game.pCharacters[i].headPos = g_Game.pCharacters[i].headOffset + g_Game.pCharacters[i].pos;
                g_Game.pCharacters[i].health = 0;
                g_Game.numDecapitations++;
                for (int n = 0; n < 10; n++)
                {
                    int vy = rand() % 50;
                    int vx = rand() % 10;
                    D3DXVECTOR3& p = g_Game.pCharacters[i].pos;
                    g_Game.particles.Add(p.x, p.y, p.z, (float)vx, (float)vy, 0.0f, PARTICLE_BLOOD);
                }
                D3DXVECTOR3& p = g_Game.pCharacters[i].pos;
                g_Game.particles.Add(p.x, p.y, p.z, 1.0f, 0.0f, 0.0f, PARTICLE_GUSH);
                g_Sound.Play(SOUND_RIFLE);
                g_Game.pCharacters[i].lastHitTime = GetTickCount();
            }
        }
    }

    if (!bHeld)
    {
        if (!bStuck)
        {
            pos += vel * g_Game.frameTime;
            if (pos.y > GROUND_Y)
                vel.y = (float)(vel.y - g_Game.frameTime * 80.0);
            else
            {
                // a bounce: a third of the speed back up, and the blade
                // turns towards lying flat
                float vy = vel.y * (-1.0f / 3.0f);
                spin *= 1.0f / 3.0f;
                pos.y = GROUND_BOUNCE_Y;
                vel.y = vy;
                bounces++;
                if (!bFacingRight)
                {
                    if (angle > 0.0f && angle < D3DX_PI)
                        angle = angle * 0.5f + angle;
                    if (angle < 0.0f && angle > -D3DX_PI)
                        angle = angle * 0.5f + angle;
                }
                else
                {
                    if (angle > 0.0f && angle < D3DX_PI)
                        angle = angle - angle * 0.5f;
                    if (angle < 0.0f && angle > -D3DX_PI)
                        angle = angle - angle * 0.5f;
                }
                if (vy < 4.0f)
                {
                    vel.y = 0.0f;
                    vel.x = 0.0f;
                    vel.y = 0.0f;
                    pos.y = GROUND_Y;
                    vel.z = 0.0f;
                    angle = bFacingRight ? 0.0f : D3DX_PI;
                    bSpawned = false;
                }
            }
        }
        if (!bHeld && vel.x == 0.0f && (vel.y == 0.0f || bStuck))
            return;
    }

    if (((g_Game.state == STATE_EXPERIMENT || g_Game.state == STATE_ENDING) && bSpawned) ||
        delay > 0.0f || type == WEAPON_JETPACK)
        return;

    for (i = 0; i < g_Game.maxCharacters; i++)
    {
        SCharacter& ch = g_Game.pCharacters[i];
        if (!ch.bActive || ch.bRespawning)
            continue;
        if (bounces > 1)
            return;
        if (owner == 1)
        {
            if (i == 0)
                continue;
        }
        else if (owner == 2 && i > 0)
            return;
        if (ch.pThrown != NULL && ch.pThrown == this)
        {
            if (ch.team > 0)
                return;
            continue;
        }

        float s = (float)(sin(angle) * 2.5);
        float c = (float)cos(angle);
        float speed = D3DXVec3Length(&vel);
        if (speed < 0.5f && speed > -0.5f)
            continue;

        // a bat knocking a flying head about
        if (i != 0 && ch.bHeadless && type == WEAPON_BAT && speed > 2.0f &&
            CornerInBox(pos.x, pos.y, pos.z, s, c, ch.headPos, 5.0f, 5.0f) &&
            (int)(GetTickCount() - g_Game.pCharacters[i].lastHitTime) > 399 &&
            (bHeld == true || (this == g_Game.pCharacters[0].pThrown && bounces < 3)))
        {
            g_Game.pCharacters[i].lastHitTime = GetTickCount();
            g_Game.pCharacters[i].headVel = (pos - lastPos) * 12.0f;
            g_Game.pCharacters[i].downTime += 3.0f;
            g_Game.pCharacters[i].headVel.y *= 12.0f;
            g_Sound.Play(SOUND_PUNCH);
        }

        SCharacter& tc = g_Game.pCharacters[i];
        if (tc.fallAngle > STANDING_ANGLE && !tc.bHeadless)
            continue;
        if (!CornerInBox(pos.x, pos.y, pos.z, s, c, tc.pos, 5.0f, 12.0f))
            continue;
        if (this == tc.pHeld || this == tc.pBack || this == tc.pJetpack || tc.fallAngle > STANDING_ANGLE)
            continue;

        if (!bSticks)
            vel.x = -vel.x * 0.6666667f;
        else if (!bHeld && D3DXVec3Length(&vel) >= 50.0f && !tc.bImpaled)
        {
            // a fast blade sticks in an enemy, killing it
            if (g_Game.PointInBox(pos, tc.pos, 5.0f, 9.0f) && i != 0)
            {
                vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                g_Game.pCharacters[i].pStuck = this;
                g_Game.pCharacters[i].bImpaled = true;
                pos.x -= g_Game.pCharacters[i].pos.x;
                pos.y -= g_Game.pCharacters[i].pos.y;
                bStuck = true;
                g_Game.pCharacters[i].health = 0;
                g_Sound.Play(SOUND_PUNCH2);
                for (int n = 0; n < 10; n++)
                {
                    int vx = bFacingRight ? rand() % 10 : -(rand() % 10);
                    D3DXVECTOR3& p = g_Game.pCharacters[i].pos;
                    g_Game.particles.Add(pos.x + p.x, p.y + pos.y, p.z + pos.z, (float)vx, (float)(rand() % 10), 0.0f, PARTICLE_BLOOD);
                }
                return;
            }
            continue;
        }

        float hitSpeed = D3DXVec3Length(&vel);
        if ((hitSpeed >= 5.0f || hitSpeed <= -5.0f) && (int)(GetTickCount() - tc.lastHitTime) > 399 && tc.health > 0)
        {
            g_Game.pCharacters[i].lastHitTime = GetTickCount();
            g_Game.pCharacters[i].vel = (pos - lastPos) * 5.0f;
            if ((g_Game.state != STATE_EXPERIMENT || i != 0) && bounces < 1)
            {
                // 1 to 3 damage by speed (lower limits in bullet time)
                g_Game.pCharacters[i].health--;
                float limit;
                if (!g_Game.bBulletTime)
                {
                    limit = 190.0f;
                    if (hitSpeed > 100.0f)
                        g_Game.pCharacters[i].health--;
                }
                else
                {
                    limit = 130.0f;
                    if (hitSpeed > 80.0f)
                        g_Game.pCharacters[i].health--;
                }
                if (hitSpeed > limit)
                    g_Game.pCharacters[i].health--;

                if (type == WEAPON_SWORD)
                {
                    SCharacter& sc = g_Game.pCharacters[i];
                    if (!CornerInBox(pos.x, pos.y, pos.z, s, c, sc.headOffset + sc.pos, 5.0f, 5.0f))
                        continue;
                    sc.bHeadless = true;
                    g_Game.pCharacters[i].headVel = (pos - lastPos) * 3.0f;
                    g_Game.pCharacters[i].headVel.y *= 3.0f;
                    g_Game.pCharacters[i].headBloodTimer = 0.2f;
                    g_Game.pCharacters[i].headPos = g_Game.pCharacters[i].headOffset + g_Game.pCharacters[i].pos;
                    g_Game.pCharacters[i].health = 0;
                    g_Sound.Play(SOUND_SWORD);
                    g_Game.numDecapitations++;
                }
                else
                    g_Sound.Play(g_Game.bBulletTime ? SOUND_HIT : SOUND_PUNCH);
            }
        }
    }
}

// 0x00026FA0
// Pointed blades fly point first, the axe spins; on the ground they lie
// flat, facing the way they were thrown. Stuck items are drawn with their
// body.
void SItem::Render()
{
    if (!bActive || bStuck)
        return;

    D3DXMATRIX matTrans, matScale, matRot;
    D3DXMatrixIdentity(&matTrans);
    D3DXMatrixIdentity(&matScale);
    D3DXMatrixIdentity(&matRot);
    if (blade == BLADE_POINT)
    {
        if (pos.y > GROUND_Y)
            angle = (float)atan2(pos.y - lastPos.y, pos.x - lastPos.x);
        else
            angle = bFacingRight ? 0.0f : D3DX_PI;
    }
    if (blade == BLADE_SPIN)
    {
        if (pos.y > GROUND_Y)
        {
            angle += spin * g_Game.frameTime;
            spin -= g_Game.frameTime;
            if (spin < 0.0f)
                spin = 0.0f;
        }
        else
            angle = bFacingRight ? 0.0f : D3DX_PI;
    }
    D3DXMatrixRotationZ(&matRot, angle);
    D3DXMatrixTranslation(&matTrans, pos.x, pos.y, pos.z);
    D3DXMatrixScaling(&matScale, g_Game.pWeaponTypes[type].width, g_Game.pWeaponTypes[type].height, 0.0f);
    matTrans = matScale * matRot * matTrans;
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
    g_Game.pDevice->SetTexture(0, g_Game.pWeaponTypes[type].pTexture);
    g_Game.pDevice->SetStreamSource(0, bFacingRight ? g_Game.pQuadVB : g_Game.pVB460, sizeof(SPRITEVERTEX));
    g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
    g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
}
