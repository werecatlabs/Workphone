#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Euler.hpp>

namespace workphone
{

    template <class T>
    workphone::Euler<T> &Euler<T>::operator=( const Matrix3<T> &matrix )
    {
        fromMatrix3( matrix );
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::operator=( const Quaternion<T> &quaternion )
    {
        fromQuaternion( quaternion );
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::operator=( const Euler<T> &src )
    {
        orientation( src.yaw(), src.pitch(), src.roll() );
        return *this;
    }

    template <class T>
    void Euler<T>::wrapAngle( T &angle )
    {
        auto rangle = angle;
        auto twoPi = Math<T>::two_pi();
        if( rangle < -Math<T>::pi() )
        {
            rangle = std::fmod( rangle, -twoPi );
            if( rangle < -Math<T>::pi() )
            {
                rangle += twoPi;
            }
            angle = rangle;
            mChanged = true;
        }
        else if( rangle > Math<T>::pi() )
        {
            rangle = std::fmod( rangle, twoPi );
            if( rangle > Math<T>::pi() )
            {
                rangle -= twoPi;
            }
            angle = rangle;
            mChanged = true;
        }
    }

    template <class T>
    void Euler<T>::limitAngle( T &angle, const T &limit )
    {
        if( angle > limit )
        {
            angle = limit;
            mChanged = true;
        }
        else if( angle < -limit )
        {
            angle = -limit;
            mChanged = true;
        }
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::operator*( const Vector3<T> &rhs ) const
    {
        return toQuaternion() * rhs;
    }

    template <class T>
    workphone::Quaternion<T> Euler<T>::operator*( Euler<T> rhs ) const
    {
        Euler e1( *this ), e2( rhs );
        return e1.toQuaternion() * e2.toQuaternion();
    }

    template <class T>
    workphone::Euler<T> Euler<T>::operator*( T rhs ) const
    {
        return Euler( mYaw * rhs, mPitch * rhs, mRoll * rhs );
    }

    template <class T>
    workphone::Euler<T> Euler<T>::operator-( const Euler &rhs ) const
    {
        return Euler( mYaw - rhs.mYaw, mPitch - rhs.mPitch, mRoll - rhs.mRoll );
    }

    template <class T>
    workphone::Euler<T> Euler<T>::operator+( const Euler &rhs ) const
    {
        return Euler( mYaw + rhs.mYaw, mPitch + rhs.mPitch, mRoll + rhs.mRoll );
    }

    template <class T>
    workphone::Euler<T> Euler<T>::rotationTo( const Vector3<T> &dir, bool setYaw, bool setPitch,
                                              bool shortest ) const
    {
        Euler t1;
        Euler t2;
        t1.direction( dir, setYaw, setPitch );
        t2 = t1 - *this;
        if( shortest && setYaw )
        {
            t2.normalise();
        }
        return t2;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::normalise( bool normYaw, bool normPitch, bool normRoll )
    {
        if( normYaw )
            wrapAngle( mYaw );

        if( normPitch )
            wrapAngle( mPitch );

        if( normRoll )
            wrapAngle( mRoll );

        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::fromQuaternion( const Quaternion<T> &quaternion )
    {
        Matrix3<T> rotmat;
        quaternion.toRotationMatrix( rotmat );
        fromMatrix3( rotmat );
        return *this;
    }

    template <class T>
    Euler<T>::operator Quaternion<T>() const
    {
        return toQuaternion();
    }

    template <class T>
    workphone::Quaternion<T> Euler<T>::toQuaternion() const
    {
        if( mChanged )
        {
            mCachedQuaternion = Quaternion<T>::angleAxis( mYaw, Vector3<T>::positiveY() ) *
                                Quaternion<T>::angleAxis( mPitch, Vector3<T>::positiveX() ) *
                                Quaternion<T>::angleAxis( mRoll, Vector3<T>::positiveZ() );

            mChanged = false;
        }

        return mCachedQuaternion;
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::forward() const
    {
        return toQuaternion() * Vector3<T>::negativeZ();
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::yaw( const T &y )
    {
        mYaw += y;
        mChanged = true;
        return *this;
    }

    template <class T>
    T Euler<T>::yaw() const
    {
        return mYaw;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::setYaw( T y )
    {
        mYaw = y;
        mChanged = true;
        return *this;
    }

    template <class T>
    Euler<T>::Euler( const Matrix3<T> &matrix )
    {
        fromMatrix3( matrix );
    }

    template <class T>
    Euler<T>::Euler( const Quaternion<T> &quaternion )
    {
        fromQuaternion( quaternion );
    }

    template <class T>
    Euler<T>::Euler() : mYaw( T( 0.0 ) ), mPitch( T( 0.0 ) ), mRoll( T( 0.0 ) ), mChanged( true )
    {
    }

    template <class T>
    Euler<T>::Euler( const T &y, const T &p, const T &r ) :
        mYaw( y ),
        mPitch( p ),
        mRoll( r ),
        mChanged( true )
    {
    }

    template <class T>
    Euler<T>::Euler( const Vector3<T> &rads ) : mYaw( rads.Y() ), mPitch( rads.X() ), mRoll( rads.Z() )
    {
    }

    template <class T>
    T Euler<T>::pitch() const
    {
        return mPitch;
    }

    template <class T>
    T Euler<T>::roll() const
    {
        return mRoll;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::setPitch( T p )
    {
        mPitch = p;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::setRoll( T r )
    {
        mRoll = r;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::orientation( const T &y, const T &p, const T &r )
    {
        mYaw = y;
        mPitch = p;
        mRoll = r;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::pitch( const T &p )
    {
        mPitch += p;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::roll( const T &r )
    {
        mRoll += r;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::rotate( const T &y, const T &p, const T &r )
    {
        mYaw += y;
        mPitch += p;
        mRoll += r;
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::right() const
    {
        return toQuaternion() * Vector3<T>::positiveX();
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::up() const
    {
        return toQuaternion() * Vector3<T>::positiveY();
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::toRadians() const
    {
        return Vector3<T>( mPitch, mYaw, mRoll );
    }

    template <class T>
    workphone::Vector3<T> Euler<T>::toDegrees() const
    {
        return Vector3<T>( mPitch, mYaw, mRoll ) * Math<T>::rad_to_deg();
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::fromMatrix3( const Matrix3<T> &matrix )
    {
        matrix.toEulerAnglesYXZ( mYaw, mPitch, mRoll );
        mChanged = true;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::direction( const Vector3<T> &v, bool setYaw, bool setPitch )
    {
        Vector3<T> d( v.normaliseCopy() );
        if( setPitch )
            mPitch = Math<T>::ASin( d.y );
        if( setYaw )
            mYaw = Math<T>::ATan2( -d.X(), -d.Z() );
        mChanged = setYaw || setPitch;
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::limitYaw( const T &limit )
    {
        limitAngle( mYaw, limit );
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::limitPitch( const T &limit )
    {
        limitAngle( mPitch, limit );
        return *this;
    }

    template <class T>
    workphone::Euler<T> &Euler<T>::limitRoll( const T &limit )
    {
        limitAngle( mRoll, limit );
        return *this;
    }

    // explicit instantiation
    template class Euler<f32>;
    template class Euler<f64>;

}  // namespace workphone
