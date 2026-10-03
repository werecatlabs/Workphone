#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CBlockGenerator.hpp"
#include <WPProcedural/CCityBlock.hpp>
#include <Workphone/Workphone.hpp>
//#include "Wm5PlanarGraph.hpp"

namespace workphone
{
    namespace procedural
    {
        CBlockGenerator::CBlockGenerator()
        {
            // m_blockPropGrp.AddProperty("Name", "CCityBlock");
            // m_blockPropGrp.AddProperty("Type", "CCityBlock");

            // m_blockNodePropGrp.AddProperty("Name", "BlockNode");
            // m_blockNodePropGrp.AddProperty("Type", "BlockNode");
        }

        CBlockGenerator::~CBlockGenerator()
        {
        }

        void CBlockGenerator::load( SmartPtr<ISharedObject> object )
        {
            //try
            //{
            //    auto data = fb::static_pointer_cast<IData>( object );

            //    auto city = getCity();
            //    auto roadNetwork = city->getRoadNetwork();

            //    auto osmData = data->getDataAsType<data::osm>();
            //    for( auto &n : osmData->node )
            //    {
            //        if( n.id == "4200057916" )
            //        {
            //            int stop = 0;
            //            stop = 0;
            //        }

            //        if( OSMUtil::isBuilding( data, n.id ) )
            //        {
            //            auto block = fb::make_ptr<CCityBlock>();

            //            auto nodeId = n.id;
            //            auto refNode = OSMUtil::getNodeFromDataById( data, nodeId );

            //            block->setName( refNode.id );
            //            OSMUtil::getProperties( data, block->getProperties(), nodeId );

            //            auto relLAtLong = city->getRelativeCoordinates(
            //                Vector2<real_Num>( refNode.lat, refNode.lon ) );

            //            // auto point = Vector3<real_Num>::fromCoords(1000.0, relLAtLong.X(),
            //            // relLAtLong.Y());
            //            ////point.Y() = real_Num(0.0); // for debug
            //            auto point = Vector3<real_Num>( relLAtLong.X(), 0.0f, relLAtLong.Y() );
            //            block->setPosition( point );

            //            city->addBlock( block );
            //        }
            //    }
            //}
            //catch( std::exception &e )
            //{
            //    WP_LOG_EXCEPTION( e );
            //}
        }

        void CBlockGenerator::unload( SmartPtr<ISharedObject> object )
        {
            try
            {
                m_selectedLayer = nullptr;
                m_city = nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CBlockGenerator::generate()
        {
            WP_ASSERT( m_city );
            generateBlocks2();
        }

        SmartPtr<IProceduralCity> CBlockGenerator::getCity() const
        {
            return m_city;
        }

        void CBlockGenerator::setCity( SmartPtr<IProceduralCity> city )
        {
            m_city = city;
        }

        void CBlockGenerator::generateBlocks( SmartPtr<scene::IGameActor> cityLayer )
        {
            m_selectedLayer = cityLayer;
            // generateBlocks();
            generateBlocks2();
        }

        void CBlockGenerator::testMemoryUsage()
        {
        }

        void CBlockGenerator::generateBlocks()
        {
            // CityLayerPtr city = m_selectedLayer;

            // VisitedNodes visitedNodes;

            // Array<SmartPtr<IGameActor>> roadNodes = city->getRoadNodes();

            // u32 numRoadNodes = roadNodes.size();
            // visitedNodes.reallocate(numRoadNodes);
            // for ( u32 i = 0; i < numRoadNodes; ++i )
            //{
            //	RoadNodePtr roadNode = roadNodes[i];
            //	const String& roadNodeName = roadNode->GetName();

            //	if ( roadNodeName.equals_ignore_case("RoadNode309") )
            //	{
            //		int halt = 0;
            //		halt = 0;
            //	}
            //	else
            //	{
            //		//continue;
            //	}

            //	if ( !visitedNodes.hasElement((u32)roadNode.getPtr()) )
            //	{
            //		visitedNodes.push_back((u32)roadNode.getPtr());
            //		//calculateBlock(roadNode, visitedNodes);
            //		calculateBlock2(roadNode, visitedNodes);
            //	}
            //}
        }

        void CBlockGenerator::generateBlocks2()
        {
            auto city = getCity();
            auto roadNetwork = city->getRoadNetwork();

            auto roadNodes = roadNetwork->getMergedNodes();
            if( roadNodes.empty() )
            {
            }

            /*
            Wm5::PlanarGraph<Vector2<real_Num>> planarGraph;

            u32 graphId = 0;
            u32 numRoadNodes = roadNodes.size();
            for (auto roadNode : roadNodes)
            {
                auto connentRoadNodes = roadNode->getConnectedNodes();
                //WP_ASSERT(connentRoadNodes.empty() == false);

                if (connentRoadNodes.empty() == false)
                {
                    auto roadNodeTransform = roadNode->getWorldTransform();
                    Vector3<real_Num> position3d = roadNodeTransform->getPosition();

                    planarGraph.InsertVertex(Vector2<real_Num>(position3d.X(), position3d.Z()), graphId);
                }

                roadNode->setGraphId(graphId);
                graphId++;
            }

            for ( auto roadNode : roadNodes)
            {
                auto roadNodeId = roadNode->getGraphId();
                WP_ASSERT(roadNodeId != -1);

                auto connentedRoadNodes = roadNode->getConnectedNodes();
                for (auto connectedRoadNode : connentedRoadNodes)
                {
                    WP_ASSERT(connectedRoadNode);

                    auto connectedRoadNodeId = connectedRoadNode->getGraphId();
                    if (connectedRoadNodeId != -1)
                    {
                        planarGraph.InsertEdge(roadNodeId, connectedRoadNodeId);
                    }
                }
            }

            Array<Wm5::PlanarGraph<Vector2<real_Num>>::Primitive*> primitives;
            planarGraph.ExtractPrimitives(primitives);
            u32 numPrimitives = primitives.size();

            for ( u32 i = 0; i < numPrimitives; ++i )
            {
                Wm5::PlanarGraph<Vector2<real_Num>>::Primitive* primitive = primitives[i];

                auto block = fb::make_ptr<CCityBlock>();

                u32 numPoints = primitive->Sequence.size();


                for ( u32 pointIdx = 0; pointIdx < numPoints; ++pointIdx )
                {
                    Vector2<real_Num> point = primitive->Sequence[pointIdx].first;

                    Vector3<real_Num> intersectionPoint = Vector3<real_Num>(point.X(), 0.0f, point.Y());
                    //TerrainManager::getSingletonPtr()->getHeightAtWorldPosition(intersectionPoint,
            intersectionPoint.Y()); block->addPoint(intersectionPoint);
                }

                city->addBlock(block);
            }
            */
        }

        void CBlockGenerator::calculateBlock2( const SmartPtr<scene::IGameActor> originalNode,
                                               Set<u32> &visitedNodes )
        {
            // SmartPtr<IEntityManager>& entityManager =
            // IApplicationManager::instance()->getEntityManager();

            // CityLayerPtr city = m_selectedLayer;
            // RoadNodePtr roadNode = originalNode;

            // Array<String> connentRoadNodes = roadNode->getConnectedNodes();
            // if ( connentRoadNodes.empty() )
            //{
            //	return;
            // }

            ///*if(connentRoadNodes.size() <= 2)
            //{
            // return;
            //}*/

            // VisitedNodes connectedVisitedNodes;
            // connectedVisitedNodes.reallocate(50);

            // for ( u32 nodeNameIdx = 0; nodeNameIdx < connentRoadNodes.size(); ++nodeNameIdx )
            //{
            //	const String& nodeName = connentRoadNodes[nodeNameIdx];
            //	RoadNodePtr connectedRoadNode = entityManager->findEntity(nodeName);

            //	if ( nodeName.equals_ignore_case("RoadNode256") )
            //	{
            //		int halt = 0;
            //		halt = 0;
            //	}

            //	if ( visitedNodes.hasElement((u32)connectedRoadNode.getPtr()) )
            //	{
            //		continue;
            //	}
            //	else
            //	{
            //		//visitedNodes.push_back((u32)connectedRoadNode.getPtr());
            //	}

            //	BlockEntPtr block = entityManager->CreateEntity(m_blockPropGrp);
            //	block->addPoint(originalNode->getPosition());

            //	connectedVisitedNodes.set_used(0);
            //	connectedVisitedNodes.push_back((u32)originalNode.getPtr());
            //	BlockInfo blockInfo = findNodesInBlock(block, originalNode, connectedRoadNode,
            // connectedVisitedNodes);

            //	//brute force check if this block matches an existing block
            //	/*bool isExistingBlock = false;
            //	u32 numBlocks = blocks.size();
            //	for(u32 blockIdx=0; blockIdx<numBlocks; ++blockIdx)
            //	{
            //	BlockEntPtr curBlock = blocks[blockIdx];
            //	if(curBlock->equals(block))
            //	{
            //	isExistingBlock = true;
            //	break;
            //	}
            //	}*/

            //	/*if(isExistingBlock)
            //	{
            //	continue;
            //	}*/

            //	if ( blockInfo.IsClosedBlock )
            //		//if(true)
            //	{
            //		city->AddChild(block);
            //	}
            //	else
            //	{
            //		//entityManager->RemoveEntity(block);
            //		continue;
            //	}

            //	/*for(iter = connectedVisitedNodes.begin(); iter != connectedVisitedNodes.end(); iter++)
            //	{
            //	RoadNodePtr connectedVisitedNode = iter->first;
            //	visitedNodes[connectedVisitedNode.getPtr()] = iter->second;
            //	}*/

            //	block->updateBlock();

            //	//return;
            //}
        }

        CBlockGenerator::BlockInfo CBlockGenerator::findNodesInBlock(
            SmartPtr<ICityBlock> block, SmartPtr<scene::IGameActor> originalNode,
            SmartPtr<scene::IGameActor> connectedNode, Set<u32> &visitedNodes )
        {
            BlockInfo blockInfo;

            // RoadNodePtr prevRoadNode = originalNode;
            // RoadNodePtr roadNode = connectedNode;

            // CityLayerPtr city = m_selectedLayer;

            // const String& nodeName = roadNode->GetName();
            // const String& originalNodeName = originalNode->GetName();

            // bool blockFinished = false;
            // while ( !blockFinished )
            //{
            //	const String& nodeName = roadNode->GetName();
            //	Array<String> connentNodeNames = roadNode->getConnectedNodes();

            //	Vector3F originalNodePosition = originalNode->getPosition();
            //	Vector3F prevRoadNodePosition = prevRoadNode->getPosition();
            //	Vector3F curRoadNodePosition = roadNode->getPosition();

            //	block->addPoint(curRoadNodePosition);

            //	if ( !visitedNodes.hasElement((u32)roadNode.getPtr()) )
            //	{
            //		visitedNodes.push_back((u32)roadNode.getPtr());
            //	}

            //	f32 curAngle = 0.f;

            //	Vector3F direction0 = (curRoadNodePosition - prevRoadNodePosition).normaliseCopy();
            //	Vector3F direction2 = (originalNodePosition - curRoadNodePosition).normaliseCopy();

            //	Vector2F directionVector3;
            //	directionVector3.X() = direction2.X() - direction0.X();
            //	directionVector3.Y() = direction2.Z() - direction0.Z();
            //	directionVector3.normalise();

            //	f32 curAngleToOriginalNode = MathF::Atan2(directionVector3.X(), directionVector3.Y());

            //	RoadNodePtr nextConnectedNode;
            //	bool bNodeSet = false;
            //	bool isConnectToOriginal = false;

            //	for ( u32 nodeNameIdx0 = 0; nodeNameIdx0 < connentNodeNames.size(); ++nodeNameIdx0 )
            //	{
            //		const String& connectedNodeName = connentNodeNames[nodeNameIdx0];
            //		RoadNodePtr connectedRoadNode = entityManager->findEntity(connectedNodeName);
            //		if ( connectedRoadNode.isNull() )
            //			continue;

            //		Vector3F curConnectedNodePosition = connectedRoadNode->getPosition();

            //		Vector3F vectorToNode = (curConnectedNodePosition -
            // curRoadNodePosition).normaliseCopy();

            //		Vector2F directionVector;
            //		directionVector.X() = vectorToNode.X();
            //		directionVector.Y() = vectorToNode.Z();
            //		directionVector.normalise();

            //		if ( connectedNodeName.equals_ignore_case(originalNodeName) )
            //		{

            //		}

            //		if ( nodeName.equals_ignore_case("RoadNode19") )
            //		{
            //			int halt = 0;
            //			halt = 0;
            //		}

            //		if ( connectedNodeName.equals_ignore_case("RoadNode19") )
            //		{
            //			int halt = 0;
            //			halt = 0;
            //		}

            //		if ( connectedNodeName.equals_ignore_case("RoadNode1") )
            //		{
            //			int halt = 0;
            //			halt = 0;
            //		}

            //		if ( visitedNodes.hasElement((u32)connectedRoadNode.getPtr()) )
            //		{
            //			if ( originalNode == connectedRoadNode && originalNode != prevRoadNode )
            //			{
            //				isConnectToOriginal = true;
            //			}

            //			continue;
            //		}

            //		//Vector3F dirToOriginal = (originalNodePosition - curRoadNodePosition);
            //		//f32 distanceToOriginal = dirToOriginal.normaliseLength();
            //		//f32 angleToOriginal = MathF::Atan2(dirToOriginal.X(),dirToOriginal.Z());

            //		//f32 directionAngle0 = MathF::Atan2(direction0.X(),direction0.Z());
            //		//f32 directionAngle1 = MathF::Atan2(direction1.X(),direction1.Z());
            //		f32 directionAngle0 = MathF::Atan2(vectorToNode.X(), vectorToNode.Z());
            //		f32 directionAngle = directionAngle0 - curAngleToOriginalNode;

            //		f32 angleDegrees = MathF::RadToFullDegrees(directionAngle);

            //		if ( !bNodeSet || fabs(curAngle) > fabs(angleDegrees) )
            //		{
            //			curAngle = fabs(angleDegrees);
            //			nextConnectedNode = connectedRoadNode;
            //			bNodeSet = true;
            //		}
            //	}

            //	if ( !nextConnectedNode.isNull() )
            //	{
            //		String nextNodeName = nextConnectedNode->GetName();
            //		if ( nextNodeName.equals_ignore_case("RoadNode45") )
            //		{
            //			int halt = 0;
            //			halt = 0;
            //		}

            //		prevRoadNode = roadNode;
            //		roadNode = nextConnectedNode;

            //		if ( isConnectToOriginal )
            //			blockFinished = true;
            //	}
            //	else
            //	{
            //		//check if the current node is connected to the original node
            //		blockInfo.IsClosedBlock = isConnectToOriginal;
            //		blockFinished = true;
            //	}
            //}

            int halt = 0;
            halt = 0;

            /*if(nextConnectedNode.isNull())
            {
            }*/

            return blockInfo;
        }
    }  // namespace procedural
}  // namespace workphone
