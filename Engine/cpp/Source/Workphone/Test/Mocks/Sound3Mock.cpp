#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Mocks/Sound3Mock.hpp>

namespace workphone
{
    namespace test
    {

        workphone::Vector3<workphone::real_Num> Sound3Mock::getPosition() const
        {
            return Vector3<real_Num>::zero();
        }

        void Sound3Mock::getMinMaxDistance( f32 &minDistance, f32 &maxDistance )
        {
        }

        void Sound3Mock::setPosition( const Vector3<real_Num> &position )
        {
        }

        void Sound3Mock::setMinMaxDistance( f32 minDistance, f32 maxDistance )
        {
        }

    }  // end namespace test
}  // namespace workphone
