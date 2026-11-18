#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/PlanarGraph.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <Workphone/Interface/Procedural/IProceduralCity.hpp>
#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

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
                SourceBudgetVector<Vector3<real_Num>> positions;
                SourceBudgetSet<Edge> edges;
            };

            Edge edgeKey( u32 a, u32 b )
            {
                return { std::min( a, b ), std::max( a, b ) };
            }

            RoadGraph buildRoadGraph( const SmartPtr<IRoadNetwork> &network )
            {
                RoadGraph result;
                // Include ordinary nodes: merged lists can contain only junctions.
                Array<SmartPtr<IProceduralNode>> nodes{
                    typename Array<SmartPtr<IProceduralNode>>::allocator_type( sourceBudget )
                };
                for( auto node : network->getMergedNodes() )
                {
                    nodes.push_back( node );
                }
                for( auto node : network->getNodes() )
                {
                    nodes.push_back( node );
                }
                SourceBudgetMap<IProceduralNode *, u32> ids;
                SourceBudgetVector<SmartPtr<IProceduralNode>> representatives;
                for( auto node : nodes )
                {
                    if( !node )
                    {
                        continue;
                    }
                    auto representative = node;
                    SourceBudgetSet<IProceduralNode *> seen;
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
                        const Vector2<real_Num> coordinate( position.X(), position.Z() );
                        u32 id;
                        if( const auto existing = result.graph.FindVertex( coordinate ) )
                        {
                            id = *existing;
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
                            result.graph.InsertVertex( coordinate, id );
                        }
                        found = ids.emplace( representative.get(), id ).first;
                        representatives.push_back( representative );
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
                        SourceBudgetSet<IProceduralNode *> seen;
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

            void validateEmbedding( const RoadGraph &input )
            {
                if( input.edges.empty() )
                {
                    return;
                }
                struct GroundPoint
                {
                    real_Num x, z;

                    bool operator<( const GroundPoint &other ) const
                    {
                        return std::tie( x, z ) < std::tie( other.x, other.z );
                    }
                };
                struct RoadBounds
                {
                    GroundPoint min, max;
                    Edge endpoints;
                };
                SourceBudgetVector<std::pair<GroundPoint, u32>> points;
                points.reserve( input.positions.size() );
                for( u32 i = 0; i < input.positions.size(); ++i )
                {
                    const auto &p = input.positions[i];
                    points.emplace_back( GroundPoint{ p.X(), p.Z() }, i );
                }
                std::sort( points.begin(), points.end() );

                SourceBudgetVector<RoadBounds> roads;
                roads.reserve( input.edges.size() );
                for( const auto &edge : input.edges )
                {
                    const auto &a = input.positions[edge.first], &b = input.positions[edge.second];
                    roads.push_back( { { std::min( a.X(), b.X() ), std::min( a.Z(), b.Z() ) },
                                       { std::max( a.X(), b.X() ), std::max( a.Z(), b.Z() ) }, edge } );
                }
                std::sort( roads.begin(), roads.end(),
                           []( const auto &a, const auto &b ) { return a.min < b.min; } );

                // Sweep closed bounding boxes in X, breaking ties in Z for north/south roads.
                // The separate point list also catches isolated nodes and collinear overlaps.
                auto firstPoint = points.begin();
                for( auto road = roads.begin(); road != roads.end(); ++road )
                {
                    while( firstPoint != points.end() && firstPoint->first < road->min )
                    {
                        ++firstPoint;
                    }
                    const auto [start, end] = road->endpoints;
                    const auto &a = input.positions[start], &b = input.positions[end];
                    for( auto point = firstPoint; point != points.end() && !( road->max < point->first );
                         ++point )
                    {
                        const auto &[p, id] = *point;
                        if( id != start && id != end && p.z >= road->min.z && p.z <= road->max.z &&
                            orientation( a, b, input.positions[id] ) == 0 )
                        {
                            throw std::invalid_argument(
                                "CBlockGenerator: split roads at junctions before generating blocks" );
                        }
                    }
                    for( auto other = road + 1; other != roads.end() && !( road->max < other->min );
                         ++other )
                    {
                        const auto [cId, dId] = other->endpoints;
                        if( other->max.z < road->min.z || other->min.z > road->max.z ||
                            start == cId || start == dId || end == cId || end == dId )
                        {
                            continue;
                        }
                        const auto &c = input.positions[cId], &d = input.positions[dId];
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
                Array<SmartPtr<ICityBlock>> blocks{ typename Array<SmartPtr<ICityBlock>>::allocator_type(
                    sourceBudget ) };
                SourceBudgetVector<u32> sortedIds;
                input.graph.VisitBoundedFaces( [&]( const SourceBudgetVector<u32> &boundary ) {
                    // A face revisiting a junction cannot be represented by a simple city block.
                    sortedIds.assign( boundary.begin(), boundary.end() );
                    std::sort( sortedIds.begin(), sortedIds.end() );
                    if( std::adjacent_find( sortedIds.begin(), sortedIds.end() ) != sortedIds.end() )
                    {
                        return;
                    }
                    auto block = workphone::make_ptr<CCityBlock>();
                    for( const auto id : boundary )
                    {
                        block->addPoint( input.positions[id] );
                    }
                    blocks.push_back( block );
                } );
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
            auto blocks = std::move( m_generatedBlocks );
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
            auto entries = makeSourceBudgetSnapshot( m_generatedBlocks );
            auto previous = std::find_if( entries.begin(), entries.end(),
                                          [&]( const auto &entry ) { return entry.city == m_city; } );
            if( previous == entries.end() )
            {
                entries.push_back( CityBlocks{ m_city } );
                previous = entries.end() - 1;
            }
            auto combined = m_city->getBlocks();
            for( const auto &block : previous->blocks )
            {
                auto found = std::find( combined.begin(), combined.end(), block );
                if( found != combined.end() )
                {
                    combined.erase( found );
                }
            }
            combined.reserve( combined.size() + blocks.size() );
            for( const auto &block : blocks )
            {
                combined.push_back( block );
            }
            previous->blocks = std::move( blocks );
            m_city->setBlocks( combined );
            m_generatedBlocks.swap( entries );
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
