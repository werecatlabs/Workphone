#pragma once
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
namespace workphone::scene
{
    /// Bakes renderer-neutral albedo, normal, ORM and height buffers from editor properties.
    /// Render integrations consume getResult() with the appropriate colour spaces.
    class WPCore_API ProceduralSurfaceTexture : public Component
    {
    public:
        ProceduralSurfaceTexture();
        ~ProceduralSurfaceTexture() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        bool regenerate();
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void setTextureForge( SmartPtr<procedural::ITextureForge> service )
        {
            m_service = service;
            m_dirty = true;
        }
        const procedural::SurfaceBakeResult &getResult() const
        {
            return m_result;
        }
        WP_CLASS_REGISTER_DECL;

    private:
        procedural::SurfaceTag m_surface = procedural::SurfaceTag::Concrete;
        procedural::SurfaceBakeParams m_params;
        procedural::SurfaceBakeResult m_result;
        SmartPtr<procedural::ITextureForge> m_service;
        u32 m_seed = 42;
        bool m_dirty = true;
        String m_error;
    };
}  // namespace workphone::scene
