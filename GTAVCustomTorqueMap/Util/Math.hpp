#pragma once
#include <inc/types.h>
#include <cmath>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

// Vector3 but double and no padding. 
struct V3D {
    V3D(Vector3 v3f)
        : x(v3f.x), y(v3f.y), z(v3f.z) { }
    V3D(double x, double y, double z)
        : x(x), y(y), z(z) { }
    V3D() = default;
    Vector3 to_v3f() {
        Vector3 v{
            static_cast<float>(x), 0,
            static_cast<float>(y), 0,
            static_cast<float>(z), 0
        };
        return v;
    }
    double x;
    double y;
    double z;
};

template <typename T>
constexpr T sgn(T val) {
    return static_cast<T>((T{} < val) - (val < T{}));
}

template<typename T, typename A>
T avg(std::vector<T, A> const& vec) {
    T average{};
    for (auto elem : vec)
        average += elem;
    return average / static_cast<T>(vec.size());
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
constexpr T rad2deg(T rad) {
    return static_cast<T>(static_cast<double>(rad) * (180.0 / M_PI));
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
constexpr T deg2rad(T deg) {
    return static_cast<T>(static_cast<double>(deg) * M_PI / 180.0);
}

template <typename T>
T map(T x, T in_min, T in_max, T out_min, T out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

namespace Math {
    template <typename T>
    bool Near(T a, T b, T deviation) {
        return (a > b - deviation && a < b + deviation);
    }
}

template <typename T>
T lerp(T a, T b, T f) {
    return a + f * (b - a);
}

template <typename Vector3T>
auto Length(Vector3T vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

template <typename Vector3T>
auto Distance(Vector3T vec1, Vector3T vec2) {
    Vector3T distance = vec1 - vec2;
    return Length(distance);
}

template <typename Vector3T>
auto Dot(Vector3T a, Vector3T b)  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename Vector3T>
Vector3T Cross(Vector3T left, Vector3T right) {
    Vector3T result{};
    result.x = left.y * right.z - left.z * right.y;
    result.y = left.z * right.x - left.x * right.z;
    result.z = left.x * right.y - left.y * right.x;
    return result;
}

template <typename Vector3T>
Vector3T operator + (Vector3T left, Vector3T right) {
    Vector3T v{};
    v.x = left.x + right.x;
    v.y = left.y + right.y;
    v.z = left.z + right.z;
    return v;
}

template <typename Vector3T>
Vector3T operator - (Vector3T left, Vector3T right) {
    Vector3T v{};
    v.x = left.x - right.x;
    v.y = left.y - right.y;
    v.z = left.z - right.z;
    return v;
}

template <typename Vector3T>
Vector3T operator * (Vector3T value, decltype(value.x) scale) {
    Vector3T v{};
    v.x = value.x * scale;
    v.y = value.y * scale;
    v.z = value.z * scale;
    return v;
}

template <typename Vector3T>
Vector3T operator * (decltype(std::declval<Vector3T>().x) scale, Vector3T vec) {
    return vec * scale;
}

template <typename Vector3T>
Vector3T Normalize(Vector3T vec) {
    Vector3T vector = {};
    auto length = Length(vec);

    if (length != static_cast<decltype(vec.x)>(0.0)) {
        vector.x = vec.x / length;
        vector.y = vec.y / length;
        vector.z = vec.z / length;
    }

    return vector;
}

template <typename Vector3T>
Vector3T GetOffsetInWorldCoords(Vector3T position, Vector3T rotation, Vector3T forward, Vector3T offset) {
    float num1 = cosf(rotation.y);
    float x = num1 * cosf(-rotation.z);
    float y = num1 * sinf(rotation.z);
    float z = sinf(-rotation.y);
    Vector3 right = { x, 0, y, 0, z, 0 };
    Vector3 up = Cross(right, forward);
    return position + (right * offset.x) + (forward * offset.y) + (up * offset.z);
}

// degrees
template <typename T>
T GetAngleBetween(T h1, T h2, T separation) {
    T diff = abs(h1 - h2);
    if (diff < separation)
        return (separation - diff) / separation;
    if (abs(diff - static_cast<T>(360.0)) < separation)
        return (separation - abs(diff - static_cast<T>(360.0))) / separation;
    return separation;
}

template <typename Vector3T>
auto GetAngleBetween(Vector3T a, Vector3T b) {
    Vector3T normal{};
    normal.z = static_cast < decltype(a.x) >(1.0);
    auto angle = acos(Dot(a, b) / (Length(a) * Length(b)));
    if (Dot(normal, Cross(a, b)) < 0.0f)
        angle = -angle;
    return angle;
}

template <typename Vector3T>
Vector3T RotationToDirection(Vector3T rot) {
    float z = rot.z;
    float num = z * 0.0174532924f;

    float x = rot.x;
    float num2 = x * 0.0174532924f;

    float num3 = abs(cos(num2));

    Vector3T dir{};
    dir.x = -sin(num) * num3;
    dir.y = cos(num) * num3;
    dir.z = sin(num2);
    return dir;
}
