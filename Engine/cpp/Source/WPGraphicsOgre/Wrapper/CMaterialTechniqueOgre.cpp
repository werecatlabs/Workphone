#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CMaterialTechniqueOgre, IMaterialTechnique );

        CMaterialTechniqueOgre::CMaterialTechniqueOgre()
        {
        }

        CMaterialTechniqueOgre::~CMaterialTechniqueOgre()
        {
            unload( nullptr );
        }

        void CMaterialTechniqueOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto pMaterial = getMaterial();
                WP_ASSERT( pMaterial );
                //WP_ASSERT( pMaterial->isValid() );

                auto material = workphone::static_pointer_cast<CMaterialOgre>( pMaterial );

                if( !m_technique )
                {
                    auto ogreMaterial = material->getMaterial();
                    auto technique = ogreMaterial->createTechnique();
                    setTechnique( technique );
                }

                WP_ASSERT( m_technique );

                if( getNumPasses() == 0 )
                {
                    if( auto p = workphone::static_pointer_cast<CMaterialPassOgre>( createPass() ) )
                    {
                        p->setMaterial( pMaterial );
                        p->setParent( this );
                    }
                }

                auto ogrePasses = m_technique->getPasses();

                auto count = 0;

                auto passes = getPasses();
                for( auto pPass : passes )
                {
                    auto pass = workphone::static_pointer_cast<CMaterialPassOgre>( pPass );

                    if( count < ogrePasses.size() )
                    {
                        pass->setPass( ogrePasses[count] );
                    }

                    pass->setMaterial( pMaterial );
                    pass->setParent( this );
                    pass->load( nullptr );

                    count++;
                }

                WP_ASSERT( pMaterial->isValid() );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialTechniqueOgre::reload( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto passes = getPasses();
                for( auto pass : passes )
                {
                    pass->reload( data );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialTechniqueOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                auto passes = getPasses();
                for( auto pass : passes )
                {
                    pass->unload( nullptr );
                }

                removeAllChildren();

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialTechniqueOgre::initialise( Ogre::Technique *technique )
        {
            setTechnique( technique );
        }

        SmartPtr<Properties> CMaterialTechniqueOgre::getProperties() const
        {
            auto properties = MaterialNode<IMaterialTechnique>::getProperties();

            auto name = getName();
            properties->setProperty( "name", name );

            return properties;
        }

        void CMaterialTechniqueOgre::setProperties( SmartPtr<Properties> properties )
        {
        }

        Array<SmartPtr<ISharedObject>> CMaterialTechniqueOgre::getChildObjects() const
        {
            auto passes = getPasses();

            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( passes.size() );

            for( auto pass : passes )
            {
                objects.push_back( pass );
            }

            return objects;
        }

        Ogre::Technique *CMaterialTechniqueOgre::getTechnique() const
        {
            return m_technique;
        }

        void CMaterialTechniqueOgre::setTechnique( Ogre::Technique *technique )
        {
            if( m_technique != technique )
            {
                m_technique = technique;

                auto ogrePasses = technique->getPasses();

                auto numPasses = getNumChildren();

                if( ogrePasses.size() != numPasses )
                {
                    removePasses();

                    for( auto pass : ogrePasses )
                    {
                        auto pPass = workphone::make_ptr<CMaterialPassOgre>();
                        pPass->setParent( this );
                        pPass->initialise( pass );
                        addPass( pPass );
                    }
                }
            }
        }

        bool CMaterialTechniqueOgre::MaterialTextureOgreStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool CMaterialTechniqueOgre::MaterialTextureOgreStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            return false;
        }

    }  // end namespace render
}  // namespace workphone
