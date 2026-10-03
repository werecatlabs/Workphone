#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Curves.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    class PathFollower;

    /**
     * @class Path
     * @brief Represents a computed navigation path.
     *        Contains path points, segments, and smoothing information.
     */
    class WPNETWORK_API NavPath
    {
        friend class PathFollower;

    public:
        static constexpr float s_pushAwayReductionFactor = 2.0f / 3.0f;
        static constexpr float s_angleThresholdRadians = 10.0f * ( 3.14159265f / 180.0f );
        static constexpr float s_pushAwayDistance = 0.5f;

        /**
         * @struct Segment
         * @brief A single segment of the path.
         */
        struct Segment
        {
            static constexpr int32_t s_polylineSubdivisions = 20;

            WPCore::Vector3 m_start;
            WPCore::Vector3 m_end;
            WPCore::Vector3 m_startTangent;
            WPCore::Vector3 m_endTangent;
            WPCore::Vector3 m_cp0;  // Control point 0 for bezier
            WPCore::Vector3 m_cp1;  // Control point 1 for bezier
            WPCore::Polyline m_polyline;
            float m_length = 0.0f;
            float m_distanceAlongPath = 0.0f;
            bool m_isBezier = false;

            Segment();
            Segment( const WPCore::Vector3 &start, const WPCore::Vector3 &end );
            Segment( const WPCore::Vector3 &start, const WPCore::Vector3 &end,
                     const WPCore::Vector3 &cp0, const WPCore::Vector3 &cp1 );
        };

        /**
         * @struct Point
         * @brief Intermediate path point used for smoothing.
         */
        struct Point
        {
            WPCore::Vector3 m_originalPoint;
            WPCore::Vector3 m_normal;
            WPCore::Vector3 m_pushedPoint;
            float m_pushDistance = 0.0f;
        };

    public:
        NavPath();
        NavPath( const WPCore::Vector3 &start, const WPCore::Vector3 &end );
        virtual ~NavPath();

        // Validity
        bool IsValid() const
        {
            return m_isValid;
        }

        // Accessors
        WPCore::Vector3 GetStartPoint() const
        {
            return m_start;
        }
        WPCore::Vector3 GetEndPoint() const
        {
            return m_end;
        }
        float GetLength() const
        {
            return m_length;
        }
        int32_t GetNumSegments() const
        {
            return m_segments.GetSize();
        }
        const WPCore::Array<Segment> &GetSegments() const
        {
            return m_segments;
        }

        // Raw path points (for debug visualization)
        const WPCore::Array<Point> &GetDebugPoints() const
        {
            return m_debugPoints;
        }

    private:
        void PerformSmoothing();
        void BuildPolyline( Segment &segment );

    private:
        bool m_isValid = false;
        WPCore::Vector3 m_start;
        WPCore::Vector3 m_end;
        float m_length = 0.0f;
        WPCore::Array<Segment> m_segments;
        WPCore::Array<Point> m_debugPoints;  // For debugging
    };

    /**
     * @class PathFollower
     * @brief Follows a computed path, providing position and direction along the path.
     */
    class WPNETWORK_API PathFollower
    {
    public:
        PathFollower();
        PathFollower( NavPath *pPath );

        // Validity
        bool IsValid() const
        {
            return m_pPath != nullptr && m_pPath->IsValid();
        }
        void Clear();

        // Path management
        void FollowPath( NavPath *pPath, float startDistanceAlongPath = 0.0f );

        // Progress
        bool IsAtTheEnd() const
        {
            return GetRemainingDistance() == 0.0f;
        }
        float GetTravelledDistance() const
        {
            return m_distanceTravelled;
        }
        float GetRemainingDistance() const;

        // Movement
        // Move distance along the path, clamped to end. Returns true if reached end.
        bool MoveAlongPath( float distanceToTravel );

        // Position and direction
        WPCore::Vector3 GetPosition() const
        {
            return m_currentPosition;
        }
        WPCore::Vector3 GetDirection() const
        {
            return m_currentDirection;
        }

        // Segment info
        int32_t GetCurrentSegmentIndex() const
        {
            return m_segmentIndex;
        }
        float GetDistanceAlongCurrentSegment() const
        {
            return m_distanceAlongSegment;
        }

    private:
        void MoveAlongSegment( const NavPath::Segment *pSegment, float distanceToTravel );
        void UpdatePositionFromSegment( const NavPath::Segment *pSegment, float t );

    private:
        NavPath *m_pPath = nullptr;
        float m_distanceTravelled = 0.0f;
        WPCore::Vector3 m_currentPosition = WPCore::Vector3::Zero;
        WPCore::Vector3 m_currentDirection = WPCore::Vector3::Forward;
        int32_t m_segmentIndex = -1;
        int32_t m_curveSegmentIndex = -1;
        float m_distanceAlongSegment = 0.0f;
    };

}  // namespace workphone
