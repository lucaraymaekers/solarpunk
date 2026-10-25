/* date = June 10th 2025 1:05 pm */

#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

inline v2
operator-(v2 A)
{
    v2 Result;
    
    Result.X = -A.X;
    Result.Y = -A.Y;
    
    return Result;
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    
    return Result;
}

inline v2
operator+(v2 A, f32 B)
{
    v2 Result;
    
    Result.X = A.X + B;
    Result.Y = A.Y + B;
    
    return Result;
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    
    return Result;
}

inline v2
operator-(v2 A, f32 B)
{
    v2 Result;
    Result = A + -B;
    return Result;
}

inline v2
operator*(f32 A, v2 B)
{
    v2 Result;
    
    Result.X = A*B.X;
    Result.Y = A*B.Y;
    
    return Result;
}

inline v2
operator*(v2 A, f32 B)
{
    return B*A;
}

inline v2
operator/(v2 A, f32 B)
{
    v2 Result;
    
    Result.X = A.X/B;
    Result.Y = A.Y/B;
    
    return Result;
}

inline v2 & 
operator*=(v2 &A, f32 B)
{
    A = B * A;
    return A;
}

inline v2 & 
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return A;
}

inline
f32 LengthSq(v2 A)
{
    f32 Result = Inner(A, A);
    return Result;
}

#endif //HANDMADE_MATH_H
