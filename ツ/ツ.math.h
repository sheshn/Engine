#pragma once

#include "ツ.types.h"

// TODO: Remove use of C runtime library!
#include <math.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

internal f32 radians(f32 degrees)
{
    return (f32)(degrees * 0.01745329251994329576923690768489);
}

internal f32 degrees(f32 radians)
{
    return (f32)(radians * 57.295779513082320876798154814105);
}

internal v2 operator+(v2 left, v2 right)
{
    return {left.x + right.x, left.y + right.y};
}

internal v3 operator+(v3 left, v3 right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

internal v4 operator+(v4 left, v4 right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w};
}

internal v2 operator-(v2 left, v2 right)
{
    return {left.x - right.x, left.y - right.y};
}

internal v3 operator-(v3 left, v3 right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

internal v4 operator-(v4 left, v4 right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w};
}

internal v2 operator*(v2 left, v2 right)
{
    return {left.x * right.x, left.y * right.y};
}

internal v3 operator*(v3 left, v3 right)
{
    return {left.x * right.x, left.y * right.y, left.z * right.z};
}

internal v4 operator*(v4 left, v4 right)
{
    return {left.x * right.x, left.y * right.y, left.z * right.z, left.w * right.w};
}

internal v2 operator/(v2 left, v2 right)
{
    return {left.x / right.x, left.y / right.y};
}

internal v3 operator/(v3 left, v3 right)
{
    return {left.x / right.x, left.y / right.y, left.z / right.z};
}

internal v4 operator/(v4 left, v4 right)
{
    return {left.x / right.x, left.y / right.y, left.z / right.z, left.w / right.w};
}

internal v2 operator*(v2 left, f32 right)
{
    return {left.x * right, left.y * right};
}

internal v3 operator*(v3 left, f32 right)
{
    return {left.x * right, left.y * right, left.z * right};
}

internal v4 operator*(v4 left, f32 right)
{
    return {left.x * right, left.y * right, left.z * right, left.w * right};
}

internal v2 operator/(v2 left, f32 right)
{
    return {left.x / right, left.y / right};
}

internal v3 operator/(v3 left, f32 right)
{
    return {left.x / right, left.y / right, left.z / right};
}

internal v4 operator/(v4 left, f32 right)
{
    return {left.x / right, left.y / right, left.z / right, left.w / right};
}

internal v2 operator*(f32 left, v2 right)
{
    return right * left;
}

internal v3 operator*(f32 left, v3 right)
{
    return right * left;
}

internal v4 operator*(f32 left, v4 right)
{
    return right * left;
}

internal v2 operator/(f32 left, v2 right)
{
    return right / left;
}

internal v3 operator/(f32 left, v3 right)
{
    return right / left;
}

internal v4 operator/(f32 left, v4 right)
{
    return right / left;
}

internal v2& operator+=(v2& left, v2 right)
{
    left = left + right;
    return left;
}

internal v3& operator+=(v3& left, v3 right)
{
    left = left + right;
    return left;
}

internal v4& operator+=(v4& left, v4 right)
{
    left = left + right;
    return left;
}

internal v2& operator-=(v2& left, v2 right)
{
    left = left - right;
    return left;
}

internal v3& operator-=(v3& left, v3 right)
{
    left = left - right;
    return left;
}

internal v4& operator-=(v4& left, v4 right)
{
    left = left - right;
    return left;
}

internal v2& operator*=(v2& left, v2 right)
{
    left = left * right;
    return left;
}

internal v3& operator*=(v3& left, v3 right)
{
    left = left * right;
    return left;
}

internal v4& operator*=(v4& left, v4 right)
{
    left = left * right;
    return left;
}

internal v2& operator/=(v2& left, v2 right)
{
    left = left / right;
    return left;
}

internal v3& operator/=(v3& left, v3 right)
{
    left = left / right;
    return left;
}

internal v4& operator/=(v4& left, v4 right)
{
    left = left / right;
    return left;
}

internal v2& operator*=(v2& left, f32 right)
{
    left = left * right;
    return left;
}

internal v3& operator*=(v3& left, f32 right)
{
    left = left * right;
    return left;
}

internal v4& operator*=(v4& left, f32 right)
{
    left = left * right;
    return left;
}

internal v2& operator/=(v2& left, f32 right)
{
    left = left / right;
    return left;
}

internal v3& operator/=(v3& left, f32 right)
{
    left = left / right;
    return left;
}

internal v4& operator/=(v4& left, f32 right)
{
    left = left / right;
    return left;
}

internal m4x4 operator*(m4x4 left, m4x4 right)
{
    m4x4 result;
    for (u64 j = 0; j < 4; ++j)
    {
        for (u64 k = 0; k < 4; ++k)
        {
            for (u64 i = 0; i < 4; ++i)
            {
                result.columns[j].data[i] += left.columns[k].data[i] * right.columns[j].data[k];
            }
        }
    }
    return result;
}

internal v4 operator*(m4x4 left, v4 right)
{
    v4 result;
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            result.data[j] += left.columns[i].data[j] * right.data[i];
        }
    }
    return result;
}

internal m4x4& operator*=(m4x4& left, m4x4 right)
{
    left = left * right;
    return left;
}

internal m4x4 identity()
{
    return {1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
}

internal m4x4 perspective_infinite(f32 fov_y, f32 aspect, f32 z_near)
{
    f32 f = 1.0f / tanf(fov_y / 2);
    return {f / aspect, 0, 0,      0,
            0,         -f, 0,      0,
            0,          0, 0,      1,
            0,          0, z_near, 0};
}

internal m4x4 translate(m4x4 m, v3 translation)
{
    m.columns[3] = translation.data[0] * m.columns[0] + translation.data[1] * m.columns[1] + translation.data[2] * m.columns[2] + m.columns[3];
    return m;
}