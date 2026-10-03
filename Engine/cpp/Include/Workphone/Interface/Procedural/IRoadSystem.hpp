#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/RoadSystemTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IRoadSystem : public ISharedObject
    {
    public:
        ~IRoadSystem() override;
        virtual void setSeed( u32 seed ) = 0;
        virtual real_Num getRoadWidth( RoadClass cls ) = 0;
        virtual real_Num getSidewalkWidth( RoadClass cls ) = 0;
        virtual u32 getLaneCount( RoadClass cls ) = 0;
        virtual RoadSurface getDefaultSurface( RoadClass cls ) = 0;
        virtual RoadSegmentResult generateSegment( const RoadSegmentSpec &spec ) = 0;
        virtual IntersectionResult generateIntersection( const IntersectionSpec &spec ) = 0;
        virtual IntersectionResult buildXCrossing( const IntersectionSpec &spec ) = 0;
        virtual IntersectionResult buildRoundabout( const IntersectionSpec &spec ) = 0;
        virtual IntersectionResult buildTJunction( const IntersectionSpec &spec ) = 0;
        virtual IntersectionResult buildLCorner( const IntersectionSpec &spec ) = 0;
        virtual RoadMesh buildOldTarmacPatches( const RoadSegmentSpec &spec, real_Num width,
                                                real_Num length ) = 0;
        virtual RoadMesh buildPotholes( const RoadSegmentSpec &spec, real_Num width,
                                        real_Num length ) = 0;
        virtual RoadMesh buildManholes( const RoadSegmentSpec &spec, real_Num width,
                                        real_Num length ) = 0;
        virtual RoadMesh buildGullyGrates( const RoadSegmentSpec &spec, real_Num width,
                                           real_Num length ) = 0;
        virtual RoadMesh buildArrows( const RoadSegmentSpec &spec, real_Num width, real_Num length ) = 0;
        virtual RoadMesh buildPedestrianCrossing( const RoadSegmentSpec &spec, real_Num width,
                                                  real_Num length ) = 0;
        virtual IntersectionResult buildIntersectionApproach( const IntersectionSpec &spec ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
