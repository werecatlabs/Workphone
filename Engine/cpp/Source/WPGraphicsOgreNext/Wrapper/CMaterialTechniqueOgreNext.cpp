#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTechniqueOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialPassOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreTechnique.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialTechniqueOgreNext, MaterialTechnique );

    CMaterialTechniqueOgreNext::CMaterialTechniqueOgreNext() = default;

    CMaterialTechniqueOgreNext::~CMaterialTechniqueOgreNext()
    {
        unload( nullptr );
    }

    void CMaterialTechniqueOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            MaterialTechnique::load( data );

            if( getNumPasses() == 0 )
            {
                auto pass = createPass();
                WP_ASSERT( pass );
            }

            auto passes = getPasses();
            for( auto &pass : passes )
            {
                pass->load( nullptr );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialTechniqueOgreNext::reload( SmartPtr<ISharedObject> data )
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

    void CMaterialTechniqueOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );
            MaterialTechnique::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialTechniqueOgreNext::initialise( Ogre::Technique *technique )
    {
        m_technique = technique;
    }

    auto CMaterialTechniqueOgreNext::getScheme() const -> hash32
    {
        return 0;
    }

    void CMaterialTechniqueOgreNext::setScheme( hash32 scheme )
    {
    }

    auto CMaterialTechniqueOgreNext::createPass() -> SmartPtr<IMaterialPass>
    {
        try
        {
            WP_ASSERT( getPasses().size() == 0 );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto pass = factoryManager->make_ptr<CMaterialPassOgreNext>();
            WP_ASSERT( pass );

            auto ogrePass = static_cast<Ogre::Pass *>( nullptr );
            if( m_technique )
            {
                ogrePass = m_technique->createPass();
                pass->initialise( ogrePass );
            }

            auto material = getMaterial();
            pass->setMaterial( material );

            pass->setParent( this );
            addPass( pass );

            WP_ASSERT( getPasses().size() == 1 );
            return pass;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CMaterialTechniqueOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto passes = getPasses();

        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( passes.size() );

        for( auto pass : passes )
        {
            objects.emplace_back( pass );
        }

        return objects;
    }

    auto CMaterialTechniqueOgreNext::getTechnique() const -> Ogre::Technique *
    {
        return m_technique;
    }

    void CMaterialTechniqueOgreNext::setTechnique( Ogre::Technique *technique )
    {
        m_technique = technique;
    }

}  // namespace workphone::render
