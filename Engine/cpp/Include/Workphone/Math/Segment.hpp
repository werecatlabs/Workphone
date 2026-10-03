#ifndef __WP_SEGMENT_H_INCLUDED__
#define __WP_SEGMENT_H_INCLUDED__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Plane3.hpp>

namespace workphone
{

    /**
     * @brief A line segment defined by a start point, direction and extent.
     *
     * @tparam T the type of the coordinates (e.g. float, double, int)
     */
    template <class T>
    class WPCore_API Segment
    {
    public:
        /**
         * @brief Creates a new segment with start point, direction, and extent.
         *
         * @param start the start point of the segment
         * @param direction the direction of the segment
         * @param extent the extent (length) of the segment
         */
        Segment( const Vector3<T> &start, const Vector3<T> &direction, T extent );

        /**
         * @brief Returns the start point of the segment.
         *
         * @return the start point of the segment
         */
        Vector3<T> getStart() const;

        /**
         * @brief Sets the start point of the segment.
         *
         * @param start the new start point of the segment
         */
        void setStart( const Vector3<T> &start );

        /**
         * @brief Returns the direction of the segment.
         *
         * @return the direction of the segment
         */
        Vector3<T> getDirection() const;

        /**
         * @brief Sets the direction of the segment.
         *
         * @param direction the new direction of the segment
         */
        void setDirection( const Vector3<T> &direction );

        /**
         * @brief Returns the extent (length) of the segment.
         *
         * @return the extent (length) of the segment
         */
        T getExtent() const;

        /**
         * @brief Sets the extent (length) of the segment.
         *
         * @param extent the new extent (length) of the segment
         */
        void setExtent( T extent );

    private:
        Vector3<T> m_start;      ///< The start point of the segment
        Vector3<T> m_direction;  ///< The direction of the segment (not necessarily normalized)
        T m_extent;              ///< The extent (length) of the segment
    };

    using SegmentI = Segment<s32>;
    using SegmentF = Segment<f32>;
    using SegmentD = Segment<f64>;

}  // namespace workphone

#endif
