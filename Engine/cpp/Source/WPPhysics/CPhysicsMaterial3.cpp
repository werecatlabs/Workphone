#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPhysicsMaterial3.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

namespace workphone::physics
{
    CPhysicsMaterial3::CPhysicsMaterial3() : m_material( wp_physics_material_create() )
    {
        if( !m_material )
        {
            throw std::runtime_error( "Failed to create a WPPhysics material." );
        }
    }
    CPhysicsMaterial3::~CPhysicsMaterial3()
    {
        wp_physics_material_destroy( m_material );
        m_material = nullptr;
    }
    f32 CPhysicsMaterial3::getFriction( s32 direction ) const
    {
        return getStaticFriction( direction );
    }
    void CPhysicsMaterial3::setFriction( f32 friction, s32 direction )
    {
        setStaticFriction( friction, direction );
        setDynamicFriction( friction, direction );
    }
    f32 CPhysicsMaterial3::getDynamicFriction( s32 ) const
    {
        return wp_physics_material_get_dynamic_friction( m_material );
    }
    void CPhysicsMaterial3::setDynamicFriction( f32 friction, s32 )
    {
        wp_physics_material_set_dynamic_friction( m_material, std::max( friction, 0.0f ) );
    }
    f32 CPhysicsMaterial3::getStaticFriction( s32 ) const
    {
        return wp_physics_material_get_static_friction( m_material );
    }
    void CPhysicsMaterial3::setStaticFriction( f32 friction, s32 )
    {
        wp_physics_material_set_static_friction( m_material, std::max( friction, 0.0f ) );
    }
    f32 CPhysicsMaterial3::getRestitution() const
    {
        return wp_physics_material_get_restitution( m_material );
    }
    void CPhysicsMaterial3::setRestitution( f32 restitution )
    {
        wp_physics_material_set_restitution( m_material, std::clamp( restitution, 0.0f, 1.0f ) );
    }

    f32 CPhysicsMaterial3::getRollingFriction() const
    {
        return wp_physics_material_get_rolling_friction( m_material );
    }
    void CPhysicsMaterial3::setRollingFriction( f32 friction )
    {
        wp_physics_material_set_rolling_friction( m_material, std::max( friction, 0.0f ) );
    }
    FrictionCombineMode CPhysicsMaterial3::getFrictionCombineMode() const
    {
        return static_cast<FrictionCombineMode>(
            wp_physics_material_get_friction_combine_mode( m_material ) );
    }
    void CPhysicsMaterial3::setFrictionCombineMode( FrictionCombineMode mode )
    {
        wp_physics_material_set_friction_combine_mode( m_material,
                                                       static_cast<wp_friction_combine_mode>( mode ) );
    }
    RestitutionCombineMode CPhysicsMaterial3::getRestitutionCombineMode() const
    {
        return static_cast<RestitutionCombineMode>(
            wp_physics_material_get_restitution_combine_mode( m_material ) );
    }
    void CPhysicsMaterial3::setRestitutionCombineMode( RestitutionCombineMode mode )
    {
        wp_physics_material_set_restitution_combine_mode(
            m_material, static_cast<wp_restitution_combine_mode>( mode ) );
    }
    String CPhysicsMaterial3::getMaterialName() const
    {
        return String( reinterpret_cast<const char *>( wp_physics_material_get_name( m_material ) ) );
    }
    void CPhysicsMaterial3::setMaterialName( const String &name )
    {
        wp_physics_material_set_name( m_material, reinterpret_cast<const wp_c8 *>( name.c_str() ) );
    }
    Vector3<real_Num> CPhysicsMaterial3::getContactPosition() const
    {
        return Vector3<real_Num>();
    }
    Vector3<real_Num> CPhysicsMaterial3::getContactNormal() const
    {
        return Vector3<real_Num>();
    }
    SmartPtr<IRigidBody3> CPhysicsMaterial3::getPhysicsBodyA() const
    {
        return nullptr;
    }
    SmartPtr<IRigidBody3> CPhysicsMaterial3::getPhysicsBodyB() const
    {
        return nullptr;
    }
    void CPhysicsMaterial3::saveToFile( const String &filePath )
    {
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_LOG_ERROR( "CPhysicsMaterial3::saveToFile: file path is empty." );
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->getFileSystem() )
            {
                WP_LOG_ERROR( "CPhysicsMaterial3::saveToFile: file system is unavailable." );
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
    void CPhysicsMaterial3::loadFromFile( const String &filePath )
    {
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_LOG_ERROR( "CPhysicsMaterial3::loadFromFile: file path is empty." );
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->getFileSystem() )
            {
                WP_LOG_ERROR( "CPhysicsMaterial3::loadFromFile: file system is unavailable." );
                return;
            }

            const auto contents = applicationManager->getFileSystem()->readAllText( filePath );
            if( StringUtil::isNullOrEmpty( contents ) )
            {
                WP_LOG_ERROR( "CPhysicsMaterial3::loadFromFile: file is empty or unavailable: " +
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
    void CPhysicsMaterial3::save()
    {
        saveToFile( getFilePath() );
    }
    void CPhysicsMaterial3::import()
    {
        loadFromFile( getFilePath() );
    }
    void CPhysicsMaterial3::reimport()
    {
        loadFromFile( getFilePath() );
    }
    UUID CPhysicsMaterial3::getFileSystemId() const
    {
        return m_fileSystemId;
    }
    void CPhysicsMaterial3::setFileSystemId( UUID id )
    {
        m_fileSystemId = id;
    }
    String CPhysicsMaterial3::getFilePath() const
    {
        return m_filePath;
    }
    void CPhysicsMaterial3::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }
    UUID CPhysicsMaterial3::getSettingsFileSystemId() const
    {
        return m_settingsFileSystemId;
    }
    void CPhysicsMaterial3::setSettingsFileSystemId( UUID id )
    {
        m_settingsFileSystemId = id;
    }
    void CPhysicsMaterial3::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_material;
        }
    }
    Array<SmartPtr<IResource>> CPhysicsMaterial3::getDependencies() const
    {
        return {};
    }
    IResourceManager *CPhysicsMaterial3::getResourceManagerPtr() const
    {
        return m_resourceManager.get();
    }
    SmartPtr<IResourceManager> CPhysicsMaterial3::getResourceManager() const
    {
        return m_resourceManager;
    }
    void CPhysicsMaterial3::setResourceManager( SmartPtr<IResourceManager> resourceManager )
    {
        m_resourceManager = resourceManager;
    }
    IStateContext *CPhysicsMaterial3::getStateContextPtr() const
    {
        return m_stateContext.get();
    }
    SmartPtr<IStateContext> CPhysicsMaterial3::getStateContext() const
    {
        return m_stateContext;
    }
    bool CPhysicsMaterial3::handleStateMessage( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool CPhysicsMaterial3::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
    SmartPtr<core::IPrototype> CPhysicsMaterial3::getParentPrototype() const
    {
        return m_parentPrototype;
    }
    void CPhysicsMaterial3::setParentPrototype( SmartPtr<core::IPrototype> prototype )
    {
        m_parentPrototype = prototype;
    }
    SmartPtr<Properties> CPhysicsMaterial3::getProperties() const
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
    void CPhysicsMaterial3::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "CPhysicsMaterial3::setProperties: properties are null." );
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
    wp_physics_material *CPhysicsMaterial3::getMaterial() const
    {
        return m_material;
    }
} // namespace workphone::physics
