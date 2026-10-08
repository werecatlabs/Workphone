#pragma once
#include <Workphone/Math/Vector3.hpp>
#include <array>
#include <memory>

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
    class VehicleVisualEffects
    {
    public:
        VehicleVisualEffects();
        ~VehicleVisualEffects();
        bool load( unsigned quality, unsigned seed );
        void unload();
        void reset();
        void update( const VehicleEffectsFrame &frame, float dt );
        size_t particles() const;
        size_t decals() const;
        size_t emitted() const;
        bool uploaded() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::advanced
