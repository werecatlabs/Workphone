#pragma once

#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::animation
{
    inline constexpr const char *animationGraphPropertiesName = "animationGraph";

    /** Serializes the authored definition, with clips referenced by their animation names. */
    WPCore_API SmartPtr<Properties> writeAnimationGraphProperties(
        const AnimationGraphDefinition &definition );

    /** Validates the stored graph before its animation clips are available. */
    WPCore_API bool validateAnimationGraphProperties( const Properties &properties );

    /** Binds the stored graph to an Animator's clips. Leaves definition unchanged on failure. */
    WPCore_API bool readAnimationGraphProperties( const Properties &properties,
                                                  const Array<SmartPtr<IAnimation>> &clips,
                                                  AnimationGraphDefinition &definition );
}  // namespace workphone::animation
