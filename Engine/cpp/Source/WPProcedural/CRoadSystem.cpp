#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadSystem.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadSystem, IRoadSystem );

    workphone::procedural::IntersectionResult CRoadSystem::buildIntersectionApproach(
        const IntersectionSpec &spec )
    {
        return mImplementation.buildIntersectionApproach( spec );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildPedestrianCrossing( const RoadSegmentSpec &spec,
                                                                          real_Num width,
                                                                          real_Num length )
    {
        return mImplementation.buildPedestrianCrossing( spec, width, length );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildArrows( const RoadSegmentSpec &spec,
                                                              real_Num width, real_Num length )
    {
        return mImplementation.buildArrows( spec, width, length );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildGullyGrates( const RoadSegmentSpec &spec,
                                                                   real_Num width, real_Num length )
    {
        return mImplementation.buildGullyGrates( spec, width, length );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildManholes( const RoadSegmentSpec &spec,
                                                                real_Num width, real_Num length )
    {
        return mImplementation.buildManholes( spec, width, length );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildPotholes( const RoadSegmentSpec &spec,
                                                                real_Num width, real_Num length )
    {
        return mImplementation.buildPotholes( spec, width, length );
    }

    workphone::procedural::RoadMesh CRoadSystem::buildOldTarmacPatches( const RoadSegmentSpec &spec,
                                                                        real_Num width, real_Num length )
    {
        return mImplementation.buildOldTarmacPatches( spec, width, length );
    }

    workphone::procedural::IntersectionResult CRoadSystem::buildLCorner( const IntersectionSpec &spec )
    {
        return mImplementation.buildLCorner( spec );
    }

    workphone::procedural::IntersectionResult CRoadSystem::buildTJunction( const IntersectionSpec &spec )
    {
        return mImplementation.buildTJunction( spec );
    }

    workphone::procedural::IntersectionResult CRoadSystem::buildRoundabout(
        const IntersectionSpec &spec )
    {
        return mImplementation.buildRoundabout( spec );
    }

    workphone::procedural::IntersectionResult CRoadSystem::buildXCrossing( const IntersectionSpec &spec )
    {
        return mImplementation.buildXCrossing( spec );
    }

    workphone::procedural::IntersectionResult CRoadSystem::generateIntersection(
        const IntersectionSpec &spec )
    {
        return mImplementation.generateIntersection( spec );
    }

    workphone::procedural::RoadSegmentResult CRoadSystem::generateSegment( const RoadSegmentSpec &spec )
    {
        return mImplementation.generateSegment( spec );
    }

    workphone::procedural::RoadSurface CRoadSystem::getDefaultSurface( RoadClass cls )
    {
        return WPRoadSystem::getDefaultSurface( cls );
    }

    u32 CRoadSystem::getLaneCount( RoadClass cls )
    {
        return WPRoadSystem::getLaneCount( cls );
    }

    workphone::real_Num CRoadSystem::getSidewalkWidth( RoadClass cls )
    {
        return WPRoadSystem::getSidewalkWidth( cls );
    }

    workphone::real_Num CRoadSystem::getRoadWidth( RoadClass cls )
    {
        return WPRoadSystem::getRoadWidth( cls );
    }

    void CRoadSystem::setSeed( u32 seed )
    {
        mImplementation.setSeed( seed );
    }
}  // namespace workphone::procedural
