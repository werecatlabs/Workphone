#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/BlockNode.hpp>
#include <Workphone/Interface/Procedural/ILot.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <iostream>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;

void check( bool condition, const char *message )
{
    if( !condition )
    {
        throw std::runtime_error( message );
    }
}

void checkBlockContracts()
{
    auto block = make_ptr<CCityBlock>();
    const Vector3<real_Num> a( 10, 2, 10 ), b( 20, 4, 10 ), c( 20, 6, 20 );
    block->addPoint( a );
    block->addPoint( b );
    block->addPoint( c );
    check( block->getNodes().size() == 3, "points did not create editable nodes" );
    auto reversed = make_ptr<CCityBlock>();
    reversed->addPoint( c );
    reversed->addPoint( b );
    reversed->addPoint( a );
    reversed->addPoint( c );
    check( block->equals( reversed ) && reversed->equals( block ),
           "winding or closing point affected equality" );
    check( !block->equals( nullptr ), "null block compared equal" );
    auto rotated = make_ptr<CCityBlock>();
    rotated->addPoint( b );
    rotated->addPoint( c );
    rotated->addPoint( a );
    check( block->equals( rotated ), "boundary rotation affected equality" );
    check( block->getBounds().getMinimum() == a &&
               block->getBounds().getMaximum() == Vector3<real_Num>( 20, 6, 20 ),
           "incorrect block bounds" );
    auto node = static_pointer_cast<BlockNode>( block->getNodes()[0] );
    node->m_point = Vector3<real_Num>( 5, 1, 5 );
    check( block->getPoints()[0] == node->m_point &&
               node->getWorldTransform().getPosition() == node->m_point,
           "legacy point editing did not update geometry" );
    Transform3<real_Num> transform = node->getWorldTransform();
    transform.setPosition( a );
    node->setWorldTransform( transform );
    check( node->m_point == a && node->getPosition() == a, "transform did not update editable point" );
    node->setPosition( b );
    check( node->m_point == b, "position did not update editable point" );
    node->setPosition( a );
    block->setName( "Block" );
    block->getProperties()->setProperty( "value", s32( 7 ) );
    auto copy = static_pointer_cast<CCityBlock>( block->clone() );
    check( copy->equals( block ) && copy->getName() == block->getName(),
           "block clone lost geometry or metadata" );
    check(
        copy->getNodes()[0] != block->getNodes()[0] && copy->getProperties() != block->getProperties(),
        "block clone shares mutable geometry or properties" );
    copy->getNodes()[0]->setPosition( Vector3<real_Num>::zero() );
    check( !copy->equals( block ) && block->getPoints()[0] == a, "clone edit changed original" );
    auto nodeCopy = static_pointer_cast<BlockNode>( node->clone() );
    check( nodeCopy->getPosition() == a && nodeCopy->getConnectedNodes().empty(),
           "node clone contract failed" );
    block->addNode( nullptr );
    block->addNode( node );
    check( block->getPoints().size() == 3, "duplicate/null child node accepted" );
    block->removePoint( b );
    check( block->getNodes().size() == 2, "point removal did not remove node" );
    block->removeNode( node );
    check( block->getPoints().size() == 1, "node removal did not remove point" );
    block->unload( nullptr );
    block->unload( nullptr );
    check( block->getPoints().empty() && block->getNodes().empty() && block->getLots().empty(),
           "unload retained geometry" );
    check( node->getPosition() == a, "block unload modified an externally retained node" );
    auto empty = make_ptr<CCityBlock>();
    check( empty->equals( make_ptr<CCityBlock>() ), "empty boundary equality failed" );
}

int main()
{
    TypeManager types;
    types.load();
    int result = 0;
    {
        try
        {
            checkBlockContracts();
            auto generator = make_ptr<CBlockGenerator>();
            generator->generate();  // No city.
            generator->load( nullptr );
            auto city = make_ptr<CProceduralCity>();
            generator->setCity( city );
            generator->generate();  // No network.
            auto network = make_ptr<CRoadNetwork>();
            city->setRoadNetwork( network );
            auto manual = make_ptr<CCityBlock>();
            city->addBlock( manual );
            Array<SmartPtr<IRoadNode>> nodes;
            for( const auto &p : { Vector3<real_Num>( 0, 3, 0 ), Vector3<real_Num>( 10, 4, 0 ),
                                   Vector3<real_Num>( 10, 5, 10 ), Vector3<real_Num>( 0, 6, 10 ),
                                   Vector3<real_Num>( 20, 5, 10 ), Vector3<real_Num>( 30, 5, 10 ) } )
            {
                auto node = make_ptr<CRoadNode>();
                node->setPosition( p );
                node->setGraphId( 123 );  // Generator must not use or change external graph IDs.
                nodes.push_back( node );
            }
            auto connect = [&]( size_t a, size_t b ) { nodes[a]->connect( nodes[b] ); };
            connect( 0, 1 );
            connect( 1, 2 );
            connect( 2, 3 );
            connect( 3, 0 );
            connect( 2, 4 );
            connect( 4, 5 );  // Three-point open chain must not become a block.
            network->setNodes( nodes );
            generator->generate();
            check( city->getBlocks().size() == 2, "square and manual block expected" );
            auto points = city->getBlocks()[1]->getPoints();
            check( points.size() == 4, "square boundary expected" );
            for( const auto &p : points )
            {
                check( p.Y() >= 3 && p.Y() <= 6, "elevation lost" );
            }
            generator->generateBlocks( nullptr );
            generator->testMemoryUsage();
            check( city->getBlocks().size() == 2, "repeated generation duplicated blocks" );
            for( auto node : nodes )
            {
                check( node->getGraphId() == 123, "graph IDs were changed" );
            }
            connect( 0, 2 );
            generator->generate();
            check( city->getBlocks().size() == 3, "diagonal should create two blocks" );
            connect( 1, 3 );
            bool rejected = false;
            try
            {
                generator->generate();
            }
            catch( const std::invalid_argument & )
            {
                rejected = true;
            }
            check( rejected && city->getBlocks().size() == 3, "invalid crossing changed prior output" );
            nodes[1]->disconnect( nodes[3] );
            nodes[0]->disconnect( nodes[2] );

            // A duplicate source node resolves through its merged representative.
            auto alias = make_ptr<CRoadNode>();
            alias->setPosition( nodes[0]->getPosition() );
            alias->setMergedNode( nodes[0] );
            nodes[3]->disconnect( nodes[0] );
            nodes[3]->connect( alias );
            nodes.push_back( alias );
            nodes.push_back( nullptr );
            network->setNodes( nodes );
            network->setMergedNodes( { nodes[0] } );
            generator->generate();
            check( city->getBlocks().size() == 2, "merged node did not close square" );
            network->setNodes( {} );
            network->setMergedNodes( {} );
            generator->generate();
            check( city->getBlocks().size() == 1 && city->getBlocks()[0] == manual,
                   "empty graph did not clear generated blocks or removed manual block" );
            generator->unload( nullptr );
            check( !generator->getCity() && !generator->getInput() && !generator->getOutput(),
                   "unload retained state" );
            alias->setMergedNode( nullptr );
            for( auto node : nodes )
            {
                if( node )
                {
                    node->clearConnections();
                }
            }
            std::cout << "Block generator contracts passed\n";
        }
        catch( const std::exception &e )
        {
            std::cerr << e.what() << '\n';
            result = 1;
        }
    }
    types.unload();
    return result;
}
