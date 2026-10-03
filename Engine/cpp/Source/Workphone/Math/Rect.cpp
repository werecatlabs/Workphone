#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Rect.hpp>

namespace workphone
{

    Rect::Rect( s32 l, s32 t, s32 r, s32 b ) : left( l ), top( t ), right( r ), bottom( b )
    {
    }

    Rect::Rect() : left( 0 ), top( 0 ), right( 0 ), bottom( 0 )
    {
    }

}  // namespace workphone
