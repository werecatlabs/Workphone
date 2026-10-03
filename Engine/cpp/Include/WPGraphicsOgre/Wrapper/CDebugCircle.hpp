#ifndef CDebugCircle_h__
#define CDebugCircle_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/DebugCircle.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CDebugCircle
         * @brief Ogre-based implementation of a debug circle for visualization purposes.
         *
         * This class provides a circle rendering primitive using Ogre's ManualObject system.
         * It is typically used for debugging, visualization, or development tools to display
         * circular shapes in 3D space with configurable position, radius, color, and orientation.
         */
        class CDebugCircle : public DebugCircle
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Creates an uninitialized debug circle with default values.
             */
            CDebugCircle();

            /**
             * @brief Parameterized constructor.
             *
             * @param id Unique hash identifier for the debug circle.
             * @param position Initial position of the circle center in 3D space.
             * @param radius Radius of the circle.
             * @param color Color value in RGBA format (packed as u32).
             */
            CDebugCircle( hash_type id, const Vector3<real_Num> &position, real_Num radius, u32 color );

            /**
             * @brief Destructor.
             *
             * Cleans up Ogre resources and releases the manual object and scene node.
             */
            ~CDebugCircle() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the circle geometry and visual representation.
             *
             * Reconstructs the circle mesh if dirty flag is set, applying current
             * position, orientation, radius, and color values.
             */
            void update() override;

            /** @copydoc IDebugLine::getLifeTime */
            f64 getLifeTime() const override;

            /** @copydoc IDebugLine::setLifeTime */
            void setLifeTime( f64 lifeTime ) override;

            /** @copydoc IDebugLine::getMaxLifeTime */
            f64 getMaxLifeTime() const override;

            /** @copydoc IDebugLine::setMaxLifeTime */
            void setMaxLifeTime( f64 maxLifeTime ) override;

            /** @copydoc IDebugLine::isVisible */
            bool isVisible() const override;

            /** @copydoc IDebugLine::setVisible */
            void setVisible( bool visible ) override;

            /**
             * @brief Sets the center position of the circle.
             * @param position New position in 3D world space.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Sets the radius of the circle.
             * @param radius New radius value (must be positive).
             */
            void setRadius( real_Num radius ) override;

            /**
             * @brief Sets the circle color.
             * @param color Color value in packed RGBA format (u32).
             */
            void setColor( u32 color ) override;

            /**
             * @brief Gets the current center position of the circle.
             * @return The circle's position in 3D world space.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Gets the current radius of the circle.
             * @return The circle's radius value.
             */
            real_Num getRadius() const override;

            /**
             * @brief Gets the current color of the circle.
             * @return Color value in packed RGBA format (u32).
             */
            u32 getColor() const override;

            /**
             * @brief Gets the name of the material used for rendering.
             * @return Material name string.
             */
            String getMaterialName() const;

            /**
             * @brief Sets the material to use for rendering the circle.
             * @param materialName Name of the Ogre material to apply.
             */
            void setMaterialName( const String &materialName );

            /** @copydoc IDebugLine::isDirty */
            bool isDirty() const override;

            /** @copydoc IDebugLine::setDirty */
            void setDirty( bool dirty ) override;

            /**
             * @brief Gets the orientation (rotation) of the circle.
             * @return Quaternion representing the circle's orientation in 3D space.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @brief Sets the orientation (rotation) of the circle.
             * @param orientation Quaternion defining the new orientation.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

        private:
            /** Ogre manual object used to render the circle geometry. */
            Ogre::ManualObject *m_manualObject = nullptr;

            /** Ogre scene node that positions and orients the circle in the scene. */
            RawPtr<Ogre::SceneNode> m_sceneNode;

            /** Current lifetime of the circle in seconds. */
            f64 m_lifeTime = 0.0;

            /** Maximum lifetime before the circle should be removed (in seconds). */
            f64 m_maxLifeTime = 1.0;

            /** Radius of the circle. */
            f32 m_radius = 1.0f;

            /** Number of line segments used to approximate the circle (higher = smoother). */
            s32 segments = 36;

            /** Center position of the circle in 3D world space. */
            Vector3<real_Num> m_position;

            /** Orientation/rotation of the circle (default is XZ plane). */
            Quaternion<real_Num> m_orientation;

            /** Circle color in packed RGBA format. */
            u32 m_colour = 0;

            /** Flag indicating if the circle geometry needs to be rebuilt. */
            atomic_bool m_dirty = false;

            /** Flag indicating if the circle should be rendered. */
            atomic_bool m_isVisible = false;

            /** Name of the Ogre material used for rendering. */
            String m_materialName;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CDebugCircle_h__
