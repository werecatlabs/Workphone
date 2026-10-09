#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/VehicleVisualEffects.h>

#include <Workphone/Workphone.hpp>
namespace workphone::advanced
{
    VehicleVisualEffects::VehicleVisualEffects() = default;
    VehicleVisualEffects::~VehicleVisualEffects() { unload(); }
    bool VehicleVisualEffects::load( unsigned quality, unsigned seed )
    {
        unload();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto factories = graphics ? graphics->getFactoryManager() : nullptr;
        if ( !factories ) return false;
        for ( auto factory : factories->getFactories() )
            if ( factory->isObjectDerivedFromByInfo( IVehicleVisualEffects::typeInfo() ) )
            {
                m_backend = factory->make_object<IVehicleVisualEffects>();
                if ( m_backend && m_backend->configure( quality, seed ) ) return true;
                unload();
            }
        return false;
    }
    void VehicleVisualEffects::unload()
    {
        if ( !m_backend ) return;
        auto backend = m_backend;
        m_backend = nullptr;
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if ( graphics && Thread::getCurrentTask() != TaskId::Render )
            graphics->unloadObject( backend );
        else
            backend->unload( nullptr );
    }
    void VehicleVisualEffects::reset()
    {
        if ( m_backend ) m_backend->reset();
    }
    void VehicleVisualEffects::update( const VehicleEffectsFrame& frame, float dt )
    {
        if ( m_backend ) m_backend->update( frame, dt );
    }
    size_t VehicleVisualEffects::particles() const
    {
        return m_backend ? m_backend->particles() : 0;
    }
    size_t VehicleVisualEffects::decals() const { return m_backend ? m_backend->decals() : 0; }
    size_t VehicleVisualEffects::emitted() const { return m_backend ? m_backend->emitted() : 0; }
    bool VehicleVisualEffects::uploaded() const { return m_backend && m_backend->uploaded(); }
}  // namespace workphone::advanced
