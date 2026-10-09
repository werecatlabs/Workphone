#pragma once
#include <Workphone/Interface/Graphics/IVehicleVisualEffects.hpp>
#include <array>
#include <memory>

namespace workphone::advanced
{
class WPCore_API VehicleVisualEffects
{
   public:
    VehicleVisualEffects();
    ~VehicleVisualEffects();
    bool load( unsigned quality, unsigned seed );
    void unload();
    void reset();
    void update( const VehicleEffectsFrame& frame, float dt );
    size_t particles() const;
    size_t decals() const;
    size_t emitted() const;
    bool uploaded() const;

   private:
    SmartPtr<IVehicleVisualEffects> m_backend;
};
}  // namespace workphone::advanced
