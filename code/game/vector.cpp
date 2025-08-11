#include "vector.h"
#include <math.h>

inline Vector2 operator+(Vector2 a, Vector2 b)
{
    Vector2 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline Vector2 operator-(Vector2 a, Vector2 b)
{
    Vector2 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline Vector2 operator*(Vector2 v, real32 s)
{
    Vector2 result = {};
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

inline Vector2 operator*(real32 s, Vector2 v)
{
    return v * s;
}

inline Vector2 operator/(Vector2 lhs, real32 rhs)
{
    Vector2 result = {};
    result.x = lhs.x / rhs;
    result.y = lhs.y / rhs;
    return result;
}

inline real32 Vector2::Length()
{
    real32 lengthSquared = LengthSquared();
    if (lengthSquared > 0)
    {
        return sqrtf(lengthSquared);
    }

    return 0.0f;
}

void Vector2::Normalize()
{
    real32 lengthSquared = LengthSquared();
    if (lengthSquared > 0)
    {
        real32 scale = (1.0f / sqrtf(lengthSquared));
        x *= scale;
        y *= scale;
    }
}

Vector2 Vector2::ToNormal()
{
    Vector2 result = { x, y };
    result.Normalize();
    return result;
}

void Vector2::SetMagnitude(real32 v)
{
    Normalize();
    x *= v;
    y *= v;
}

inline void Vector2::Truncate(real32 max)
{
    real32 lengthSquared = LengthSquared();
    if (lengthSquared > max * max)
    {
        real32 scale = (max / sqrtf(lengthSquared));
        x *= scale;
        y *= scale;
    } 
}

real32 Vector2::DistanceSquared(Vector2 v)
{
    real32 dx = v.x - x;
    real32 dy = v.y - y;
    return (dx * dx + dy * dy);
}
