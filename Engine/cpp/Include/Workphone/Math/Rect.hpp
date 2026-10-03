#ifndef Rect_h__
#define Rect_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    struct Rect
    {
        Rect();
        Rect( s32 l, s32 t, s32 r, s32 b );

        s32 left;    ///< Left coordinate of the rectangle.
        s32 top;     ///< Top coordinate of the rectangle.
        s32 right;   ///< Right coordinate of the rectangle.
        s32 bottom;  ///< Bottom coordinate of the rectangle.
    };
}  // namespace workphone

#endif  // Rect_h__
