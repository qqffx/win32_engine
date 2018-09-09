#if !defined(_INTRINSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   ======================================================================== */

//TODO: conert all of this into intrinsics
#include "math.h"


inline int32
RoundReal32ToUInt32(real32 Value)
{
    uint32 Result = (uint32)roundf(Value);
    return(Result);
}


inline int32
RoundReal32ToInt32(real32 Value)
{
    int32 Result = (int32)roundf(Value);
    return(Result);
}

inline int32
FloorReal32ToInt32(real32 Value)
{
    int32 Result  =  (int)floorf(Value);
    return(Result);
}

inline int32
TruncateReal32ToInt32(real32 Value)
{
    int32 Result = (int)Value;
    return(Result);
}

inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return(Result);
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return(Result);
}

inline real32
Atan2(real32 Y, real32 X)
{
    real32 Result = atan2f(Y, X);
    return(Result);
}


#define _INTRINSICS_H
#endif
