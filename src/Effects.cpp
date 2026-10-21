// Particles (blood, shells, smoke, bullet trails, bubbles) and bullets.
#include "Game.h"

// 0x0001AEE0
void CParticles::Init()
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        SParticle& p = particles[i];
        p.vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        p.pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        p.bActive = false;
        p.life = 0.0f;
        p.type = PARTICLE_NONE;
        p.bFacingRight = false;
        p.size = 0.0f;
        p.gravity = 0.0f;
    }
    dwInitTime = GetTickCount();
}

// 0x0001AF90
// Takes the first free particle; does nothing if all 300 are in use.
void CParticles::Add(float x, float y, float z, float vx, float vy, float vz, int type)
{
    int i = 0;
    while (particles[i].bActive)
    {
        if (++i >= MAX_PARTICLES)
            return;
    }

    SParticle& p = particles[i];
    p.pos = D3DXVECTOR3(x, y, z);
    p.vel = D3DXVECTOR3(vx, vy, vz);
    p.life = 0.5f;
    p.bActive = true;
    p.type = (short)type;
    p.size = 1.0f;
    switch (type)
    {
    case PARTICLE_SMOKE:
        p.gravity = -4.5f;      // rises
        p.life = 5.0f;
        break;
    case PARTICLE_TRAIL:
        p.gravity = 0.0f;
        p.life = 0.3f;
        break;
    case PARTICLE_BUBBLE:
        p.gravity = -4.0f;
        p.life = 15.0f;
        break;
    case PARTICLE_GUSH:
        p.gravity = 35.0f;
        p.life = 10.0f;
        p.size = 2.0f;
        break;
    default:
        p.gravity = 35.0f;
        p.life = 10.0f;
        break;
    }
}

// 0x0001B0A0
// Moves the particles that have gravity. Below the floor, shells bounce
// (clinking while they still bounce high enough) and everything else stops,
// blood spreading out.
void CParticles::Update()
{
    int i;
    for (i = 0; i < MAX_PARTICLES; i++)
    {
        SParticle& p = particles[i];
        if (!p.bActive)
            continue;

        p.life -= g_Game.frameTime;
        if (p.type == PARTICLE_BUBBLE)
        {
            if (p.gravity != 0.0f)
            {
                p.pos += p.vel * g_Game.frameTime;
                p.vel.y -= g_Game.frameTime * p.gravity;
            }
        }
        else if (p.gravity != 0.0f)
        {
            p.pos += p.vel * g_Game.frameTime;
            p.vel.y -= g_Game.frameTime * p.gravity;
            if (p.pos.y < GROUND_Y)
            {
                if (p.type == PARTICLE_SHELL)
                {
                    p.vel.y *= -0.5f;
                    p.pos.y = GROUND_BOUNCE_Y;
                    if (p.vel.y >= 3.0f)
                    {
                        g_Sound.Play(SOUND_SHELL);
                        continue;
                    }
                }
                else if (p.type == PARTICLE_BIGSHELL)
                {
                    p.pos.y = GROUND_BOUNCE_Y;
                    p.vel.y = (float)(-p.vel.y * (2.0 / 3.0));
                    if (p.vel.y >= 5.0f)
                    {
                        g_Sound.Play(SOUND_SHELL);
                        continue;
                    }
                }
                else
                {
                    p.vel.x = 0.0f;
                    p.vel.y = 0.0f;
                    p.pos.y = GROUND_Y;
                    p.vel.z = 0.0f;
                    p.size = (float)(g_Game.frameTime * 1.5 + p.size);
                    continue;
                }
                p.vel.y = 0.0f;
                p.vel.x = 0.0f;
                p.vel.y = 0.0f;
                p.pos.y = GROUND_Y;
                p.vel.z = 0.0f;
            }
        }
    }

    for (i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].bActive && particles[i].life <= 0.0f)
            particles[i].bActive = false;
    }
}

// 0x0001B330
// Each particle is a textured quad. Smoke fades out (additively blended
// through the texture factor); a trail stretches out and points along its
// bullet's path.
void CParticles::Render()
{
    GetTickCount();
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        SParticle& p = particles[i];
        if (!p.bActive)
            continue;

        D3DXMATRIX matTrans, matScale, matRot;
        D3DXMatrixIdentity(&matTrans);
        D3DXMatrixIdentity(&matScale);
        D3DXMatrixIdentity(&matRot);
        D3DXMatrixTranslation(&matTrans, p.pos.x, p.pos.y, p.pos.z);
        D3DXMatrixScaling(&matScale, 0.5f, 0.5f, 0.0f);

        // 0 to 1 over the particle's life
        float age;
        if (p.type == PARTICLE_SMOKE)
            age = (7.0f - p.life) * (1.0f / 7.0f);
        else if (p.type == PARTICLE_TRAIL)
            age = (0.3f - p.life) * 5.0f;
        else
            age = (10.0f - p.life) * 0.1f;

        int alpha;
        switch (p.type)
        {
        case PARTICLE_BLOOD:
            D3DXMatrixScaling(&matScale, (float)(p.size * 0.5), 0.5f, 0.0f);
            g_Game.pDevice->SetTexture(0, g_Game.texBlood);
            break;
        case PARTICLE_SHELL:
            g_Game.pDevice->SetTexture(0, g_Game.texShotgunShell);
            D3DXMatrixScaling(&matScale, 1.5f, 0.5f, 0.0f);
            break;
        case PARTICLE_SMOKE:
            D3DXMatrixScaling(&matScale, age * 8.0f, age * 8.0f, 0.0f);
            g_Game.pDevice->SetTexture(0, g_Game.texSmoke);
            alpha = (int)(255.0f - age * 255.0f);
            if (alpha < 0)
                alpha = 0;
            break;
        case PARTICLE_TRAIL:
            g_Game.pDevice->SetTexture(0, g_Game.texBulletTrail);
            D3DXMatrixScaling(&matScale, 0.2f, (float)(age * 1.5), 0.0f);
            D3DXMatrixRotationZ(&matRot, (float)atan2(p.vel.y, p.vel.x));
            break;
        case PARTICLE_BIGSHELL:
            g_Game.pDevice->SetTexture(0, g_Game.texShotgunShell);
            D3DXMatrixScaling(&matScale, 0.5f, 0.5f, 0.0f);
            break;
        case PARTICLE_BUBBLE:
            g_Game.pDevice->SetTexture(0, g_Game.texBubble);
            D3DXMatrixScaling(&matScale, 5.0f, 5.0f, 0.0f);
            break;
        case PARTICLE_GUSH:
            D3DXMatrixScaling(&matScale, (float)(p.size * 2.5), 2.0f, 0.0f);
            g_Game.pDevice->SetTexture(0, g_Game.texBlood);
            break;
        }

        matTrans = matScale * matRot * matTrans;
        g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
        g_Game.pDevice->SetStreamSource(0, p.bFacingRight ? g_Game.pQuadVB : g_Game.pVB460, sizeof(SPRITEVERTEX));
        if (p.type == PARTICLE_SMOKE)
        {
            g_Game.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            g_Game.pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            g_Game.pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            g_Game.pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, (alpha << 24) | 0xFFFFFF);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            g_Game.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        }
        g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
        g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
        if (p.type == PARTICLE_SMOKE)
        {
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            g_Game.pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
    }
}

// 0x000244F0
void CBullets::Init()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        SBullet& b = bullets[i];
        b.trailTimer = 0.0f;
        b.life = 0.0f;
        b.bActive = FALSE;
        b.vx = 0.0f;
        b.vy = 0.0f;
        b.x = 0.0f;
        b.y = 0.0f;
        b.bFacingRight = false;
    }
}

// 0x00024550
// Fires a bullet along (dx, dy) at ten times that speed; nothing happens
// if all 15 are in flight.
void CBullets::Add(float x, float y, float dx, float dy, BOOL bActive, bool bFacingRight)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        SBullet& b = bullets[i];
        if (!b.bActive)
        {
            b.x = x;
            b.y = y;
            b.vx = dx * 10.0f;
            b.vy = dy * 10.0f;
            b.bActive = bActive;
            b.trailTimer = 0.1f;
            b.life = 10.0f;
            b.bFacingRight = bFacingRight;
            return;
        }
    }
}

// 0x000245E0
// Moves the bullets and hits the standing characters: a bullet in the body
// box (4 x 10) or the head box (4 x 4) leaves a wound and takes 1 health,
// 3 more in the head. All five wounds used: no damage, but the bullet still
// stops. A bullet can hit several characters in the same frame.
void CBullets::Update()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        SBullet& b = bullets[i];
        if (!b.bActive)
            continue;

        b.x += b.vx * g_Game.frameTime;
        b.y += b.vy * g_Game.frameTime;
        b.trailTimer -= g_Game.frameTime;
        b.life -= g_Game.frameTime;

        for (int c = 0; c < g_Game.maxCharacters; c++)
        {
            SCharacter* pChar = &g_Game.pCharacters[c];
            if (!pChar->bActive || pChar->fallAngle > STANDING_ANGLE)
                continue;

            if (b.x < pChar->pos.x - 2.0f || b.x > pChar->pos.x + 2.0f ||
                b.y < pChar->pos.y - 5.0f || b.y > pChar->pos.y + 5.0f)
            {
                float hx = pChar->headOffset.x + pChar->pos.x;
                float hy = pChar->headOffset.y + pChar->pos.y;
                if (b.x < hx - 2.0f || b.x > hx + 2.0f || b.y < hy - 2.0f || b.y > hy + 2.0f)
                    continue;
            }

            for (int w = 0; w < MAX_WOUNDS; w++)
            {
                if (!pChar->wounds[w].bUsed)
                {
                    pChar->wounds[w].bUsed = true;
                    g_Game.pCharacters[c].health--;
                    float hx = g_Game.pCharacters[c].headOffset.x + g_Game.pCharacters[c].pos.x;
                    float hy = g_Game.pCharacters[c].headOffset.y + g_Game.pCharacters[c].pos.y;
                    if (hx - 2.0f <= b.x && b.x <= hx + 2.0f && hy - 2.0f <= b.y && b.y <= hy + 2.0f)
                        g_Game.pCharacters[c].health -= 3;
                    g_Game.pCharacters[c].wounds[w].x = 0.0f;
                    g_Game.pCharacters[c].wounds[w].y = b.y - g_Game.pCharacters[c].pos.y;
                    break;
                }
            }
            b.bActive = FALSE;
        }

        if (b.life <= 0.0f)
            b.bActive = FALSE;
        if (b.trailTimer <= 0.0f)
        {
            g_Game.particles.Add(b.x, b.y, 1.0f, b.vx, b.vy, 0.0f, PARTICLE_TRAIL);
            b.trailTimer = 0.1f;
        }
    }
}

// 0x00024B40
void CBullets::Render()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        SBullet& b = bullets[i];
        if (!b.bActive)
            continue;

        GetTickCount();
        g_Game.pDevice->SetTexture(0, g_Game.texBullet);
        D3DXMATRIX matTrans, matScale, matRot;
        D3DXMatrixIdentity(&matTrans);
        D3DXMatrixIdentity(&matScale);
        D3DXMatrixIdentity(&matRot);
        D3DXMatrixTranslation(&matTrans, b.x, b.y, 1.0f);
        D3DXMatrixScaling(&matScale, 0.5f, 0.5f, 0.0f);
        D3DXMatrixRotationZ(&matRot, (float)atan2(b.vy, b.vx));
        matTrans = matScale * matRot * matTrans;
        g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
        g_Game.pDevice->SetStreamSource(0, b.bFacingRight ? g_Game.pQuadVB : g_Game.pVB460, sizeof(SPRITEVERTEX));
        g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
        g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
    }
}
