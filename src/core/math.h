#pragma once

#include <cmath>
#include <cstring>

const float SBK_PI = 3.14159265358979323846f;
const float SBK_FLOAT_EPSILON = 1.192092896e-07f;
const float SBK_DEG_TO_RAD = SBK_PI / 180.f;
const float SBK_RAD_TO_DEG = 180.0f / SBK_PI;

// VEC2

struct vec2 {
    float x;
    float y;

    vec2() = default;
    vec2(float p_x, float p_y) {
        x = p_x;
        y = p_y;
    }

    static vec2 up() { return vec2(0.0f, 1.0f); }
    static vec2 right() { return vec2(1.0f, 0.0f); }
    static vec2 down() { return vec2(0.0f, -1.0f); }
    static vec2 left() { return vec2(-1.0f, 0.0f); }

    bool operator==(const vec2& other) const {
        if (std::abs(x - other.x) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(y - other.y) > SBK_FLOAT_EPSILON) {
            return false;
        }
        return true;
    }
    bool operator!=(const vec2& other) const { return !((*this) == other); }

    vec2 operator+(const vec2& other) const {
        return vec2(x + other.x, y + other.y);
    }
    vec2& operator+=(const vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2 operator-(const vec2& other) const {
        return vec2(x - other.x, y - other.y);
    }
    vec2& operator-=(const vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vec2 operator*(float scaler) const { return vec2(x * scaler, y * scaler); }
    vec2& operator*=(float scaler) {
        x *= scaler;
        y *= scaler;
        return *this;
    }

    vec2 operator/(float scaler) const { return vec2(x / scaler, y / scaler); }
    vec2& operator/=(float scaler) {
        x /= scaler;
        y /= scaler;
        return *this;
    }

    float length_squared() const { return (x * x) + (y * y); }
    float length() const { return std::sqrt(length_squared()); }

    void normalize() {
        const float _length = length();
        x /= _length;
        y /= _length;
    }
    vec2 normalized() const { return (*this) / length(); }

    static float distance(const vec2& a, const vec2& b) {
        return (a - b).length();
    }
};

// IVEC2

struct ivec2 {
    int x = 0;
    int y = 0;

    ivec2() = default;

    ivec2(int p_x, int p_y) {
        x = p_x;
        y = p_y;
    }

    static ivec2 from_vec2(vec2 v) {
        return ivec2((int)v.x, (int)v.y);
    }
    vec2 to_vec2() const {
        return vec2((float)x, (float)y);
    }

    bool operator==(const ivec2& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const ivec2& other) const { return !((*this) == other); }

    ivec2 operator+(const ivec2& other) const {
        return ivec2(x + other.x, y + other.y);
    }
    ivec2& operator+=(const ivec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    ivec2 operator-(const ivec2& other) const {
        return ivec2(x - other.x, y - other.y);
    }
    ivec2& operator-=(const ivec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    ivec2 operator*(float scaler) const {
        return ivec2(x * scaler, y * scaler);
    }
    ivec2& operator*=(float scaler) {
        x *= scaler;
        y *= scaler;
        return *this;
    }

    ivec2 operator/(float scaler) const {
        return ivec2(x / scaler, y / scaler);
    }
    ivec2& operator/=(float scaler) {
        x /= scaler;
        y /= scaler;
        return *this;
    }

    float length_squared() const {
        return (x * x) + (y * y);
    }
};

// VEC3

struct vec3 {
    float x;
    float y;
    float z;

    vec3() = default;
    vec3(float p_x, float p_y, float p_z) {
        x = p_x;
        y = p_y;
        z = p_z;
    }

    static vec3 up() { return vec3(0.0f, 1.0f, 0.0f); }
    static vec3 right() { return vec3(1.0f, 0.0f, 0.0f); }
    static vec3 down() { return vec3(0.0f, -1.0f, 0.0f); }
    static vec3 left() { return vec3(-1.0f, 0.0f, 0.0f); }
    static vec3 forward() { return vec3(0.0f, 0.0f, -1.0f); }
    static vec3 back() { return vec3(0.0f, 0.0f, 1.0f); }

    bool operator==(const vec3& other) const {
        if (std::abs(x - other.x) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(y - other.y) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(z - other.z) > SBK_FLOAT_EPSILON) {
            return false;
        }
        return true;
    }
    bool operator!=(const vec3& other) const { return !((*this) == other); }

    vec3 operator+(const vec3& other) const {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
    vec3& operator+=(const vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    vec3 operator-(const vec3& other) const {
        return vec3(x - other.x, y - other.y, z - other.z);
    }
    vec3& operator-=(const vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    vec3 operator*(float scaler) const {
        return vec3(x * scaler, y * scaler, z * scaler);
    }
    vec3& operator*=(float scaler) {
        x *= scaler;
        y *= scaler;
        z *= scaler;
        return *this;
    }

    vec3 operator/(float scaler) const {
        return vec3(x / scaler, y / scaler, z / scaler);
    }
    vec3& operator/=(float scaler) {
        x /= scaler;
        y /= scaler;
        z /= scaler;
        return *this;
    }

    float length_squared() const { return (x * x) + (y * y) + (z * z); }
    float length() const { return std::sqrt(length_squared()); }

    void normalize() {
        const float _length = length();
        x /= _length;
        y /= _length;
        z /= _length;
    }
    vec3 normalized() const { return (*this) / length(); }

    static float distance(const vec3& a, const vec3& b) {
        return (a - b).length();
    }

    static float dot(const vec3& a, const vec3& b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }
    static vec3 cross(const vec3& a, const vec3& b) {
        return vec3((a.y * b.z) - (a.z * b.y),
                    (a.z * b.x) - (a.x * b.z),
                    (a.x * b.y) - (a.y * b.x));
    }
};

// VEC4

struct vec4 {
    float x;
    float y;
    float z;
    float w;

    vec4() = default;
    vec4(float p_x, float p_y, float p_z, float p_w) {
        x = p_x;
        y = p_y;
        z = p_z;
        w = p_w;
    }
    vec4(const vec3& v3, float p_w) {
        x = v3.x;
        y = v3.y;
        z = v3.z;
        w = p_w;
    }

    vec3 to_vec3() const {
        return vec3(x, y, z);
    }

    bool operator==(const vec4& other) const {
        if (std::abs(x - other.x) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(y - other.y) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(z - other.z) > SBK_FLOAT_EPSILON) {
            return false;
        }
        if (std::abs(w - other.w) > SBK_FLOAT_EPSILON) {
            return false;
        }
        return true;
    }
    bool operator!=(const vec4& other) const {
        return !((*this) == other);
    }

    vec4 operator+(const vec4& other) const {
        return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    vec4& operator+=(const vec4& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    vec4 operator-(const vec4& other) const {
        return vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    vec4& operator-=(const vec4& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    vec4 operator*(float scaler) const {
        return vec4(x * scaler, y * scaler, z * scaler, w * scaler);
    }
    vec4& operator*=(float scaler) {
        x *= scaler;
        y *= scaler;
        z *= scaler;
        w *= scaler;
        return *this;
    }

    vec4 operator/(float scaler) const {
        return vec4(x / scaler, y / scaler, z / scaler, w / scaler);
    }
    vec4& operator/=(float scaler) {
        x /= scaler;
        y /= scaler;
        z /= scaler;
        w /= scaler;
        return *this;
    }

    float length_squared() const {
        return (x * x) + (y * y) + (z * z) + (w * w);
    }
    float length() const { return std::sqrt(length_squared()); }

    void normalize() {
        const float _length = length();
        x /= _length;
        y /= _length;
        z /= _length;
        w /= _length;
    }
    vec4 normalized() const { return (*this) / length(); }

    static float dot(const vec4& a, const vec4& b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }
};

// MAT4

struct mat4 {
    float data[16];

    mat4() {
        memset(data, 0, sizeof(data));
    }

    static mat4 identity() {
        mat4 m;
        m.data[0] = 1.0f;
        m.data[5] = 1.0f;
        m.data[10] = 1.0f;
        m.data[15] = 1.0f;
        return m;
    }

    mat4 operator*(const mat4& other) const {
        mat4 result = mat4::identity();
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                result.data[col + (row * 4)] =
                    data[0 + (row * 4)] * other.data[col + 0] +
                    data[1 + (row * 4)] * other.data[col + 4] +
                    data[2 + (row * 4)] * other.data[col + 8] +
                    data[3 + (row * 4)] * other.data[col + 12];
            }
        }

        return result;
    }

    vec4 operator*(const vec4& vec) const {
        return vec4(
            (data[0] * vec.x) + (data[1] * vec.y) + (data[2] * vec.z) + (data[3] * vec.w),
            (data[4] * vec.x) + (data[5] * vec.y) + (data[6] * vec.z) + (data[7] * vec.w),
            (data[8] * vec.x) + (data[9] * vec.y) + (data[10] * vec.z) + (data[11] * vec.w),
            (data[12] * vec.x) + (data[13] * vec.y) + (data[14] * vec.z) + (data[15] * vec.w));
    }

    static mat4 ortho(float left, float right, float top, float bottom, float near, float far) {
        mat4 result = mat4::identity();

        result.data[0] = 2.0f / (right - left);
        result.data[5] = -2.0f / (top - bottom);
        result.data[10] = -2.0f / (far - near);

        result.data[3] = - (right + left) / (right - left);
        result.data[7] = - (top + bottom) / (top - bottom);
        result.data[11] = - (far + near)  / (far - near);

        return result;
    }

    static mat4 perspective(float fov_radians, float aspect_ratio, float near, float far) {
        float half_tan_fov = tanf(fov_radians * 0.5f);

        mat4 result;
        result.data[0] = 1.0f / (aspect_ratio * half_tan_fov);
        result.data[5] = -1.0f / half_tan_fov; // -1.0 flips the Y axis
        result.data[10] = -((far + near) / (far - near));
        result.data[11] = -1.0f;
        result.data[14] = -((2.0f * far * near) / (far - near));

        return result;
    }

    static mat4 look_at(vec3 position, vec3 target, vec3 up) {
        mat4 result;

        vec3 z_axis = (target - position).normalized();
        vec3 x_axis = vec3::cross(z_axis, up).normalized();
        vec3 y_axis = vec3::cross(x_axis, z_axis);

        result.data[0] = x_axis.x;
        result.data[1] = y_axis.x;
        result.data[2] = -z_axis.x;
        result.data[3] = 0.0f;

        result.data[4] = x_axis.y;
        result.data[5] = y_axis.y;
        result.data[6] = -z_axis.y;
        result.data[7] = 0.0f;

        result.data[8] = x_axis.z;
        result.data[9] = y_axis.z;
        result.data[10] = -z_axis.z;
        result.data[11] = 0.0f;

        result.data[12] = -vec3::dot(x_axis, position);
        result.data[13] = -vec3::dot(y_axis, position);
        result.data[14] = vec3::dot(z_axis, position);
        result.data[15] = 1.0f;

        return result;
    }

    mat4 transposed() const {
        mat4 result = mat4::identity();

        result.data[0] = data[0];
        result.data[1] = data[4];
        result.data[2] = data[8];
        result.data[3] = data[12];
        result.data[4] = data[1];
        result.data[5] = data[5];
        result.data[6] = data[9];
        result.data[7] = data[13];
        result.data[8] = data[2];
        result.data[9] = data[6];
        result.data[10] = data[10];
        result.data[11] = data[14];
        result.data[12] = data[3];
        result.data[13] = data[7];
        result.data[14] = data[11];
        result.data[15] = data[15];

        return result;
    }

    mat4 inversed() const {
        float t0 = data[10] * data[15];
        float t1 = data[14] * data[11];
        float t2 = data[6] * data[15];
        float t3 = data[14] * data[7];
        float t4 = data[6] * data[11];
        float t5 = data[10] * data[7];
        float t6 = data[2] * data[15];
        float t7 = data[14] * data[3];
        float t8 = data[2] * data[11];
        float t9 = data[10] * data[3];
        float t10 = data[2] * data[7];
        float t11 = data[6] * data[3];
        float t12 = data[8] * data[13];
        float t13 = data[12] * data[9];
        float t14 = data[4] * data[13];
        float t15 = data[12] * data[5];
        float t16 = data[4] * data[9];
        float t17 = data[8] * data[5];
        float t18 = data[0] * data[13];
        float t19 = data[12] * data[1];
        float t20 = data[0] * data[9];
        float t21 = data[8] * data[1];
        float t22 = data[0] * data[5];
        float t23 = data[4] * data[1];

        mat4 result;

        result.data[0] = (t0 * data[5] + t3 * data[9] + t4 * data[13]) - (t1 * data[5] + t2 * data[9] + t5 * data[13]);
        result.data[1] = (t1 * data[1] + t6 * data[9] + t9 * data[13]) - (t0 * data[1] + t7 * data[9] + t8 * data[13]);
        result.data[2] = (t2 * data[1] + t7 * data[5] + t10 * data[13]) - (t3 * data[1] + t6 * data[5] + t11 * data[13]); result.data[3] = (t5 * data[1] + t8 * data[5] + t11 * data[9]) - (t4 * data[1] + t9 * data[5] + t10 * data[9]);

        float d = 1.0f / (data[0] * result.data[0] + data[4] * result.data[1] + data[8] * result.data[2] + data[12] * result.data[3]);

        result.data[0] = d * result.data[0];
        result.data[1] = d * result.data[1];
        result.data[2] = d * result.data[2];
        result.data[3] = d * result.data[3];
        result.data[4] = d * ((t1 * data[4] + t2 * data[8] + t5 * data[12]) - (t0 * data[4] + t3 * data[8] + t4 * data[12]));
        result.data[5] = d * ((t0 * data[0] + t7 * data[8] + t8 * data[12]) - (t1 * data[0] + t6 * data[8] + t9 * data[12]));
        result.data[6] = d * ((t3 * data[0] + t6 * data[4] + t11 * data[12]) - (t2 * data[0] + t7 * data[4] + t10 * data[12]));
        result.data[7] = d * ((t4 * data[0] + t9 * data[4] + t10 * data[8]) - (t5 * data[0] + t8 * data[4] + t11 * data[8]));
        result.data[8] = d * ((t12 * data[7] + t15 * data[11] + t16 * data[15]) - (t13 * data[7] + t14 * data[11] + t17 * data[15]));
        result.data[9] = d * ((t13 * data[3] + t18 * data[11] + t21 * data[15]) - (t12 * data[3] + t19 * data[11] + t20 * data[15]));
        result.data[10] = d * ((t14 * data[3] + t19 * data[7] + t22 * data[15]) - (t15 * data[3] + t18 * data[7] + t23 * data[15]));
        result.data[11] = d * ((t17 * data[3] + t20 * data[7] + t23 * data[11]) - (t16 * data[3] + t21 * data[7] + t22 * data[11]));
        result.data[12] = d * ((t14 * data[10] + t17 * data[14] + t13 * data[6]) - (t16 * data[14] + t12 * data[6] + t15 * data[10]));
        result.data[13] = d * ((t20 * data[14] + t12 * data[2] + t19 * data[10]) - (t18 * data[10] + t21 * data[14] + t13 * data[2]));
        result.data[14] = d * ((t18 * data[6] + t23 * data[14] + t15 * data[2]) - (t22 * data[14] + t14 * data[2] + t19 * data[6]));
        result.data[15] = d * ((t22 * data[10] + t16 * data[2] + t21 * data[6]) - (t20 * data[6] + t23 * data[10] + t17 * data[2]));

        return result;
    }

    static mat4 translation(vec3 position) {
        mat4 result = mat4::identity();
        result.data[12] = position.x;
        result.data[13] = position.y;
        result.data[14] = position.z;
        return result;
    }

    static mat4 scale(vec3 scale) {
        mat4 result = mat4::identity();
        result.data[0] = scale.x;
        result.data[5] = scale.y;
        result.data[10] = scale.z;
        return result;
    }

    static mat4 euler_x(float angle_radians) {
        mat4 result = mat4::identity();
        float cos_angle = cos(angle_radians);
        float sin_angle = sin(angle_radians);

        result.data[5] = cos_angle;
        result.data[6] = sin_angle;
        result.data[9] = -sin_angle;
        result.data[10] = cos_angle;

        return result;
    }

    static mat4 euler_y(float angle_radians) {
        mat4 result = mat4::identity();
        float cos_angle = cos(angle_radians);
        float sin_angle = sin(angle_radians);

        result.data[0] = cos_angle;
        result.data[2] = -sin_angle;
        result.data[8] = sin_angle;
        result.data[10] = cos_angle;

        return result;
    }

    static mat4 euler_z(float angle_radians) {
        mat4 result = mat4::identity();
        float cos_angle = cos(angle_radians);
        float sin_angle = sin(angle_radians);

        result.data[0] = cos_angle;
        result.data[1] = sin_angle;
        result.data[4] = -sin_angle;
        result.data[5] = cos_angle;

        return result;
    }

    static mat4 euler_xyz(float x_radians, float y_radians, float z_radians) {
        mat4 result_x = mat4::euler_x(x_radians);
        mat4 result_y = mat4::euler_y(y_radians);
        mat4 result_z = mat4::euler_z(z_radians);

        return (result_x * result_y) * result_z;
    }

    static mat4 euler_vec3(vec3 radians) {
        mat4 result_x = mat4::euler_x(radians.x);
        mat4 result_y = mat4::euler_y(radians.y);
        mat4 result_z = mat4::euler_z(radians.z);

        return (result_x * result_y) * result_z;
    }

    vec3 forward() const {
        return vec3(-data[2], -data[6], -data[10]).normalized();
    }

    vec3 backward() const {
        return vec3(data[2], data[6], data[10]).normalized();
    }

    vec3 up() const {
        return vec3(data[1], data[5], data[9]).normalized();
    }

    vec3 down() const {
        return vec3(-data[1], -data[5], -data[9]).normalized();
    }

    vec3 left() const {
        return vec3(-data[0], -data[4], -data[8]).normalized();
    }

    vec3 right() const {
        return vec3(data[0], data[4], data[8]).normalized();
    }
};

// QUAT

struct quat {
    float x;
    float y;
    float z;
    float w;

    quat() = default;

    quat(float p_x, float p_y, float p_z, float p_w) {
        x = p_x;
        y = p_y;
        z = p_z;
        w = p_w;
    }

    static quat identity() { return quat(0.0f, 0.0f, 0.0f, 1.0f); }

    float normal() const {
        return std::sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    void normalize() {
        float _normal = normal();
        x /= _normal;
        y /= _normal;
        z /= _normal;
        w /= _normal;
    }

    quat normalized() const {
        float _normal = normal();
        return quat(x / _normal, y / _normal, z / _normal, w / _normal);
    }

    quat conjugate() const { return quat(-x, -y, -z, w); }

    quat inverse() const { return conjugate().normalized(); }

    quat operator*(const quat& other) const {
        quat result;

        result.x = (x * other.w) + (y * other.z) - (z * other.y) + (w * other.x);
        result.y = (-x * other.z) + (y * other.w) + (z * other.x) + (w * other.y);
        result.z = (x * other.y) - (y * other.x) + (z * other.w) + (w * other.z);
        result.w = (-x * other.x) - (y * other.y) - (z * other.z) + (w * other.w);

        return result;
    }

    static float dot(const quat& a, const quat& b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    mat4 to_mat4() const {
        mat4 result = mat4::identity();
        quat n = normalized();

        result.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
        result.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
        result.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

        result.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
        result.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
        result.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

        result.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
        result.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
        result.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

        return result;
    }

    mat4 to_rotation_matrix(vec3 center) const {
        mat4 result;

        result.data[0] = (x * x) - (y * y) - (z * z) + (w * w);
        result.data[1] = 2.0f * ((x * y) + (z * w));
        result.data[2] = 2.0f * ((x * z) - (y * w));
        result.data[3] = center.x - center.x * result.data[0] - center.y * result.data[1] - center.z * result.data[2];

        result.data[4] = 2.0f * ((x * y) - (z * w));
        result.data[5] = -(x * x) + (y * y) - (z * z) + (w * w);
        result.data[6] = 2.0f * ((y * z) + (x * w));
        result.data[7] = center.y - center.x * result.data[4] -
                         center.y * result.data[5] - center.z * result.data[6];

        result.data[8] = 2.0f * ((x * z) + (y * w));
        result.data[9] = 2.0f * ((y * z) - (x * w));
        result.data[10] = -(x * x) - (y * y) + (z * z) + (w * w);
        result.data[11] = center.z - center.x * result.data[8] -
                          center.y * result.data[9] -
                          center.z * result.data[10];

        result.data[12] = 0.0f;
        result.data[13] = 0.0f;
        result.data[14] = 0.0f;
        result.data[15] = 1.0f;

        return result;
    }

    static quat from_axis_angle(vec3 axis, float angle, bool normalize) {
        float sin_angle = sinf(0.5f * angle);
        float cos_angle = cosf(0.5f * angle);

        quat result = quat(sin_angle * axis.x, sin_angle * axis.y, sin_angle * axis.z, cos_angle);
        if (normalize) {
            result.normalize();
        }

        return result;
    }

    quat slerp(const quat& dest, float percent) {
        quat from = normalized();
        quat to = dest.normalized();
        float _dot = quat::dot(from, to);

        if (_dot < 0.0f) {
            to.x = -to.x;
            to.y = -to.y;
            to.z = -to.z;
            to.w = -to.w;
            _dot = -_dot;
        }

        const float DOT_THRESHOLD = 0.9995f;
        if (_dot > DOT_THRESHOLD) {
            // If inputs are too close to safely acos, lerp and normalize the
            // result
            return quat(from.x + ((to.x - from.x) * percent),
                        from.y + ((to.y - from.y) * percent),
                        from.z + ((to.z - from.z) * percent),
                        from.w + ((to.w - from.w) * percent))
                .normalized();
        }

        float theta_0 = acos(_dot);
        float theta = theta_0 * percent;
        float sin_theta = sin(theta);
        float sin_theta_0 = sin(theta_0);
        float s0 = cos(theta) - (_dot * (sin_theta / sin_theta_0));
        float s1 = sin_theta / sin_theta_0;

        return quat((from.x * s0) + (to.x * s1), (from.y * s0) + (to.y * s1),
                    (from.z * s0) + (to.z * s1), (from.w * s0) + (to.w * s1));
    }
};

// VERTEX 3D

struct Vertex3d {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};
