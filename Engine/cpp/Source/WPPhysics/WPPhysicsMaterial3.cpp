#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsMaterial3.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

namespace workphone::physics
{
    WPPhysicsMaterial3::WPPhysicsMaterial3() : m_material( wp_physics_material_create() )
    {
        if( !m_material )
        {
            throw std::runtime_error( "Failed to create a WPPhysics material." );
        }
    }
    WPPhysicsMaterial3::~WPPhysicsMaterial3()
    {
        wp_physics_material_destroy( m_material );
        m_material = nullptr;
    }
    f32 WPPhysicsMaterial3::getFriction( s32 direction ) const
    {
        return getStaticFriction( direction );
    }
    void WPPhysicsMaterial3::setFriction( f32 friction, s32 direction )
    {
        setStaticFriction( friction, direction );
        setDynamicFriction( friction, direction );
    }
    f32 WPPhysicsMaterial3::getDynamicFriction( s32 ) const
    {
        return wp_physics_material_get_dynamic_friction( m_material );
    }
    void WPPhysicsMaterial3::setDynamicFriction( f32 friction, s32 )
    {
        wp_physics_material_set_dynamic_friction( m_material, std::max( friction, 0.0f ) );
    }
    f32 WPPhysicsMaterial3::getStaticFriction( s32 ) const
    {
        return wp_physics_material_get_static_friction( m_material );
    }
    void WPPhysicsMaterial3::setStaticFriction( f32 friction, s32 )
    {
        wp_physics_material_set_static_friction( m_material, std::max( friction, 0.0f ) );
    }
    f32 WPPhysicsMaterial3::getRestitution() const
    {
        return wp_physics_material_get_restitution( m_material );
    }
    void WPPhysicsMaterial3::setRestitution( f32 restitution )
    {
        wp_physics_material_set_restitution( m_material, std::clamp( restitution, 0.0f, 1.0f ) );
    }

    f32 WPPhysicsMaterial3::getRollingFriction() const
    {
        return wp_physics_material_get_rolling_friction( m_material );
    }
    void WPPhysicsMaterial3::setRollingFriction( f32 friction )
    {
        wp_physics_material_set_rolling_friction( m_material, std::max( friction, 0.0f ) );
    }
    FrictionCombineMode WPPhysicsMaterial3::getFrictionCombineMode() const
    {
        return static_cast<FrictionCombineMode>(
            wp_physics_material_get_friction_combine_mode( m_material ) );
    }
    void WPPhysicsMaterial3::setFrictionCombineMode( FrictionCombineMode mode )
    {
        wp_physics_material_set_friction_combine_mode( m_material,
                                                       static_cast<wp_friction_combine_mode>( mode ) );
    }
    RestitutionCombineMode WPPhysicsMaterial3::getRestitutionCombineMode() const
    {
        return static_cast<RestitutionCombineMode>(
            wp_physics_material_get_restitution_combine_mode( m_material ) );
    }
    void WPPhysicsMaterial3::setRestitutionCombineMode( RestitutionCombineMode mode )
    {
        wp_physics_material_set_restitution_combine_mode(
            m_material, static_cast<wp_restitution_combine_mode>( mode ) );
    }
    String WPPhysicsMaterial3::getMaterialName() const
    {
        return String( reinterpret_cast<const char *>( wp_physics_material_get_name( m_material ) ) );
    }
    void WPPhysicsMaterial3::setMaterialName( const String &name )
    {
        wp_physics_material_set_name( m_material, reinterpret_cast<const wp_c8 *>( name.c_str() ) );
    }
    Vector3<real_Num> WPPhysicsMaterial3::getContactPosition() const
    {
        return Vector3<real_Num>();
    }
    Vector3<real_Num> WPPhysicsMaterial3::getContactNormal() const
    {
        return Vector3<real_Num>();
    }
    SmartPtr<IRigidBody3> WPPhysicsMaterial3::getPhysicsBodyA() const
    {
        return nullptr;
    }
    SmartPtr<IRigidBody3> WPPhysicsMaterial3::getPhysicsBodyB() const
    {
        return nullptr;
    }
    void WPPhysicsMaterial3::saveToFile( const String &filePath )
    {
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_LOG_ERROR( "WPPhysicsMaterial3::saveToFile: file path is empty." );
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->getFileSystem() )
            {
                WP_LOG_ERROR( "WPPhysicsMaterial3::saveToFile: file system is unavailable." );
                return;
            }

            auto properties = getProperties();
            applicationManager->getFileSystem()->writeAllText(
                filePath, DataUtil::toString( properties.get(), true ) );
            setFilePath( filePath );

            FileInfo fileInfo;
            if( applicationManager->getFileSystem()->findFileInfo( filePath, fileInfo ) )
            {
                setFileSystemId( fileInfo.fileId );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
    void WPPhysicsMaterial3::loadFromFile( const String &filePath )
    {
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_LOG_ERROR( "WPPhysicsMaterial3::loadFromFile: file path is empty." );
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->getFileSystem() )
            {
                WP_LOG_ERROR( "WPPhysicsMaterial3::loadFromFile: file system is unavailable." );
                return;
            }

            const auto contents = applicationManager->getFileSystem()->readAllText( filePath );
            if( StringUtil::isNullOrEmpty( contents ) )
            {
                WP_LOG_ERROR( "WPPhysicsMaterial3::loadFromFile: file is empty or unavailable: " +
                              filePath );
                return;
            }

            auto properties = workphone::make_ptr<Properties>();
            DataUtil::parse( contents, properties.get() );
            setProperties( properties );
            setFilePath( filePath );

            FileInfo fileInfo;
            if( applicationManager->getFileSystem()->findFileInfo( filePath, fileInfo ) )
            {
                setFileSystemId( fileInfo.fileId );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
    void WPPhysicsMaterial3::save()
    {
        saveToFile( getFilePath() );
    }
    void WPPhysicsMaterial3::import()
    {
        loadFromFile( getFilePath() );
    }
    void WPPhysicsMaterial3::reimport()
    {
        loadFromFile( getFilePath() );
    }
    UUID WPPhysicsMaterial3::getFileSystemId() const
    {
        return m_fileSystemId;
    }
    void WPPhysicsMaterial3::setFileSystemId( UUID id )
    {
        m_fileSystemId = id;
    }
    String WPPhysicsMaterial3::getFilePath() const
    {
        return m_filePath;
    }
    void WPPhysicsMaterial3::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }
    UUID WPPhysicsMaterial3::getSettingsFileSystemId() const
    {
        return m_settingsFileSystemId;
    }
    void WPPhysicsMaterial3::setSettingsFileSystemId( UUID id )
    {
        m_settingsFileSystemId = id;
    }
    void WPPhysicsMaterial3::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_material;
        }
    }
    Array<SmartPtr<IResource>> WPPhysicsMaterial3::getDependencies() const
    {
        return {};
    }
    IResourceManager *WPPhysicsMaterial3::getResourceManagerPtr() const
    {
        return m_resourceManager.get();
    }
    SmartPtr<IResourceManager> WPPhysicsMaterial3::getResourceManager() const
    {
        return m_resourceManager;
    }
    void WPPhysicsMaterial3::setResourceManager( SmartPtr<IResourceManager> resourceManager )
    {
        m_resourceManager = resourceManager;
    }
    IStateContext *WPPhysicsMaterial3::getStateContextPtr() const
    {
        return m_stateContext.get();
    }
    SmartPtr<IStateContext> WPPhysicsMaterial3::getStateContext() const
    {
        return m_stateContext;
    }
    bool WPPhysicsMaterial3::handleStateMessage( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool WPPhysicsMaterial3::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
    SmartPtr<core::IPrototype> WPPhysicsMaterial3::getParentPrototype() const
    {
        return m_parentPrototype;
    }
    void WPPhysicsMaterial3::setParentPrototype( SmartPtr<core::IPrototype> prototype )
    {
        m_parentPrototype = prototype;
    }
    SmartPtr<Properties> WPPhysicsMaterial3::getProperties() const
    {
        auto properties = m_properties ? workphone::make_ptr<Properties>( *m_properties )
                                       : workphone::make_ptr<Properties>();
        properties->setProperty( "materialName", getMaterialName() );
        properties->setProperty( "staticFriction", getStaticFriction( 0 ) );
        properties->setProperty( "dynamicFriction", getDynamicFriction( 0 ) );
        properties->setProperty( "rollingFriction", getRollingFriction() );
        properties->setProperty( "restitution", getRestitution() );

        {
            Array<String> frictionCombineLabels;
            frictionCombineLabels.push_back( "Average" );
            frictionCombineLabels.push_back( "Min" );
            frictionCombineLabels.push_back( "Max" );
            frictionCombineLabels.push_back( "Multiply" );
            properties->setPropertyAsEnum( "frictionCombineMode",
                                           static_cast<s32>( getFrictionCombineMode() ),
                                           frictionCombineLabels );
        }
        {
            Array<String> restitutionCombineLabels;
            restitutionCombineLabels.push_back( "Average" );
            restitutionCombineLabels.push_back( "Min" );
            restitutionCombineLabels.push_back( "Max" );
            restitutionCombineLabels.push_back( "Multiply" );
            properties->setPropertyAsEnum( "restitutionCombineMode",
                                           static_cast<s32>( getRestitutionCombineMode() ),
                                           restitutionCombineLabels );
        }
        return properties;
    }
    void WPPhysicsMaterial3::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "WPPhysicsMaterial3::setProperties: properties are null." );
            return;
        }

        auto   staticFriction = getStaticFriction( 0 );
        auto   dynamicFriction = getDynamicFriction( 0 );
        auto   rollingFriction = getRollingFriction();
        auto   restitution = getRestitution();
        String materialName = getMaterialName();
        s32    frictionCombine = static_cast<s32>( getFrictionCombineMode() );
        s32    restitutionCombine = static_cast<s32>( getRestitutionCombineMode() );

        properties->getPropertyValue( "materialName", materialName );
        properties->getPropertyValue( "staticFriction", staticFriction );
        properties->getPropertyValue( "dynamicFriction", dynamicFriction );
        properties->getPropertyValue( "rollingFriction", rollingFriction );
        properties->getPropertyValue( "restitution", restitution );
        properties->getPropertyValue( "frictionCombineMode", frictionCombine );
        properties->getPropertyValue( "restitutionCombineMode", restitutionCombine );

        setMaterialName( materialName );
        setStaticFriction( staticFriction, 0 );
        setDynamicFriction( dynamicFriction, 0 );
        setRollingFriction( rollingFriction );
        setRestitution( restitution );
        setFrictionCombineMode( static_cast<FrictionCombineMode>( frictionCombine ) );
        setRestitutionCombineMode( static_cast<RestitutionCombineMode>( restitutionCombine ) );
        m_properties = workphone::make_ptr<Properties>( *properties );
    }
    wp_physics_material *WPPhysicsMaterial3::getMaterial() const
    {
        return m_material;
    }
} // namespace workphone::physics
