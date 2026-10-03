#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/MathBind.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include "WPLuabind/WPLuabindTypes.hpp"
#include "WPLuabind/Helpers/Vector3Helper.hpp"
#include "WPLuabind/Helpers/Vector4Helper.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    s32 vector2i_getX( const Vector2I &vec )
    {
        return vec.X();
    }

    s32 vector2i_getY( const Vector2I &vec )
    {
        return vec.Y();
    }

    f32 vector2f_getX( const Vector2F &vec )
    {
        return vec.X();
    }

    f32 vector2f_getY( const Vector2F &vec )
    {
        return vec.Y();
    }

    f64 vector2d_getX( const Vector2D &vec )
    {
        return vec.X();
    }

    f64 vector2d_getY( const Vector2D &vec )
    {
        return vec.Y();
    }

    template <class T>
    int PointToWatch( const Vector2<T> &vec )
    {
        // auto applicationManager = core::ApplicationManager::instance();
        // auto luaMgr = lioncat::static_pointer_cast<LuaManager>( applicationManager->getScriptManager()
        // ); auto L = luaMgr->getLuaState();

        // lua_newtable( L );
        // int table = lua_gettop( L );

        // lua_pushstring( L, "x" );
        // lua_pushnumber( L, vec.X() );
        // lua_settable( L, table );
        // lua_pushstring( L, "y" );
        // lua_pushnumber( L, vec.Y() );
        // lua_settable( L, table );

        return 1;
    }

    bool box2IntersectsBox( const AABB2F &boxA, const AABB2F &boxB )
    {
        return boxA.intersects( boxB );
    }

    f32 quaternion_getW( const QuaternionF &vec )
    {
        return vec.W();
    }

    f32 quaternion_getX( const QuaternionF &vec )
    {
        return vec.X();
    }

    f32 quaternion_getY( const QuaternionF &vec )
    {
        return vec.Y();
    }

    f32 quaternion_getZ( const QuaternionF &vec )
    {
        return vec.Z();
    }

    lua_Integer _rangedRandom( lua_Integer min_, lua_Integer max_ )
    {
        return Math<lua_Integer>::RangedRandom( min_, max_ );
    }

    lua_Number __rangedRandomNumber( lua_Number min_, lua_Number max_ )
    {
        return Math<lua_Number>::RangedRandom( min_, max_ );
    }

    template <class T>
    void register_sphere3( lua_State *L, const char *name )
    {
        using namespace luabind;

        module( L )[class_<Sphere3<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<const Vector3<T> &, T>() )
                        .def( constructor<const Sphere3<T> &>() )
                        .def( "operator=", &Sphere3<T>::operator= )
                        .def( "getCenter", &Sphere3<T>::getCenter )
                        .def( "setCenter", &Sphere3<T>::setCenter )
                        .def( "getRadius", &Sphere3<T>::getRadius )
                        .def( "setRadius", &Sphere3<T>::setRadius )
                    //.def( "intersects", &Sphere3<T>::intersects )
                    //.def( "intersectsSQ", &Sphere3<T>::intersectsSQ )
                    //.def( "intersects", &Sphere3<T>::intersects )
        ];
    }

    template <class T>
    void register_transform3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module(
            L )[class_<Transform3<T>>( name )
                    .def( constructor<>() )
                    .def( constructor<const Vector3<T> &, const Quaternion<T> &>() )
                    .def( constructor<const Vector3<T> &, const Quaternion<T> &, const Vector3<T> &>() )
                    .def( "getPosition",
                          ( const Vector3<T> &(Transform3<T>::*)() const ) & Transform3<T>::getPosition )
                    .def( "setPosition", &Transform3<T>::setPosition )
                    .def( "getScale",
                          ( const Vector3<T> &(Transform3<T>::*)() const ) & Transform3<T>::getScale )
                    .def( "setScale", &Transform3<T>::setScale )
                    .def( "getOrientation", ( const Quaternion<T> &(Transform3<T>::*)() const ) &
                                                Transform3<T>::getOrientation )
                    .def( "setOrientation", &Transform3<T>::setOrientation )
                    .def( "getRotation", &Transform3<T>::getRotation )
                    .def( "setRotation", &Transform3<T>::setRotation )
                    .def( "transformPoint", &Transform3<T>::transformPoint )
                    .def( "transformVector", &Transform3<T>::transformVector )
                    .def( "inverseTransformPoint", &Transform3<T>::inverseTransformPoint )
                    .def( "inverseTransformVector", &Transform3<T>::inverseTransformVector )
                    .def( "inverseRotate", &Transform3<T>::inverseRotate )
                    .def( "up", &Transform3<T>::up )
                    .def( "forward", &Transform3<T>::forward )
                    .def( "right", &Transform3<T>::right )
                    .def( self == other<Transform3<T>>() )];
    }

    template <class T>
    void register_aabb2( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<AABB2<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<T, T, T, T>() )
                        .def( constructor<const Vector2<T> &, const Vector2<T> &>() )
                        .def( "getArea", &AABB2<T>::getArea )
                        .def( "isInside", &AABB2<T>::isInside )
                        .def( "intersects",
                              ( bool ( AABB2<T>::* )( const AABB2<T> & ) const ) & AABB2<T>::intersects )
                        .def( "clipAgainst", &AABB2<T>::clipAgainst )
                        .def( "constrainTo", &AABB2<T>::constrainTo )
                        .def( "getWidth", &AABB2<T>::getWidth )
                        .def( "getHeight", &AABB2<T>::getHeight )
                        .def( "repair", &AABB2<T>::repair )
                        .def( "isValid", &AABB2<T>::isValid )
                        .def( "getCenter", &AABB2<T>::getCenter )
                        .def( "getSize", &AABB2<T>::getSize )
                        .def( "getHalfSize", &AABB2<T>::getHalfSize )
                        .def( "getMin", &AABB2<T>::getMin )
                        .def( "setMin", &AABB2<T>::setMin )
                        .def( "getMax", &AABB2<T>::getMax )
                        .def( "setMax", &AABB2<T>::setMax )
                        .def( self == other<AABB2<T>>() )];
    }

    template <class T>
    void register_ray3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<Ray3<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<const Vector3<T> &, const Vector3<T> &>() )
                        .def( "getOrigin", &Ray3<T>::getOrigin )
                        .def( "setOrigin", &Ray3<T>::setOrigin )
                        .def( "getDirection", &Ray3<T>::getDirection )
                        .def( "setDirection", &Ray3<T>::setDirection )
                        .def( "isValid", &Ray3<T>::isValid )];
    }

    template <class T>
    void register_aabb3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<AABB3<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<const Vector3<T> &, const Vector3<T> &>() )
                        .def( "isPointInside", &AABB3<T>::isPointInside )
                        .def( "isPointTotalInside", &AABB3<T>::isPointTotalInside )
                        .def( "intersects", &AABB3<T>::intersects )
                        .def( "isFullInside", &AABB3<T>::isFullInside )
                        .def( "getCenter", &AABB3<T>::getCenter )
                        .def( "getExtent", &AABB3<T>::getExtent )
                        .def( "getSize", &AABB3<T>::getSize )
                        .def( "isEmpty", &AABB3<T>::isEmpty )
                        .def( "repair", &AABB3<T>::repair )
                        .def( "getMinimum", &AABB3<T>::getMinimum )
                        .def( "setMinimum", &AABB3<T>::setMinimum )
                        .def( "getMaximum", &AABB3<T>::getMaximum )
                        .def( "setMaximum", &AABB3<T>::setMaximum )
                        .def( "getRadius", &AABB3<T>::getRadius )
                        .def( "isNull", &AABB3<T>::isNull )
                        .def( "isFinite", &AABB3<T>::isFinite )
                        .def( "isInfinite", &AABB3<T>::isInfinite )
                        .def( "isValid", &AABB3<T>::isValid )
                        .def( self == other<AABB3<T>>() )];
    }

    template <class T>
    void register_line3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<Line3<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<const Vector3<T> &, const Vector3<T> &>() )
                        .def( "getLength", &Line3<T>::getLength )
                        .def( "getLengthSQ", &Line3<T>::getLengthSQ )
                        .def( "getMiddle", &Line3<T>::getMiddle )
                        .def( "getVector", &Line3<T>::getVector )
                        .def( "getDirection", &Line3<T>::getDirection )
                        .def( "getClosestPoint", &Line3<T>::getClosestPoint )
                        .def( "isPointBetweenStartAndEnd", &Line3<T>::isPointBetweenStartAndEnd )
                        .def( "getStart", &Line3<T>::getStart )
                        .def( "setStart", &Line3<T>::setStart )
                        .def( "getEnd", &Line3<T>::getEnd )
                        .def( "setEnd", &Line3<T>::setEnd )
                        .def( self == other<Line3<T>>() )];
    }

    template <class T>
    void register_plane3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<Plane3<T>>( name )
                        .def( constructor<>() )
                        .def( constructor<const Vector3<T> &, const Vector3<T> &>() )
                        .def( "classifyPointRelation", &Plane3<T>::classifyPointRelation )
                        .def( "recalculateD", &Plane3<T>::recalculateD )
                        .def( "getMemberPoint", &Plane3<T>::getMemberPoint )
                        .def( "existsInterSection", &Plane3<T>::existsInterSection )
                        .def( "isFrontFacing", &Plane3<T>::isFrontFacing )
                        .def( "getDistance", ( T ( Plane3<T>::* )( const Vector3<T> & ) const ) &
                                                 Plane3<T>::getDistance )
                        .def( "getNormal", &Plane3<T>::getNormal )
                        .def( "setNormal", &Plane3<T>::setNormal )
                        .def( self == other<Plane3<T>>() )];
    }

    template <class T>
    void register_matrix3( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<Matrix3<T>>( name )
                        .def( constructor<>() )
                        .def( "transpose", &Matrix3<T>::transpose )
                        .def( self == other<Matrix3<T>>() )
                        .def( self + other<Matrix3<T>>() )
                        .def( self - other<Matrix3<T>>() )
                        .def( self * other<Matrix3<T>>() )
                        .def( self * other<Vector3<T>>() )];
    }

    template <class T>
    void register_matrix4( lua_State *L, const char *name )
    {
        using namespace luabind;
        module( L )[class_<Matrix4<T>>( name )
                        .def( constructor<>() )
                        .def( "isAffine", &Matrix4<T>::isAffine )
                        .def( "concatenate", &Matrix4<T>::concatenate )
                        .def( "concatenateAffine", &Matrix4<T>::concatenateAffine )
                        .def( "makeTransform", &Matrix4<T>::makeTransform )
                        .def( "makePerspective", &Matrix4<T>::makePerspective )
                        .def( "transpose", &Matrix4<T>::transpose )
                        .def( "inverse", &Matrix4<T>::inverse )
                        .def( "getTranslation", &Matrix4<T>::getTranslation )
                        .def( "getRow", &Matrix4<T>::getRow )
                        .def( self * other<Matrix4<T>>() )
                        .def( self * other<Vector3<T>>() )];
    }

    void bindMath( lua_State *L )
    {
        using namespace luabind;

        using math_int = Math<s32>;
        using math_num = Math<f64>;

        module( L )[class_<math_int>( "MathI" )
                        .scope[def( "Round", &math_int::Round ), def( "Min", &math_int::min ),
                               def( "Max", &math_int::max ), def( "Sin", &math_int::Sin ),
                               def( "Cos", &math_int::Cos ),
                               // def("Tan", &math_int::Tan ),
                               def( "Atan", &math_int::Atan ), def( "Atan2", &math_int::ATan2 ),
                               def( "RangedRandom", &math_int::RangedRandom ),
                               def( "RangedRandom", _rangedRandom )]];

        module( L )[class_<math_num>( "Math" )
                        .scope[def( "Round", &math_num::Round ), def( "Min", &math_num::min ),
                               def( "Max", &math_num::max ), def( "Sin", &math_num::Sin ),
                               def( "Cos", &math_num::Cos ),
                               // def("Tan", &math_num::Tan ),
                               def( "Atan", &math_num::Atan ), def( "Atan2", &math_num::ATan2 ),
                               def( "Abs", &math_num::Abs ), def( "wrap", &math_num::wrap ),
                               def( "RadToDeg", &math_num::RadToDeg ),
                               def( "RangedRandom", &math_num::RangedRandom ),
                               def( "RangedRandom", __rangedRandomNumber )]];

        module( L )[class_<QuaternionF>( "Quaternion" )
                        .def( constructor<>() )
                        .def( constructor<QuaternionF &>() )
                        .def( constructor<f32, f32, f32, f32>() )
                        .def( "W", quaternion_getW )
                        .def( "X", quaternion_getX )
                        .def( "Y", quaternion_getY )
                        .def( "Z", quaternion_getZ )

                        // Operators
                        .def( self + other<QuaternionF>() )
                        .def( self - other<QuaternionF>() )
                        .def( self * other<QuaternionF>() )
                        .def( self * f32() )];

        module( L )[class_<Vector2I>( "Vector2I" )
                        .def( constructor<>() )
                        .def( constructor<Vector2I &>() )
                        .def( constructor<s32, s32>() )
                        .def( "X", vector2i_getX )
                        .def( "Y", vector2i_getY )
                        .def( "__towatch", PointToWatch<s32> )

                        // Operators
                        .def( self == other<Vector2I>() )
                        .def( self + other<Vector2I>() )
                        .def( self - other<Vector2I>() )
                        .def( self * other<Vector2I>() )
                        .def( self * s32() )];

        module( L )[class_<Vector2F>( "Vector2F" )
                        .def( constructor<>() )
                        .def( constructor<Vector2F &>() )
                        .def( constructor<f32, f32>() )
                        .def( "__towatch", PointToWatch<f32> )
                        .def( "X", vector2f_getX )
                        .def( "Y", vector2f_getY )

                        .def( "dotProduct", &Vector2F::dotProduct )
                        .def( "normalise", &Vector2F::normalise )
                        .def( "normaliseLength", &Vector2F::normaliseLength )
                        .def( "normaliseCopy", &Vector2F::normaliseCopy )
                        .def( "getLength", &Vector2F::length )
                        .def( "rotateBy", &Vector2F::rotateBy )

                        // Operators
                        .def( self == other<Vector2F>() )
                        .def( self + other<Vector2F>() )
                        .def( self - other<Vector2F>() )
                        .def( self * other<Vector2F>() )
                        .def( self * f32() )];

        module( L )[class_<Vector2D>( "Vector2D" )
                        .def( constructor<>() )
                        .def( constructor<Vector2D &>() )
                        .def( constructor<f64, f64>() )
                        .def( "X", vector2d_getX )
                        .def( "Y", vector2d_getY )
                        .def( "dotProduct", &Vector2D::dotProduct )
                        .def( "normalise", &Vector2D::normalise )
                        .def( "normaliseLength", &Vector2D::normaliseLength )
                        .def( "normaliseCopy", &Vector2D::normaliseCopy )
                        .def( "getLength", &Vector2D::length )
                        .def( "rotateBy", &Vector2D::rotateBy )
                        .def( self == other<Vector2D>() )
                        .def( self + other<Vector2D>() )
                        .def( self - other<Vector2D>() )
                        .def( self * other<Vector2D>() )
                        .def( self * f64() )];

        // module(L)
        //	[
        //		class_<Vector2D>( "Vector2D" )
        //		.def(constructor<>())
        //		.def(constructor<Vector2D&>())
        //		.def(constructor<f64, f64>())
        //		.def("__towatch", PointToWatch<f64> )

        //		.def("X", vector2d_getX )
        //		.def("Y", vector2d_getY )

        //		.def("dotProduct", &Vector2D::dotProduct )
        //		.def("normalize", &Vector2D::normalize )
        //		.def("normalizeLength", &Vector2D::normalizeLength )
        //		.def("normaliseCopy", &Vector2D::normaliseCopy )
        //		.def("getLength", &Vector2D::getLength )
        //		.def("rotateBy", &Vector2D::rotateBy )

        //		// Operators
        //		.def( self == other<Vector2D>() )
        //		.def( self + other<Vector2D>() )
        //		.def( self - other<Vector2D>() )
        //		.def( self * other<Vector2D>() )
        //		.def( self * f64() )
        //		.def(tostring(self))
        //	];

        LUA_CONST_START( Vector2I )
        LUA_CONST( Vector2I, ZERO );
        LUA_CONST( Vector2I, UNIT_X );
        LUA_CONST( Vector2I, UNIT_Y );
        LUA_CONST( Vector2I, UNIT );
        LUA_CONST_END;

        LUA_CONST_START( Vector2F )
        LUA_CONST( Vector2F, ZERO );
        LUA_CONST( Vector2F, UNIT_X );
        LUA_CONST( Vector2F, UNIT_Y );
        LUA_CONST( Vector2F, UNIT );
        LUA_CONST_END;

        // LUA_CONST_START( Vector2D )
        // LUA_CONST( Vector2D, ZERO);
        // LUA_CONST( Vector2D, UNIT_X);
        // LUA_CONST( Vector2D, UNIT_Y);
        // LUA_CONST( Vector2D, UNIT);
        // LUA_CONST_END;

        module( L )[class_<Vector3I>( "Vector3I" )
                        .def( constructor<>() )
                        .def( constructor<Vector3I &>() )
                        .def( constructor<s32, s32, s32>() )

                        .def( "X", Vector3Helper<s32>::setX )
                        .def( "Y", Vector3Helper<s32>::setY )
                        .def( "Z", Vector3Helper<s32>::setZ )

                        .def( "X", Vector3Helper<s32>::getX )
                        .def( "Y", Vector3Helper<s32>::getY )
                        .def( "Z", Vector3Helper<s32>::getZ )

                        // Operators
                        .def( self + other<Vector3I>() )
                        .def( self - other<Vector3I>() )
                        .def( self * other<Vector3I>() )
                        .def( self * s32() )];

        module( L )[class_<Vector3F>( "Vector3F" )
                        .def( constructor<>() )
                        .def( constructor<f32, f32, f32>() )
                        .def_readwrite( "x", &Vector3F::x )
                        .def_readwrite( "y", &Vector3F::y )
                        .def_readwrite( "z", &Vector3F::z )
                        .def( self + other<Vector3F>() )
                        .def( self - other<Vector3F>() )
                        .def( self * other<f32>() )
                        .def( self / other<f32>() )
                        .def( self == other<Vector3F>() )
                        //.def( self != other<Vector3F>() )
                        .def( self < other<Vector3F>() )
                        .def( self <= other<Vector3F>() )
                        //.def( self > other<Vector3F>() )
                        //.def( self >= other<Vector3F>() )
                        .def( "makeFloor", &Vector3F::makeFloor )
                        .def( "makeCeil", &Vector3F::makeCeil )
                        .def( "perpendicular", &Vector3F::perpendicular )
                        .def( "crossProduct", &Vector3F::crossProduct )
                        //.def( "randomDeviant", &Vector3F::randomDeviant )
                        //.def( "getRotationTo", &Vector3F::getRotationTo )
                        //.def( "angleBetween", &Vector3F::angleBetween )
                        //.def( "distance", &Vector3F::distance )
                        //.def( "squaredDistance", &Vector3F::squaredDistance )
                        .def( "dotProduct", &Vector3F::dotProduct )
                        .def( "normalise", &Vector3F::normalise )
                        .def( "midPoint", &Vector3F::midPoint )
                        .def( "perpendicular", &Vector3F::perpendicular )
                        //.def( "reflect", &Vector3F::reflect )
                        .def( "isZeroLength", &Vector3F::isZeroLength )
                        .def( "normalisedCopy", &Vector3F::normaliseCopy )
                        .def( "crossProduct", &Vector3F::crossProduct )
                    //.def( "getRotationTo", &Vector3F::getRotationTo )
                    //.def( "getRotationTo", &Vector3F::getRotationTo )
                    //.def( "getRotationTo", &Vector3F::getRotationTo )
        ];

        module( L )[class_<Vector3D>( "Vector3D" )
                        .def( constructor<>() )
                        .def( constructor<Vector3D &>() )
                        .def( constructor<f64, f64, f64>() )

                        .def( "normalise", &Vector3D::normalise )

                        .def( "X", Vector3Helper<f64>::setX )
                        .def( "Y", Vector3Helper<f64>::setY )
                        .def( "Z", Vector3Helper<f64>::setZ )

                        .def( "X", Vector3Helper<f64>::getX )
                        .def( "Y", Vector3Helper<f64>::getY )
                        .def( "Z", Vector3Helper<f64>::getZ )

                        // Operators
                        .def( self + other<Vector3D>() )
                        .def( self - other<Vector3D>() )
                        .def( self * other<Vector3D>() )
                        .def( self * f64() )];

        LUA_CONST_START( Vector3I )
        LUA_CONST( Vector3I, ZERO );
        LUA_CONST( Vector3I, UNIT_X );
        LUA_CONST( Vector3I, UNIT_Y );
        LUA_CONST( Vector3I, UNIT );
        LUA_CONST_END;

        LUA_CONST_START( Vector3F )
        LUA_CONST( Vector3F, ZERO );
        LUA_CONST( Vector3F, UNIT_X );
        LUA_CONST( Vector3F, UNIT_Y );
        LUA_CONST( Vector3F, UNIT );
        LUA_CONST_END;

        LUA_CONST_START( Vector3D )
        LUA_CONST( Vector3D, ZERO );
        LUA_CONST( Vector3D, UNIT_X );
        LUA_CONST( Vector3D, UNIT_Y );
        LUA_CONST( Vector3D, UNIT );
        LUA_CONST_END;

        module( L )[class_<Vector4I>( "Vector4I" )
                        .def( constructor<>() )
                        .def( constructor<Vector4I &>() )
                        .def( constructor<s32, s32, s32, s32>() )

                        .def( "X", Vector4Helper<s32>::setX )
                        .def( "Y", Vector4Helper<s32>::setY )
                        .def( "Z", Vector4Helper<s32>::setZ )

                        .def( "X", Vector4Helper<s32>::getX )
                        .def( "Y", Vector4Helper<s32>::getY )
                        .def( "Z", Vector4Helper<s32>::getZ )

                        // Operators
                        .def( self + other<Vector4I>() )
                        .def( self - other<Vector4I>() )
                        .def( self * other<Vector4I>() )
                        .def( self * s32() )];

        module( L )[class_<Vector4F>( "Vector4F" )
                        .def( constructor<>() )
                        .def( constructor<Vector4F &>() )
                        .def( constructor<f32, f32, f32, f32>() )

                        //.def("normalise", &Vector4F::normalise)
                        //.def("getLength", &Vector4F::getLength)
                        //.def("getLengthSquared", &Vector4F::getLengthSQ)

                        .def( "X", Vector4Helper<f32>::setX )
                        .def( "Y", Vector4Helper<f32>::setY )
                        .def( "Z", Vector4Helper<f32>::setZ )

                        .def( "X", Vector4Helper<f32>::getX )
                        .def( "Y", Vector4Helper<f32>::getY )
                        .def( "Z", Vector4Helper<f32>::getZ )

                        // Operators
                        .def( self + other<Vector4F>() )
                        .def( self - other<Vector4F>() )
                        .def( self * other<Vector4F>() )
                        .def( self * f32() )];

        module( L )[class_<Vector4D>( "Vector4D" )
                        .def( constructor<>() )
                        .def( constructor<Vector4D &>() )
                        .def( constructor<f64, f64, f64, f64>() )

                        //.def("normalise", &Vector4D::normalise)

                        .def( "X", Vector4Helper<f64>::setX )
                        .def( "Y", Vector4Helper<f64>::setY )
                        .def( "Z", Vector4Helper<f64>::setZ )

                        .def( "X", Vector4Helper<f64>::getX )
                        .def( "Y", Vector4Helper<f64>::getY )
                        .def( "Z", Vector4Helper<f64>::getZ )

                        // Operators
                        .def( self + other<Vector4D>() )
                        .def( self - other<Vector4D>() )
                        .def( self * other<Vector4D>() )
                        .def( self * f64() )];

        LUA_CONST_START( Vector4I )
        LUA_CONST( Vector4I, ZERO );
        LUA_CONST( Vector4I, UNIT_X );
        LUA_CONST( Vector4I, UNIT_Y );
        LUA_CONST( Vector4I, UNIT );
        LUA_CONST_END;

        LUA_CONST_START( Vector4F )
        LUA_CONST( Vector4F, ZERO );
        LUA_CONST( Vector4F, UNIT_X );
        LUA_CONST( Vector4F, UNIT_Y );
        LUA_CONST( Vector4F, UNIT );
        LUA_CONST_END;

        LUA_CONST_START( Vector4D )
        LUA_CONST( Vector4D, ZERO );
        LUA_CONST( Vector4D, UNIT_X );
        LUA_CONST( Vector4D, UNIT_Y );
        LUA_CONST( Vector4D, UNIT );
        LUA_CONST_END;

        register_sphere3<int>( L, "Sphere3i" );
        register_sphere3<float>( L, "Sphere3f" );
        register_sphere3<double>( L, "Sphere3d" );

        register_transform3<f32>( L, "Transform3F" );
        register_aabb2<f32>( L, "AABB2F" );
        register_aabb2<f64>( L, "AABB2D" );
        register_aabb3<f32>( L, "AABB3F" );
        register_aabb3<f64>( L, "AABB3D" );
        register_ray3<f32>( L, "Ray3F" );
        register_ray3<f64>( L, "Ray3D" );
        register_line3<f32>( L, "Line3F" );
        register_line3<f64>( L, "Line3D" );
        register_plane3<f32>( L, "Plane3F" );
        register_plane3<f64>( L, "Plane3D" );
        register_matrix3<f32>( L, "Matrix3F" );
        register_matrix3<f64>( L, "Matrix3D" );
        register_matrix4<f32>( L, "Matrix4F" );
        register_matrix4<f64>( L, "Matrix4D" );
    }
} // namespace workphone
