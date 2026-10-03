#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    const String MaterialTechnique::nameStr = String( "name" );
    const String MaterialTechnique::schemeStr = String( "scheme" );
    const String MaterialTechnique::passStr = String( "passes" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialTechnique, IMaterialTechnique );

    MaterialTechnique::MaterialTechnique() = default;

    MaterialTechnique::~MaterialTechnique()
    {
        unload( nullptr );
    }

    void MaterialTechnique::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto passes = getPasses();
            for( auto &pass : passes )
            {
                pass->setParent( this );
                pass->load( nullptr );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialTechnique::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto passes = getPasses();
            for( auto &pass : passes )
            {
                pass->reload( data );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialTechnique::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto passes = getPasses();
            for( auto &pass : passes )
            {
                if( pass )
                {
                    pass->unload( nullptr );
                }
            }

            removeAllChildren();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTechnique::getScheme() const -> hash32
    {
        return m_scheme;
    }

    void MaterialTechnique::setScheme( hash32 scheme )
    {
        m_scheme = scheme;
    }

    auto MaterialTechnique::getNumPasses() const -> u32
    {
        return static_cast<u32>( m_children.size() );
    }

    auto MaterialTechnique::createPass() -> SmartPtr<IMaterialPass>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            // Prefer the graphics system factory manager, but fall back to the
            // application manager factory manager when no graphics system is
            // available (headless tests, editor without rendering, etc.).
            IFactoryManager *factoryManager = nullptr;
            if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
            {
                factoryManager = graphicsSystem->getFactoryManagerPtr();
            }
            if( !factoryManager )
            {
                factoryManager = applicationManager->getFactoryManagerPtr();
            }
            WP_ASSERT( factoryManager );

            // Try to construct via the factory manager first (this picks up
            // the registered IMaterialPass subclass such as ClawMaterialPass
            // when the graphics plugin is loaded). If that fails, construct a
            // plain MaterialPass so callers still get a usable object in
            // headless / unit-test setups where the plugin isn't loaded.
            auto pass = factoryManager->make_object<IMaterialPass>();
            if( !pass )
            {
                pass = workphone::make_ptr<MaterialPass>();
            }
            WP_ASSERT( pass );

            auto material = getMaterial();
            pass->setMaterial( material );

            pass->setParent( this );
            addPass( pass );
            return pass;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialTechnique::addPass( SmartPtr<IMaterialPass> pass )
    {
        m_children.push_back( pass );
    }

    void MaterialTechnique::removePass( SmartPtr<IMaterialPass> pass )
    {
        m_children.erase( std::remove( m_children.begin(), m_children.end(), pass ), m_children.end() );
    }

    void MaterialTechnique::removePasses()
    {
        m_children.clear();
    }

    auto MaterialTechnique::getPasses() const -> Array<SmartPtr<IMaterialPass>>
    {
        auto children = getChildren();

        auto passes = Array<SmartPtr<IMaterialPass>>();
        passes.reserve( children.size() );

        for( auto &child : children )
        {
            auto pass = workphone::static_pointer_cast<IMaterialPass>( child );
            passes.push_back( pass );
        }

        return passes;
    }

    void MaterialTechnique::setPasses( Array<SmartPtr<IMaterialPass>> passes )
    {
        m_children = { passes.begin(), passes.end() };
    }

    SmartPtr<IMaterialPass> MaterialTechnique::getPass( u32 index ) const
    {
        if( index < m_children.size() )
        {
            return m_children[index];
        }

        return nullptr;
    }

    auto MaterialTechnique::toData() const -> SmartPtr<ISharedObject>
    {
        auto data = workphone::make_ptr<Properties>();
        data->setProperty( schemeStr, getScheme() );

        auto passes = getPasses();
        for( auto pass : passes )
        {
            auto passData = workphone::static_pointer_cast<Properties>( pass->toData() );
            passData->setName( passStr );
            data->addChild( passData );
        }

        return data;
    }

    void MaterialTechnique::fromData( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::static_pointer_cast<Properties>( data );

        hash32 scheme = 0;
        if( properties->getPropertyValue( schemeStr, scheme ) )
        {
            setScheme( scheme );
        }

        auto count = 0;

        auto currentPasses = getPasses();

        auto passes = properties->getChildrenByName( "passes" );
        for( auto &pass : passes )
        {
            auto pPass = SmartPtr<IMaterialPass>();

            if( count < currentPasses.size() )
            {
                pPass = currentPasses[count];
            }
            else
            {
                pPass = createPass();
            }

            pPass->fromData( pass );

            count++;
        }
    }

    auto MaterialTechnique::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = MaterialNode<IMaterialTechnique>::getProperties();

        auto handle = getHandle();
        properties->setProperty( MaterialTechnique::nameStr, getName() );
        properties->setProperty( schemeStr, getScheme() );

        return properties;
    }

    void MaterialTechnique::setProperties( SmartPtr<Properties> properties )
    {
        MaterialNode<IMaterialTechnique>::setProperties( properties );

        hash32 scheme = 0;
        if( properties->getPropertyValue( schemeStr, scheme ) )
        {
            setScheme( scheme );
        }
    }

    auto MaterialTechnique::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
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

    bool MaterialTechnique::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto passes = getPasses();
        for( auto &pass : passes )
        {
            if( pass )
            {
                if( pass->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool MaterialTechnique::handleStateChanged( SmartPtr<IState> &state )
    {
        auto passes = getPasses();
        for( auto &pass : passes )
        {
            if( pass )
            {
                if( pass->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool MaterialTechnique::MaterialTechniqueStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool MaterialTechnique::MaterialTechniqueStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    MaterialTechnique::MaterialTechniqueStateListener::MaterialTechniqueStateListener() = default;

    MaterialTechnique::MaterialTechniqueStateListener::~MaterialTechniqueStateListener() = default;

}  // namespace workphone::render
