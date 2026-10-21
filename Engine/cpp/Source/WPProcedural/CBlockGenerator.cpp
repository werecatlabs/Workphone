#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/PlanarGraph.hpp>
#include <Workphone/Interface/Procedural/IProceduralCity.hpp>
#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CBlockGenerator, CProceduralGenerator<IBlockGenerator> );

        namespace
        {
            using Graph = PlanarGraph<Vector2<real_Num>>;
            using Edge = std::pair<u32, u32>;

            struct RoadGraph
            {
                Graph graph;
                std::vector<Vector3<real_Num>> positions;
                std::set<Edge> edges;
            };

            Edge edgeKey( u32 a, u32 b )
            {
                return { std::min( a, b ), std::max( a, b ) };
            }

            RoadGraph buildRoadGraph( const SmartPtr<IRoadNetwork> &network )
            {
                RoadGraph result;
                // Include ordinary nodes: merged lists can contain only junctions.
                Array<SmartPtr<IProceduralNode>> nodes;
                for( auto node : network->getMergedNodes() )
                {
                    nodes.push_back( node );
                }
                for( auto node : network->getNodes() )
                {
                    nodes.push_back( node );
                }
                std::map<IProceduralNode *, u32> ids;
                std::map<std::pair<real_Num, real_Num>, u32> coordinates;
                std::vector<SmartPtr<IProceduralNode>> representatives;
                for( auto node : nodes )
                {
                    if( !node )
                    {
                        continue;
                    }
                    auto representative = node;
                    std::set<IProceduralNode *> seen;
                    while( auto merged = representative->getMergedNode() )
                    {
                        if( !seen.insert( representative.get() ).second )
                        {
                            throw std::invalid_argument(
                                "CBlockGenerator: cyclic merged-node references" );
                        }
                        representative = merged;
                    }
                    auto found = ids.find( representative.get() );
                    if( found == ids.end() )
                    {
                        const auto position = representative->getPosition();
                        if( !std::isfinite( position.X() ) || !std::isfinite( position.Y() ) ||
                            !std::isfinite( position.Z() ) )
                        {
                            throw std::invalid_argument( "CBlockGenerator: non-finite road position" );
                        }
                        const auto coordinate = std::make_pair( position.X(), position.Z() );
                        auto existing = coordinates.find( coordinate );
                        u32 id;
                        if( existing != coordinates.end() )
                        {
                            id = existing->second;
                            if( result.positions[id].Y() != position.Y() )
                            {
                                throw std::invalid_argument(
                                    "CBlockGenerator: vertically overlapping road nodes" );
                            }
                        }
                        else
                        {
                            if( result.positions.size() >= std::numeric_limits<u32>::max() )
                            {
                                throw std::length_error( "CBlockGenerator: too many road nodes" );
                            }
                            id = static_cast<u32>( result.positions.size() );
                            result.positions.push_back( position );
                            coordinates.emplace( coordinate, id );
                            result.graph.InsertVertex( Vector2<real_Num>( position.X(), position.Z() ),
                                                       id );
                        }
                        ids.emplace( representative.get(), id );
                        representatives.push_back( representative );
                        found = ids.find( representative.get() );
                    }
                    ids[node.get()] = found->second;
                }
                // Source-node connections can contain edges absent from the merged representative.
                nodes.insert( nodes.end(), representatives.begin(), representatives.end() );
                for( auto node : nodes )
                {
                    if( !node )
                    {
                        continue;
                    }
                    const auto from = ids.find( node.get() );
                    for( auto neighbor : node->getConnectedNodes() )
                    {
                        if( !neighbor )
                        {
                            continue;
                        }
                        auto to = ids.find( neighbor.get() );
                        // Resolve connections to source nodes omitted from the network's node lists.
                        std::set<IProceduralNode *> seen;
                        while( to == ids.end() && neighbor )
                        {
                            if( !seen.insert( neighbor.get() ).second )
                            {
                                break;
                            }
                            neighbor = neighbor->getMergedNode();
                            if( neighbor )
                            {
                                to = ids.find( neighbor.get() );
                            }
                        }
                        if( to != ids.end() && from->second != to->second )
                        {
                            result.edges.insert( edgeKey( from->second, to->second ) );
                        }
                    }
                }
                for( const auto &edge : result.edges )
                {
                    result.graph.InsertEdge( edge.first, edge.second );
                }
                return result;
            }

            long double orientation( const Vector3<real_Num> &a, const Vector3<real_Num> &b,
                                     const Vector3<real_Num> &c )
            {
                return ( static_cast<long double>( b.X() ) - a.X() ) *
                           ( static_cast<long double>( c.Z() ) - a.Z() ) -
                       ( static_cast<long double>( b.Z() ) - a.Z() ) *
                           ( static_cast<long double>( c.X() ) - a.X() );
            }

            bool onSegment( const Vector3<real_Num> &a, const Vector3<real_Num> &b,
                            const Vector3<real_Num> &p )
            {
                return orientation( a, b, p ) == 0 && p.X() >= std::min( a.X(), b.X() ) &&
                       p.X() <= std::max( a.X(), b.X() ) && p.Z() >= std::min( a.Z(), b.Z() ) &&
                       p.Z() <= std::max( a.Z(), b.Z() );
            }

            void validateEmbedding( const RoadGraph &input )
            {
                // Reject unsplit crossings, overlaps and T-junctions before publishing any blocks.
                for( const auto &edge : input.edges )
                {
                    const auto &a = input.positions[edge.first], &b = input.positions[edge.second];
                    for( size_t i = 0; i < input.positions.size(); ++i )
                    {
                        if( i != edge.first && i != edge.second &&
                            onSegment( a, b, input.positions[i] ) )
                        {
                            throw std::invalid_argument(
                                "CBlockGenerator: split roads at junctions before generating blocks" );
                        }
                    }
                    for( auto other = input.edges.upper_bound( edge ); other != input.edges.end();
                         ++other )
                    {
                        const auto &c = input.positions[other->first],
                                   &d = input.positions[other->second];
                        const auto abC = orientation( a, b, c ), abD = orientation( a, b, d );
                        const auto cdA = orientation( c, d, a ), cdB = orientation( c, d, b );
                        if( ( ( abC < 0 && abD > 0 ) || ( abC > 0 && abD < 0 ) ) &&
                            ( ( cdA < 0 && cdB > 0 ) || ( cdA > 0 && cdB < 0 ) ) )
                        {
                            throw std::invalid_argument(
                                "CBlockGenerator: crossing roads require explicit junction nodes" );
                        }
                    }
                }
            }

            Array<SmartPtr<ICityBlock>> buildBlocks( const SmartPtr<IRoadNetwork> &network )
            {
                auto input = buildRoadGraph( network );
                validateEmbedding( input );
                Array<Graph::Primitive *> primitives;

                struct Cleanup
                {
                    Array<Graph::Primitive *> &items;

                    ~Cleanup()
                    {
                        for( auto item : items )
                        {
                            delete item;
                        }
                    }
                } cleanup{ primitives };

                input.graph.ExtractPrimitives( primitives );
                Array<SmartPtr<ICityBlock>> blocks;
                for( const auto primitive : primitives )
                {
                    const auto &sequence = primitive->Sequence;
                    if( sequence.size() < 3 || !input.edges.count( edgeKey( sequence.front().second,
                                                                            sequence.back().second ) ) )
                    {
                        continue;  // Isolated vertices and bridge chains are not city blocks.
                    }
                    std::set<u32> vertices;
                    for( const auto &point : sequence )
                    {
                        vertices.insert( point.second );
                    }
                    if( vertices.size() != sequence.size() )
                    {
                        continue;  // A block must be a simple polygon.
                    }
                    auto block = workphone::make_ptr<CCityBlock>();
                    for( const auto &point : sequence )
                    {
                        block->addPoint( input.positions[point.second] );
                    }
                    blocks.push_back( block );
                }
                return blocks;
            }
        }  // namespace

        CBlockGenerator::CBlockGenerator() = default;
        CBlockGenerator::~CBlockGenerator() = default;

        void CBlockGenerator::load( SmartPtr<ISharedObject> object )
        {
            setLoadingState( LoadingState::Loaded );
        }

        void CBlockGenerator::unload( SmartPtr<ISharedObject> object )
        {
            m_generatedBlocks.clear();
            m_city = nullptr;
            m_input = nullptr;
            m_output = nullptr;
            m_parent = nullptr;
            setLoadingState( LoadingState::Unloaded );
        }

        void CBlockGenerator::generate()
        {
            if( !m_city )
            {
                return;
            }
            const auto network = m_city->getRoadNetwork();
            if( !network )
            {
                return;
            }
            // Build and validate first so malformed input preserves the previous result.
            auto blocks = buildBlocks( network );
            auto previous = std::find_if( m_generatedBlocks.begin(), m_generatedBlocks.end(),
                                          [&]( const auto &entry ) { return entry.city == m_city; } );
            if( previous == m_generatedBlocks.end() )
            {
                m_generatedBlocks.push_back( CityBlocks{ m_city, {} } );
                previous = m_generatedBlocks.end() - 1;
            }
            previous->blocks.reserve( blocks.size() );
            for( auto block : previous->blocks )
            {
                m_city->removeBlock( block );
            }
            previous->blocks.clear();
            for( auto block : blocks )
            {
                m_city->addBlock( block );
                previous->blocks.push_back( block );
            }
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
            // Legacy entry point: cityLayer has no road-network API; the assigned city supplies it.
            generate();
        }

        void CBlockGenerator::testMemoryUsage()
        {
            // Legacy diagnostic entry point: exercise allocation/cleanup without changing city blocks.
            if( m_city )
            {
                if( auto network = m_city->getRoadNetwork() )
                {
                    buildBlocks( network );
                }
            }
        }

        CBlockGenerator::BlockInfo::BlockInfo( bool isClosedBlock ) : IsClosedBlock( isClosedBlock )
        {
        }
    }  // namespace procedural
}  // namespace workphone
