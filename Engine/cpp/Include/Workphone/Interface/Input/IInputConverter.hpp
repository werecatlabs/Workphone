#ifndef _WP_IInputConverter_H
#define _WP_IInputConverter_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    class WPCore_API IInputConverter : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IInputConverter() override;

        /** Gets a game input id from the name passed. */
        virtual u32 getInputId( const String &inputName ) const = 0;

        /** Gets a game name from the id passed. */
        virtual String getInputName( u32 inputId ) const = 0;

        /** Gets the id for the action name. */
        virtual u32 getActionId( const String &actionName ) const = 0;

        /** Gets an action name from the id provided. */
        virtual String getActionName( u32 actionId ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // _WP_IInputConverter_H
