#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Matrix2.hpp>

namespace workphone
{

    template <class T>
    Matrix2<T>::Matrix2()
    {
    }

    template <class T>
    Matrix2<T>::Matrix2( const Matrix2 &other )
    {
        *this = other;
    }

    template <class T>
    Matrix2<T>::Matrix2( T angle )
    {
        fromAngle( angle );
    }

    template <class T>
    workphone::Matrix2<T> Matrix2<T>::identity()
    {
        Matrix2 mat;
        // Initialize all at once - potentially better optimization by compiler
        mat.M[0] = T( 1 );
        mat.M[3] = T( 1 );
        mat.M[1] = mat.M[2] = T( 0 );
        return mat;
    }

    template <class T>
    void Matrix2<T>::toAngle( T &rfAngle ) const
    {
        // assert:  matrix is a rotation
        rfAngle = Math<T>::ATan2( M[2], M[0] );
    }

    template <class T>
    void Matrix2<T>::fromAngle( T fAngle )
    {
        M[0] = Math<T>::Cos( fAngle );
        M[2] = Math<T>::Sin( fAngle );
        M[1] = -M[2];
        M[3] = M[0];
    }

    template <class T>
    workphone::Vector2<T> Matrix2<T>::operator*( const Vector2<T> &rkV ) const
    {
        return Vector2<T>( M[0] * rkV[0] + M[1] * rkV[1], M[2] * rkV[0] + M[3] * rkV[1] );
    }

    // explicit instantiation
    template class Matrix2<s32>;
    template class Matrix2<f32>;
    template class Matrix2<f64>;

}  // namespace workphone
