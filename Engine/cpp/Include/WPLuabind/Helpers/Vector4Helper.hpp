#ifndef Vector4Helper_h__
#define Vector4Helper_h__

#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    template <class T>
    class Vector4Helper
    {
    public:
        static void setX( Vector4<T> &vec, lua_Number num )
        {
            vec.X() = static_cast<T>( num );
        }

        static void setY( Vector4<T> &vec, lua_Number num )
        {
            vec.Y() = static_cast<T>( num );
        }

        static void setZ( Vector4<T> &vec, lua_Number num )
        {
            vec.Z() = static_cast<T>( num );
        }

        static void setW( Vector4<T> &vec, lua_Number num )
        {
            vec.W() = static_cast<T>( num );
        }

        static T getX( const Vector4<T> &vec )
        {
            return vec.X();
        }

        static T getY( const Vector4<T> &vec )
        {
            return vec.Y();
        }

        static T getZ( const Vector4<T> &vec )
        {
            return vec.Z();
        }

        static T getW( const Vector4<T> &vec )
        {
            return vec.W();
        }
    };
} // namespace workphone

#endif // Vector4Helper_h__
