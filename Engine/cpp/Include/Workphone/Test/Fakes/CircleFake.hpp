#ifndef CircleFake_h__
#define CircleFake_h__

#include <Workphone/Test/Fakes/ShapeFake.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>

namespace workphone
{
    namespace test
    {

        class CircleFake : public ShapeFake
        {
        public:
            void virtualfunc() const override;
        };
    }  // end namespace test
}  // namespace workphone

#endif  // CircleFake_h__
