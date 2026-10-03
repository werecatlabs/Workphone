#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CProceduralCityCenter.hpp"
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    namespace procedural
    {
        CProceduralCityCenter::CProceduralCityCenter()
        {
        }

        CProceduralCityCenter::~CProceduralCityCenter()
        {
        }

        Transform3<real_Num> CProceduralCityCenter::getTransform() const
        {
            return m_transform;
        }

        void CProceduralCityCenter::setTransform( Transform3<real_Num> transform )
        {
            m_transform = transform;
        }
    }  // namespace procedural
}  // namespace workphone
