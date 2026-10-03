#ifndef AnimationGraphNodes_h__
#define AnimationGraphNodes_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace animation
    {
        /**
         * @file AnimationGraphNodes.hpp
         * @brief Data-driven pose node definitions for the animation graph.
         *
         * These types adapt Esoterica's PoseNode hierarchy (single-clip, parameterized 1D
         * blend and selector nodes) to the Workphone type system. A state in an
         * AnimationGraphDefinition may reference either a single clip (the legacy flat
         * state-machine behaviour) or a pose node tree assembled from these definitions,
         * allowing blend spaces and parameter-driven selection to be authored as data
         * and exposed to the editor without changing the runtime evaluation contract.
         */

        /** Type of a pose node in the data-driven animation graph. */
        enum class AnimationGraphPoseNodeType
        {
            Clip,      ///< Samples a single animation clip.
            Blend1D,   ///< Parameterized 1D blend between ordered child pose nodes.
            Selector,  ///< Selects one child pose node by parameter value.
        };

        /**
         * @brief A blend segment between two child inputs over a parameter range.
         *
         * Mirrors Esoterica's BlendRange: when the driving parameter falls inside
         * [parameterLow, parameterHigh] the node blends between the pose produced by
         * inputIndex0 and inputIndex1 using the normalized position within the range.
         */
        struct WPCore_API AnimationGraphBlendRange
        {
            u32 inputIndex0 = 0;  ///< Index into the parent node's childNodeIds.
            u32 inputIndex1 = 0;  ///< Index into the parent node's childNodeIds.
            f32 parameterLow = 0.0f;
            f32 parameterHigh = 0.0f;

            bool isValid() const;
            bool contains( f32 parameterValue ) const;
            f32 blendWeight( f32 parameterValue ) const;
        };

        /**
         * @brief Serializable definition of a pose node.
         *
         * Clip nodes reference a clip id and a per-clip playback speed/loop flag. Blend1D
         * and Selector nodes reference an ordered list of child pose node ids and a driving
         * parameter. Blend1D nodes additionally own an ordered list of blend ranges that
         * tile the parameter space. All references are by string id so the definition is
         * fully data-driven and independent of the runtime instance.
         */
        struct WPCore_API AnimationGraphPoseNodeDefinition
        {
            String id;
            AnimationGraphPoseNodeType type = AnimationGraphPoseNodeType::Clip;

            // Clip node fields
            String clipId;
            f32 speed = 1.0f;
            bool loop = true;

            // Blend1D / Selector node fields
            Array<String> childNodeIds;  ///< Ordered child pose node ids.
            String parameter;            ///< Float parameter driving the blend/selection.

            // Blend1D only: ordered, non-overlapping blend ranges across the parameter space.
            Array<AnimationGraphBlendRange> blendRanges;

            bool isValid() const;
        };

    }  // namespace animation
}  // namespace workphone

#endif  // AnimationGraphNodes_h__
