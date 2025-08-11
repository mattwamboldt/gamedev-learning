#pragma once
#include "globals.h"

struct Vector2
{
    real32 x;
    real32 y;

    inline real32 LengthSquared() { return x * x + y * y; }
    inline real32 Length();

    void Normalize();
    Vector2 ToNormal();

    void SetMagnitude(real32 v);

    real32 DistanceSquared(Vector2 v);

    // Cuts off the length to given max
    inline void Truncate(real32 max);
    inline Vector2 Perp() { return { -y, x }; }

    const Vector2& operator*=(real32 rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    const Vector2& operator+=(Vector2 rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};

inline Vector2 operator+(Vector2 a, Vector2 b);
inline Vector2 operator-(Vector2 a, Vector2 b);
inline Vector2 operator*(Vector2 a, real32 rhs);
inline Vector2 operator/(Vector2 lhs, real32 rhs);
