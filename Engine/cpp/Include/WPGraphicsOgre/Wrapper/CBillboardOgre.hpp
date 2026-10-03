#ifndef _CBillboard_H
#define _CBillboard_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IBillboard.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Ogre wrapper for a billboard object.
         *
         * This class implements the engine's `IBillboard` interface and holds
         * a pointer to an underlying Ogre `Billboard` instance. It provides
         * simple setters/getters for transform, colour, dimensions and
         * render-specific data used by the renderer.
         *
         * Lifetime: the wrapper does not implicitly create or destroy the
         * underlying Ogre billboard unless explicitly managed through the
         * engine's higher-level systems; use `initialise` to attach a native
         * billboard pointer and `unload`/`load` for lifecycle hooks.
         */
        class CBillboardOgre : public IBillboard
        {
        public:
            /**
             * @brief Construct a new CBillboardOgre wrapper.
             *
             * Initializes internal state; no native billboard is attached.
             */
            CBillboardOgre();

            /**
             * @brief Destroy the CBillboardOgre wrapper.
             *
             * Does not assume ownership of the underlying Ogre::Billboard pointer
             * unless documented elsewhere; cleanup should be handled by the
             * owning renderer/manager.
             */
            ~CBillboardOgre() override;

            /**
             * @brief Called when the object is loaded by the engine.
             *
             * @param data Optional shared data object provided by the loader.
             *
             * Typical implementations perform resource acquisition or register
             * the billboard with renderer subsystems here.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called when the object is unloaded by the engine.
             *
             * @param data Optional shared data object provided by the loader.
             *
             * Typical implementations perform resource release or
             * de-registration from rendering subsystems here.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Attach an existing native Ogre billboard to this wrapper.
             *
             * @param bb Pointer to an Ogre::Billboard instance to attach.
             *
             * After calling this the wrapper will relay operations to the native
             * billboard. The caller remains responsible for the native object's
             * lifetime unless otherwise specified by the manager using this wrapper.
             */
            void initialise( Ogre::Billboard *bb );

            /**
             * @brief Set the world-space position of the billboard.
             *
             * @param position 3D position to set.
             */
            void setPosition( const Vector3F &position ) override;

            /**
             * @brief Get the world-space position of the billboard.
             *
             * @return Vector3F Current position.
             */
            Vector3F getPosition() const override;

            /**
             * @brief Set the 2D dimensions (width, height) of the billboard.
             *
             * @param dimensions Width and height in world or local units (as used by the renderer).
             */
            void setDimensions( const Vector2F &dimensions );

            /**
             * @brief Get the 2D dimensions (width, height) of the billboard.
             *
             * @return Vector2F Current dimensions.
             */
            Vector2F getDimensions() const;

            /**
             * @brief Set the vertex colour / tint of the billboard.
             *
             * @param colour Colour to apply.
             *
             * This typically affects vertex colours or material parameters on the native billboard.
             */
            void setColour( const ColourF &colour ) override;

            /**
             * @brief Get the currently applied colour / tint.
             *
             * @return ColourF Current colour.
             */
            ColourF getColour() const override;

            /**
             * @brief Retrieve the underlying native object pointer.
             *
             * @param ppObject Output pointer location that will receive the native pointer.
             *
             * If `ppObject` is non-null, this method will write a raw pointer to the native
             * VGA/Ogre object (typically `Ogre::Billboard*`). The exact concrete type is
             * renderer-dependent.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Utility used to test whether an element slot is free.
             *
             * @param element Pointer to a slot or element representation.
             * @return true if the provided element should be considered free/unused.
             *
             * The semantics of "free" depend on the calling code (object pools, arrays, etc.).
             */
            static bool IsFree( void *element );

            /**
             * @brief Set the billboard orientation.
             *
             * @param orientation Quaternion representing rotation.
             *
             * Note: Many billboards are camera-facing; orientation may be ignored
             * or combined with billboarding logic by the renderer.
             */
            void setOrientation( const QuaternionF &orientation ) override;

            /**
             * @brief Get the billboard orientation.
             *
             * @return QuaternionF Current orientation.
             */
            QuaternionF getOrientation() const override;

            /**
             * @brief Set a non-uniform scale for the billboard.
             *
             * @param dimensions Scale along each axis.
             */
            void setScale( const Vector3F &dimensions ) override;

            /**
             * @brief Get the current scale applied to the billboard.
             *
             * @return Vector3F Current scale.
             */
            Vector3F getScale() const override;

            /**
             * @brief Get a pointer to the render-system-specific transform.
             *
             * @return void* Pointer to the native transform data used by the renderer, or nullptr.
             *
             * Ownership and exact type are renderer-specific.
             */
            void *_getRenderSystemTransform() const override;

            /**
             * @brief Get arbitrary render-related user data associated with this billboard.
             *
             * @return void* Opaque pointer previously set via `setRenderData`.
             */
            void *getRenderData() const override;

            /**
             * @brief Set arbitrary render-related user data.
             *
             * @param renderData Opaque pointer the renderer or caller can use to associate data with this billboard.
             *
             * No ownership semantics are implied by this API; the caller is responsible for the lifetime
             * of the pointed-to data unless documented elsewhere.
             */
            void setRenderData( void *renderData ) override;

        protected:
            /** @brief Native Ogre billboard pointer attached to this wrapper (may be null). */
            Ogre::Billboard *m_bb = nullptr;

            /** @brief Cached position of the billboard in world space. */
            Vector3F m_position;
        };
    }  // end namespace render
}  // namespace workphone

#endif
