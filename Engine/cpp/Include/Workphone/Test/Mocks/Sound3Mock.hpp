#ifndef Sound3Mock_h__
#define Sound3Mock_h__

#include <Workphone/Interface/Sound/ISound.hpp>

namespace workphone
{
    namespace test
    {

        class Sound3Mock : public ISound
        {
        public:
            Vector3<real_Num> getPosition() const override;

            void getMinMaxDistance( f32 &minDistance, f32 &maxDistance ) override;

            void setPosition( const Vector3<real_Num> &position ) override;

            void setMinMaxDistance( f32 minDistance, f32 maxDistance ) override;
        };
    }  // end namespace test
}  // namespace workphone

#endif  // Sound3Mock_h__
