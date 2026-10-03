#ifndef Image_h__
#define Image_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{

    class Image
    {
    public:
        Image();
        ~Image();

        ColourF &at( s32 x, s32 y );

        const ColourF &at( s32 x, s32 y ) const;

        s32 width = 0;
        s32 height = 0;
        Array<ColourF> pixels;
    };

}  // namespace workphone

#endif  // Image_h__
