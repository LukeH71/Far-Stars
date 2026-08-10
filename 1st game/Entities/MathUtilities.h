
#include <simd/simd.h>
#include <stdlib.h>
#include <string>
#include <algorithm>

matrix_float4x4 matrix_make_rows(
                                   float m00, float m10, float m20, float m30,
                                   float m01, float m11, float m21, float m31,
                                   float m02, float m12, float m22, float m32,
                                   float m03, float m13, float m23, float m33) {
    return (matrix_float4x4){ {
        { m00, m01, m02, m03 },     // each line here provides column data
        { m10, m11, m12, m13 },
        { m20, m21, m22, m23 },
        { m30, m31, m32, m33 } } };
}

static vector_float3 vector_make(float x, float y, float z) {
    return (vector_float3){ x, y, z };
}

matrix_float4x4 matrix_perspective_left_hand(float fovyRadians, float aspect, float nearZ, float farZ) {
    float ys = 1 / tanf(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (farZ - nearZ);
    return matrix_make_rows(xs,  0,  0,           0,
                             0, ys,  0,           0,
                             0,  0, zs, -nearZ * zs,
                             0,  0,  1,           0 );
}

matrix_float4x4 matrix_look_at_left_hand(vector_float3 eye,
                                                            vector_float3 target,
                                                            vector_float3 up) {
    vector_float3 z = vector_normalize(target - eye);
    vector_float3 x = vector_normalize(vector_cross(up, z));
    vector_float3 y = vector_cross(z, x);
    vector_float3 t = vector_make(-vector_dot(x, eye), -vector_dot(y, eye), -vector_dot(z, eye));

    return matrix_make_rows(x.x, x.y, x.z, t.x,
                            y.x, y.y, y.z, t.y,
                            z.x, z.y, z.z, t.z,
                              0,   0,   0,   1 );
}

float radians_from_degrees(float degrees) {
    return (degrees / 180) * M_PI;
}


matrix_float4x4 matrix4x4_translation(float tx, float ty, float tz) {
    return matrix_make_rows(1, 0, 0, tx,
                            0, 1, 0, ty,
                            0, 0, 1, tz,
                            0, 0, 0,  1 );
}

matrix_float4x4 matrix4x4_translation(vector_float3 t) {
    return matrix_make_rows(1, 0, 0, t.x,
                            0, 1, 0, t.y,
                            0, 0, 1, t.z,
                            0, 0, 0,   1 );
}


matrix_float4x4 matrix4x4_scale(float sx, float sy, float sz) {
    return matrix_make_rows(sx,  0,  0, 0,
                             0, sy,  0, 0,
                             0,  0, sz, 0,
                             0,  0,  0, 1 );
}

matrix_float4x4 matrix4x4_scale(vector_float3 s) {
    return matrix_make_rows(s.x,   0,   0, 0,
                              0, s.y,   0, 0,
                              0,   0, s.z, 0,
                              0,   0,   0, 1 );
}


matrix_float4x4 matrix4x4_rotation(float radians, vector_float3 axis) {
    axis = vector_normalize(axis);
    float ct = cosf(radians);
    float st = sinf(radians);
    float ci = 1 - ct;
    float x = axis.x, y = axis.y, z = axis.z;
    return matrix_make_rows(
                        ct + x * x * ci, x * y * ci - z * st, x * z * ci + y * st, 0,
                    y * x * ci + z * st,     ct + y * y * ci, y * z * ci - x * st, 0,
                    z * x * ci - y * st, z * y * ci + x * st,     ct + z * z * ci, 0,
                                      0,                   0,                   0, 1);
}

matrix_float4x4 matrix4x4_rotation(float radians, float x, float y, float z) {
    return matrix4x4_rotation(radians, vector_make(x, y, z));
}


                             
std::string trimTrailingZeros(const std::string& str) {
    auto it = std::find_if(str.rbegin(), str.rend()-1, [](char ch) { return ch != '0' && ch != '.'; });
    return std::string(str.begin(), it.base());
}
