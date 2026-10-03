// ---------------------------------------------------------------------------
//  AssetDatabaseEditorDatabase.hpp
//
//  C++17 port of the C# Database wrapper (Tools/csharp/AssetDatabaseTool/Database.cs).
//  Provides:
//   *  A thin façade over the engine's WPSQLite backed @c IDatabase
//   *  Helpers for the project's main SQL queries (configured_actors, ref_*
//      tables, attribs, ...)
//
//  The implementation stays close to the original SQL strings so that the
//  existing *.db files continue to work without schema changes.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorDatabase_h__
#define AssetDatabaseEditorDatabase_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseDomain.hpp>
#include <WPSQLite/SQLiteDatabase.hpp>
#include <Workphone/Workphone.hpp>

#include <map>
#include <string>
#include <vector>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Database access wrapper used by the editor windows.
         *
         * The class mirrors the responsibilities of the C# `fb.Database` type
         * but routes every query through the engine's `IDatabase` /
         * `IDatabaseQuery` interfaces.  This allows the editor to share the
         * same SQLite connection pool and lifecycle with the rest of the
         * engine.
         */
        class AssetDatabaseEditorDatabase : public ISharedObject
        {
        public:
            /**
             * @brief Lightweight pair used to expose individual query fields.
             */
            struct Field
            {
                String name;
                String value;
            };

            /**
             * @brief Row of named fields returned by a query.
             */
            struct Row
            {
                std::vector<Field> fields;
                String getFieldValue( const String &columnName ) const;
                s32 getFieldValueAsInt( const String &columnName ) const;
                f32 getFieldValueAsFloat( const String &columnName ) const;
            };

            /**
             * @brief Result of a query.  Each row exposes the column/value pairs
             *        produced by the SELECT statement.
             */
            struct Result
            {
                std::vector<Row> rows;
                bool empty() const { return rows.empty(); }
                size_t size() const { return rows.size(); }
            };

            AssetDatabaseEditorDatabase();
            ~AssetDatabaseEditorDatabase() override;

            /**
             * @brief Open a SQLite database file.
             *
             * @param filePath Absolute path to the .db file.
             * @return true when the database was opened successfully.
             */
            bool open( const String &filePath );

            /**
             * @brief Close the currently open database.
             */
            void close();

            /**
             * @brief Check whether a database is currently open.
             */
            bool isOpen() const;

            /**
             * @brief Set the file path that should be used the next time @c open
             *        is called.
             */
            void setConnectionString( const String &filePath );

            /**
             * @brief Execute a SELECT query and return the result.
             */
            Result executeQuery( const String &sql );

            /**
             * @brief Execute a DML statement (INSERT/UPDATE/DELETE) and return
             *        the number of affected rows.
             */
            s32 executeDML( const String &sql );

            // -------------------------------------------------------------------
            //  High level operations ported from the C# code
            // -------------------------------------------------------------------

            /**
             * @brief Populate the list of models whose `parent_id` is 1.
             */
            std::vector<Row> getModels();

            /**
             * @brief Get the value of an arbitrary field for a model row.
             */
            String getModelFieldData( s32 actorId, const String &fieldName );

            /**
             * @brief Update an arbitrary field on a configured_actors row.
             */
            void setModelFieldData( s32 actorId, const String &fieldName, const String &fieldValue );

            /**
             * @brief Add a new child node under the supplied parent and return
             *        its row id.
             */
            s32 addChildNode( s32 parentId, const String &name );

            /**
             * @brief Remove a node and its entire sub-tree.
             */
            void removeNode( s32 nodeId );

            /**
             * @brief Return the child count of a configured_actors row.
             */
            s32 getChildCount( s32 parentId );

            /**
             * @brief Return the immediate child ids of a configured_actors row.
             */
            std::vector<s32> getNodeChildren( s32 parentId );

            /**
             * @brief Look up a child node by name.  Returns -1 if not found.
             */
            s32 getNodeId( s32 parentId, const String &name );

            /**
             * @brief Return the resource name associated with a configured actor.
             */
            String getResourceValue( const String &name );

            /**
             * @brief Update a single column on the configured_actors table.
             */
            s32 updateConfiguredActor( s32 rowId, const String &columnName, const String &value );

            /**
             * @brief Update a single column on the actor_objects table.
             */
            s32 updateActorObject( s32 rowId, const String &columnName, const String &value );

            /**
             * @brief Update a single column on the attribs table.
             */
            s32 updateAttrib( s32 rowId, const String &columnName, const String &value );

            /**
             * @brief Update a single column on the ref_attribs table.
             */
            s32 updateRefAttrib( s32 rowId, const String &columnName, const String &value );

            /**
             * @brief Update a single column on the object_attributes table.
             */
            s32 updateObjectAttribute( s32 rowId, const String &columnName, const String &value );

            /**
             * @brief Clone a configured_actor row including its attribs, virtual_tx
             *        rows and the actor_objects/object_attributes records.  Mirrors
             *        the C# `CloneModel` implementation.
             */
            s32 cloneModel( s32 refId );

            /**
             * @brief Attach a second database to the current connection.  Used
             *        when importing `.mxp` model files.
             */
            bool attach( const String &alias, const String &filePath );

            /**
             * @brief Detach a previously attached database.
             */
            bool detach( const String &alias );

            /**
             * @brief Access the underlying engine database.  Returns nullptr if
             *        no database is open.
             */
            SmartPtr<IDatabase> getDatabase() const { return m_database; }

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IDatabase> m_database;
            String m_connectionString;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorDatabase_h__
