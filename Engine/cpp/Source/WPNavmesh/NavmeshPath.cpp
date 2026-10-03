#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/NavmeshPath.hpp"
#include <cmath>

namespace workphone
{

    //=============================================================================
    // Path::Segment
    //=============================================================================

    NavPath::Segment::Segment()
    {
    }

    NavPath::Segment::Segment( const WPCore::Vector3 &start, const WPCore::Vector3 &end ) :
        m_start( start ),
        m_end( end ),
        m_startTangent( ( end - start ).GetNormalized() ),
        m_endTangent( m_startTangent ),
        m_cp0( start ),
        m_cp1( end ),
        m_isBezier( false )
    {
        m_length = ( end - start ).Length();
    }

    NavPath::Segment::Segment( const WPCore::Vector3 &start, const WPCore::Vector3 &end,
                            const WPCore::Vector3 &cp0, const WPCore::Vector3 &cp1 ) :
        m_start( start ),
        m_end( end ),
        m_cp0( cp0 ),
        m_cp1( cp1 ),
        m_isBezier( true )
    {
        m_startTangent = ( cp0 - start ).GetNormalized();
        m_endTangent = ( end - cp1 ).GetNormalized();
        m_length = ( end - start ).Length();
        BuildPolyline( *this );
    }

    void NavPath::Segment::BuildPolyline( Segment &segment )
    {
        if( !segment.m_isBezier )
        {
            // For straight segments, just create two points
            segment.m_polyline.AddPoint( segment.m_start, 0.0f );
            segment.m_polyline.AddPoint( segment.m_end, segment.m_length );
            return;
        }

        // For bezier curves, create subdivided polyline
        const int32_t numSubdivisions = s_polylineSubdivisions;
        float step = 1.0f / numSubdivisions;

        segment.m_polyline.Clear();
        segment.m_polyline.AddPoint( segment.m_start, 0.0f );

        for( int32_t i = 1; i < numSubdivisions; ++i )
        {
            float t = i * step;
            float t2 = t * t;
            float t3 = t2 * t;
            float invT = 1.0f - t;
            float invT2 = invT * invT;
            float invT3 = invT2 * invT;

            WPCore::Vector3 point = invT3 * segment.m_start + 3.0f * invT2 * t * segment.m_cp0 +
                                    3.0f * invT * t2 * segment.m_cp1 + t3 * segment.m_end;

            float dist = t * segment.m_length;
            segment.m_polyline.AddPoint( point, dist );
        }

        segment.m_polyline.AddPoint( segment.m_end, segment.m_length );
    }

    //=============================================================================
    // Path
    //=============================================================================

    Path::Path() : m_isValid( false )
    {
    }

    Path::Path( const WPCore::Vector3 &start, const WPCore::Vector3 &end ) :
        m_start( start ),
        m_end( end ),
        m_isValid( false )
    {
        // Create single straight segment
        Segment segment( start, end );
        segment.m_distanceAlongPath = 0.0f;
        m_segments.Add( segment );
        m_length = segment.m_length;
        m_isValid = true;
    }

    Path::~Path()
    {
        m_segments.Clear();
        m_debugPoints.Clear();
    }

    void Path::PerformSmoothing()
    {
        // Simplified smoothing - in a full implementation, this would use
        // the NavPower library to smooth the path while keeping it on the navmesh
        // For now, the raw path is used directly
    }

    //=============================================================================
    // PathFollower
    //=============================================================================

    PathFollower::PathFollower() : m_pPath( nullptr ), m_segmentIndex( -1 )
    {
    }

    PathFollower::PathFollower( Path *pPath )
    {
        FollowPath( pPath, 0.0f );
    }

    void PathFollower::Clear()
    {
        *this = PathFollower();
    }

    void PathFollower::FollowPath( Path *pPath, float startDistanceAlongPath )
    {
        m_pPath = pPath;
        m_distanceTravelled = startDistanceAlongPath;

        if( !IsValid() )
        {
            return;
        }

        // Find starting segment
        m_segmentIndex = 0;
        m_distanceAlongSegment = 0.0f;
        m_curveSegmentIndex = 0;

        // Find segment containing the starting distance
        float accumulatedDist = 0.0f;
        for( int32_t i = 0; i < pPath->GetNumSegments(); ++i )
        {
            const Path::Segment &seg = pPath->GetSegments()[i];
            if( accumulatedDist + seg.m_length >= startDistanceAlongPath )
            {
                m_segmentIndex = i;
                m_distanceAlongSegment = startDistanceAlongPath - accumulatedDist;
                break;
            }
            accumulatedDist += seg.m_length;
        }

        // Update position
        if( m_segmentIndex >= 0 && m_segmentIndex < pPath->GetNumSegments() )
        {
            const Path::Segment &seg = pPath->GetSegments()[m_segmentIndex];
            float t = ( seg.m_length > 0.0f ) ? ( m_distanceAlongSegment / seg.m_length ) : 0.0f;
            m_currentPosition = WPCore::Vector3::Lerp( seg.m_start, seg.m_end, t );
            m_currentDirection = ( seg.m_end - seg.m_start ).GetNormalized();
        }
    }

    float PathFollower::GetRemainingDistance() const
    {
        if( !IsValid() )
        {
            return 0.0f;
        }
        return m_pPath->GetLength() - m_distanceTravelled;
    }

    bool PathFollower::MoveAlongPath( float distanceToTravel )
    {
        if( !IsValid() || IsAtTheEnd() )
        {
            return true;
        }

        float remaining = distanceToTravel;

        while( remaining > 0.0f && !IsAtTheEnd() )
        {
            const Path::Segment *pSegment = &m_pPath->GetSegments()[m_segmentIndex];
            float distanceOnSegment = pSegment->m_length - m_distanceAlongSegment;

            if( remaining >= distanceOnSegment )
            {
                // Move to end of segment
                remaining -= distanceOnSegment;
                m_distanceTravelled += distanceOnSegment;

                // Move to next segment
                m_segmentIndex++;
                m_distanceAlongSegment = 0.0f;

                if( m_segmentIndex >= m_pPath->GetNumSegments() )
                {
                    // Reached the end
                    m_distanceTravelled = m_pPath->GetLength();
                    m_currentPosition = m_pPath->GetEndPoint();
                    return true;
                }
            }
            else
            {
                // Partial move on current segment
                MoveAlongSegment( pSegment, remaining );
                m_distanceTravelled += remaining;
                return false;
            }
        }

        return IsAtTheEnd();
    }

    void PathFollower::MoveAlongSegment( const Path::Segment *pSegment, float distanceToTravel )
    {
        m_distanceAlongSegment += distanceToTravel;

        // Clamp to segment length
        if( m_distanceAlongSegment > pSegment->m_length )
        {
            m_distanceAlongSegment = pSegment->m_length;
        }

        UpdatePositionFromSegment( pSegment, m_distanceAlongSegment / pSegment->m_length );
    }

    void PathFollower::UpdatePositionFromSegment( const Path::Segment *pSegment, float t )
    {
        t = WPCore::Math::Clamp( t, 0.0f, 1.0f );

        if( !pSegment->m_isBezier )
        {
            m_currentPosition = WPCore::Vector3::Lerp( pSegment->m_start, pSegment->m_end, t );
            m_currentDirection = ( pSegment->m_end - pSegment->m_start ).GetNormalized();
        }
        else
        {
            // Bezier interpolation
            float invT = 1.0f - t;
            m_currentPosition = invT * invT * invT * pSegment->m_start +
                                3.0f * invT * invT * t * pSegment->m_cp0 +
                                3.0f * invT * t * t * pSegment->m_cp1 + t * t * t * pSegment->m_end;

            // Calculate tangent
            WPCore::Vector3 tangent = 3.0f * invT * invT * ( pSegment->m_cp0 - pSegment->m_start ) +
                                      6.0f * invT * t * ( pSegment->m_cp1 - pSegment->m_cp0 ) +
                                      3.0f * t * t * ( pSegment->m_end - pSegment->m_cp1 );
            m_currentDirection = tangent.GetNormalized();
        }
    }

}  // namespace workphone
