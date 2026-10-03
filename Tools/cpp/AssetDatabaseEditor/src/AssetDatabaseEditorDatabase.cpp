// ---------------------------------------------------------------------------
//  AssetDatabaseEditorDatabase.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <WPSQLite/SQLiteDatabase.hpp>
#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>

#include <algorithm>
#include <sstream>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AssetDatabaseEditorDatabase, ISharedObject );

    namespace
    {
        // Trivial helpers used throughout the port
        String TrimQuotes( const String &value )
        {
            if( value.size() >= 2 && value.front() == '\'' && value.back() == '\'' )
            {
                return value.substr( 1, value.size() - 2 );
            }
            return value;
        }

        String QuoteSql( const String &value )
        {
            String result = value;
            String::size_type pos = 0;
            while( ( pos = result.find( '\'', pos ) ) != String::npos )
            {
                result.insert( pos, 1, '\'' );
                pos += 2;
            }
            return "'" + result + "'";
        }

        String ColumnCase( const String &binding )
        {
            // Mirror the C# binding path -> SQL column name mapping used in
            // the various DataGrid.CellEditEnding handlers.
            if( binding == "Ident" ) return "ident";
            if( binding == "Class" ) return "class";
            if( binding == "ModelId" ) return "actor_id";
            if( binding == "GroupType" ) return "group_type";
            if( binding == "OriginalId" ) return "original_id";
            if( binding == "Name" ) return "title";
            if( binding == "Value" ) return "value";
            if( binding == "ObjectId" ) return "object_id";
            if( binding == "RefComponentId" ) return "ref_component_id";
            return binding;
        }
    }  // namespace

    // ---------------------------------------------------------------------
    //  Field/Row helpers
    // ---------------------------------------------------------------------

    String AssetDatabaseEditorDatabase::Row::getFieldValue( const String &columnName ) const
    {
        for( const auto &field : fields )
        {
            if( field.name == columnName )
            {
                return field.value;
            }
        }
        return String();
    }

    s32 AssetDatabaseEditorDatabase::Row::getFieldValueAsInt( const String &columnName ) const
    {
        return StringUtil::parseInt( getFieldValue( columnName ) );
    }

    f32 AssetDatabaseEditorDatabase::Row::getFieldValueAsFloat( const String &columnName ) const
    {
        return StringUtil::parseFloat( getFieldValue( columnName ) );
    }

    // ---------------------------------------------------------------------
    //  Construction / connection management
    // ---------------------------------------------------------------------

    AssetDatabaseEditorDatabase::AssetDatabaseEditorDatabase() = default;

    AssetDatabaseEditorDatabase::~AssetDatabaseEditorDatabase()
    {
        close();
    }

    bool AssetDatabaseEditorDatabase::open( const String &filePath )
    {
        close();

        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            return false;
        }

        m_connectionString = filePath;

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return false;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            return false;
        }

        // WPSQLite registers its implementation with the engine factory.  Use
        // that interface instead of constructing a DLL implementation class
        // directly (SQLiteDatabase is intentionally not exported).
        m_database = factoryManager->make_object<IDatabase>( "SQLiteDatabase" );
        if( !m_database )
        {
            return false;
        }
        m_database->loadFromFile( filePath );

        if( !m_database->isLoaded() )
        {
            m_database = nullptr;
            return false;
        }

        return true;
    }

    void AssetDatabaseEditorDatabase::close()
    {
        if( m_database )
        {
            m_database->close();
            m_database = nullptr;
        }
    }

    bool AssetDatabaseEditorDatabase::isOpen() const
    {
        return static_cast<bool>( m_database );
    }

    void AssetDatabaseEditorDatabase::setConnectionString( const String &filePath )
    {
        m_connectionString = filePath;
    }

    AssetDatabaseEditorDatabase::Result AssetDatabaseEditorDatabase::executeQuery(
        const String &sql )
    {
        Result result;

        if( !m_database )
        {
            return result;
        }

        auto query = m_database->query( sql );
        if( !query )
        {
            return result;
        }

        if( query->eof() )
        {
            return result;
        }

        while( !query->eof() )
        {
            Row row;
            const auto numFields = query->getNumFields();
            for( size_t i = 0; i < numFields; ++i )
            {
                Field field;
                field.name = query->getFieldName( static_cast<u32>( i ) );
                field.value = query->getFieldValue( static_cast<u32>( i ) );
                row.fields.push_back( field );
            }
            result.rows.push_back( std::move( row ) );
            query->nextRow();
        }

        return result;
    }

    s32 AssetDatabaseEditorDatabase::executeDML( const String &sql )
    {
        if( !m_database )
        {
            return 0;
        }

        m_database->queryDML( sql );
        // WPSQLite's CppSQLite3DB doesn't return the affected row count through
        // the engine API; the C# code only inspected the return value to detect
        // hard failures.  We rely on the engine logging for errors and return 0
        // to keep the API shape similar.
        return 0;
    }

    // ---------------------------------------------------------------------
    //  Helpers
    // ---------------------------------------------------------------------

    std::vector<AssetDatabaseEditorDatabase::Row> AssetDatabaseEditorDatabase::getModels()
    {
        return executeQuery( "select * from configured_actors where parent_id = 1" ).rows;
    }

    String AssetDatabaseEditorDatabase::getModelFieldData( s32 actorId, const String &fieldName )
    {
        auto result = executeQuery( "select * from configured_actors where id = " +
                                    StringUtil::toString( actorId ) );
        if( result.empty() )
        {
            return String();
        }
        return result.rows.front().getFieldValue( fieldName );
    }

    void AssetDatabaseEditorDatabase::setModelFieldData( s32 actorId,
                                                         const String &fieldName,
                                                         const String &fieldValue )
    {
        executeDML( "UPDATE configured_actors SET '" + fieldName + "' = '" + fieldValue +
                    "' WHERE id = " + StringUtil::toString( actorId ) + ";" );
    }

    s32 AssetDatabaseEditorDatabase::getChildCount( s32 parentId )
    {
        const auto leftQuery = executeQuery( "select lft from configured_actors where id = " +
                                            StringUtil::toString( parentId ) );
        if( leftQuery.empty() )
        {
            return 0;
        }

        const auto left = leftQuery.rows.front().getFieldValueAsInt( "lft" );
        return static_cast<s32>( executeQuery( "select * from configured_actors where lft > " +
                                               StringUtil::toString( left ) + " and parent_id = " +
                                               StringUtil::toString( parentId ) )
                                     .rows.size() );
    }

    std::vector<s32> AssetDatabaseEditorDatabase::getNodeChildren( s32 parentId )
    {
        std::vector<s32> result;

        const auto leftQuery = executeQuery( "select lft from configured_actors where id = " +
                                            StringUtil::toString( parentId ) );
        if( leftQuery.empty() )
        {
            return result;
        }

        const auto left = leftQuery.rows.front().getFieldValueAsInt( "lft" );
        const auto childResult = executeQuery( "select id from configured_actors where lft > " +
                                              StringUtil::toString( left ) +
                                              " and parent_id = " + StringUtil::toString( parentId ) );
        for( const auto &row : childResult.rows )
        {
            result.push_back( row.getFieldValueAsInt( "id" ) );
        }
        return result;
    }

    s32 AssetDatabaseEditorDatabase::getNodeId( s32 parentId, const String &name )
    {
        const auto children = getNodeChildren( parentId );
        for( const auto child : children )
        {
            const auto value = getModelFieldData( child, "name" );
            if( value == name )
            {
                return child;
            }
        }
        return -1;
    }

    s32 AssetDatabaseEditorDatabase::addChildNode( s32 parentId, const String &name )
    {
        const auto leftQuery = executeQuery( "select lft from configured_actors where id = " +
                                            StringUtil::toString( parentId ) );
        if( leftQuery.empty() )
        {
            return -1;
        }

        const auto left = leftQuery.rows.front().getFieldValueAsInt( "lft" );

        executeDML( "update configured_actors set rght = rght + 2 where rght > " +
                    StringUtil::toString( left ) );
        executeDML( "update configured_actors set lft = lft + 2 where lft > " +
                    StringUtil::toString( left ) );

        const auto sql = "insert into configured_actors (name, parent_id, lft, rght) values (" +
                         QuoteSql( name ) + ", " + StringUtil::toString( parentId ) + ", " +
                         StringUtil::toString( left ) + "+1, " + StringUtil::toString( left ) + "+2)";
        executeDML( sql );

        const auto maxIdQuery = executeQuery( "select max(id) as id from configured_actors" );
        if( maxIdQuery.empty() )
        {
            return -1;
        }
        return maxIdQuery.rows.front().getFieldValueAsInt( "id" );
    }

    void AssetDatabaseEditorDatabase::removeNode( s32 nodeId )
    {
        const auto node = executeQuery( "select lft, rght from configured_actors where id = " +
                                       StringUtil::toString( nodeId ) );
        if( node.empty() )
        {
            return;
        }

        const auto left = node.rows.front().getFieldValueAsInt( "lft" );
        const auto right = node.rows.front().getFieldValueAsInt( "rght" );

        executeDML( "delete from configured_actors where lft between " +
                    StringUtil::toString( left ) + " and " + StringUtil::toString( right ) );

        const s32 offset = right - left + 1;
        executeDML( "update configured_actors set lft = lft - " + StringUtil::toString( offset ) +
                    " where lft > " + StringUtil::toString( left ) );
        executeDML( "update configured_actors set rght = rght - " + StringUtil::toString( offset ) +
                    " where rght > " + StringUtil::toString( right ) );
        executeDML( "delete from attribs where configured_actor_id = " +
                    StringUtil::toString( nodeId ) );
    }

    String AssetDatabaseEditorDatabase::getResourceValue( const String &name )
    {
        const auto result = executeQuery( "select name from resourcemap where name = " +
                                          QuoteSql( name ) );
        if( result.empty() )
        {
            return name;
        }
        return result.rows.front().getFieldValue( "name" );
    }

    s32 AssetDatabaseEditorDatabase::updateConfiguredActor( s32 rowId,
                                                            const String &columnName,
                                                            const String &value )
    {
        const String col = ColumnCase( columnName );
        executeDML( "UPDATE `configured_actors` SET `" + col + "` = " + QuoteSql( value ) +
                    " WHERE `id` = " + StringUtil::toString( rowId ) + ";" );
        return 0;
    }

    s32 AssetDatabaseEditorDatabase::updateActorObject( s32 rowId,
                                                       const String &columnName,
                                                       const String &value )
    {
        const String col = ColumnCase( columnName );
        executeDML( "UPDATE `actor_objects` SET `" + col + "` = " + QuoteSql( value ) +
                    " WHERE `id` = " + StringUtil::toString( rowId ) + ";" );
        return 0;
    }

    s32 AssetDatabaseEditorDatabase::updateAttrib( s32 rowId,
                                                   const String &columnName,
                                                   const String &value )
    {
        const String col = ColumnCase( columnName );
        executeDML( "UPDATE `attribs` SET `" + col + "` = " + QuoteSql( value ) +
                    " WHERE `id` = " + StringUtil::toString( rowId ) + ";" );
        return 0;
    }

    s32 AssetDatabaseEditorDatabase::updateRefAttrib( s32 rowId,
                                                     const String &columnName,
                                                     const String &value )
    {
        const String col = ColumnCase( columnName );
        executeDML( "UPDATE `ref_attribs` SET `" + col + "` = " + QuoteSql( value ) +
                    " WHERE `id` = " + StringUtil::toString( rowId ) + ";" );
        return 0;
    }

    s32 AssetDatabaseEditorDatabase::updateObjectAttribute( s32 rowId,
                                                            const String &columnName,
                                                            const String &value )
    {
        const String col = ColumnCase( columnName );
        executeDML( "UPDATE `object_attributes` SET `" + col + "` = " + QuoteSql( value ) +
                    " WHERE `id` = " + StringUtil::toString( rowId ) + ";" );
        return 0;
    }

    s32 AssetDatabaseEditorDatabase::cloneModel( s32 refId )
    {
        if( !m_database )
        {
            return -1;
        }

        // 1. Find the maximum id so we can offset the copy.
        const auto maxIdQuery =
            executeQuery( "select max(id) as id from configured_actors" );
        if( maxIdQuery.empty() )
        {
            return -1;
        }
        const auto maxId = maxIdQuery.rows.front().getFieldValueAsInt( "id" );

        // 2. Locate the right hand side of the "Std" container in the same
        //    parent so the new copy lives in a sensible position.
        const auto placement =
            executeQuery( "SELECT (lft)-1 AS lft, (rght)-1 AS rght FROM configured_actors WHERE name = 'Std' "
                          "AND parent_id = (SELECT id FROM configured_actors WHERE name = "
                          "(SELECT name FROM configured_actors WHERE parent_id = 1 AND lft < "
                          "(SELECT lft FROM configured_actors WHERE id = " +
                          StringUtil::toString( refId ) + ") AND rght > (SELECT rght FROM "
                          "configured_actors WHERE id = " + StringUtil::toString( refId ) +
                          ")))" );
        if( placement.empty() )
        {
            return -1;
        }
        const auto maxRght = placement.rows.front().getFieldValueAsInt( "rght" );
        const auto maxLft = maxRght - 1;

        // 3. Make room for the copy.
        executeDML( "update configured_actors set lft = lft + (select (rght - lft) + 1 from "
                    "configured_actors where id = " + StringUtil::toString( refId ) + ") where lft > " +
                    StringUtil::toString( maxRght ) );
        executeDML( "update configured_actors set rght = rght + (select (rght - lft) + 1 from "
                    "configured_actors where id = " + StringUtil::toString( refId ) + ") where rght > " +
                    StringUtil::toString( maxRght ) );

        // 4. Copy the tree with offsets.
        executeDML( "insert into configured_actors (parent_id, name, ref_component_id, scale, nitro, "
                    "standard, lft, rght, scenenode, crud, config_type, model_image, camera_data, "
                    "ref_actor_row_id, resource_name, channel ) select 0, name, ref_component_id, "
                    "scale, nitro, standard, lft + " + StringUtil::toString( maxRght ) + " - (select "
                    "lft from configured_actors where id = " + StringUtil::toString( refId ) +
                    ") + 1, rght + " + StringUtil::toString( maxRght ) + " - (select lft from "
                    "configured_actors where id = " + StringUtil::toString( refId ) + ") + 1, "
                    "scenenode, crud, config_type, model_image, camera_data, id, resource_name, "
                    "channel from configured_actors where lft between (select lft from "
                    "configured_actors where id = " + StringUtil::toString( refId ) + ") and (select "
                    "rght from configured_actors where id = " + StringUtil::toString( refId ) +
                    ") order by lft asc" );

        // 5. Repair parent ids.
        executeDML( "update configured_actors set parent_id = ( select cm.id from configured_actors "
                    "as cm where configured_actors.lft between cm.lft and cm.rght and "
                    "configured_actors.lft != cm.lft order by cm.lft desc limit 1 )" );

        executeDML( "update configured_actors set name = 'New Model' where id = 1 + " +
                    StringUtil::toString( maxId ) );

        // 6. Copy attribs for the new model.
        executeDML( "insert into main.attribs (title, value, ref_component_id, actor_id, "
                    "configured_actor_id, applied) select attribs.title, attribs.value, "
                    "attribs.ref_component_id, 1 + " + StringUtil::toString( maxId ) + ", "
                    "main.configured_actors.id as 'configured_actor_id', attribs.applied from "
                    "main.configured_actors, attribs where main.configured_actors.ref_actor_row_id = "
                    "attribs.configured_actor_id and attribs.actor_id = " +
                    StringUtil::toString( refId ) + " and main.configured_actors.lft between "
                    "(select lft from main.configured_actors where id = 1 + " +
                    StringUtil::toString( maxId ) + ") and (select rght from main.configured_actors "
                    "where id = 1 + " + StringUtil::toString( maxId ) + ")" );

        executeDML( "insert into main.virtual_tx (actor_id, param, value) select 1 + " +
                    StringUtil::toString( maxId ) + ", param, value from virtual_tx where actor_id = " +
                    StringUtil::toString( refId ) );

        // 7. Clone the actor_objects and their object_attributes.
        executeDML( "insert into actor_objects (actor_id, class, ident, group_type) select 1 + " +
                    StringUtil::toString( maxId ) + ", class, ident, group_type from actor_objects "
                    "where actor_id = " + StringUtil::toString( refId ) );

        const auto originalObjects = executeQuery( "select id from actor_objects where actor_id = " +
                                                   StringUtil::toString( refId ) );
        const auto newObjects = executeQuery( "select id from actor_objects where actor_id = 1 + " +
                                              StringUtil::toString( maxId ) );
        const auto count = std::min( originalObjects.rows.size(), newObjects.rows.size() );
        for( size_t i = 0; i < count; ++i )
        {
            const auto srcId = originalObjects.rows[i].getFieldValue( "id" );
            const auto dstId = newObjects.rows[i].getFieldValue( "id" );
            executeDML( "insert into object_attributes (object_id, name, value) select " + dstId +
                        ", name, value from object_attributes where object_id = " + srcId );
        }

        return maxId + 1;
    }

    bool AssetDatabaseEditorDatabase::attach( const String &alias, const String &filePath )
    {
        if( !m_database )
        {
            return false;
        }
        executeDML( "ATTACH '" + filePath + "' AS " + alias );
        return true;
    }

    bool AssetDatabaseEditorDatabase::detach( const String &alias )
    {
        if( !m_database )
        {
            return false;
        }
        executeDML( "DETACH " + alias );
        return true;
    }
}  // namespace workphone::adbeditor
