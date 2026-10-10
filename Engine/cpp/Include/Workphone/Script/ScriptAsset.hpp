#ifndef WPScriptAsset_h__
#define WPScriptAsset_h__
#include <Workphone/System/Resource.hpp>

namespace workphone
{
    /// Cataloged source identity and metadata; never owns a Lua state or instance.
    class WPCore_API ScriptAsset final : public Resource<IResource>
    {
    public:
        void loadFromFile( const String &path ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        SmartPtr<Properties> getProperties() const override;
        WP_CLASS_REGISTER_DECL;
    };
}
#endif
