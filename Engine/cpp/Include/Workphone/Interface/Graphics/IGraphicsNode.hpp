#ifndef __WP_INode_h__
#define __WP_INode_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a node in a scene graph. */
        class WPCore_API IGraphicsNode : public ISharedObject
        {
        public:
            /** Destructor. */
            ~IGraphicsNode() override;

            /** Gets this node's parent (NULL if this is the root).
             */
            virtual SmartPtr<IGraphicsNode> getParent() const = 0;

            virtual Quaternion<real_Num> getOrientation() const = 0;
            virtual void setOrientation( const Quaternion<real_Num> &q ) = 0;

            virtual Vector3<real_Num> getPosition() const = 0;
            virtual void setPosition( const Vector3<real_Num> &pos ) = 0;

            virtual Vector3<real_Num> getScale() const = 0;
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // __WP_INode_h__
