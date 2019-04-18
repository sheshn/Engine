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
internal f32 sin(f32 angle)
{
    // NOTE: This approximation is only valid from -π to π
    angle = angle - (u32)(angle / PI) * PI;

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

internal m4x4 operator*(f32 left, m4x4 right)
{
    for (u64 i = 0; i < 16; ++i)
    {
        right.data[i] *= left;
    }
    return right;
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

internal m4x4 scale(m4x4 m, f32 scale)
{
    m4x4 result;
    result.columns[0] = m.columns[0] * scale;
    result.columns[1] = m.columns[1] * scale;
    result.columns[2] = m.columns[2] * scale;
    result.columns[3] = m.columns[3];
    return result;
}

internal m4x4 inverse(m4x4 m)
{
    f32 coef_00 = m.columns[2].data[2] * m.columns[3].data[3] - m.columns[3].data[2] * m.columns[2].data[3];
    f32 coef_02 = m.columns[1].data[2] * m.columns[3].data[3] - m.columns[3].data[2] * m.columns[1].data[3];
    f32 coef_03 = m.columns[1].data[2] * m.columns[2].data[3] - m.columns[2].data[2] * m.columns[1].data[3];

    f32 coef_04 = m.columns[2].data[1] * m.columns[3].data[3] - m.columns[3].data[1] * m.columns[2].data[3];
    f32 coef_06 = m.columns[1].data[1] * m.columns[3].data[3] - m.columns[3].data[1] * m.columns[1].data[3];
    f32 coef_07 = m.columns[1].data[1] * m.columns[2].data[3] - m.columns[2].data[1] * m.columns[1].data[3];

    f32 coef_08 = m.columns[2].data[1] * m.columns[3].data[2] - m.columns[3].data[1] * m.columns[2].data[2];
    f32 coef_10 = m.columns[1].data[1] * m.columns[3].data[2] - m.columns[3].data[1] * m.columns[1].data[2];
    f32 coef_11 = m.columns[1].data[1] * m.columns[2].data[2] - m.columns[2].data[1] * m.columns[1].data[2];

    f32 coef_12 = m.columns[2].data[0] * m.columns[3].data[3] - m.columns[3].data[0] * m.columns[2].data[3];
    f32 coef_14 = m.columns[1].data[0] * m.columns[3].data[3] - m.columns[3].data[0] * m.columns[1].data[3];
    f32 coef_15 = m.columns[1].data[0] * m.columns[2].data[3] - m.columns[2].data[0] * m.columns[1].data[3];

    f32 coef_16 = m.columns[2].data[0] * m.columns[3].data[2] - m.columns[3].data[0] * m.columns[2].data[2];
    f32 coef_18 = m.columns[1].data[0] * m.columns[3].data[2] - m.columns[3].data[0] * m.columns[1].data[2];
    f32 coef_19 = m.columns[1].data[0] * m.columns[2].data[2] - m.columns[2].data[0] * m.columns[1].data[2];

    f32 coef_20 = m.columns[2].data[0] * m.columns[3].data[1] - m.columns[3].data[0] * m.columns[2].data[1];
    f32 coef_22 = m.columns[1].data[0] * m.columns[3].data[1] - m.columns[3].data[0] * m.columns[1].data[1];
    f32 coef_23 = m.columns[1].data[0] * m.columns[2].data[1] - m.columns[2].data[0] * m.columns[1].data[1];

    v4 fac_0 = {coef_00, coef_00, coef_02, coef_03};
    v4 fac_1 = {coef_04, coef_04, coef_06, coef_07};
    v4 fac_2 = {coef_08, coef_08, coef_10, coef_11};
    v4 fac_3 = {coef_12, coef_12, coef_14, coef_15};
    v4 fac_4 = {coef_16, coef_16, coef_18, coef_19};
    v4 fac_5 = {coef_20, coef_20, coef_22, coef_23};

    v4 vec_0 = {m.columns[1].data[0], m.columns[0].data[0], m.columns[0].data[0], m.columns[0].data[0]};
    v4 vec_1 = {m.columns[1].data[1], m.columns[0].data[1], m.columns[0].data[1], m.columns[0].data[1]};
    v4 vec_2 = {m.columns[1].data[2], m.columns[0].data[2], m.columns[0].data[2], m.columns[0].data[2]};
    v4 vec_3 = {m.columns[1].data[3], m.columns[0].data[3], m.columns[0].data[3], m.columns[0].data[3]};

    v4 inv_0 = {vec_1 * fac_0 - vec_2 * fac_1 + vec_3 * fac_2};
    v4 inv_1 = {vec_0 * fac_0 - vec_2 * fac_3 + vec_3 * fac_4};
    v4 inv_2 = {vec_0 * fac_1 - vec_1 * fac_3 + vec_3 * fac_5};
    v4 inv_3 = {vec_0 * fac_2 - vec_1 * fac_4 + vec_2 * fac_5};

    v4 sign_a = {+1, -1, +1, -1};
    v4 sign_b = {-1, +1, -1, +1};

    m4x4 inverse;
    inverse.columns[0] = inv_0 * sign_a;
    inverse.columns[1] = inv_1 * sign_b;
    inverse.columns[2] = inv_2 * sign_a;
    inverse.columns[3] = inv_3 * sign_b;

    v4 row_0 = {inverse.columns[0].data[0], inverse.columns[1].data[0], inverse.columns[2].data[0], inverse.columns[3].data[0]};
    v4 dot_0 = m.columns[0] * row_0;
    f32 dot_1 = (dot_0.x + dot_0.y) + (dot_0.z + dot_0.w);
    return (1 / dot_1) * inverse;
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

internal f32 lerp(f32 a, f32 b, f32 t)
{
    return a + t * (b - a);
}

internal v2 lerp(v2 a, v2 b, f32 t)
{
    return a + t * (b - a);
}

internal v3 lerp(v3 a, v3 b, f32 t)
{
    return a + t * (b - a);
}

internal v4 lerp(v4 a, v4 b, f32 t)
{
    return a + t * (b - a);
}