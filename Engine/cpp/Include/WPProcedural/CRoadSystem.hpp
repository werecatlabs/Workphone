#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <WPProcedural/WPRoadSystem.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CRoadSystem : public IRoadSystem
    {
    public:
        void setSeed( u32 seed ) override;
        real_Num getRoadWidth( RoadClass cls ) override;
        real_Num getSidewalkWidth( RoadClass cls ) override;
        u32 getLaneCount( RoadClass cls ) override;
        RoadSurface getDefaultSurface( RoadClass cls ) override;
        RoadSegmentResult generateSegment( const RoadSegmentSpec &spec ) override;
        IntersectionResult generateIntersection( const IntersectionSpec &spec ) override;
        IntersectionResult buildXCrossing( const IntersectionSpec &spec ) override;
        IntersectionResult buildRoundabout( const IntersectionSpec &spec ) override;
        IntersectionResult buildTJunction( const IntersectionSpec &spec ) override;
        IntersectionResult buildLCorner( const IntersectionSpec &spec ) override;
        RoadMesh buildOldTarmacPatches( const RoadSegmentSpec &spec, real_Num width,
                                        real_Num length ) override;
        RoadMesh buildPotholes( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override;
        RoadMesh buildManholes( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override;
        RoadMesh buildGullyGrates( const RoadSegmentSpec &spec, real_Num width,
                                   real_Num length ) override;
        RoadMesh buildArrows( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override;
        RoadMesh buildPedestrianCrossing( const RoadSegmentSpec &spec, real_Num width,
                                          real_Num length ) override;
        IntersectionResult buildIntersectionApproach( const IntersectionSpec &spec ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        WPRoadSystem mImplementation;
    };
}  // namespace workphone::procedural
