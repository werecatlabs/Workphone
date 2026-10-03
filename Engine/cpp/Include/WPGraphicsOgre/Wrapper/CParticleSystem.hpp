#ifndef _CParticleSystem_H
#define _CParticleSystem_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleNode.hpp>
#include <Workphone/Core/HashMap.hpp>

#if WP_OGRE_USE_PARTICLE_UNIVERSE
#    include "ParticleUniverseSystemManager.h"
#endif

namespace workphone
{
    namespace render
    {
#if WP_OGRE_USE_PARTICLE_UNIVERSE

        class PUParticleSystem : public CGraphicsObjectOgre<ParticleSystem>
        {
        public:
            static const hash_type STATE_MESSAGE_RENDER_QUEUE_GROUP;

            PUParticleSystem();
            PUParticleSystem( SmartPtr<IGraphicsScene> creator );
            ~PUParticleSystem() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void initialise( ParticleUniverse::ParticleSystem *particleSystem );

            void update() override;

            // IGraphicsObject functions
            void setName( const String &name );
            String getName() const;
            u32 getId() const;

            void setMaterialName( const String &materialName, s32 index = -1 ) override;
            String getMaterialName( s32 index = -1 ) const override;

            void setCastShadows( bool castShadows ) override;
            bool getCastShadows() const override;

            void setRecieveShadows( bool recieveShadows );
            bool getRecieveShadows() const;

            void setVisible( bool isVisible ) override;
            bool isVisible() const override;

            void setRenderQueueGroup( u8 renderQueue );

            void setVisibilityFlags( u32 flags ) override;
            u32 getVisibilityFlags() const override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            const String &getComponentType() const;

            void setPropertyValue( const String &name, const String &value );
            bool getPropertyValue( const String &name, String &value );

            void setProperties( const Properties &propertyGroup );
            bool getProperties( Properties &propertyGroup, u32 flags = AllProperties ) const;

            //IGraphicsSceneNode *getOwner() const;
            //void setOwner( IGraphicsSceneNode *sceneNode );

            // IParticleSystem functions

            void setFastForward( f32 time, f32 interval ) override;
            f32 getFastForwardTime() const override;
            f32 getFastForwardInterval() const override;

            void setTemplateName( const String &templateName ) override;
            String getTemplateName( void ) const override;

            void setScale( const Vector3F &scale ) override;
            Vector3F getScale() const override;

            SmartPtr<IParticleTechnique> getTechnique( const String &name ) const override;

            ParticleUniverse::ParticleSystem *getSystem() const;

            void setParticleSystem( ParticleUniverse::ParticleSystem *particleSystem );

            //
            // IScriptObjects
            //

            /** Gets an object call script functions. */
            virtual SmartPtr<IScriptInvoker> &getInvoker();

            /** Gets an object call script functions. */
            virtual const SmartPtr<IScriptInvoker> &getInvoker() const;

            /** Sets an object call script functions. */
            virtual void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /** Gets an object to receive script calls. */
            virtual SmartPtr<IScriptReceiver> &getReceiver();

            /** Gets an object to receive script calls. */
            virtual const SmartPtr<IScriptReceiver> &getReceiver() const;

            /** Sets an object to receive script calls. */
            virtual void setReceiver( SmartPtr<IScriptReceiver> receiver );

            s32 getObject( u32 hash, SmartPtr<IObject> &object ) const;

            //void _updateRenderQueue( SmartPtr<IRenderQueue> queue );

        protected:
            class ParticleSystemStateListener : public IStateListener
            {
            public:
                ParticleSystemStateListener( PUParticleSystem *puSystem );

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                PUParticleSystem *m_puSystem;
            };

            SmartPtr<IScriptInvoker> m_scriptInvoker;
            SmartPtr<IScriptReceiver> m_scriptReceiver;

            ParticleUniverse::ParticleSystem *m_particleSystem = nullptr;

            using Techniques = HashMap<u32, SmartPtr<IParticleTechnique>>;
            Techniques m_techniques;

            SmartPtr<IGraphicsScene> m_creator;
            String m_name;
            String m_templateName;

            atomic_bool m_visible;
            atomic_u32 m_renderQueueGroup;
        };

#endif
    }  // namespace render
}  // namespace workphone

#endif
