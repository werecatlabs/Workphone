#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/ExternalKeymap.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ExternalKeymap, IKeymap );

    ExternalKeymap::ExternalKeymap()
    {
    }

    ExternalKeymap::~ExternalKeymap()
    {
    }

    s32 ExternalKeymap::getKeycode( s32 keycode ) const
    {
        return keycode;
    }

    s32 ExternalKeymap::getExternalKeycode( s32 keycode ) const
    {
        return keycode;
    }

}  // namespace workphone
