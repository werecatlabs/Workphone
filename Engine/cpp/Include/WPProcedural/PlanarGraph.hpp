#ifndef PlanarGraph_h__
#define PlanarGraph_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace workphone
{
    namespace procedural
    {

        /// An undirected, straight-line planar embedding. T supplies X() and Y().
        /// Positions must be finite and distinct; edges may meet only at shared vertices.
        /// Crossings and vertices in the interior of an edge must be split by the caller.
        template <typename T>
        class PlanarGraph : public SourceBudgetObject
        {
            using Index = size_t;
            using PositionKey = decltype( std::make_pair( std::declval<const T &>().X(),
                                                          std::declval<const T &>().Y() ) );

            struct Vertex
            {
                T position;
                u32 id;
                SourceBudgetVector<Index> outgoing;
            };

            // Edge i owns two directed half-edges, 2*i and 2*i+1. Flipping their low bit
            // crosses the road to the opposite endpoint.
            struct Edge
            {
                Index ends[2];
            };

            enum class EdgeKind : u8
            {
                Cycle,
                Bridge,
                EmittedBridge
            };

        public:
            struct Primitive : SourceBudgetObject
            {
                Array<std::pair<T, u32>> Sequence{ Allocator<std::pair<T, u32>>( sourceBudget ) };
            };

            PlanarGraph() = default;
            PlanarGraph( const PlanarGraph & ) = default;
            PlanarGraph &operator=( PlanarGraph && ) = default;

            PlanarGraph( PlanarGraph &&other ) noexcept( noexcept( PlanarGraph() ) ) : PlanarGraph()
            {
                // Some map implementations allocate their sentinel on construction.
                // Finish that allocation before taking any of the source graph's storage.
                *this = std::move( other );
            }

            PlanarGraph &operator=( const PlanarGraph &other )
            {
                // Vertex indices, coordinate lookup and half-edges must change together.
                if( this != &other )
                {
                    *this = PlanarGraph( other );
                }
                return *this;
            }

            /// Returns the ID at this exact position, if present.
            std::optional<u32> FindVertex( const T &position ) const
            {
                if( isFinite( position ) )
                {
                    const auto it = m_positions.find( { position.X(), position.Y() } );
                    if( it != m_positions.end() )
                    {
                        return it->second;
                    }
                }
                return std::nullopt;
            }

            /// IDs need not be contiguous. Repeating an ID is an error.
            void InsertVertex( const T &position, u32 id )
            {
                if( m_indices.find( id ) != m_indices.end() )
                {
                    throw std::invalid_argument( "PlanarGraph: duplicate vertex ID" );
                }
                if( !isFinite( position ) )
                {
                    throw std::invalid_argument( "PlanarGraph: non-finite position" );
                }
                const auto entry = m_positions.try_emplace( { position.X(), position.Y() }, id );
                if( !entry.second )
                {
                    throw std::invalid_argument( "PlanarGraph: coincident vertices" );
                }
                const auto index = m_vertices.size();
                try
                {
                    m_vertices.push_back( Vertex{ position, id, {} } );
                    m_indices.emplace( id, index );
                }
                catch( ... )
                {
                    if( m_vertices.size() > index )
                    {
                        m_vertices.pop_back();
                    }
                    m_positions.erase( entry.first );
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
                // A road only needs to be looked up at the less connected endpoint.
                auto source = a->second, target = b->second;
                if( m_vertices[source].outgoing.size() > m_vertices[target].outgoing.size() )
                {
                    std::swap( source, target );
                }
                auto &from = m_vertices[source].outgoing;
                auto &to = m_vertices[target].outgoing;
                for( auto h : from )
                {
                    if( origin( h ^ 1 ) == target )
                    {
                        return;
                    }
                }
                const auto h = 2 * m_edges.size();
                m_edges.push_back( Edge{ { source, target } } );
                try
                {
                    from.push_back( h );
                    to.push_back( h ^ 1 );
                }
                catch( ... )
                {
                    if( !from.empty() && from.back() == h )
                    {
                        from.pop_back();
                    }
                    m_edges.pop_back();
                    throw;
                }
            }

            /// Visits bounded CCW face walks. The ID buffer is borrowed during the callback.
            /// Closing IDs are omitted. A face that touches itself may revisit a vertex.
            /// Disconnected nested cycles are emitted individually, without hole grouping.
            template <class Visitor>
            void VisitBoundedFaces( Visitor &&visit ) const
            {
                SourceBudgetVector<u32> ids;
                visitFaces( findBridges(), [&]( const auto &walk ) {
                    ids.clear();
                    ids.reserve( walk.size() );
                    for( auto v : walk )
                    {
                        ids.push_back( m_vertices[v].id );
                    }
                    visit( std::as_const( ids ) );
                } );
            }

            /// Appends isolated points, maximal bridge chains, and bounded CCW face boundaries.
            /// Cycles omit the repeated closing vertex. Extraction preserves the graph.
            /// The caller owns each appended Primitive and must delete it.
            /// Disconnected nested cycles are emitted individually, without hole grouping.
            void ExtractPrimitives( Array<Primitive *> &primitives ) const
            {
                SourceBudgetVector<std::unique_ptr<Primitive>> results;
                auto emit = [&]( const SourceBudgetVector<Index> &walk ) {
                    auto primitive = std::make_unique<Primitive>();
                    primitive->Sequence.reserve( walk.size() );
                    for( auto v : walk )
                    {
                        primitive->Sequence.push_back( { m_vertices[v].position, m_vertices[v].id } );
                    }
                    results.push_back( std::move( primitive ) );
                };
                auto kinds = findBridges();
                visitPointsAndChains( kinds, emit );
                visitFaces( kinds, emit );

                auto output = makeSourceBudgetSnapshot( primitives );
                output.reserve( output.size() + results.size() );
                for( const auto &result : results )
                {
                    output.push_back( result.get() );
                }
                primitives.swap( output );
                for( auto &result : results )
                {
                    result.release();
                }
            }

        private:
            static bool isFinite( const T &p )
            {
                return std::isfinite( static_cast<long double>( p.X() ) ) &&
                       std::isfinite( static_cast<long double>( p.Y() ) );
            }

            Index origin( Index h ) const
            {
                return m_edges[h / 2].ends[h % 2];
            }

            SourceBudgetVector<EdgeKind> findBridges() const
            {
                SourceBudgetVector<EdgeKind> kinds( m_edges.size(), EdgeKind::Cycle );
                if( m_edges.empty() )
                {
                    return kinds;
                }

                struct Search
                {
                    Index discovery, low, parentEdge, next;
                };

                const auto unseen = m_vertices.size();
                SourceBudgetVector<Search> search( unseen, Search{ unseen, unseen, m_edges.size(), 0 } );
                SourceBudgetVector<Index> stack;
                Index time = 0;
                for( Index root = 0; root < m_vertices.size(); ++root )
                {
                    if( search[root].discovery != unseen || m_vertices[root].outgoing.empty() )
                    {
                        continue;
                    }
                    search[root].discovery = search[root].low = time++;
                    stack.push_back( root );
                    while( !stack.empty() )
                    {
                        const auto v = stack.back();
                        auto &state = search[v];
                        const auto &outgoing = m_vertices[v].outgoing;
                        if( state.next == outgoing.size() )
                        {
                            stack.pop_back();
                            if( !stack.empty() )
                            {
                                auto &parent = search[stack.back()];
                                parent.low = std::min( parent.low, state.low );
                                // No back edge from this subtree reaches its parent or an ancestor.
                                if( state.low > parent.discovery )
                                {
                                    kinds[state.parentEdge] = EdgeKind::Bridge;
                                }
                            }
                            continue;
                        }
                        const auto h = outgoing[state.next++];
                        const auto w = origin( h ^ 1 );
                        if( search[w].discovery == unseen )
                        {
                            search[w] = { time, time, h / 2, 0 };
                            ++time;
                            stack.push_back( w );
                        }
                        else if( h / 2 != state.parentEdge )
                        {
                            state.low = std::min( state.low, search[w].discovery );
                        }
                    }
                }
                return kinds;
            }

            template <class Visitor>
            void visitPointsAndChains( SourceBudgetVector<EdgeKind> &kinds, Visitor &visit ) const
            {
                SourceBudgetVector<Index> walk;
                for( Index v = 0; v < m_vertices.size(); ++v )
                {
                    const auto &outgoing = m_vertices[v].outgoing;
                    if( outgoing.empty() )
                    {
                        walk.assign( 1, v );
                        visit( walk );
                    }
                    // Start maximal chains only at endpoints, branches or vertices on cycles.
                    if( outgoing.size() == 2 && kinds[outgoing[0] / 2] != EdgeKind::Cycle &&
                        kinds[outgoing[1] / 2] != EdgeKind::Cycle )
                    {
                        continue;
                    }
                    for( auto h : outgoing )
                    {
                        if( kinds[h / 2] != EdgeKind::Bridge )
                        {
                            continue;
                        }
                        walk.assign( 1, v );
                        while( kinds[h / 2] == EdgeKind::Bridge )
                        {
                            kinds[h / 2] = EdgeKind::EmittedBridge;
                            const auto w = origin( h ^ 1 );
                            walk.push_back( w );
                            const auto &next = m_vertices[w].outgoing;
                            if( next.size() != 2 )
                            {
                                break;
                            }
                            h = next[0] == ( h ^ 1 ) ? next[1] : next[0];
                        }
                        visit( walk );
                    }
                }
            }

            // Counterclockwise from just below the negative X axis, matching atan2's
            // usual branch cut. Comparing orientations keeps close directions distinct.
            static bool directionLess( const T &center, const T &a, const T &b )
            {
                auto firstHalf = [&]( const T &p ) {
                    return p.Y() < center.Y() || ( p.Y() == center.Y() && p.X() > center.X() );
                };
                const bool firstA = firstHalf( a ), firstB = firstHalf( b );
                if( firstA != firstB )
                {
                    return firstA;
                }
                using Wide = long double;
                using Limits = std::numeric_limits<Wide>;
                if constexpr( std::numeric_limits<typename PositionKey::first_type>::digits <=
                                  Limits::digits &&
                              std::numeric_limits<typename PositionKey::second_type>::digits <=
                                  Limits::digits )
                {
                    const Wide ax = static_cast<Wide>( a.X() ) - center.X();
                    const Wide ay = static_cast<Wide>( a.Y() ) - center.Y();
                    const Wide bx = static_cast<Wide>( b.X() ) - center.X();
                    const Wide by = static_cast<Wide>( b.Y() ) - center.Y();
                    const Wide left = ax * by, right = ay * bx;
                    const Wide det = left - right;
                    // Allow for rounding the coordinate differences, products and subtraction.
                    const Wide error = 8 * Limits::epsilon() * ( std::abs( left ) + std::abs( right ) );
                    if( std::isfinite( det ) && std::abs( det ) > std::max( Limits::min(), error ) )
                    {
                        return det > 0;
                    }
                }
                // Near cancellation or outside the floating-point exponent range, compare
                // the determinant of the original coordinates exactly. A sort comparator
                // must give mutually consistent answers even for nearly coincident rays.
                namespace mp = boost::multiprecision;
                using Integer = mp::cpp_int_backend<0, 0, mp::signed_magnitude, mp::unchecked,
                                                    SourceBudgetAllocator<mp::limb_type>>;
                using Exact = mp::number<mp::rational_adaptor<Integer>, mp::et_off>;
                const Exact cx( center.X() ), cy( center.Y() );
                return ( Exact( a.X() ) - cx ) * ( Exact( b.Y() ) - cy ) >
                       ( Exact( a.Y() ) - cy ) * ( Exact( b.X() ) - cx );
            }

            template <class Visitor>
            void visitFaces( const SourceBudgetVector<EdgeKind> &kinds, Visitor &&visit ) const
            {
                const auto cyclic = std::count( kinds.begin(), kinds.end(), EdgeKind::Cycle );
                if( cyclic == 0 )
                {
                    return;
                }

                struct Step
                {
                    Index next;
                    bool wedgeCrossesNegativeX;
                };

                const auto visited = 2 * m_edges.size();
                SourceBudgetVector<Step> steps( visited );
                SourceBudgetVector<Index> order;
                order.reserve( 2 * cyclic );
                for( const auto &vertex : m_vertices )
                {
                    const auto first = order.size();
                    for( auto h : vertex.outgoing )
                    {
                        if( kinds[h / 2] == EdgeKind::Cycle )
                        {
                            order.push_back( h );
                        }
                    }
                    if( order.size() == first )
                    {
                        continue;
                    }
                    std::sort( order.begin() + first, order.end(), [&]( Index a, Index b ) {
                        return directionLess( vertex.position, m_vertices[origin( a ^ 1 )].position,
                                              m_vertices[origin( b ^ 1 )].position );
                    } );
                    // On arrival take the clockwise predecessor of the reverse half-edge.
                    auto previous = order.back();
                    steps[previous].wedgeCrossesNegativeX = true;
                    for( auto i = first; i < order.size(); ++i )
                    {
                        steps[order[i] ^ 1].next = previous;
                        previous = order[i];
                    }
                }

                SourceBudgetVector<Index> walk;
                for( auto start : order )
                {
                    if( steps[start].next == visited )
                    {
                        continue;
                    }
                    auto leftmost = origin( start );
                    bool exterior = false;
                    walk.clear();
                    auto h = start;
                    do
                    {
                        const auto v = origin( h );
                        const auto &p = m_vertices[v].position;
                        const auto &q = m_vertices[leftmost].position;
                        if( p.X() < q.X() || ( p.X() == q.X() && p.Y() < q.Y() ) )
                        {
                            leftmost = v;
                            exterior = false;
                        }
                        // At the leftmost point of this walk, its incident boundary edges
                        // lead right or vertically. A wedge containing the negative X ray
                        // there opens onto the exterior, even if the walk touches itself.
                        if( v == leftmost && steps[h].wedgeCrossesNegativeX )
                        {
                            exterior = true;
                        }
                        walk.push_back( v );
                        h = std::exchange( steps[h].next, visited );
                    } while( h != start );
                    if( !exterior )
                    {
                        visit( walk );
                    }
                }
            }

            SourceBudgetVector<Vertex> m_vertices;
            SourceBudgetVector<Edge> m_edges;
            SourceBudgetMap<u32, Index> m_indices;
            SourceBudgetMap<PositionKey, u32> m_positions;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // PlanarGraph_h__
