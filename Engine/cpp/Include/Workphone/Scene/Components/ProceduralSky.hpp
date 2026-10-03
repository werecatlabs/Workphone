#pragma once
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
namespace workphone::scene
{
    class Light;
    /// Editor-driven atmosphere; applies ambient light, fog, and a directional Light.
    class WPCore_API ProceduralSky : public Component
    {
    public:
        ProceduralSky();
        ~ProceduralSky() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void setAtmosphere( SmartPtr<procedural::ISkyAtmosphere> service )
        {
            m_service = service;
            m_dirty = true;
        }
        bool regenerate();
        const procedural::SkyResults &getResults() const
        {
            return m_results;
        }
        WP_CLASS_REGISTER_DECL;

    private:
        procedural::SkyState m_state;
        procedural::SkyResults m_results;
        SmartPtr<procedural::ISkyAtmosphere> m_service;
        SmartPtr<Light> m_light;
        bool m_ownsLight = false;
        bool m_dirty = true;
        bool m_applyLighting = true;
        bool m_applyFog = true;
        String m_error;
    };
}  // namespace workphone::scene
