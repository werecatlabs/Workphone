#ifndef CProceduralCityCenter_h__
#define CProceduralCityCenter_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralCityCenter.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CProceduralCityCenter : public IProceduralCityCenter
        {
        public:
            CProceduralCityCenter();
            ~CProceduralCityCenter() override;

            Transform3<real_Num> getTransform() const override;
            void setTransform( Transform3<real_Num> transform ) override;

        protected:
            Transform3<real_Num> m_transform;
        };
    }  // namespace procedural
}  // namespace workphone

#endif
