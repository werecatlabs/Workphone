#ifndef UnityKeymap_h__
#define UnityKeymap_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Input/ExternalKeymap.hpp>
#include <map>

namespace workphone
{

    class UnityKeymap : public ExternalKeymap
    {
    public:
        UnityKeymap();
        ~UnityKeymap() override;

        s32 getKeycode( s32 keycode ) const override;
        s32 getExternalKeycode( s32 keycode ) const override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // UnityKeymap_h__

