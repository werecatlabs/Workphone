#ifndef __CLightOgreNext_H
#define __CLightOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>

namespace workphone
{
    namespace render
    {
        /** Implementation of the ILight interface for OgreNext. */
        class CLightOgreNext : public CGraphicsObjectOgreNext<GraphicsLight>
        {
        public:
            /** Constructor. */
            CLightOgreNext();

            /** Constructor. */
            CLightOgreNext( const CLightOgreNext &other ) = delete;

            /** Constructor. */
            CLightOgreNext( SmartPtr<IGraphicsScene> creator );

            /** Destructor. */
            ~CLightOgreNext() override;

            /** @copydoc Light::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Light::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Light::clone */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /** @copydoc Light::_getObject */
            void _getObject( void **ppObject ) const override;

            /** @copydoc Light::getDerivedDirection */
            Vector3<real_Num> getDerivedDirection() const override;

            /** Gets the Ogre light object. */
            Ogre::Light *getLight() const;

            /** @copydoc CGraphicsObjectOgreNext<Light>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc CGraphicsObjectOgreNext<Light>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc CGraphicsObjectOgreNext<Light>::handleStateChanged */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /** @copydoc CGraphicsObjectOgreNext<Light>::handleStateChanged */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            Ogre::SceneNode *getLightNode() const;
            void setLightNode( Ogre::SceneNode *node );

            WP_CLASS_REGISTER_DECL;

        protected:
            void setupStateContext();

            // The Ogre light object.
            AtomicRawPtr<Ogre::Light> m_light;

            AtomicRawPtr<Ogre::SceneNode> m_lightNode;

            // Used to generate a unique name for the Ogre light object.
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
