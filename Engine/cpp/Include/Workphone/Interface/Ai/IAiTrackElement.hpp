#ifndef ITrackElement_h__
#define ITrackElement_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /**
     * @class IAiTrackElement
     * @brief Interface for an element within an AI track.
     *
     * Defines the basic spatial properties of a track element, such as its center,
     * start and end points, direction, and extents.
     */
    class WPCore_API IAiTrackElement : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for IAiTrackElement.
         */
        ~IAiTrackElement() override;

        /**
         * @brief Gets the center position of the track element.
         * @return The center position as a Vector3.
         */
        virtual Vector3<real_Num> getCenter() const = 0;
        /**
         * @brief Sets the center position of the track element.
         * @param center The new center position.
         */
        virtual void setCenter( const Vector3<real_Num> &center ) = 0;

        /**
         * @brief Gets the start position of the track element.
         * @return The start position as a Vector3.
         */
        virtual Vector3<real_Num> getStart() const = 0;
        /**
         * @brief Sets the start position of the track element.
         * @param start The new start position.
         */
        virtual void setStart( const Vector3<real_Num> &start ) = 0;

        /**
         * @brief Gets the end position of the track element.
         * @return The end position as a Vector3.
         */
        virtual Vector3<real_Num> getEnd() const = 0;
        /**
         * @brief Sets the end position of the track element.
         * @param end The new end position.
         */
        virtual void setEnd( const Vector3<real_Num> &end ) = 0;

        /**
         * @brief Gets the direction vector of the track element.
         * @return The direction vector.
         */
        virtual Vector3<real_Num> getDirection() const = 0;
        /**
         * @brief Sets the direction vector of the track element.
         * @param direction The new direction vector.
         */
        virtual void setDirection( const Vector3<real_Num> &direction ) = 0;

        /**
         * @brief Gets the extents (dimensions) of the track element.
         * @return The extents as a Vector3.
         */
        virtual Vector3<real_Num> getExtents() const = 0;
        /**
         * @brief Sets the extents (dimensions) of the track element.
         * @param extents The new extents.
         */
        virtual void setExtents( const Vector3<real_Num> &extents ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // ITrackElement_h__
