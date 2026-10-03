#ifndef __AnimationMorphKeyFrame_h__
#define __AnimationMorphKeyFrame_h__

#include <Workphone/Interface/Animation/IAnimationMorphKeyFrame.hpp>

namespace workphone
{
    /**
     * @class AnimationMorphKeyFrame
     * @brief Represents a keyframe specifically for morph target animations, storing the timestamp and
     * the associated vertex buffer.
     */
    class AnimationMorphKeyFrame : public IAnimationMorphKeyFrame
    {
    public:
        /** @brief Default constructor. */
        AnimationMorphKeyFrame();

        /** @brief Gets the timestamp of this morph keyframe. */
        f32 getTime() const override;

        /** @brief Sets the timestamp of this morph keyframe. */
        void setTime( f32 time ) override;

        /** @brief Sets the vertex buffer containing the morph target data. */
        void setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer ) override;

        /** @brief Gets the vertex buffer associated with this morph keyframe. */
        SmartPtr<IVertexBuffer> getVertexBuffer() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        f32 m_time = 0.0f;                       ///< The timestamp of the keyframe in seconds.
        SmartPtr<IVertexBuffer> m_vertexBuffer;  ///< The vertex buffer for the morph target.
    };
}  // namespace workphone

#endif  // __AnimationMorphKeyFrame_h__
