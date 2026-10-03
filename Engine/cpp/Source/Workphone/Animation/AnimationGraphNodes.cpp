#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationGraphNodes.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::animation
{
    namespace
    {
        constexpr f32 Epsilon = std::numeric_limits<f32>::epsilon();
    }  // namespace

    bool AnimationGraphBlendRange::isValid() const
    {
        return inputIndex0 != inputIndex1 && std::isfinite( parameterLow ) &&
               std::isfinite( parameterHigh ) && parameterHigh >= parameterLow + Epsilon;
    }

    bool AnimationGraphBlendRange::contains( f32 parameterValue ) const
    {
        return parameterValue >= parameterLow && parameterValue <= parameterHigh;
    }

    f32 AnimationGraphBlendRange::blendWeight( f32 parameterValue ) const
    {
        const auto span = parameterHigh - parameterLow;
        if( span <= Epsilon )
        {
            return 0.0f;
        }
        return std::clamp( ( parameterValue - parameterLow ) / span, 0.0f, 1.0f );
    }

    bool AnimationGraphPoseNodeDefinition::isValid() const
    {
        if( id.empty() || !std::isfinite( speed ) || speed < 0.0f )
        {
            return false;
        }

        switch( type )
        {
        case AnimationGraphPoseNodeType::Clip:
            return !clipId.empty();
        case AnimationGraphPoseNodeType::Blend1D:
        {
            if( parameter.empty() || childNodeIds.size() < 2 )
            {
                return false;
            }
            return std::all_of(
                blendRanges.begin(), blendRanges.end(),
                []( const AnimationGraphBlendRange &range ) { return range.isValid(); } );
        }
        case AnimationGraphPoseNodeType::Selector:
            return !parameter.empty() && !childNodeIds.empty();
        default:
            return false;
        }
    }
}  // namespace workphone::animation
