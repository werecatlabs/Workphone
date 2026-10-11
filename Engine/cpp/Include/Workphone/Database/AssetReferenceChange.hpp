#ifndef WP_ASSET_REFERENCE_CHANGE_HPP
#define WP_ASSET_REFERENCE_CHANGE_HPP
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Script/ScriptAsset.hpp>

namespace workphone
{
    /** Owner-thread edit of an existing string/UUID reference property. The
     * caller supplies the currently active catalog on every operation. */
    class AssetReferenceChange
    {
    public:
        static bool typeMatches( const String &storedType, hash_type requested )
        {
            auto types = TypeManager::instance();
            if( !types || requested == 0 )
                return false;
            const auto actual = storedType == "Material" ? render::IMaterial::typeInfo() :
                                storedType == "Texture" ? render::ITexture::typeInfo() :
                                storedType == "Actor" ? scene::IGameActor::typeInfo() :
                                storedType == "script" ? ScriptAsset::typeInfo() :
                                types->getIdFromName( storedType );
            return actual != 0 && types->isDerived( actual, requested );
        }
        bool prepare( SmartPtr<AssetDatabaseManager> catalog,
                      const AssetDatabaseManager::EntrySnapshot &entry,
                      SmartPtr<ISharedObject> target, const String &property,
                      hash_type expectedType )
        {
            if( m_prepared )
                return fail( "Reference edit has already been prepared." );
            if( !catalog || !target || !typeMatches( entry.type, expectedType ) ||
                !catalog->isEntryCurrent( entry ) )
                return fail( "Reference type or catalog generation no longer matches." );
            auto properties = target->getProperties();
            if( !properties || !properties->hasProperty( property ) ||
                properties->getPropertyObject( property ).isReadOnly() )
                return fail( "Reference property is missing or read-only." );
            m_declaredType = properties->getPropertyObject( property ).getAttribute( "resourceType" );
            auto types = TypeManager::instance();
            if( !m_declaredType.empty() &&
                !typeMatches( entry.type, types->getIdFromName( m_declaredType ) ) )
                return fail( "Asset type does not match the property's declared reference type." );
            m_catalog = catalog;
            m_target = target;
            m_entry = entry;
            m_property = property;
            m_before = properties->getProperty( property );
            m_prepared = true;
            m_error.clear();
            return true;
        }
        bool apply( SmartPtr<AssetDatabaseManager> catalog )
        {
            if( !canApply( catalog ) )
                return false;
            if( !write( m_before, m_entry.uuid ) )
                return false;
            m_applied = true;
            m_executed = true;
            return true;
        }
        bool canApply( SmartPtr<AssetDatabaseManager> catalog )
        {
            if( !m_prepared || m_applied )
                return fail( "Reference edit is not ready to apply." );
            if( !sessionCurrent( catalog ) )
                return false;
            AssetDatabaseManager::EntrySnapshot current;
            if( !catalog->tryGetEntry( m_entry.uuid, current ) || current.path != m_entry.path ||
                current.type != m_entry.type || current.kind != m_entry.kind ||
                ( !m_executed && current.generation != m_entry.generation ) )
                return fail( "Referenced asset changed; choose it again." );
            return propertyCurrent( m_before );
        }
        bool undo( SmartPtr<AssetDatabaseManager> catalog )
        {
            if( !canUndo( catalog ) || !write( m_entry.uuid, m_before ) )
                return false;
            m_applied = false;
            return true;
        }
        bool canUndo( SmartPtr<AssetDatabaseManager> catalog )
        {
            if( !m_applied )
                return fail( "Reference edit is not applied." );
            return sessionCurrent( catalog ) && propertyCurrent( m_entry.uuid );
        }
        bool applied() const { return m_applied; }
        const String &error() const { return m_error; }
        SmartPtr<ISharedObject> target() const { return m_target.lock(); }
    private:
        bool fail( const String &error ) { m_error = error; return false; }
        bool sessionCurrent( SmartPtr<AssetDatabaseManager> active )
        {
            if( !active || active != m_catalog.lock() )
                return fail( "Active project catalog changed." );
            AssetDatabaseManager::EntryQuery query;
            query.pageSize = 1;
            auto page = active->queryEntries( query );
            if( !page.succeeded || page.next.catalogInstance != m_entry.catalogInstance )
                return fail( "Project catalog closed or reopened; the reference edit has expired." );
            return true;
        }
        bool propertyCurrent( const String &expected )
        {
            auto object = m_target.lock();
            auto properties = object ? object->getProperties() : nullptr;
            if( !properties || !properties->hasProperty( m_property ) ||
                properties->getPropertyObject( m_property ).isReadOnly() ||
                properties->getProperty( m_property ) != expected ||
                properties->getPropertyObject( m_property ).getAttribute( "resourceType" ) != m_declaredType )
                return fail( "Reference property was removed, locked or independently changed." );
            m_error.clear();
            return true;
        }
        bool write( const String &expected, const String &value )
        {
            if( !propertyCurrent( expected ) )
                return false;
            auto object = m_target.lock();
            auto properties = object->getProperties();
            auto replacement = make_ptr<Properties>( *properties );
            replacement->getPropertyObject( m_property ).setValue( value );
            try
            {
                object->setProperties( replacement );
                auto installed = object->getProperties();
                if( !installed || installed->getProperty( m_property ) != value )
                {
                    object->setProperties( properties );
                    return fail( "The target did not accept the reference property." );
                }
            }
            catch( const std::exception & )
            {
                try { object->setProperties( properties ); } catch( ... ) {}
                return fail( "The target threw while applying the reference property." );
            }
            m_error.clear();
            return true;
        }
        WeakPtr<AssetDatabaseManager> m_catalog;
        WeakPtr<ISharedObject> m_target;
        AssetDatabaseManager::EntrySnapshot m_entry;
        String m_property;
        String m_declaredType;
        String m_before;
        String m_error;
        bool m_prepared = false;
        bool m_applied = false;
        bool m_executed = false;
    };
}
#endif
