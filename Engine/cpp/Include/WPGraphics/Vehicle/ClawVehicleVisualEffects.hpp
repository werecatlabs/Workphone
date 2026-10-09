#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IVehicleVisualEffects.hpp>
#include <memory>
namespace workphone::advanced
{
class WPGraphics_API ClawVehicleVisualEffects : public IVehicleVisualEffects
{
   public:
    ClawVehicleVisualEffects();
    ~ClawVehicleVisualEffects() override;
    bool configure( unsigned quality, unsigned seed ) override;
    void unload( SmartPtr<ISharedObject> data ) override;
    void reset() override;
    void update( const VehicleEffectsFrame& frame, float dt ) override;
    size_t particles() const override;
    size_t decals() const override;
    size_t emitted() const override;
    bool uploaded() const override;

    WP_CLASS_REGISTER_DECL;

   private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace workphone::advanced
