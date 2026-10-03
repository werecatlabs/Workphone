#ifndef __Render_DebugCircle_h__
#define __Render_DebugCircle_h__

#include <Workphone/Interface/Graphics/IDebugCircle.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Lightweight debug representation of a 3D circle.
         *
         * The DebugCircle provides a simple, non-physical visual aid for debugging:
         * - position, orientation and radius define the circle in world space
         * - a color and material name can be provided
         * - lifetime controls automatic fading / removal (managed by owning debug system)
         *
         * It is intended to be cheap and used only for visualization of runtime data.
         */
        class WPCore_API DebugCircle : public IDebugCircle
        {
        public:
            /**
             * @brief Construct a DebugCircle with default values.
             *
             * Defaults:
             * - lifetime = 0.0
             * - max lifetime = 1.0
             * - radius = 1.0
             * - segments = 36
             * - colour = 0
             */
            DebugCircle();

            /**
             * @brief Virtual destructor.
             */
            ~DebugCircle() override;

            /**
             * @copydoc ISharedObject::load
             *
             * Accepts optional data used by the concrete implementation to create GPU resources
             * or other render-side state required by the debug primitive.
             *
             * @param data Optional shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Releases any resources acquired in load().
             *
             * @param data Optional shared object used for unloading semantics.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update per-frame logic for the debug circle.
             *
             * Typical responsibilities:
             * - decrement lifetime
             * - update GPU-side buffers if the circle is dirty
             * - notify visibility changes
             */
            void update() override;

            /**
             * @copydoc IDebugLine::getLifeTime
             * @return Current remaining lifetime in seconds.
             */
            f64 getLifeTime() const override;

            /**
             * @copydoc IDebugLine::setLifeTime
             * @param lifeTime New remaining lifetime in seconds.
             */
            void setLifeTime( f64 lifeTime ) override;

            /**
             * @copydoc IDebugLine::getMaxLifeTime
             * @return Maximum lifetime (used for normalization, fading, etc.).
             */
            f64 getMaxLifeTime() const override;

            /**
             * @copydoc IDebugLine::setMaxLifeTime
             * @param maxLifeTime New maximum lifetime value in seconds.
             */
            void setMaxLifeTime( f64 maxLifeTime ) override;

            /**
             * @copydoc IDebugLine::isVisible
             * @return True when the circle should be rendered.
             */
            bool isVisible() const override;

            /**
             * @copydoc IDebugLine::setVisible
             * @param visible True to make the circle visible.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Set world-space center of the circle.
             * @param position New center position.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Set the circle radius.
             * @param radius Radius in world units.
             */
            void setRadius( real_Num radius ) override;

            /**
             * @brief Set the RGBA color (packed u32).
             * @param color Packed 32-bit color value.
             */
            void setColor( u32 color ) override;

            /**
             * @brief Get world-space center of the circle.
             * @return Circle center position.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Get circle radius.
             * @return Radius in world units.
             */
            real_Num getRadius() const override;

            /**
             * @brief Get packed color value.
             * @return 32-bit packed color (RGBA or engine-defined layout).
             */
            u32 getColor() const override;

            /**
             * @brief Get the material name used when rendering the circle.
             * @return Material name string. Empty if none set.
             */
            String getMaterialName() const;

            /**
             * @brief Set the material name to use for rendering.
             * @param materialName Name of the material/shader to apply.
             */
            void setMaterialName( const String &materialName );

            /**
             * @brief Query whether geometry or GPU state needs updating.
             * @return True when the object is marked dirty and needs synchronization with renderer.
             */
            bool isDirty() const override;

            /**
             * @brief Mark or clear the dirty flag.
             * @param dirty True to mark the object as needing an update.
             */
            void setDirty( bool dirty ) override;

            /**
             * @brief Get the circle's orientation as a quaternion.
             * @return Orientation quaternion in local-to-world space.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @brief Set the circle's orientation.
             * @param orientation New orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

        private:
            /// Circle center position in world space.
            Vector3<real_Num> m_position;

            /// Circle orientation as a quaternion (local -> world).
            Quaternion<real_Num> m_orientation;

            /// Current remaining lifetime in seconds.
            f64 m_lifeTime = 0.0;

            /// Maximum lifetime used for normalization/fading.
            f64 m_maxLifeTime = 1.0;

            /// Circle radius used for rendering (in world units).
            f32 m_radius = 1.0f;

            /**
             * @brief Number of line segments used to approximate the circle.
             *
             * Higher values produce a smoother circle at the cost of more geometry.
             * Default: 36.
             */
            s32 segments = 36;

            /// Packed colour value (engine-defined layout).
            u32 m_colour = 0;

            /// When true, the renderable needs to be rebuilt / synced.
            atomic_bool m_dirty = false;

            /// Visibility flag used by the debug rendering system.
            atomic_bool m_isVisible = false;

            /// Optional material name used to select rendering technique.
            FixedString<WP_MAX_PATH> m_materialName;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CDebugCircle_h__
