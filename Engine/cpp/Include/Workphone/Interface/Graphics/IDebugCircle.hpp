#ifndef IDebugCircle_h__
#define IDebugCircle_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class IDebugCircle
         * @brief Interface for rendering a debug circle in the graphics system.
         *
         * This interface provides methods to manipulate and query the properties of a debug circle,
         * such as its position, orientation, radius, color, visibility, and lifetime. It is primarily
         * used for debugging purposes to visualize circles in the 3D scene.
         */
        class WPCore_API IDebugCircle : public ISharedObject
        {
        public:
            /** @brief Destructor. */
            ~IDebugCircle() override;

            /**
             * @brief Gets the position of the debug circle in the 3D space.
             * @return The position as a 3D vector.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Sets the position of the debug circle in the 3D space.
             * @param position The position as a 3D vector.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the orientation of the debug circle.
             * @return The orientation as a quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Sets the orientation of the debug circle.
             * @param orientation The orientation as a quaternion.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Gets the lifetime of the debug circle.
             * @return The lifetime in seconds.
             */
            virtual time_interval getLifeTime() const = 0;

            /**
             * @brief Sets the lifetime of the debug circle.
             * @param lifeTime The lifetime in seconds.
             */
            virtual void setLifeTime( time_interval lifeTime ) = 0;

            /**
             * @brief Gets the maximum lifetime of the debug circle.
             * @return The maximum lifetime in seconds.
             */
            virtual time_interval getMaxLifeTime() const = 0;

            /**
             * @brief Sets the maximum lifetime of the debug circle.
             * @param maxLifeTime The maximum lifetime in seconds.
             */
            virtual void setMaxLifeTime( time_interval maxLifeTime ) = 0;

            /**
             * @brief Checks if the debug circle is visible.
             * @return True if the debug circle is visible, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Sets the visibility of the debug circle.
             * @param visible True to make the debug circle visible, false to hide it.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Gets the radius of the debug circle.
             * @return The radius of the circle.
             */
            virtual real_Num getRadius() const = 0;

            /**
             * @brief Sets the radius of the debug circle.
             * @param radius The radius of the circle.
             */
            virtual void setRadius( real_Num radius ) = 0;

            /**
             * @brief Gets the color of the debug circle.
             * @return The color as a 32-bit unsigned integer (e.g., ARGB format).
             */
            virtual u32 getColor() const = 0;

            /**
             * @brief Sets the color of the debug circle.
             * @param color The color as a 32-bit unsigned integer (e.g., ARGB format).
             */
            virtual void setColor( u32 color ) = 0;

            /**
             * @brief Checks if the debug circle is marked as dirty.
             *
             * A dirty debug circle indicates that its properties have been modified
             * and need to be updated in the rendering system.
             *
             * @return True if the debug circle is dirty, false otherwise.
             */
            virtual bool isDirty() const = 0;

            /**
             * @brief Marks the debug circle as dirty or clean.
             * @param dirty True to mark the debug circle as dirty, false to mark it as clean.
             */
            virtual void setDirty( bool dirty ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // IDebugCircle_h__
