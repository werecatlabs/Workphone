#ifndef __WPParamList_H__
#define __WPParamList_H__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

#if USE_FIXED_SIZE_PARAMS
#    include <Workphone/Core/FixedArray.hpp>
#else
#    include <Workphone/Core/Array.hpp>
#endif

#define USE_FIXED_SIZE_PARAMS 0

namespace workphone
{

    /**
     * @brief Generic container for a single typed parameter.
     *
     * Parameter wraps a small set of primitive and complex types used to pass
     * arguments in the Workphone engine. The actual payload is determined by
     * the 'type' field and stored in the union 'Data' or the object/string/array
     * members for non-trivial types.
     */
    class WPCore_API Parameter
    {
    public:
        /**
         * @brief Default constructs a VOID parameter.
         */
        Parameter();

        /** Construct from a boolean value. */
        explicit Parameter( bool data );

        /** Construct from a C-string pointer. The string is copied into 'str'. */
        explicit Parameter( const c8 *data );

        /** Construct from an unsigned 8-bit integer. */
        explicit Parameter( u8 data );

        /** Construct from an unsigned 16-bit integer. */
        explicit Parameter( u16 data );

        /** Construct from a signed 32-bit integer. */
        explicit Parameter( s32 data );

        /** Construct from an unsigned 32-bit integer. */
        explicit Parameter( u32 data );

        /** Construct from a 32-bit float. */
        explicit Parameter( f32 data );

        /** Construct from a signed 64-bit integer. */
        explicit Parameter( s64 data );

        /** Construct from a 64-bit float (double). */
        explicit Parameter( f64 data );

        /** Construct from a raw pointer. Stored in the union pData. */
        explicit Parameter( void *data );

        /** Construct from a workphone String. */
        explicit Parameter( const String &data );

        /** Construct from a shared object pointer. */
        explicit Parameter( SmartPtr<ISharedObject> data );

        /** Construct from an array of Parameters. */
        explicit Parameter( const Array<Parameter> &data );

        /** Destructor cleans up non-trivial stored data. */
        ~Parameter();

        /* Setters - set the parameter value and update the type accordingly. */
        void setBool( bool data );
        void setCharPtr( const c8 *data );
        void setU8( u8 data );
        void setU16( u16 data );
        void setS32( s32 data );
        void setU32( u32 data );
        void setF32( f32 data );
        void setS64( s64 data );
        void setF64( f64 data );
        void setPtr( void *data );

        void setObject( SmartPtr<ISharedObject> data );
        void setArray( const Array<Parameter> &data );
        void setStr( const String &data );

        /* Getters - retrieve the stored value. Caller must check 'type' first. */
        bool getBool() const;
        c8 *getCharPtr() const;
        u8 getU8() const;
        u16 getU16() const;
        s32 getS32() const;
        u32 getU32() const;
        f32 getF32() const;
        s64 getS64() const;
        f64 getF64() const;
        void *getPtr() const;

        /** Vector and quaternion helpers for mathematical types. */
        Vector2F getVector2() const;
        void setVector2( const Vector2F &data );
        Vector3<real_Num> getVector3() const;
        void setVector3( const Vector3<real_Num> &data );
        Quaternion<real_Num> getQuaternion() const;
        void setQuaternion( const Quaternion<real_Num> &data );

        SmartPtr<ISharedObject> getObject() const;

        /** Return a const reference to the contained array of Parameters. */
        const Array<Parameter> &getArray() const;
        /** Return a copy of the contained string. */
        String getStr() const;

        /** Compare two parameters for equality (type and contained value). */
        bool operator==( const Parameter &other ) const;

        /**
         * @brief Lightweight storage union for primitive and pointer types.
         *
         * Note: complex types such as String, SmartPtr and Array are stored in
         * separate members and are not part of this union.
         */
        union Data
        {
            s32 bData;   /**< boolean stored as s32 for alignment. */
            s32 iData;   /**< integer data (32-bit). */
            s64 i64Data; /**< integer data (64-bit). */
            f32 fData;   /**< float data (32-bit). */
            f64 f64Data; /**< float data (64-bit). */

            /** Pointer-sized storage for raw pointers. */
            void *pData;
        };

        /// The parameter type discriminator.
        ParameterType type;

        /// The primitive / pointer parameter data.
        Data data;

        /// Object data stored for OBJECT type parameters.
        SmartPtr<ISharedObject> object;

        /// String data stored for STRING type parameters.
        String str;

        /** Array data stored for ARRAY type parameters. */
        Array<Parameter> array;

        /**< Pointer to the next parameter in a linked list. */
        Parameter *next = nullptr;

        /**
         * @brief A static instance representing a VOID (empty) parameter.
         */
        static const Parameter VOID_PARAM;
    };

#if USE_FIXED_SIZE_PARAMS
    static const u32 NUM_MAX_PARAMS = 4;
    using Parameters = FixedArray<Parameter, NUM_MAX_PARAMS>;
#else
    static const u32 NUM_MAX_PARAMS = 8;
    typedef Array<Parameter> Parameters;
#endif
}  // namespace workphone

#endif
