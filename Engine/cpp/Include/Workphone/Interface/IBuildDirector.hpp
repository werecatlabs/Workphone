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
#ifndef __IBuildDirector_h__
#define __IBuildDirector_h__

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{

    /**
     * @class IBuildDirector
     * @brief Interface for a director class responsible for resource construction and scene hierarchy
     * management.
     *
     * The IBuildDirector class serves as a base interface for constructing resources and managing
     * parent-child relationships between scene objects. It maintains essential data for resource
     * construction and provides methods to manipulate the scene hierarchy in a thread-safe manner.
     *
     * @author Zane Desir
     * @version 1.0
     */
    class WPCore_API IBuildDirector : public IResource
    {
    public:
        IBuildDirector();

        IBuildDirector( u32 poolTypeId );

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~IBuildDirector() override;

        /**
         * @brief Configures the initial header properties required for object construction.
         *
         * These properties typically include metadata and basic configuration settings.
         *
         * @param properties Header properties to be set up.
         */
        virtual void setupHeaderProperties( SmartPtr<Properties> properties ) const = 0;

        /**
         * @brief Gets the parent director of this object.
         *
         * @return SmartPtr<IBuildDirector> Parent director object, or nullptr if this is a root object.
         */
        virtual SmartPtr<IBuildDirector> getParent() const = 0;

        /**
         * @brief Sets the parent director for this object.
         *
         * Used internally to establish parent-child relationships in the scene hierarchy.
         *
         * @param parent Parent director to be set.
         */
        virtual void setParent( SmartPtr<IBuildDirector> parent ) = 0;

        /**
         * @brief Adds a child director to this object.
         *
         * Establishes a parent-child relationship by adding the specified child to this director's
         * children collection.
         *
         * @param child Child director to be added.
         */
        virtual void addChild( SmartPtr<IBuildDirector> child ) = 0;

        /**
         * @brief Removes a specific child director from this object.
         *
         * @param child Child director to be removed.
         */
        virtual void removeChild( SmartPtr<IBuildDirector> child ) = 0;

        /**
         * @brief Removes all child directors from this object.
         *
         * Clears the entire collection of child directors.
         */
        virtual void removeChildren() = 0;

        /**
         * @brief Finds a child director by its name.
         *
         * Searches through the children collection for a director with the specified name.
         *
         * @param name Name of the child director to find.
         * @return SmartPtr<IBuildDirector> Found child director, or nullptr if not found.
         */
        virtual SmartPtr<IBuildDirector> findChild( const String &name ) = 0;

        /**
         * @brief Gets all child directors.
         *
         * @return Array<SmartPtr<IBuildDirector>> Array containing all child directors.
         */
        virtual Array<SmartPtr<IBuildDirector>> getChildren() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // __IBuildDirector_h__
