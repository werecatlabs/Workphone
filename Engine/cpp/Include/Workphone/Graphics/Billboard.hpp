#ifndef __WP_Billboard_h__
#define __WP_Billboard_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IBillboard.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Represents a screen-aligned billboard in the scene.
         *
         * A billboard is a lightweight scene object typically used for sprites,
         * particles, or other quads that should face the camera or follow a simple
         * transform. This concrete class implements the IBillboard interface and
         * stores transform, scale, colour and backend render-data used by the
         * render system.
         */
        class WPCore_API Billboard : public IBillboard
        {
        public:
            /**
             * @brief Construct a new Billboard with default transform, scale and colour.
             *
             * Default scale is (1,1,1) and default colour is opaque white (1,1,1,1).
             */
            Billboard();

            /**
             * @brief Destroy the Billboard.
             *
             * Virtual destructor override to allow correct cleanup through interface pointers.
             */
            ~Billboard() override;

            /**
             * @brief Set the world-space position of the billboard.
             * @param position The new position as a 3D vector.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get the world-space position of the billboard.
             * @return The current position as a 3D vector.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Set the orientation of the billboard.
             *
             * Orientation is stored as a quaternion. For typical screen-aligned
             * billboards the orientation may be ignored or constrained by the
             * rendering system to face the camera.
             *
             * @param orientation The new orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            /**
             * @brief Get the stored orientation quaternion.
             * @return The current orientation.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @brief Set the scale (dimensions) of the billboard in local space.
             * @param dimensions Scale vector where each component scales the billboard axes.
             */
            void setScale( const Vector3<real_Num> &dimensions ) override;

            /**
             * @brief Get the billboard's local scale.
             * @return The current scale vector.
             */
            Vector3<real_Num> getScale() const override;

            /**
             * @brief Return a pointer to the render-system-specific transform.
             *
             * This method provides access to any transform object used by the
             * underlying render system (for example an engine-specific matrix or node).
             * The pointer is opaque (void*) and is an implementation detail of the
             * render backend.
             *
             * @return Opaque pointer to the render system transform, or nullptr if none.
             */
            void *_getRenderSystemTransform() const override;

            /**
             * @brief Set the colour (RGBA) applied to the billboard when rendered.
             * @param colour Colour as floating point RGBA (values typically 0.0 - 1.0).
             */
            void setColour( const ColourF &colour ) override;

            /**
             * @brief Get the current display colour for the billboard.
             * @return ColourF representing RGBA colour.
             */
            ColourF getColour() const override;

            /**
             * @brief Get backend render data associated with this billboard.
             *
             * Render data is an opaque pointer used by the render implementation to
             * reference GPU resources, handles or engine-specific objects.
             *
             * @return Opaque pointer to render data, or nullptr if not set.
             */
            void *getRenderData() const override;

            /**
             * @brief Set backend render data for this billboard.
             * @param renderData Opaque pointer managed by the render system.
             */
            void setRenderData( void *renderData ) override;

            /**
             * @brief Retrieve the underlying engine object for this billboard.
             *
             * Some systems require a pointer-to-pointer output parameter so the
             * caller can obtain and use the native object. The method writes the
             * internal object pointer to the supplied location.
             *
             * @param ppObject Pointer to a void* that will receive the object pointer.
             *                 If the billboard has no underlying object, *ppObject will be set to
             * nullptr.
             */
            void _getObject( void **ppObject ) const override;

            /** Class registration macro used by reflection/serialization systems. */
            WP_CLASS_REGISTER_DECL;

        private:
            /** World position of the billboard. */
            Vector3<real_Num> m_position{};

            /** Local orientation (quaternion). Renderers may override for camera-facing behavior. */
            Quaternion<real_Num> m_orientation{};

            /** Local scale/dimensions. Defaults to (1,1,1). */
            Vector3<real_Num> m_scale{ 1, 1, 1 };

            /** Modulation colour applied at render time. Defaults to opaque white. */
            ColourF m_colour{ 1, 1, 1, 1 };

            /**
             * @brief Opaque pointer to renderer-specific data (GPU handles, nodes, etc).
             *
             * Ownership semantics: the render system that sets this pointer is responsible
             * for allocating and freeing the pointed-to resources. This class only stores
             * the pointer and does not assume ownership.
             */
            void *m_renderData{ nullptr };
        };

    }  // namespace render
}  // namespace workphone

#endif  // Billboard_h__
