// Collision tests: rays (laser bolts) against spheres and oriented boxes,
// and a rough oriented box against an axis-aligned one.
#include "Game.h"

// 0x00012CB0
// A ray from origin along the unit vector dir: does it pass within radius
// of center, ahead of the origin? *t is where it enters the sphere.
bool RaySphereIntersect(const D3DXVECTOR3* pCenter, float fRadius, const D3DXVECTOR3* pOrigin,
                        const D3DXVECTOR3* pDir, float* pT)
{
    D3DXVECTOR3 v = *pCenter - *pOrigin;
    float b = D3DXVec3Dot(pDir, &v);
    if (b < 0.0f)
        return false;
    float d = fRadius * fRadius - D3DXVec3Dot(&v, &v) + b * b;
    if (d < 0.0f)
        return false;
    *pT = b - sqrtf(d);
    return true;
}

// 0x00012860
// The slab test of a ray against a box at pCenter with half sizes
// pExtents along pAxisU (z), pAxisV (y) and their cross product (x); *pT is
// where it enters. A ray parallel to a slab is rejected when it's inside
// the slab rather than outside (quirk), and still divided by.
bool RayBoxIntersect(const D3DXVECTOR3* pOrigin, const D3DXVECTOR3* pDir, const D3DXVECTOR3* pExtents,
                     const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU, const D3DXVECTOR3* pAxisV,
                     float* pT)
{
    D3DXVECTOR3 w;
    D3DXVec3Cross(&w, pAxisV, pAxisU);
    D3DXVECTOR3 p = *pCenter - *pOrigin;
    *pT = -1e11f;
    float tMax = 1e11f;

    float f = D3DXVec3Dot(&w, pDir);
    float e = D3DXVec3Dot(&w, &p);
    if (fabs(f) < 1e-8 && (-e - pExtents->x > 0.0f || pExtents->x - e > 0.0f))
        return false;
    float t1 = (pExtents->x + e) * (1.0f / f);
    float t2 = (e - pExtents->x) * (1.0f / f);
    if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
    if (t1 > *pT)
        *pT = t1;
    if (t2 < tMax)
        tMax = t2;
    if (*pT > tMax || tMax < 0.0f)
        return false;

    f = D3DXVec3Dot(pDir, pAxisU);
    e = D3DXVec3Dot(pAxisU, &p);
    if (fabs(f) < 1e-8 && (-e - pExtents->z > 0.0f || pExtents->z - e > 0.0f))
        return false;
    t1 = (pExtents->z + e) * (1.0f / f);
    t2 = (e - pExtents->z) * (1.0f / f);
    if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
    if (t1 > *pT)
        *pT = t1;
    if (t2 < tMax)
        tMax = t2;
    if (*pT > tMax || tMax < 0.0f)
        return false;

    if (fabs(D3DXVec3Dot(pDir, pAxisV)) < 1e-8)
    {
        e = D3DXVec3Dot(pAxisV, &p);
        if (-e - pExtents->y > 0.0f || pExtents->y - e > 0.0f)
            return false;
    }
    e = D3DXVec3Dot(pAxisV, &p);
    f = D3DXVec3Dot(pAxisV, pDir);
    t1 = (pExtents->y + e) * (1.0f / f);
    t2 = (e - pExtents->y) * (1.0f / f);
    if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
    if (t1 > *pT)
        *pT = t1;
    if (t2 < tMax)
        tMax = t2;
    if (*pT > tMax || tMax < 0.0f)
        return false;
    return true;
}

// 0x00012DB0
// Does a box with half sizes pExtents at pCenter, oriented by pAxisU and
// pAxisV, reach into the axis-aligned box pBox? The axes aren't made
// positive, so only roughly.
bool BoxTest(const SBox* pBox, const float* pExtents, const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU,
             const D3DXVECTOR3* pAxisV)
{
    float cx = (pBox->vMin.x + pBox->vMax.x) * 0.5f;
    float cy = (pBox->vMin.y + pBox->vMax.y) * 0.5f;
    float cz = (pBox->vMin.z + pBox->vMax.z) * 0.5f;
    D3DXVECTOR3 w;
    D3DXVec3Cross(&w, pAxisV, pAxisU);
    if (fabsf(pCenter->x - cx) <= (pBox->vMax.x - cx) + pAxisV->x * pExtents[1] + pAxisU->x * pExtents[2] +
                                      w.x * pExtents[0] &&
        fabsf(pCenter->y - cy) <= pAxisV->y * pExtents[1] + pExtents[0] * w.y + pAxisU->y * pExtents[2] +
                                      (pBox->vMax.y - cy) &&
        fabsf(pCenter->z - cz) <= pExtents[0] * w.z + pAxisU->z * pExtents[2] + pAxisV->z * pExtents[1] +
                                      (pBox->vMax.z - cz))
        return true;
    return false;
}
