#ifndef Vector3Helper_h__
#define Vector3Helper_h__

#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    template <class T>
    class Vector3Helper
    {
    public:
        static void setX( Vector3<T> &vec, lua_Number num )
        {
            vec.X() = static_cast<T>( num );
        }

        static void setY( Vector3<T> &vec, lua_Number num )
        {
            vec.Y() = static_cast<T>( num );
        }

        static void setZ( Vector3<T> &vec, lua_Number num )
        {
            vec.Z() = static_cast<T>( num );
        }

        static T getX( const Vector3<T> &vec )
        {
            return vec.X();
        }

        static T getY( const Vector3<T> &vec )
        {
            return vec.Y();
        }

        static T getZ( const Vector3<T> &vec )
        {
            return vec.Z();
        }
    };
} // namespace workphone

#endif // Vector3Helper_h__
