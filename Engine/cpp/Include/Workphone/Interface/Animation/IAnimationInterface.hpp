#ifndef IAnimationInterface_h__
#define IAnimationInterface_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for an AnimationInterface. */
    class WPCore_API IAnimationInterface : public ISharedObject
    {
    public:
        /** Destructor */
        ~IAnimationInterface() override;

        /** Creates a new Animation object for vertex animating this mesh.
        @param name
            The name of this animation.
        @param length
            The length of the animation in seconds.
        */
        virtual SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) = 0;

        /** Returns the named vertex Animation object.
        @param name
            The name of the animation.
        */
        virtual SmartPtr<IAnimation> getAnimation( const String &name ) const = 0;

        /** Returns whether this mesh contains the named vertex animation. */
        virtual bool hasAnimation( const String &name ) const = 0;

        /** Removes vertex Animation from this mesh. */
        virtual void removeAnimation( const String &name ) = 0;

        /** Gets the number of morph animations in this mesh. */
        virtual u16 getNumAnimations() const = 0;

        /** Gets a single morph animation by index.
         */
        virtual SmartPtr<IAnimation> getAnimation( u16 index ) const = 0;

        /** Gets a pointer to a vertex data element based on a morph animation track handle.
        @remarks
            0 means the shared vertex data, 1+ means a submesh vertex data (index+1)
        */
        virtual void *getVertexDataByTrackHandle( u16 handle ) = 0;

        /** Clones the animation interface.*/
        virtual SmartPtr<IAnimationInterface> clone() = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IAnimationInterface_h__
