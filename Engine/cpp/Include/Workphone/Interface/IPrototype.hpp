/*
zlib License

Copyright (c) 2026 ${Your Name}

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented.
2. Altered source versions must be plainly marked as such.
3. This notice may not be removed or altered from any source distribution.
*/
#ifndef IPrototype_h__
#define IPrototype_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace core
    {
        /**
         * @brief Interface for a prototype, a design pattern used to create new objects based on an
         * existing prototype instance.
         *
         * @note Cloning functionality should be implemented by the managers of the derived classes.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API IPrototype : public ISharedObject
        {
        public:
            IPrototype();

            IPrototype( u32 typeId );

            /** Destructor. */
            ~IPrototype() override;

            /**
             * Gets the parent prototype.
             * @return The parent prototype.
             */
            virtual SmartPtr<IPrototype> getParentPrototype() const = 0;

            /**
             * Sets the parent prototype.
             * @param prototype The parent prototype.
             */
            virtual void setParentPrototype( SmartPtr<IPrototype> prototype ) = 0;

            /**
             * Gets the data as a properties object.
             * @return The data as a properties object.
             */
            SmartPtr<Properties> getProperties() const override = 0;

            /**
             * Sets the data as a properties object.
             * @param properties The properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace core
}  // namespace workphone

#endif  // IPrototype_h__
