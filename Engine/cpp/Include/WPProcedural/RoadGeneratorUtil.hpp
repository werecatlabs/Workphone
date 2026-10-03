#ifndef __RoadGeneratorUtil_h__
#define __RoadGeneratorUtil_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API StraightRoadData : public ISharedObject
        {
        public:
            StraightRoadData() = default;
            ~StraightRoadData() override = default;

            SmartPtr<IProceduralCity> getCity() const;
            void setCity( SmartPtr<IProceduralCity> value );

            Vector3F getStartPosition() const;
            void setStartPosition( const Vector3F &value );

            Vector3F getStartDirection() const;
            void setStartDirection( const Vector3F &value );

        protected:
            SmartPtr<IProceduralCity> m_city;
            Vector3F m_startPosition;
            Vector3F m_startDirection;
        };

        class HighwayInfo : public ISharedObject
        {
        public:
            HighwayInfo() = default;

            SmartPtr<ICityCenter> CityCenter;
            SmartPtr<IRoad> TargetRoad;
            Vector3F StartPosition;
            Vector3F TargetPosition;
            Vector3F InitialDirection;
        };

        class RadialPatternInfo : public ISharedObject
        {
        public:
            SmartPtr<ICityCenter> CityCenter;
            SmartPtr<IRoad> ConnentedRoad;
            Vector3F StartPos;
            Vector3F StartDir;
        };

        class StraightRoadInfo : public ISharedObject
        {
        public:
            StraightRoadInfo() : MinRoadDistance( 50.0f )
            {
            }

            SmartPtr<ICityCenter> CityCenter;
            SmartPtr<IRoad> ConnentedRoad;
            Vector3F StartPos;
            Vector3F StartDir;

            /// the minimum distance distance this road can be from another
            f32 MinRoadDistance;
        };

        class RoadFinishedInfo : public ISharedObject
        {
        public:
            RoadFinishedInfo() : IsInCityBoundaries( true ), CollidesWithTerrain( false )
            {
            }

            bool IsInCityBoundaries;
            bool CollidesWithTerrain;
            SmartPtr<IRoad> CollidingRoad;
        };

        // class ConnectIntersectingNodes2 : public ISharedObject
        //{
        //	ConnectIntersectingNodes2()
        //		: FinishRoadOnIntersection(true)
        //	{}

        //	SmartPtr<IRoad>			CurrentRoad;
        //	SmartPtr<IRoad>			TargetRoad;
        //	SmartPtr<IRoadNode>		RoadNode;
        //	bool			FinishRoadOnIntersection;

        //	Array<SmartPtr<IRoadNode>> NodesConnected;
        //	Array<SmartPtr<IRoad>>	Roads;
        //};

        class RoadGeneratorUtil : public ISharedObject
        {
        public:
            static SmartPtr<IRoadElement> createRoadSegment( const Vector3F &posA, const Vector3F &posB,
                                                             const Vector3F &tangentA,
                                                             const Vector3F &tangentB, f32 widthA,
                                                             f32 widthB );

            static SmartPtr<IMesh> buildMesh( const Array<SmartPtr<IRoadElement>> &roadSegments );

            static void createMeshSegment( const Vector3F &posA, const Vector3F &posB,
                                           const Vector3F &tangentA, const Vector3F &tangentB,
                                           f32 widthA, f32 widthB, Array<Vector3F> &vertexPositions,
                                           Array<Vector2F> &uvs );

            static SmartPtr<IRoad> createHighway( SmartPtr<HighwayInfo> highwayInfo );
            static SmartPtr<IRoad> createRadialPatternRoad(
                SmartPtr<RadialPatternInfo> radialPatternInfo );
            static SmartPtr<IRoad> createStraightRoad( SmartPtr<StraightRoadInfo> straightRoadInfo );

            static SmartPtr<IRoad> createHighway( SmartPtr<ICityCenter> targetCityCenter,
                                                  const Vector3F &startPos,
                                                  const Vector3F &targetPosition,
                                                  const Vector3F &startDir, SmartPtr<IRoad> targetRoad );

            static SmartPtr<IRoad> createStraightRoad( SmartPtr<StraightRoadData> straightRoadData );
            static SmartPtr<IRoad> createRoad( SmartPtr<IRoad> connentedRoad, Vector3F startPos,
                                               Vector3F startDir );
            static SmartPtr<IRoad> createRingRoad( SmartPtr<ICityCenter> targetCityCenter,
                                                   f32 distanceAlongRoad );
            static SmartPtr<IRoad> createRoundAbout( const Vector3F &centerPosition, f32 radius );
            static SmartPtr<IRoad> createBranch( SmartPtr<IRoad> connentedRoad, Vector3F startPos,
                                                 Vector3F startDir );
            static void createCheckerPattern( SmartPtr<ICityBlock> block,
                                              Array<SmartPtr<IRoad>> &roads );

            static bool isRoadFinished( SmartPtr<RoadFinishedInfo> info );

            static bool isNearExistingRoadNode( const Array<SmartPtr<IRoad>> &roads,
                                                SmartPtr<IRoad> currentRoad, Sphere3F nodeSphere,
                                                SmartPtr<IRoadNode> &roadNode,
                                                SmartPtr<IRoad> &collidingRoad );

            // static bool connectIntersectingNodesFunc(SmartPtr<ConnectIntersectingNodes2> info);
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // RoadGeneratorUtil_h__
