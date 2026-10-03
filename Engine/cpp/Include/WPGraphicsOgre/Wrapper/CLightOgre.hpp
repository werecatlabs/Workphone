#ifndef _CLight_H
#define _CLight_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>

namespace workphone
{
    namespace render
    {
        class CLightOgre : public CGraphicsObjectOgre<GraphicsLight>
        {
        public:
            class CLightStateListener : public GraphicsObjectOgreStateListener
            {
            public:
                CLightStateListener();
                ~CLightStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                WP_CLASS_REGISTER_DECL;
            };

            CLightOgre();
            ~CLightOgre() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) override;
            void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            // ILight functions
            Ogre::Light *getLight() const;
            void setLight( Ogre::Light *light );

            WP_CLASS_REGISTER_DECL;

        protected:
            void setupStateObject() override;

            Ogre::SceneNode *m_dummyNode = nullptr;
            Ogre::Light *m_light = nullptr;

            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
