#ifndef PlanarGraph_h__
#define PlanarGraph_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /// An undirected, straight-line planar embedding. T supplies X() and Y().
        /// Positions must be finite and distinct; edges may meet only at shared vertices.
        /// Crossings and vertices in the interior of an edge must be split by the caller.
        template <typename T>
        class PlanarGraph
        {
        public:
            struct Primitive : SourceBudgetObject
            {
                Array<std::pair<T, u32>> Sequence{ Allocator<std::pair<T, u32>>( sourceBudget ) };
            };

            /// IDs need not be contiguous. Repeating an ID is an error.
            void InsertVertex( const T &position, u32 id )
            {
                if( m_indices.find( id ) != m_indices.end() )
                {
                    throw std::invalid_argument( "PlanarGraph: duplicate vertex ID" );
                }
                if( !std::isfinite( static_cast<long double>( position.X() ) ) ||
                    !std::isfinite( static_cast<long double>( position.Y() ) ) )
                {
                    throw std::invalid_argument( "PlanarGraph: non-finite position" );
                }
                for( const auto &vertex : m_vertices )
                {
                    if( vertex.Position.X() == position.X() && vertex.Position.Y() == position.Y() )
                    {
                        throw std::invalid_argument( "PlanarGraph: coincident vertices" );
                    }
                }
                const auto index = m_vertices.size();
                m_vertices.push_back( Vertex{ position, id } );
                try
                {
                    m_indices.emplace( id, index );
                }
                catch( ... )
                {
                    m_vertices.pop_back();
                    throw;
                }
            }

            /// Duplicate undirected edges are ignored; unknown IDs and self edges are errors.
            void InsertEdge( u32 vertexId0, u32 vertexId1 )
            {
                const auto a = m_indices.find( vertexId0 );
                const auto b = m_indices.find( vertexId1 );
                if( a == m_indices.end() || b == m_indices.end() || vertexId0 == vertexId1 )
                {
                    throw std::invalid_argument( "PlanarGraph: invalid edge endpoints" );
                }
                auto &edges0 = m_vertices[a->second].ConnectedVertices;
                auto &edges1 = m_vertices[b->second].ConnectedVertices;
                if( std::find( edges0.begin(), edges0.end(), b->second ) == edges0.end() )
                {
                    edges0.push_back( b->second );
                    try
                    {
                        edges1.push_back( a->second );
                    }
                    catch( ... )
                    {
                        edges0.pop_back();
                        throw;
                    }
                }
            }

            /// Appends isolated points, maximal bridge chains, and bounded CCW face boundaries.
            /// Cycles omit the repeated closing vertex. Extraction preserves the graph.
            /// The caller owns each appended Primitive and must delete it.
            /// Disconnected nested cycles are emitted individually, without hole grouping.
            void ExtractPrimitives( Array<Primitive *> &primitives ) const
            {
                using Index = size_t;
                const Index count = m_vertices.size();
                const Index none = count;
                SourceBudgetVector<SourceBudgetVector<Index>> adjacency( count );
                for( Index i = 0; i < count; ++i )
                {
                    adjacency[i].assign( m_vertices[i].ConnectedVertices.begin(),
                                         m_vertices[i].ConnectedVertices.end() );
                }

                // Iterative low-link traversal identifies bridges without recursion depth limits.
                SourceBudgetVector<Index> discovery( count, none ), low( count ), parent( count, none );
                SourceBudgetVector<Index> next( count, 0 ), stack;
                SourceBudgetMap<std::pair<Index, Index>, bool> bridges;
                auto edgeKey = []( Index a, Index b ) { return std::make_pair( std::min( a, b ), std::max( a, b ) ); };
                Index time = 0;
                for( Index root = 0; root < count; ++root )
                {
                    if( discovery[root] != none )
                    {
                        continue;
                    }
                    discovery[root] = low[root] = time++;
                    stack.push_back( root );
                    while( !stack.empty() )
                    {
                        const auto v = stack.back();
                        if( next[v] < adjacency[v].size() )
                        {
                            const auto w = adjacency[v][next[v]++];
                            if( discovery[w] == none )
                            {
                                parent[w] = v;
                                discovery[w] = low[w] = time++;
                                stack.push_back( w );
                            }
                            else if( w != parent[v] )
                            {
                                low[v] = std::min( low[v], discovery[w] );
                            }
                        }
                        else
                        {
                            stack.pop_back();
                            if( parent[v] != none )
                            {
                                const auto p = parent[v];
                                low[p] = std::min( low[p], low[v] );
                                if( low[v] > discovery[p] )
                                {
                                    bridges.emplace( edgeKey( p, v ), false );
                                }
                            }
                        }
                    }
                }

                SourceBudgetVector<std::unique_ptr<Primitive>> results;
                auto emit = [&]( const SourceBudgetVector<Index> &sequence ) {
                    auto primitive = std::make_unique<Primitive>();
                    for( auto index : sequence )
                    {
                        primitive->Sequence.push_back( { m_vertices[index].Position, m_vertices[index].Id } );
                    }
                    results.push_back( std::move( primitive ) );
                };
                for( Index v = 0; v < count; ++v )
                {
                    if( adjacency[v].empty() )
                    {
                        emit( { v } );
                    }
                    // A bridge chain stops at a branch or a vertex belonging to a cycle.
                    if( adjacency[v].size() == 2 &&
                        bridges.count( edgeKey( v, adjacency[v][0] ) ) &&
                        bridges.count( edgeKey( v, adjacency[v][1] ) ) )
                    {
                        continue;
                    }
                    for( auto w : adjacency[v] )
                    {
                        auto bridge = bridges.find( edgeKey( v, w ) );
                        if( bridge == bridges.end() || bridge->second )
                        {
                            continue;
                        }
                        SourceBudgetVector<Index> sequence{ v };
                        auto previous = v;
                        auto current = w;
                        for( ;; )
                        {
                            bridges.at( edgeKey( previous, current ) ) = true;
                            sequence.push_back( current );
                            if( adjacency[current].size() != 2 )
                            {
                                break;
                            }
                            const auto candidate = adjacency[current][0] == previous ? adjacency[current][1] : adjacency[current][0];
                            if( !bridges.count( edgeKey( current, candidate ) ) )
                            {
                                break;
                            }
                            previous = current;
                            current = candidate;
                        }
                        emit( sequence );
                    }
                }

                // Remove bridges, then order outgoing half-edges counterclockwise.
                for( Index v = 0; v < count; ++v )
                {
                    auto &neighbors = adjacency[v];
                    neighbors.erase( std::remove_if( neighbors.begin(), neighbors.end(),
                        [&]( Index w ) { return bridges.count( edgeKey( v, w ) ) != 0; } ), neighbors.end() );
                    auto angle = [&]( Index w ) {
                        return std::atan2( static_cast<long double>( m_vertices[w].Position.Y() ) - m_vertices[v].Position.Y(),
                                           static_cast<long double>( m_vertices[w].Position.X() ) - m_vertices[v].Position.X() );
                    };
                    std::sort( neighbors.begin(), neighbors.end(), [&]( Index a, Index b ) {
                        const auto aa = angle( a ), ab = angle( b );
                        return aa == ab ? a < b : aa < ab;
                    } );
                }
                SourceBudgetMap<std::pair<Index, Index>, bool> visited;
                for( Index v = 0; v < count; ++v )
                {
                    for( auto w : adjacency[v] )
                    {
                        if( visited.count( { v, w } ) )
                        {
                            continue;
                        }
                        SourceBudgetVector<Index> sequence;
                        auto a = v, b = w;
                        long double area = 0;
                        do
                        {
                            visited.emplace( std::make_pair( a, b ), true );
                            sequence.push_back( a );
                            // Translate to the starting point to reduce cancellation.
                            const auto &origin = m_vertices[v].Position;
                            const auto &p = m_vertices[a].Position;
                            const auto &q = m_vertices[b].Position;
                            area += ( static_cast<long double>( p.X() ) - origin.X() ) *
                                    ( static_cast<long double>( q.Y() ) - origin.Y() ) -
                                    ( static_cast<long double>( p.Y() ) - origin.Y() ) *
                                    ( static_cast<long double>( q.X() ) - origin.X() );
                            const auto &neighbors = adjacency[b];
                            const auto incoming = std::find( neighbors.begin(), neighbors.end(), a );
                            const auto offset = static_cast<Index>( incoming - neighbors.begin() );
                            a = b;
                            b = neighbors[( offset + neighbors.size() - 1 ) % neighbors.size()];
                        } while( a != v || b != w );
                        if( area > 0 )
                        {
                            emit( sequence );
                        }
                    }
                }
                auto output = makeSourceBudgetSnapshot( primitives );
                output.reserve( output.size() + results.size() );
                for( auto &result : results ) output.push_back( result.get() );
                primitives.swap( output );
                for( auto &result : results ) result.release();
            }

        private:
            struct Vertex
            {
                T Position;
                u32 Id;
                Array<size_t> ConnectedVertices{ Allocator<size_t>( sourceBudget ) };
            };

            Array<Vertex> m_vertices{ Allocator<Vertex>( sourceBudget ) };
            SourceBudgetMap<u32, size_t> m_indices;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // PlanarGraph_h__
