#ifndef ParticleState_h__
#define ParticleState_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace render
    {
        class ParticleState
        {
        public:
            ParticleState();

            Vector4F m_colour;
            QuaternionF m_orientation;
            Vector3F m_position;
            Vector3F m_scale;
            Vector3F m_velocity;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleState_h__
