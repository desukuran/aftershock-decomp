// CGame's geometry helpers.
#include "Game.h"

// 0x0001B8D0
// Whether p is in the box center +- (halfWidth, halfHeight); z is ignored.
bool CGame::PointInBox(D3DXVECTOR3 p, D3DXVECTOR3 center, float halfWidth, float halfHeight)
{
    if (center.x - halfWidth > p.x || p.x > halfWidth + center.x)
        return false;
    if (center.y - halfHeight > p.y || p.y > halfHeight + center.y)
        return false;
    return true;
}

// 0x0001B930
// Where the ray (ox, oy) + t (dx, dy) crosses the circle around (cx, cy):
// both t into t[0] and t[1], returning the larger, or -1 if it misses. The
// direction needn't be a unit vector; the t are in its length's units.
float CGame::RayCircle(float ox, float oy, float dx, float dy, float cx, float cy, float r, float* t)
{
    float fx = ox - cx;
    float fy = oy - cy;
    float b = fx * dx + dy * fy;
    float dist = sqrtf(fx * fx + fy * fy);
    float len = sqrtf(dx * dx + dy * dy);
    float disc = b * b - (dist * dist - r * r) * len * len;
    if (disc < 0.0f)
        return -1.0f;

    disc = sqrtf(disc);
    t[0] = ((disc - dy * fy) - fx * dx) * (1.0f / len);
    t[1] = ((0.0f - b) - disc) * (1.0f / len);
    return t[0] > t[1] ? t[0] : t[1];
}

// Whether any corner of the box (x +- dx, y +- dy) is in the box around b
// (the original repeats these four tests inline everywhere).
bool CornerInBox(float x, float y, float z, float dx, float dy, const D3DXVECTOR3& b, float w, float h)
{
    return g_Game.PointInBox(D3DXVECTOR3(x + dx, y + dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x - dx, y + dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x + dx, y - dy, z), b, w, h) ||
           g_Game.PointInBox(D3DXVECTOR3(x - dx, y - dy, z), b, w, h);
}
