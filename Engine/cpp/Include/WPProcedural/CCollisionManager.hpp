#ifndef CollisionManager_h__
#define CollisionManager_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralCollision.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Triangle3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CCollisionManager : public IProceduralCollision
        {
        public:
            CCollisionManager();
            ~CCollisionManager() override;

            bool rayTest( const Vector3F &start, const Vector3F &dir, Vector3F &outHitPos ) override;
            bool rayTest( const Vector3F &start, const Vector3F &dir, Vector3F &outHitPos,
                          Triangle3F &outTriangle ) override;
            bool rayTest( const Vector3F &start, const Vector3F &dir, Vector3F &outHitPos,
                          Triangle3F &outTriangle, const Array<String> &filter ) override;

        protected:
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // CollisionManager_h__
