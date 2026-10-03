/**
 * @file DebugLine.hpp
 * @brief Declaration of the DebugLine class, an OgreNext implementation of IDebugLine.
 *
 * This header defines a debug line primitive used for temporary visual debugging in the
 * workphone engine. It supports lifetime management, colour and material customization,
 * visibility toggling, and provides thread‑safe access via atomic members.
 */
#ifndef __Render_DebugLine_h__
#define __Render_DebugLine_h__

#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Implementation of a debug line for rendering temporary visual debugging
         * information.
         *
         * DebugLine is a concrete implementation of the IDebugLine interface that provides
         * debug line rendering capabilities. Debug lines are
         * typically used for visual debugging, physics visualization, path display, and other
         * temporary rendering needs in development and runtime debugging scenarios.
         *
         * The class supports:
         * - Timed lifetime management with automatic expiration
         * - 3D positioning and direction vectors
         * - Material and color customization
         * - Visibility control
         * - Thread-safe operations through atomic members
         * - Integration with Ogre's manual object system
         *
         * @note This class is not copyable by design to prevent resource management issues.
         *
         * @see IDebugLine for the interface definition
         * @see CDebugOgreNext for the debug system manager
         *
         * @author workphone
         * @since Engine v1.0
         */
        class WPCore_API DebugLine : public IDebugLine
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the debug line with default values:
             * - Vector: (0, 1, 0) - unit Y direction
             * - Position: (0, 0, 0) - origin
             * - Lifetime: 0.0 seconds
             * - Max lifetime: 1.0 second
             * - Color: 1 (default color)
             * - Visibility: false
             * - Dirty flag: true
             */
            DebugLine();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of Ogre resources and removes the line from the scene.
             */
            ~DebugLine() override;

            /**
             * @brief Loads and initializes the debug line with optional data.
             *
             * Sets up the object, scene node, and other rendering resources
             * required for line visualization.
             *
             * @param data Optional shared object containing initialization parameters
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads and cleans up all allocated resources.
             *
             * Destroys the object, detaches from scene node, and releases
             * all graphics resources associated with this debug line.
             *
             * @param data Optional shared object for cleanup parameters
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the debug line state and handles lifetime management.
             *
             * This method should be called each frame to:
             * - Update the line's lifetime counter
             * - Check if the line has expired and should be hidden
             * - Rebuild the geometry if the dirty flag is set
             * - Update the visual representation in the scene
             *
             * The line automatically becomes invisible when its lifetime exceeds maxLifeTime.
             */
            void update() override;

            /**
             * @brief Gets the direction vector of the debug line.
             *
             * The vector represents the direction and length of the line from its position.
             * The line is drawn from 'position' to 'position + vector'.
             *
             * @return The current direction vector
             */
            Vector3<real_Num> getVector() const override;

            /**
             * @brief Sets the direction vector of the debug line.
             *
             * @param vector The new direction vector. The line will be drawn from the current
             *               position to position + vector.
             */
            void setVector( const Vector3<real_Num> &vector ) override;

            /**
             * @brief Gets the starting position of the debug line.
             *
             * @return The current starting position in world coordinates
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Sets the starting position of the debug line.
             *
             * @param position The new starting position in world coordinates
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Gets the current lifetime of the debug line.
             *
             * The lifetime represents how long the line has been active since creation
             * or since the last reset.
             *
             * @return Current lifetime in seconds
             */
            f64 getLifeTime() const override;

            /**
             * @brief Sets the current lifetime of the debug line.
             *
             * @param lifeTime The new lifetime value in seconds
             */
            void setLifeTime( f64 lifeTime ) override;

            /**
             * @brief Gets the maximum lifetime before the line expires.
             *
             * When the current lifetime exceeds this value, the line will automatically
             * become invisible.
             *
             * @return Maximum lifetime in seconds
             */
            f64 getMaxLifeTime() const override;

            /**
             * @brief Sets the maximum lifetime before the line expires.
             *
             * @param maxLifeTime The new maximum lifetime in seconds. Set to 0 or negative
             *                    for infinite lifetime.
             */
            void setMaxLifeTime( f64 maxLifeTime ) override;

            /**
             * @brief Checks if the debug line is currently visible.
             *
             * @return true if the line is visible in the scene, false otherwise
             */
            bool isVisible() const override;

            /**
             * @brief Sets the visibility state of the debug line.
             *
             * @param visible true to make the line visible, false to hide it
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Gets the name of the material used for line rendering.
             *
             * @return The material name as a string
             */
            String getMaterialName() const;

            /**
             * @brief Sets the name of the material to use for line rendering.
             *
             * @param materialName The name of the material to apply to the line
             */
            void setMaterialName( const String &materialName );

            /**
             * @brief Gets the color of the debug line.
             *
             * @return The current color as a 32-bit unsigned integer (RGBA format)
             */
            u32 getColour() const;

            /**
             * @brief Sets the color of the debug line.
             *
             * @param colour The new color as a 32-bit unsigned integer (RGBA format)
             */
            void setColour( u32 colour );

            /**
             * @brief Checks if the debug line needs to be rebuilt.
             *
             * The dirty flag indicates whether the line geometry needs to be regenerated
             * due to changes in position, vector, color, or other properties.
             *
             * @return true if the line needs rebuilding, false otherwise
             */
            bool isDirty() const;

            /**
             * @brief Sets the dirty flag to indicate if rebuilding is needed.
             *
             * @param dirty true to mark the line as needing rebuilding, false otherwise
             */
            void setDirty( bool dirty );

            /**
             * @brief Locks the object for thread-safe access.
             *
             * Provides exclusive access to the object's state. Must be paired with unlock().
             */
            void lock() override;

            /**
             * @brief Attempts to lock the object without blocking.
             *
             * @return true if the lock was acquired, false if already locked
             */
            bool try_lock() override;

            /**
             * @brief Unlocks the object after exclusive access.
             *
             * Must be called after a successful lock() or try_lock() call.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Direction vector of the line (atomic for thread safety) */
            AtomicObject<Vector3<real_Num>> m_vector = Vector3<real_Num>::unitY();

            /** @brief Starting position of the line (atomic for thread safety) */
            AtomicObject<Vector3<real_Num>> m_position = Vector3<real_Num>::zero();

            /** @brief Current lifetime in seconds (atomic for thread safety) */
            atomic_f64 m_lifeTime = 0.0;

            /** @brief Maximum lifetime before expiration (atomic for thread safety) */
            atomic_f64 m_maxLifeTime = 1.0;

            /** @brief Line color as RGBA value (atomic for thread safety) */
            atomic_u32 m_colour = 1;

            /** @brief Flag indicating if geometry needs rebuilding (atomic for thread safety) */
            atomic_bool m_isDirty = true;

            /** @brief Flag indicating if the line is visible (atomic for thread safety) */
            atomic_bool m_isVisible = false;

            /** @brief Name of the material used for rendering */
            FixedString<WP_MAX_PATH> m_materialName;

            /** @brief Static counter for generating unique object names */
            static u32 m_nameExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CDebugLine_h__
