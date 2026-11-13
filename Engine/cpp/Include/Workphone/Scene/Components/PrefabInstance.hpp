#pragma once
#include <Workphone/Scene/Components/Component.hpp>

namespace workphone::scene
{
    // Authoring provenance only. This component never registers frame updates.
    class WPCore_API PrefabInstance : public Component
    {
    public:
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        String getSourcePath() const;
        String getSourceAssetUuid() const;
        SmartPtr<Properties> getIdentityMap() const;
        WP_CLASS_REGISTER_DECL;
    private:
        String m_sourcePath;
        String m_sourceAssetUuid;
        SmartPtr<Properties> m_identities;
    };
}
