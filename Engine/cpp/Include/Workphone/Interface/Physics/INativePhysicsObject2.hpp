#ifndef INativePhysicsObject2_h__
#define INativePhysicsObject2_h__

#include <Workphone/WorkphonePrerequisites.hpp>

namespace workphone::physics
{
    class INativePhysicsObject2
    {
    public:
        virtual ~INativePhysicsObject2() = default;

        virtual void *getNativeObject() const = 0;
    };
}  // namespace workphone::physics

#endif
