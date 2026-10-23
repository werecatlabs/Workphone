#pragma once
#include <Workphone/Scene/SceneWorld.hpp>
#include <algorithm>
#include <numeric>
#include <map>

namespace workphone::scene
{
    // Bounds may cross many grid cells. Index the actual coverage once instead
    // of expanding ownership into a dense grid or scanning every cell per tick.
    class SceneWorldIndex
    {
        struct Node
        {
            Vector3F minimum, maximum;
            u32 begin = 0, end = 0, left = 0, right = 0;
        };
        Array<Node> m_nodes;
        Array<u32> m_order;
        u32 buildNode( const Array<WorldCell> &cells, u32 begin, u32 end )
        {
            Node node; node.begin = begin; node.end = end;
            node.minimum = cells[m_order[begin]].minimum;
            node.maximum = cells[m_order[begin]].maximum;
            for( auto i = begin + 1; i < end; ++i )
            {
                const auto &cell = cells[m_order[i]];
                node.minimum.x = std::min( node.minimum.x, cell.minimum.x );
                node.minimum.z = std::min( node.minimum.z, cell.minimum.z );
                node.maximum.x = std::max( node.maximum.x, cell.maximum.x );
                node.maximum.z = std::max( node.maximum.z, cell.maximum.z );
            }
            const auto index = u32( m_nodes.size() ); m_nodes.push_back( node );
            if( end - begin > 8 )
            {
                const bool x = double( node.maximum.x ) - node.minimum.x >=
                               double( node.maximum.z ) - node.minimum.z;
                const auto middle = begin + ( end - begin ) / 2;
                std::nth_element( m_order.begin() + begin, m_order.begin() + middle,
                                  m_order.begin() + end, [&]( u32 a, u32 b ) {
                    const auto &ca = cells[a]; const auto &cb = cells[b];
                    const double ac = x ? double( ca.minimum.x ) + ca.maximum.x : double( ca.minimum.z ) + ca.maximum.z;
                    const double bc = x ? double( cb.minimum.x ) + cb.maximum.x : double( cb.minimum.z ) + cb.maximum.z;
                    return ac == bc ? ca.key < cb.key : ac < bc;
                } );
                node.left = buildNode( cells, begin, middle );
                node.right = buildNode( cells, middle, end );
                m_nodes[index] = node;
            }
            return index;
        }
        void queryNode( u32 index, const Array<WorldCell> &cells, const Vector3F &p,
                        double radiusSquared, Array<u32> &found, u32 &visited, u32 &tested ) const
        {
            const auto &node = m_nodes[index]; ++visited;
            const double x = std::max( { double( node.minimum.x ) - p.x, 0.0, double( p.x ) - node.maximum.x } );
            const double z = std::max( { double( node.minimum.z ) - p.z, 0.0, double( p.z ) - node.maximum.z } );
            if( x * x + z * z > radiusSquared ) return;
            if( node.right )
            {
                queryNode( node.left, cells, p, radiusSquared, found, visited, tested );
                queryNode( node.right, cells, p, radiusSquared, found, visited, tested );
            }
            else for( auto i = node.begin; i < node.end; ++i )
            {
                ++tested;
                if( cells[m_order[i]].distanceSquared( p ) <= radiusSquared ) found.push_back( m_order[i] );
            }
        }
    public:
        std::map<u64, u32> byKey;
        u32 persistent = ~u32( 0 );
        void build( const WorldManifest &manifest )
        {
            *this = {};
            for( u32 i = 0; i < manifest.cells.size(); ++i )
            {
                byKey.emplace( manifest.cells[i].key, i );
                if( manifest.cells[i].id == "persistent" ) persistent = i;
                else m_order.push_back( i );
            }
            m_nodes.reserve( m_order.size() / 4 + 1 );
            if( !m_order.empty() ) buildNode( manifest.cells, 0, u32( m_order.size() ) );
        }
        void query( const WorldManifest &manifest, const Vector3F &p, double radiusSquared,
                    Array<u32> &found, u32 &visited, u32 &tested ) const
        {
            if( !m_nodes.empty() ) queryNode( 0, manifest.cells, p, radiusSquared, found, visited, tested );
        }
    };
}
