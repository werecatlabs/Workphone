#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>

namespace workphone::advanced
{
struct DecalPoint
{
    float x = 0, y = 0, z = 0;
};
struct SkidDecal
{
    DecalPoint start, end, normal;
    float width = 0, intensity = 0, age = 0;
};
class SkidDecalPool
{
   public:
    explicit SkidDecalPool( size_t capacity ) : m_capacity( capacity ) {}
    void clear()
    {
        m_marks.clear();
        m_connected.fill( false );
    }
    void breakTrail( size_t wheel )
    {
        if ( wheel < 4 ) m_connected[wheel] = false;
    }
    bool sample( size_t wheel, DecalPoint point, DecalPoint normal, float width, float intensity )
    {
        if ( wheel >= 4 || !m_capacity || !finite( point ) || !finite( normal ) ||
             normal.x * normal.x + normal.y * normal.y + normal.z * normal.z < .0001f ||
             !std::isfinite( width ) || width <= 0 || !std::isfinite( intensity ) ||
             intensity <= 0 )
        {
            breakTrail( wheel );
            return false;
        }
        auto previous = m_previous[wheel];
        m_previous[wheel] = point;
        if ( !m_connected[wheel] )
        {
            m_connected[wheel] = true;
            return false;
        }
        const auto dx = point.x - previous.x, dy = point.y - previous.y, dz = point.z - previous.z;
        const auto distance = std::sqrt( dx * dx + dy * dy + dz * dz );
        if ( distance < .15f )
        {
            m_previous[wheel] = previous;
            return false;
        }
        // Teleports, resets and contact discontinuities must not bridge the scene.
        if ( distance > 3.f || std::abs( dy ) > .3f ) return false;
        if ( m_marks.size() == m_capacity ) m_marks.pop_front();
        m_marks.push_back(
            { previous, point, normal, width, std::clamp( intensity, 0.f, 1.f ), 0 } );
        return true;
    }
    void advance( float dt )
    {
        if ( !std::isfinite( dt ) || dt <= 0 ) return;
        for ( auto& mark : m_marks ) mark.age += dt;
        while ( !m_marks.empty() && m_marks.front().age >= 20.f ) m_marks.pop_front();
    }
    static float opacity( const SkidDecal& mark )
    {
        return mark.intensity * std::clamp( ( 20.f - mark.age ) / 4.f, 0.f, 1.f );
    }
    const std::deque<SkidDecal>& marks() const { return m_marks; }

   private:
    static bool finite( DecalPoint p )
    {
        return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z );
    }
    size_t m_capacity;
    std::deque<SkidDecal> m_marks;
    std::array<DecalPoint, 4> m_previous{};
    std::array<bool, 4> m_connected{};
};
}  // namespace workphone::advanced
