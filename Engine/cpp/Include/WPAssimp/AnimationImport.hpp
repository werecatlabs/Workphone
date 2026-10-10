#ifndef WP_ASSIMP_ANIMATION_IMPORT_H
#define WP_ASSIMP_ANIMATION_IMPORT_H

#include <assimp/scene.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace workphone::animation_import
{
    // The legacy transform track is absolute local TRS, linear translation/scale and
    // shortest-path spherical rotation. Unsupported source interpolation is rejected;
    // it must not silently become a different curve in the legacy XML cache.
    struct TransformSample
    {
        float time = 0;
        aiVector3D position;
        aiQuaternion rotation;
        aiVector3D scale;
    };

    inline bool finite( const aiVector3D &v )
    {
        return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
    }

    inline bool finite( const aiQuaternion &q )
    {
        const double norm = double( q.w ) * q.w + double( q.x ) * q.x +
                            double( q.y ) * q.y + double( q.z ) * q.z;
        return std::isfinite( norm ) && norm > 1e-20;
    }

    inline aiQuaternion normalized( aiQuaternion q )
    {
        const double norm = std::sqrt( double( q.w ) * q.w + double( q.x ) * q.x +
                                       double( q.y ) * q.y + double( q.z ) * q.z );
        q.w = static_cast<ai_real>( q.w / norm );
        q.x = static_cast<ai_real>( q.x / norm );
        q.y = static_cast<ai_real>( q.y / norm );
        q.z = static_cast<ai_real>( q.z / norm );
        return q;
    }

    inline bool equivalent( const aiVector3D &a, const aiVector3D &b )
    {
        return ( a - b ).SquareLength() <= 1e-10f;
    }

    inline bool equivalent( const aiQuaternion &a, const aiQuaternion &b )
    {
        const auto p = normalized( a ), q = normalized( b );
        return std::abs( p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z ) >= 1.0f - 1e-5f;
    }

    inline aiVector3D interpolate( const aiVector3D &a, const aiVector3D &b, double t )
    {
        return { static_cast<ai_real>( double( a.x ) + ( double( b.x ) - a.x ) * t ),
                 static_cast<ai_real>( double( a.y ) + ( double( b.y ) - a.y ) * t ),
                 static_cast<ai_real>( double( a.z ) + ( double( b.z ) - a.z ) * t ) };
    }

    inline aiQuaternion interpolate( const aiQuaternion &a, const aiQuaternion &b, double t )
    {
        aiQuaternion result;
        aiQuaternion::Interpolate( result, normalized( a ), normalized( b ), static_cast<ai_real>( t ) );
        return normalized( result );
    }

    template <class Key, class Value>
    bool validateKeys( const Key *keys, unsigned count, const Value &reference, double duration,
                       aiAnimBehaviour pre, aiAnimBehaviour post, std::set<double> &times,
                       std::string &error )
    {
        if( count > 65535 || ( count && !keys ) )
        {
            error = "invalid or oversized animation channel";
            return false;
        }
        for( unsigned i = 0; i < count; ++i )
        {
            if( !std::isfinite( keys[i].mTime ) || keys[i].mTime < 0 || keys[i].mTime > duration ||
                ( i && keys[i].mTime <= keys[i - 1].mTime ) || !finite( keys[i].mValue ) )
            {
                error = "animation keys must be finite, strictly ordered and inside the clip";
                return false;
            }
            if( keys[i].mInterpolation != aiAnimInterpolation_Linear &&
                keys[i].mInterpolation != aiAnimInterpolation_Spherical_Linear )
            {
                error = "STEP and CUBICSPLINE channels require baking before legacy track import";
                return false;
            }
            times.insert( keys[i].mTime );
        }
        if( !count ) return true;
        if( pre != aiAnimBehaviour_DEFAULT && pre != aiAnimBehaviour_CONSTANT )
        {
            error = "pre-key repetition/extrapolation requires baking before legacy track import";
            return false;
        }
        if( post != aiAnimBehaviour_DEFAULT && post != aiAnimBehaviour_CONSTANT )
        {
            error = "post-key repetition/extrapolation requires baking before legacy track import";
            return false;
        }
        // DEFAULT outside a keyed range can introduce a discontinuity. The legacy
        // linear track cannot encode that discontinuity, so never approximate it.
        if( ( keys[0].mTime > 0 && pre == aiAnimBehaviour_DEFAULT &&
              !equivalent( keys[0].mValue, reference ) ) ||
            ( keys[count - 1].mTime < duration && post == aiAnimBehaviour_DEFAULT &&
              !equivalent( keys[count - 1].mValue, reference ) ) )
        {
            error = "discontinuous default-pose channel boundary requires baking";
            return false;
        }
        return true;
    }

    template <class Key, class Value>
    Value sampleKeys( const Key *keys, unsigned count, const Value &reference, double time )
    {
        if( !count ) return reference;
        if( time <= keys[0].mTime ) return keys[0].mValue;
        if( time >= keys[count - 1].mTime ) return keys[count - 1].mValue;
        const auto upper = std::upper_bound( keys, keys + count, time,
                                            []( double t, const Key &key ) { return t < key.mTime; } );
        const auto lower = upper - 1;
        return interpolate( lower->mValue, upper->mValue,
                            ( time - lower->mTime ) / ( upper->mTime - lower->mTime ) );
    }

    inline bool sampleChannel( const aiNodeAnim &channel, const aiMatrix4x4 &bind,
                               double duration, double ticksPerSecond,
                               std::vector<TransformSample> &output, std::string &error )
    {
        error.clear();
        if( !std::isfinite( duration ) || duration < 0 || !std::isfinite( ticksPerSecond ) ||
            ticksPerSecond < 0 )
        {
            error = "invalid animation duration or tick rate";
            return false;
        }
        if( ticksPerSecond == 0 ) ticksPerSecond = 25; // Existing Assimp fallback, seconds after import.
        if( duration / ticksPerSecond > std::numeric_limits<float>::max() )
        {
            error = "animation duration cannot be represented in seconds";
            return false;
        }
        for( unsigned row = 0; row < 4; ++row )
            for( unsigned column = 0; column < 4; ++column )
                if( !std::isfinite( bind[row][column] ) )
                {
                    error = "non-finite animation reference transform";
                    return false;
                }
        aiVector3D scale, position;
        aiQuaternion rotation;
        bind.Decompose( scale, rotation, position );
        if( !finite( scale ) || !finite( position ) || !finite( rotation ) ||
            scale.x == 0 || scale.y == 0 || scale.z == 0 )
        {
            error = "singular or invalid animation reference transform";
            return false;
        }
        rotation = normalized( rotation );
        std::set<double> times{ 0, duration };
        if( !validateKeys( channel.mPositionKeys, channel.mNumPositionKeys, position, duration,
                           channel.mPreState, channel.mPostState, times, error ) ||
            !validateKeys( channel.mRotationKeys, channel.mNumRotationKeys, rotation, duration,
                           channel.mPreState, channel.mPostState, times, error ) ||
            !validateKeys( channel.mScalingKeys, channel.mNumScalingKeys, scale, duration,
                           channel.mPreState, channel.mPostState, times, error ) )
            return false;
        std::vector<TransformSample> candidate;
        if( times.size() > 65535 )
        {
            error = "merged animation channel exceeds the legacy 65535-key limit";
            return false;
        }
        candidate.reserve( times.size() );
        for( const auto time : times )
        {
            TransformSample sample;
            sample.time = static_cast<float>( time / ticksPerSecond );
            if( !candidate.empty() &&
                sample.time - candidate.back().time <= std::numeric_limits<float>::epsilon() )
            {
                error = "distinct animation key times collapse at legacy runtime precision";
                return false;
            }
            sample.position = sampleKeys( channel.mPositionKeys, channel.mNumPositionKeys, position, time );
            sample.rotation = normalized( sampleKeys( channel.mRotationKeys, channel.mNumRotationKeys, rotation, time ) );
            sample.scale = sampleKeys( channel.mScalingKeys, channel.mNumScalingKeys, scale, time );
            candidate.push_back( sample );
        }
        output.swap( candidate );
        return true;
    }
}

#endif
