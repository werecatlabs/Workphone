#ifndef __CConstraintFixed2_h__
#define __CConstraintFixed2_h__

#include <Workphone/Interface/Physics/IConstraintFixed2.hpp>

namespace workphone
{
    namespace physics
    {
        class CConstraintFixed2 : public IConstraintFixed2
        {
        public:
            CConstraintFixed2();
            virtual ~CConstraintFixed2() override;

            virtual void *getUserData() const override;
            virtual void  setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    } // namespace physics
} // namespace workphone
#endif
