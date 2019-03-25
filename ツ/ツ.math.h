#pragma once

#include <xmmintrin.h>
#include "ツ.types.h"

#define PI 3.14159265358979323846

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

internal f32 sqrt(f32 x)
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&x)));
}

// TODO: Choose better approximations for sin, cos and tan?
// NOTE: This approximation is only valid from -π to π
internal f32 sin(f32 angle)
{
    f32 a2 = angle * angle;
    return angle + (angle * a2) * (-0.16612511580269618f + a2 * (8.0394356072977748e-3f + a2 * -1.49414020045938777495e-4f));
}

internal f32 cos(f32 angle)
{
    return sin(angle + PI / 2);
}

internal f32 tan(f32 angle)
{
    return sin(angle) / cos(angle);
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

internal v3 operator*(v3 left, quat right)
{
    return {((left.x * ((1 - (right.y * (right.y + right.y))) - (right.z * (right.z + right.z)))) + (left.y * ((right.x * (right.y + right.y)) - (right.w * (right.z + right.z))))) + (left.z * ((right.x * (right.z + right.z)) + (right.w * (right.y + right.y)))),
            ((left.x * ((right.x * (right.y + right.y)) + (right.w * (right.z + right.z)))) + (left.y * ((1 - (right.x * (right.x + right.x))) - (right.z * (right.z + right.z))))) + (left.z * ((right.y * (right.z + right.z)) - (right.w * (right.x + right.x)))),
            ((left.x * ((right.x * (right.z + right.z)) - (right.w * (right.y + right.y)))) + (left.y * ((right.y * (right.z + right.z)) + (right.w * (right.x + right.x))))) + (left.z * ((1 - (right.x * (right.x + right.x))) - (right.y * (right.y + right.y))))};
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

internal v3& operator*=(v3& left, quat right)
{
    left = left * right;
    return left;
}

internal m4x4 operator*(m4x4 left, m4x4 right)
{
    m4x4 result = {};
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
    v4 result = {};
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

internal quat operator*(quat left, quat right)
{
    return {left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
            left.w * right.y + left.y * right.w + left.z * right.x - left.x * right.z,
            left.w * right.z + left.z * right.w + left.x * right.y - left.y * right.x,
            left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z};
}

internal quat& operator*=(quat& left, quat right)
{
    left = left * right;
    return left;
}

internal f32 dot(v2 left, v2 right)
{
    return left.x * right.x + left.y * right.y;
}

internal f32 dot(v3 left, v3 right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

internal f32 dot(v4 left, v4 right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;
}

internal f32 length(v2 v)
{
    return sqrt(dot(v, v));
}

internal f32 length(v3 v)
{
    return sqrt(dot(v, v));
}

internal f32 length(v4 v)
{
    return sqrt(dot(v, v));
}

internal v2 normalize(v2 v)
{
    return v / length(v);
}

internal v3 normalize(v3 v)
{
    return v / length(v);
}

internal v4 normalize(v4 v)
{
    return v / length(v);
}

internal v3 cross(v3 left, v3 right)
{
    return {left.y * right.z - right.y * left.z, left.z * right.x - right.z * left.x, left.x * right.y - right.x * left.y};
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
    f32 f = 1 / tan(fov_y / 2);
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

internal quat axis_angle(v3 axis, f32 angle)
{
    f32 a = angle * 0.5f;
    f32 s = sin(a);
    return {axis.x * s, axis.y * s, axis.z * s, cos(a)};
}

internal quat normalize(quat q)
{
    f32 length = sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return {q.x / length, q.y / length, q.z / length, q.w / length};
}

internal quat conjugate(quat q)
{
    return {-q.x, -q.y, -q.z, q.w};
}

internal m4x4 quat_to_m4x4(quat q)
{
    m4x4 result = {};
    f32 qxx = q.x * q.x;
    f32 qyy = q.y * q.y;
    f32 qzz = q.z * q.z;
    f32 qxz = q.x * q.z;
    f32 qxy = q.x * q.y;
    f32 qyz = q.y * q.z;
    f32 qwx = q.w * q.x;
    f32 qwy = q.w * q.y;
    f32 qwz = q.w * q.z;

    result.columns[0].data[0] = 1 - 2 * (qyy + qzz);
    result.columns[0].data[1] = 2 * (qxy + qwz);
    result.columns[0].data[2] = 2 * (qxz - qwy);

    result.columns[1].data[0] = 2 * (qxy - qwz);
    result.columns[1].data[1] = 1 - 2 * (qxx + qzz);
    result.columns[1].data[2] = 2 * (qyz + qwx);

    result.columns[2].data[0] = 2 * (qxz + qwy);
    result.columns[2].data[1] = 2 * (qyz - qwx);
    result.columns[2].data[2] = 1 - 2 * (qxx + qyy);

    result.columns[3].data[3] = 1;
    return result;
}