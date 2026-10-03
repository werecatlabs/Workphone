#ifndef IAnimationMorphKeyFrame_h__
#define IAnimationMorphKeyFrame_h__

#include <Workphone/Interface/Animation/IAnimationKeyFrame.hpp>

namespace workphone
{

    /** Interface for an AnimationMorphKeyFrame. */
    class WPCore_API IAnimationMorphKeyFrame : public IAnimationKeyFrame
    {
    public:
        ~IAnimationMorphKeyFrame() override;

        /** Sets the vertex buffer for this morph keyframe. */
        virtual void setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer ) = 0;

        /** Gets the vertex buffer for this morph keyframe. */
        virtual SmartPtr<IVertexBuffer> getVertexBuffer() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationMorphKeyFrame_h__
