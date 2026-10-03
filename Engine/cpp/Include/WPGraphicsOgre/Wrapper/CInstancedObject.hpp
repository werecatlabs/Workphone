#ifndef CInstanceObject_h__
#define CInstanceObject_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IInstancedObject.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>

namespace workphone
{
    namespace render
    {
        class CInstancedObject : public CGraphicsObjectOgre<IInstancedObject>
        {
        public:
            CInstancedObject( SmartPtr<IGraphicsScene> smgr, const String &materialName,
                              const String &instanceManagerName );
            ~CInstancedObject() override;

            void update() override;

            void setPosition( const Vector3F &position ) override;
            Vector3F getPosition() const override;

            void setOrientation( const QuaternionF &orientation ) override;
            QuaternionF getOrientation() const override;

            void setScale( const Vector3F &scale ) override;
            Vector3F getScale() const override;

            void detachFromParent();

            void _attachToParent( SmartPtr<IGraphicsSceneNode> parent );

            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            String getMaterialName( s32 index = -1 ) const override;

            void setCastShadows( bool castShadows ) override;

            bool getCastShadows() const override;

            void setRecieveShadows( bool recieveShadows );

            bool getRecieveShadows() const;

            void setVisible( bool isVisible ) override;

            bool isVisible() const override;

            void setRenderQueueGroup( u8 renderQueueGroup );

            void setVisibilityFlags( u32 flags ) override;

            u32 getVisibilityFlags() const override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            void setPropertyValue( const String &name, const String &value );

            bool getPropertyValue( const String &name, String &value );

            void setProperties( const Properties &propertyGroup );

            bool getProperties( Properties &propertyGroup, u32 flags = AllProperties ) const;

            void setFlag( u32 flag, bool value ) override;

            bool getFlag( u32 flag ) const override;

            AABB3F getLocalAABB() const override;

            void setLocalAABB( const AABB3F &localAABB ) override;

            Ogre::InstancedEntity *getInstancedEntity() const;
            void setInstancedEntity( Ogre::InstancedEntity *instancedEntity );

            SmartPtr<IStateContext> getStateContext() const;
            void setStateContext( SmartPtr<IStateContext> stateContext );

            void setCustomParam( unsigned char idx, const Vector4F &newParam ) override;
            Vector4F getCustomParam( unsigned char idx ) override;

            bool isAttached() const override;

        protected:
            class InstancedObjectStateListener : public IStateListener
            {
            public:
                InstancedObjectStateListener( CInstancedObject *instancedObject );
                ~InstancedObjectStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                CInstancedObject *m_owner = nullptr;
            };

            class InstancedObjectState : public IState
            {
            public:
                InstancedObjectState();
                ~InstancedObjectState() override;

                bool isModified() const;
                void setModified( bool modified );

                u32 getModifiedFlags() const;
                void setModifiedFlags( u32 modifiedFlags );

                bool isDirty() const override;
                void setDirty( bool dirty ) override;

                void update() override;

                bool isRegistered() const;

                void setRegistered( bool registered );

                SpinRWMutex Mutex;
                u32 m_visibilityMask;
                u32 m_renderQueueGroup;
                //UpdateCounter m_updateCount;
                bool m_isVisible;
                atomic_bool m_isRegistered;
                Vector4F m_customParams[8];
            };

            using InstancedObjectStatePtr = SmartPtr<InstancedObjectState>;

            void setState( SmartPtr<IState> state );
            SmartPtr<IState> getState() const;

            void updateState();

            AABB3F m_localAABB;
            SmartPtr<IGraphicsScene> m_smgr;
            Ogre::InstancedEntity *m_instancedEntity;
            IGraphicsSceneNode *m_owner = nullptr;
            SmartPtr<IStateContext> m_stateContext;
            SmartPtr<IStateListener> m_stateListener;
            InstancedObjectStatePtr m_state;

            String m_name;

            String m_materialName;
            String m_instanceManagerName;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CInstanceObject_h__
