#pragma once

#include <cmath>

#include "matrix3x4.h"

struct Vector {
    constexpr operator bool() const noexcept
    {
        return x || y || z;
    }

    constexpr float operator[](int i) const noexcept
    {
        return ((float*)this)[i];
    }

    constexpr float& operator[](int i) noexcept
    {
        return ((float*)this)[i];
    }

    constexpr Vector& operator=(const float array[3]) noexcept
    {
        x = array[0];
        y = array[1];
        z = array[2];
        return *this;
    }

    constexpr Vector& operator+=(const Vector& v) noexcept
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    constexpr Vector& operator-=(const Vector& v) noexcept
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    constexpr auto operator-(const Vector& v) const noexcept
    {
        return Vector{ x - v.x, y - v.y, z - v.z };
    }

    constexpr auto operator+(const Vector& v) const noexcept
    {
        return Vector{ x + v.x, y + v.y, z + v.z };
    }

    constexpr Vector& operator/=(float div) noexcept
    {
        x /= div;
        y /= div;
        z /= div;
        return *this;
    }

    constexpr auto operator*(float mul) const noexcept
    {
        return Vector{ x * mul, y * mul, z * mul };
    }

    constexpr auto operator*=(float mul) noexcept
    {
        x *= mul;
        y *= mul;
        z *= mul;
        return *this;
    }

    constexpr void normalize() noexcept
    {
        x = std::isfinite(x) ? std::remainder(x, 360.0f) : 0.0f;
        y = std::isfinite(y) ? std::remainder(y, 360.0f) : 0.0f;
        z = 0.0f;
    }

    auto distance(const Vector& v) const noexcept
    {
        return std::hypot(x - v.x, y - v.y, z - v.z);
    }

    auto length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    auto length2D() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    constexpr auto squareLength() const noexcept
    {
        return x * x + y * y + z * z;
    }

    constexpr auto dotProduct(const Vector& v) const noexcept
    {
        return x * v.x + y * v.y + z * v.z;
    }

    constexpr auto transform(const matrix3x4& mat) const noexcept
    {
        return Vector{ dotProduct({ mat[0][0], mat[0][1], mat[0][2] }) + mat[0][3],
                       dotProduct({ mat[1][0], mat[1][1], mat[1][2] }) + mat[1][3],
                       dotProduct({ mat[2][0], mat[2][1], mat[2][2] }) + mat[2][3] };
    }

    void VectorCrossProduct(const Vector& a, const Vector& b, Vector& result)
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    Vector Cross(const Vector& vOther)
    {
        Vector res;
        VectorCrossProduct(*this, vOther, res);
        return res;
    }

    //this vector struct is kinda shit
    //heres some clamp math, youll want to use this along with normalize 90% of the time
    void Clamp() {
        while (this->x < -89.0f)
            this->x += 89.0f;

        if (this->x > 89.0f)
            this->x = 89.0f;

        while (this->y < -180.0f)
            this->y += 360.0f;

        while (this->y > 180.0f)
            this->y -= 360.0f;

        this->z = 0.0f;
    }

    float x, y, z;
};
