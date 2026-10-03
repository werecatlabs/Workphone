#ifndef CTrackElement_h__
#define CTrackElement_h__

#include <Workphone/Interface/Ai/IAiTrackElement.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @class AiTrackElement
     * @brief Concrete implementation of an AI track element.
     *
     * This class provides the implementation for the IAiTrackElement interface,
     * storing spatial data for elements within an AI track.
     */
    class WPCore_API AiTrackElement : public IAiTrackElement
    {
    public:
        /**
         * @brief Default constructor.
         */
        AiTrackElement();
        /**
         * @brief Destructor.
         */
        ~AiTrackElement() override;

        /** @brief Implementation of getCenter. */
        Vector3<real_Num> getCenter() const override;

        /** @brief Implementation of setCenter. */
        void setCenter( const Vector3<real_Num> &center ) override;

        /** @brief Implementation of getStart. */
        Vector3<real_Num> getStart() const override;

        /** @brief Implementation of setStart. */
        void setStart( const Vector3<real_Num> &start ) override;

        /** @brief Implementation of getEnd. */
        Vector3<real_Num> getEnd() const override;

        /** @brief Implementation of setEnd. */
        void setEnd( const Vector3<real_Num> &end ) override;

        /** @brief Implementation of getDirection. */
        Vector3<real_Num> getDirection() const override;

        /** @brief Implementation of setDirection. */
        void setDirection( const Vector3<real_Num> &direction ) override;

        /** @brief Implementation of getExtents. */
        Vector3<real_Num> getExtents() const override;

        /** @brief Implementation of setExtents. */
        void setExtents( const Vector3<real_Num> &extents ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector3<real_Num> m_center;     ///< The center position of the track element.
        Vector3<real_Num> m_start;      ///< The start position of the track element.
        Vector3<real_Num> m_end;        ///< The end position of the track element.
        Vector3<real_Num> m_direction;  ///< The direction vector of the track element.
        Vector3<real_Num> m_extents;    ///< The extents (dimensions) of the track element.
    };
}  // namespace workphone

#endif  // CTrackElement_h__
