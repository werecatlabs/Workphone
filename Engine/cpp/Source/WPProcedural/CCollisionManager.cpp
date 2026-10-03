#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CCollisionManager.hpp"

namespace workphone
{
    namespace procedural
    {
        CCollisionManager::CCollisionManager()
        {
        }

        CCollisionManager::~CCollisionManager()
        {
        }

        bool CCollisionManager::rayTest( const Vector3F &start, const Vector3F &dir, Vector3F &outHitPos,
                                         Triangle3F &outTriangle, const Array<String> &filter )
        {
            return false;
        }

        bool CCollisionManager::rayTest( const Vector3F &start, const Vector3F &dir, Vector3F &outHitPos,
                                         Triangle3F &outTriangle )
        {
            return false;
        }

        bool CCollisionManager::rayTest( const Vector3F &start, const Vector3F &dir,
                                         Vector3F &outHitPos )
        {
            return false;
        }
    }  // namespace procedural
}  // namespace workphone
