#ifndef ClawSceneNode_h__
#define ClawSceneNode_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include "workphone_graphics_scenenode.h"

namespace workphone
{
    namespace render
    {

        /**
         * @class ClawSceneNode
         * @brief Implementation of a scene node specifically for Claw graphics integration.
         *
         * This class bridges the workphone GraphicsSceneNode interface with the native
         * wp_scenenode implementation.
         */
        class WPGraphics_API ClawSceneNode : public GraphicsSceneNode
        {
        public:
            ClawSceneNode();

            /**
             * @brief Constructs a ClawSceneNode with a specified scene creator.
             * @param creator The graphics scene responsible for creating this node.
             */
            ClawSceneNode( SmartPtr<IGraphicsScene> creator );

            ~ClawSceneNode() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Retrieves the underlying native Claw scene node.
             * @return Pointer to the native wp_scenenode.
             */
            wp_scenenode *getNativeNode() const;

            void setTransform( const Transform3<real_Num> &transform ) override;

            void setWorldTransform( const Transform3<real_Num> &transform ) override;
            Transform3<real_Num> getWorldTransform() const override;
            void setPosition( const Vector3<real_Num> &position ) override;
            void setOrientation( const Quaternion<real_Num> &orientation ) override;
            void setScale( const Vector3<real_Num> &scale ) override;
            Vector3<real_Num> getWorldPosition() const override;
            Quaternion<real_Num> getWorldOrientation() const override;
            Vector3<real_Num> getWorldScale() const override;

            void setParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            void attachObject( SmartPtr<IGraphicsObject> object ) override;
            void detachObject( SmartPtr<IGraphicsObject> object ) override;

            void _getObject( void **ppObject ) const override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void applyNativeTransform( wp_scenenode *node, const Transform3<real_Num> &transform );

            /**
             * @brief Configures the state context for the scene node.
             */
            void setupStateContext();

            wp_scenenode *m_node;  ///< Pointer to the native Claw scene node implementation.
            bool m_sceneOwnsNode = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawSceneNode_h__
