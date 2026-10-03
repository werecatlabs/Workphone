/*!\file CFontOgreNext.hpp
 * \brief OgreNext font wrapper for the Workphone rendering Font interface.
 *
 * This header declares `CFontOgreNext`, an implementation of `workphone::render::Font`
 * that wraps an `Ogre::Font` resource from OgreNext. The class provides load/reload/
 * unload semantics and exposes property accessors used by the engine resource system.
 */

#ifndef __CFont_h__
#define __CFont_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>
#include <OgreFont.h>

namespace workphone
{
    namespace render
    {
        /**
         * \class CFontOgreNext
         * \brief Concrete Font implementation using OgreNext's font system.
         *
         * `CFontOgreNext` adapts the engine's `Font` interface to an `Ogre::FontPtr`.
         * It is responsible for loading font data from the resource system, applying
         * properties, and providing access to any child resources owned by the font.
         */
        class CFontOgreNext : public Font
        {
        public:
            /**
             * \brief Default constructor.
             *
             * Constructs an empty font wrapper. The underlying Ogre font resource is
             * not created until `load` is called.
             */
            CFontOgreNext();

            /**
             * \brief Construct and register with a resource manager.
             * \param resourceManager Pointer to the resource manager that owns this font.
             */
            CFontOgreNext( IResourceManager *resourceManager );

            /** \brief Virtual destructor. Releases any held Ogre font resources. */
            ~CFontOgreNext() override;

            /**
             * \brief Load font data from the provided shared object.
             * \param data Shared object containing font resource data or descriptors.
             *
             * This will create or acquire the underlying `Ogre::Font` and apply
             * the stored properties.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * \brief Reload font data from the provided shared object.
             * \param data Shared object containing updated font resource data.
             *
             * Used when the underlying font data changes and must be re-applied
             * without destroying the resource handle.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * \brief Unload the font and free associated resources.
             * \param data Optional shared object associated with the resource.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** \copydoc Font::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** \copydoc Font::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** \copydoc Font::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Handle to the underlying Ogre font resource. */
            Ogre::FontPtr m_font;

            /**
             * \brief Static name extension used to generate unique font names.
             *
             * Incremented for each created font wrapper so that generated resource
             * names do not collide.
             */
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CFont_h__
