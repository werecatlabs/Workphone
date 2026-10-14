#ifndef WORKPHONE_SCENE_PICKING_HPP
#define WORKPHONE_SCENE_PICKING_HPP

#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <cmath>
#include <limits>
#include <set>

namespace workphone::scene
{
    inline bool makeScenePickingRay( const Matrix4<real_Num> &view, const Matrix4<real_Num> &projection,
                                     Vector2F position, Ray3<real_Num> &ray, real_Num &maximumDistance )
    {
        const auto inverse = ( projection * view ).inverse();
        const auto unproject = [&]( real_Num z ) {
            const real_Num clip[4]{ 2 * position.x - 1, 1 - 2 * position.y, z, 1 };
            real_Num result[4]{};
            // Matrix4/Vector4 multiplication preserves the input w in this engine.
            // Compute all four homogeneous components before dividing explicitly.
            for( u32 row = 0; row < 4; ++row )
            {
                for( u32 column = 0; column < 4; ++column )
                {
                    result[row] += inverse[row][column] * clip[column];
                }
            }
            return Vector3<real_Num>( result[0] / result[3], result[1] / result[3],
                                      result[2] / result[3] );
        };
        const auto nearPoint = unproject( -1 );
        const auto farPoint = unproject( 1 );
        const auto direction = farPoint - nearPoint;
        maximumDistance = direction.length();
        if( !nearPoint.isValid() || !farPoint.isValid() || !std::isfinite( maximumDistance ) ||
            maximumDistance <= 0 )
        {
            return false;
        }
        ray = Ray3<real_Num>( nearPoint, direction / maximumDistance );
        return true;
    }

    // Test world-space geometry so nonuniform and mirrored actor scales are preserved.
    inline bool pickMeshTriangles( const Ray3<real_Num> &ray, const Matrix4<real_Num> &world,
                                   const Array<Vector3<real_Num>> &points, const Array<u32> &indices,
                                   real_Num &nearest )
    {
        if( points.empty() || !ray.getOrigin().isValid() || !ray.getDirection().isValid() )
        {
            return false;
        }
        auto transformed = points;
        AABB3<real_Num> bounds;
        for( auto &point : transformed )
        {
            point = world.transformAffine( point );
            if( !point.isValid() )
            {
                return false;
            }
            bounds.merge( point );
        }
        const auto origin = ray.getOrigin();
        const auto direction = ray.getDirection();
        real_Num entry = 0, exit = nearest;
        for( u32 axis = 0; axis < 3; ++axis )
        {
            if( direction[axis] == 0 )
            {
                if( origin[axis] < bounds.getMinimum()[axis] ||
                    origin[axis] > bounds.getMaximum()[axis] )
                {
                    return false;
                }
                continue;
            }
            auto a = ( bounds.getMinimum()[axis] - origin[axis] ) / direction[axis];
            auto b = ( bounds.getMaximum()[axis] - origin[axis] ) / direction[axis];
            if( a > b )
            {
                std::swap( a, b );
            }
            entry = std::max( entry, a );
            exit = std::min( exit, b );
            if( entry > exit )
            {
                return false;
            }
        }
        bool hit = false;
        for( size_t i = 0; i + 2 < indices.size(); i += 3 )
        {
            if( indices[i] >= transformed.size() || indices[i + 1] >= transformed.size() ||
                indices[i + 2] >= transformed.size() )
            {
                continue;
            }
            const auto a = transformed[indices[i]];
            const auto edge1 = transformed[indices[i + 1]] - a;
            const auto edge2 = transformed[indices[i + 2]] - a;
            const auto p = direction.crossProduct( edge2 );
            const auto determinant = edge1.dotProduct( p );
            const auto tolerance = std::numeric_limits<real_Num>::epsilon() * edge1.length() *
                                   edge2.length() * direction.length();
            if( std::abs( determinant ) <= tolerance )
            {
                continue;
            }
            const auto offset = origin - a;
            const auto u = offset.dotProduct( p ) / determinant;
            const auto q = offset.crossProduct( edge1 );
            const auto v = direction.dotProduct( q ) / determinant;
            if( u < 0 || v < 0 || u + v > 1 )
            {
                continue;
            }
            const auto distance = edge2.dotProduct( q ) / determinant;
            if( std::isfinite( distance ) && distance >= 0 && distance < nearest )
            {
                nearest = distance;
                hit = true;
            }
        }
        return hit;
    }

    inline SmartPtr<IGameActor> pickSceneActor( const Array<SmartPtr<IGameActor>> &roots,
                                                const Ray3<real_Num> &ray, real_Num maximumDistance )
    {
        SmartPtr<IGameActor> result;
        auto nearest = maximumDistance;
        std::set<IGameActor *> visited;
        auto pending = roots;
        while( !pending.empty() )
        {
            auto actor = pending.back();
            pending.pop_back();
            if( !actor || !visited.insert( actor.get() ).second || !actor->isEnabledInScene() ||
                !actor->isVisible() || actor->getFlag( IGameActor::ActorFlagIsEditor ) )
            {
                continue;
            }
            bool visible = true;
            for( auto parent = actor->getParent(); parent; parent = parent->getParent() )
            {
                if( !parent->isVisible() || !parent->isEnabledInScene() )
                {
                    visible = false;
                    break;
                }
            }
            if( !visible )
            {
                continue;
            }
            const auto children = actor->getChildren();
            pending.insert( pending.end(), children.begin(), children.end() );
            auto renderer = actor->getComponent<MeshRenderer>();
            auto component = actor->getComponent<Mesh>();
            auto resource = component ? component->getMeshResource() : nullptr;
            auto mesh = resource ? resource->getMesh() : nullptr;
            if( renderer && renderer->isEnabled() && component && component->isEnabled() && mesh &&
                pickMeshTriangles( ray, actor->getWorldTransform().getTransformationMatrix(),
                                   MeshUtil::getPoints( mesh ), MeshUtil::getIndices( mesh ), nearest ) )
            {
                result = actor;
            }
        }
        return result;
    }
}  // namespace workphone::scene

#endif
