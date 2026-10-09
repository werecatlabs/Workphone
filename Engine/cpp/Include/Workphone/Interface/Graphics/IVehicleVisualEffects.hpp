#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <array>
namespace workphone::advanced
{
struct VehicleEffectsFrame
{
    std::array<Vector3F, 4> contact{}, normal{};
    std::array<float, 4> slip{}, width{};
    std::array<bool, 4> grounded{}, onRoad{};
    Vector3F position, velocity;
    bool playing = true;
};

/// Renderer-owned implementation; callers configure and emit on the render task.
class WPCore_API IVehicleVisualEffects : public ISharedObject
{
   public:
    ~IVehicleVisualEffects() override;
    virtual bool configure( unsigned quality, unsigned seed ) = 0;
    virtual void reset() = 0;
    virtual void update( const VehicleEffectsFrame& frame, float dt ) = 0;
    virtual size_t particles() const = 0;
    virtual size_t decals() const = 0;
    virtual size_t emitted() const = 0;
    virtual bool uploaded() const = 0;
    WP_CLASS_REGISTER_DECL;
};
}  // namespace workphone::advanced
