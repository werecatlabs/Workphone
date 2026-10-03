#ifndef IScriptReceiver_h__
#define IScriptReceiver_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{

    /**
     * @brief Interface for receiving and handling script calls in a generic way.
     *
     * This interface allows objects to receive property set/get requests and function calls
     * from scripts, using hashed identifiers for properties and functions. It supports
     * multiple types of property values and function invocation patterns.
     */
    class WPCore_API IScriptReceiver : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IScriptReceiver() override;

        /**
         * @brief Sets a string property by hash identifier.
         *
         * @param hash Hash identifier of the property name.
         * @param value String value to set.
         * @return Error code (implementation-defined).
         */
        virtual s32 setProperty( hash_type hash, const String &value ) = 0;

        /**
         * @brief Gets a string property by hash identifier.
         *
         * @param hash Hash identifier of the property name.
         * @param value [out] String value to retrieve.
         * @return Error code (implementation-defined).
         */
        virtual s32 getProperty( hash_type hash, String &value ) const = 0;

        /**
         * @brief Sets a property using a Parameter object.
         *
         * @param hash Hash identifier of the property.
         * @param param Parameter value to set.
         * @return Error code (implementation-defined).
         */
        virtual s32 setProperty( hash_type hash, const Parameter &param ) = 0;

        /**
         * @brief Sets a property using a Parameters collection.
         *
         * @param hash Hash identifier of the property.
         * @param params Collection of parameters to set.
         * @return Error code (implementation-defined).
         */
        virtual s32 setProperty( hash_type hash, const Parameters &params ) = 0;

        /**
         * @brief Sets a property using a raw pointer.
         *
         * @param hash Hash identifier of the property.
         * @param param Pointer to the value to set.
         * @return Error code (implementation-defined).
         * @note The ownership and type safety of the pointer is the responsibility of the caller.
         */
        virtual s32 setProperty( hash_type hash, void *param ) = 0;

        /**
         * @brief Gets a property as a Parameter object.
         *
         * @param hash Hash identifier of the property.
         * @param param [out] Parameter value to retrieve.
         * @return Error code (implementation-defined).
         */
        virtual s32 getProperty( hash_type hash, Parameter &param ) const = 0;

        /**
         * @brief Gets a property as a Parameters collection.
         *
         * @param hash Hash identifier of the property.
         * @param params [out] Collection of parameters to retrieve.
         * @return Error code (implementation-defined).
         */
        virtual s32 getProperty( hash_type hash, Parameters &params ) const = 0;

        /**
         * @brief Gets a property as a raw pointer.
         *
         * @param hash Hash identifier of the property.
         * @param param [out] Pointer to the value to retrieve.
         * @return Error code (implementation-defined).
         * @note The ownership and type safety of the pointer is the responsibility of the caller.
         */
        virtual s32 getProperty( hash_type hash, void *param ) const = 0;

        /**
         * @brief Calls a function by hash identifier with parameters and retrieves results.
         *
         * @param hash Hash identifier of the function.
         * @param params Parameters to pass to the function.
         * @param results [out] Results returned by the function.
         * @return Error code (implementation-defined).
         */
        virtual s32 callFunction( hash_type hash, const Parameters &params, Parameters &results ) = 0;

        /**
         * @brief Calls a function by hash identifier on a script object and retrieves results.
         *
         * @param hash Hash identifier of the function.
         * @param object Script object to invoke the function on.
         * @param results [out] Results returned by the function.
         * @return Error code (implementation-defined).
         */
        virtual s32 callFunction( hash_type hash, SmartPtr<ISharedObject> object,
                                  Parameters &results ) = 0;

        /**
         * @brief Macro for class registration (implementation-specific).
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IScriptReceiver_h__
