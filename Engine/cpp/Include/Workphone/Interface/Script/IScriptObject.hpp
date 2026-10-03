#ifndef IStandardObject_h__
#define IStandardObject_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for an object that can be called from a script.
     *
     * This interface provides methods to manage script invokers and receivers, allowing
     * objects to interact with scripting systems. It inherits from ISharedObject for
     * shared ownership semantics.
     */
    class WPCore_API IScriptObject : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IScriptObject() override;

        /**
         * @brief Gets the script invoker associated with this object.
         *
         * The invoker is responsible for calling script functions from C++.
         * @return Reference to a smart pointer holding the script invoker.
         */
        virtual SmartPtr<IScriptInvoker> &getInvoker() = 0;

        /**
         * @brief Gets the script invoker associated with this object (const version).
         *
         * @return Const reference to a smart pointer holding the script invoker.
         */
        virtual const SmartPtr<IScriptInvoker> &getInvoker() const = 0;

        /**
         * @brief Sets the script invoker for this object.
         *
         * @param invoker Smart pointer to the script invoker to associate with this object.
         */
        virtual void setInvoker( SmartPtr<IScriptInvoker> invoker ) = 0;

        /**
         * @brief Gets the script receiver associated with this object.
         *
         * The receiver is responsible for receiving calls from scripts into C++.
         * @return Reference to a smart pointer holding the script receiver.
         */
        virtual SmartPtr<IScriptReceiver> &getReceiver() = 0;

        /**
         * @brief Gets the script receiver associated with this object (const version).
         *
         * @return Const reference to a smart pointer holding the script receiver.
         */
        virtual const SmartPtr<IScriptReceiver> &getReceiver() const = 0;

        /**
         * @brief Sets the script receiver for this object.
         *
         * @param receiver Smart pointer to the script receiver to associate with this object.
         */
        virtual void setReceiver( SmartPtr<IScriptReceiver> receiver ) = 0;

        /**
         * @brief Macro for class registration (implementation-specific).
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IStandardObject_h__
