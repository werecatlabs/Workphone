////using System;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data;
using System.Data.SQLite;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using Path = System.IO.Path;

namespace fb
{

    class Database
    {
        public string ConnectionString { get; set; }

        public List<string> States = new List<string>();
        public List<KeyValuePair<string, string>> AttachedDatabases =
            new List<KeyValuePair<string, string>>();

        public void open()
        {
            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }
        }

        public void close()
        {
        }

        public void SetModelFieldData( int actor_id, string fieldName, string fieldValue )
        {
            var sql = "UPDATE configured_actors SET '" + fieldName + "' = '" + fieldValue +
                      "' WHERE id = " + actor_id + ";";
            executeQuery( sql );
        }

        public string GetModelFieldData( int actor_id, string fieldName )
        {
            var sql = "select * from configured_actors where id = " + actor_id + ";";
            var query = executeQuery( sql );
            foreach( var r in query.rows )
            {
                return r.GetFieldValue( fieldName );
            }

            return "";
        }

        public List<int> GetModels()
        {
            var sql = "select * from configured_actors where parent_id = 1";
            var query = executeQuery( sql );

            List<int> models = new List<int>();
            foreach( var r in query.rows )
            {
                var id = r.GetFieldValueAsInt( "id" );
                models.Add( id );
            }

            return models;
        }

        public int CreateModel( string name )
        {
            return AddChildNode( 1, name );
        }

        void DeleteUserModelAttribs( int actor_id )
        {
            try
            {
                var sql = "delete from main.attribs where actor_id = " + actor_id;
                executeQuery( sql );
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        public void DeleteModel( int actor_id )
        {
            try
            {
                var sql = "select * from main.configured_actors where id = " + actor_id;
                var rs = executeQuery( sql );

                var lft = "";
                var rght = "";

                if( rs != null )
                {
                    foreach( var r in rs.rows )
                    {
                        lft = r.GetFieldValue( "lft" );
                        rght = r.GetFieldValue( "rght" );
                    }
                }
                else
                {
                    MessageBox.Show( "error: model id not found for delete" );
                    return;
                }

                DeleteUserModelAttribs( actor_id );

                sql = "delete from main.configured_actors where lft between " + lft + " and " + rght;
                executeQuery( sql );

                var offset = ( ( int.Parse( rght ) - int.Parse( lft ) ) + 1 ).ToString();

                // cleanup configured models tree
                //sql = "update main.configured_actors set lft = lft - " + offset + ", rght = rght - " + offset + "  where lft > " + lft;

                //process lft's first
                sql = "update main.configured_actors set lft = lft - " + offset + "  where lft > " + lft;
                executeQuery( sql );

                //process rght's second
                sql = "update main.configured_actors set rght = rght - " + offset + "  where rght > " +
                      rght;
                executeQuery( sql );
                //sql =	std::string("update main.configured_actors set parent_id = ") +
                //	std::string(" ( select cm.id from main.configured_actors as cm ") +
                //	std::string(" where configured_actors.lft between cm.lft and cm.rght ") +
                //	std::string(" and configured_actors.lft != cm.lft ") +
                //	std::string(" order by cm.lft desc limit 1 );");
                //executeQuery(sql);
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        public void DeleteAllModels( int parentId )
        {
            var queryStr = "select * from configured_actors where parent_id = '" + parentId + "';";
            var result = executeQuery( queryStr );

            foreach( var r in result.rows )
            {
                var id = r.GetFieldValueAsInt( "id" );
                DeleteModel( id );
            }
        }

        public void DeleteAllModels()
        {
            DeleteAllModels( 217 );
            DeleteAllModels( 220 );
            DeleteAllModels( 2903 );

            var queryStr = "delete from configured_actors where parent_id <> 0";
            executeQuery( queryStr );
        }

        public void DeleteAll()
        {
            DeleteAllModels( 217 );
            DeleteAllModels( 220 );
            DeleteAllModels( 2903 );

            var queryStr = "delete from attribs";
            executeQuery( queryStr );

            queryStr = "delete from actor_objects";
            executeQuery( queryStr );

            queryStr = "delete from configured_actors where parent_id <> 0";
            executeQuery( queryStr );
        }

        public void SetupNewData()
        {
            DeleteAllModels( 217 );
            DeleteAllModels( 220 );
            DeleteAllModels( 2903 );

            var queryStr = "delete from attribs";
            executeQuery( queryStr );

            queryStr = "delete from actor_objects";
            executeQuery( queryStr );

            queryStr = "delete from configured_actors where parent_id <> 0";
            executeQuery( queryStr );
        }

        public int CloneModel( int refid )
        {
            try
            {
                //m_database->close();
                //load("..\\Media\\data\\saracenRef.db");
                //attach("..\\Media\\data\\db");

                int id = 0;

                var sql = "select parent_id from configured_actors where id = " + refid;
                var rs = executeQuery( sql );

                var parent_id = "";
                var maxID = "";
                var maxLft = "";
                var maxRght = "";
                var defaultResponseMode = "1";

                if( rs.rows.Count > 0 )
                {
                    parent_id = rs.rows[0].GetFieldValue( "parent_id" );
                }

                sql = "select max(id) from configured_actors";
                rs = executeQuery( sql );
                if( rs.rows.Count > 0 )
                {
                    maxID = rs.rows[0].GetFieldValue( 0 );
                }

                // get default response mode
                sql = "select value from settings where param='defaultresponsemode'";
                rs = executeQuery( sql );
                if( rs.rows.Count > 0 )
                {
                    defaultResponseMode = rs.rows[0].GetFieldValue( 0 );
                }

                // where to put copyied model
                sql =
                    "SELECT (lft)-1, (rght)-1 FROM configured_actors WHERE name = 'Std' AND parent_id = " +
                    "(SELECT id FROM configured_actors WHERE name = " +
                    "(SELECT name FROM configured_actors WHERE parent_id = 1 AND lft < (SELECT lft FROM configured_actors WHERE id = " +
                    refid + ") AND rght > (SELECT rght FROM configured_actors WHERE id = " + refid +
                    ")))";

                rs = executeQuery( sql );
                if( rs.rows.Count > 0 )
                {
                    //maxLft = rs->getFieldValue(0);
                    //maxRght = rs->getFieldValue(1);

                    maxRght = rs.rows[0].GetFieldValue( 1 );
                    maxLft = ( int.Parse( maxRght ) - 1 ).ToString();
                }

                int containerID = int.Parse( rs.rows[0].GetFieldValue( 0 ) );

                // 1. Make room to copy the branch for model refid from refdb
                sql = "update configured_actors set " +
                      "lft = lft + (select (rght - lft) + 1 from configured_actors where id = " + refid +
                      ") " + "where lft > " + maxRght;
                executeQuery( sql );

                sql =
                    "update configured_actors set " +
                    "rght = rght + (select (rght - lft) + 1 from configured_actors where id = " + refid +
                    ") " + "where rght > " + maxRght;
                executeQuery( sql );

                // 2. Copy the tree with offsets
                sql =
                    "insert into configured_actors " +
                    "(parent_id, name, ref_component_id, scale, nitro, standard, lft, rght, scenenode, crud, config_type, model_image, camera_data, ref_actor_row_id, resource_name, channel ) " +
                    "select 0, name, ref_component_id, scale, nitro, standard, " + "lft + " + maxRght +
                    " - (select lft from configured_actors where id = " + refid + ") + 1, " + "rght + " +
                    maxRght + " - (select lft from configured_actors where id = " + refid + ") + 1, " +
                    "scenenode, crud, config_type, model_image, camera_data , id, resource_name, channel  " +
                    "from configured_actors " +
                    "where lft between (select lft from configured_actors where id = " + refid + ") " +
                    "and (select rght from configured_actors where id = " + refid + ") " +
                    "order by lft asc ";
                executeQuery( sql );

                // 3. now update parent id's
                sql = "update configured_actors set parent_id = " +
                      " ( select cm.id from configured_actors as cm " +
                      " where configured_actors.lft between cm.lft and cm.rght " +
                      " and configured_actors.lft != cm.lft " + " order by cm.lft desc limit 1 );";
                executeQuery( sql );

                var name = "New Model";
                sql = "update configured_actors set name = '" + name + "' where id = 1 + " + maxID;
                executeQuery( sql );

                // copy attributes
                sql =
                    "insert into main.attribs (title, value, ref_component_id, actor_id, configured_actor_id, applied) " +
                    "select attribs.title, attribs.value, attribs.ref_component_id, 1 + " + maxID +
                    ", " + "main.configured_actors.id as 'configured_actor_id', attribs.applied " +
                    "from main.configured_actors, attribs " +
                    "where main.configured_actors.ref_actor_row_id = attribs.configured_actor_id " +
                    "and attribs.actor_id = " + refid +
                    " and main.configured_actors.lft between (select lft from main.configured_actors where id = 1 + " +
                    maxID + ") " + "and (select rght from main.configured_actors where id = 1 + " +
                    maxID + ")";
                executeQuery( sql );

                // copy  virtual tx rows
                sql = "insert into main.virtual_tx (actor_id, param, value) select 1 + " + maxID +
                      ", param, value from virtual_tx where actor_id = " + refid;
                rs = executeQuery( sql );

                sql =
                    "select id from attribs where title = 'FlEqFlybarless' and value = 'true' and actor_id = 1 + " +
                    maxID;
                var checkFlybarless = executeQuery( sql );
                if( checkFlybarless != null )
                {
                    if( checkFlybarless.rows.Count > 0 )
                    {
                        // apply default responsemode
                        sql = "update attribs set value = '" + defaultResponseMode +
                              "' where actor_id = 1 + " + maxID + " and title = 'ResponseMode'";
                        rs = executeQuery( sql );

                        var VBarSettings = "";
                        if( defaultResponseMode == "1" )
                            sql =
                                "select value from attribs where title = 'VBarSportMode' and actor_id = 1 + " +
                                maxID;
                        else if( defaultResponseMode == "2" )
                            sql =
                                "select value from attribs where title = 'VBarExpertMode' and actor_id = 1 + " +
                                maxID;
                        else if( defaultResponseMode == "3" )
                            sql =
                                "select value from attribs where title = 'VBarProMode' and actor_id = 1 + " +
                                maxID;
                        else if( defaultResponseMode == "4" )
                            sql =
                                "select value from attribs where title = 'VBarCustomMode' and actor_id = 1 + " +
                                maxID;
                        rs = executeQuery( sql );
                        if( rs.rows.Count > 0 )
                        {
                            VBarSettings = rs.rows[0].GetFieldValue( 0 );
                        }

                        var tempSplit = VBarSettings.Split( ',' );
                        if( tempSplit.Length >= 12 )
                        {
                            var VBarStickSensitivity = tempSplit[0];
                            var VBarAilGain = float.Parse( tempSplit[1] ) / 100.0f;
                            var VBarEleGain = float.Parse( tempSplit[2] ) / 100.0f;
                            var VBarDecayTime = ( 101.0f - ( float.Parse( tempSplit[3] ) ) ) / 10.0f;
                            var VBarCollectiveMin = float.Parse( tempSplit[4] ) / 100.0f;
                            var VBarCollectiveMax = float.Parse( tempSplit[5] ) / 100.0f;
                            var VBarStickExponential = float.Parse( tempSplit[6] ) / 100.0f;
                            var VBarStabilityGain = float.Parse( tempSplit[7] ) / 100.0f;
                            var VBarDirectMix = float.Parse( tempSplit[8] ) / 100.0f;
                            var VBarAilFilter = float.Parse( tempSplit[9] ) / 100.0f;
                            var VBarEleFilter = float.Parse( tempSplit[10] ) / 100.0f;
                            var VBarDeadband = float.Parse( tempSplit[11] ) / 100.0f;
                            var VBarQuickTrim = float.Parse( tempSplit[12] ) / 50.0f;
                            var VBarCollectiveManager = 0.8f;

                            if( tempSplit.Length > 13 )
                            {
                                VBarCollectiveManager = float.Parse( tempSplit[13] ) / 100.0f;
                            }

                            sql = "UPDATE attribs" + " SET value = CASE title" +
                                  " WHEN 'FlEqVBarStickSensitivity' THEN '" + VBarStickSensitivity +
                                  "'" + " WHEN 'FlEqVBarDecayTime' THEN '" + VBarDecayTime + "'" +
                                  " WHEN 'FlEqVBarAilGain' THEN '" + VBarAilGain + "'" +
                                  " WHEN 'FlEqVBarEleGain' THEN '" + VBarEleGain + "'" +
                                  " WHEN 'FlEqVBarStickExponential' THEN '" + VBarStickExponential +
                                  "'" + " WHEN 'FlEqVBarStabGain' THEN '" + VBarStabilityGain + "'" +
                                  " WHEN 'FlybarCtrlCollectiveLow' THEN '" + VBarCollectiveMin + "'" +
                                  " WHEN 'FlybarCtrlCollectiveHigh' THEN '" + VBarCollectiveMax + "'" +
                                  " WHEN 'FlEqVBarDirectMix' THEN '" + VBarDirectMix + "'" +
                                  " WHEN 'FlEqVBarAilStickFilter' THEN '" + VBarAilFilter + "'" +
                                  " WHEN 'FlEqVBarEleStickFilter' THEN '" + VBarEleFilter + "'" +
                                  " WHEN 'FlEqVBarStickDeadband' THEN '" + VBarDeadband + "'" +
                                  " WHEN 'FlEqGyroServoOffset' THEN '" + VBarQuickTrim + "'" +
                                  " WHEN 'FlEqRL40MainGain' THEN '" + VBarCollectiveManager + "'" +
                                  " Else value" + " END" +
                                  " where applied = 'true' AND actor_id = 1 + " + maxID;
                            rs = executeQuery( sql );
                        }
                    }
                }

                var targetModel = int.Parse( maxID ) + 1;
                sql = "insert  into actor_objects (actor_id, class, ident, group_type ) select " +
                      targetModel +
                      ",class, ident, group_type from actor_objects where actor_id = " + refid;
                var query = executeQuery( sql );

                sql = "select * from actor_objects where actor_id=" + refid;
                var originalObjects = executeQuery( sql );

                sql = "select * from actor_objects where actor_id=" + targetModel;
                var newObjects = executeQuery( sql );

                var idMap = new List<KeyValuePair<string, string>>();
                foreach( var r in originalObjects.rows )
                {
                    var originalId = r.GetFieldValue( "id" );
                    idMap.Add( new KeyValuePair<string, string>( originalId, "" ) );
                }

                int currentEntry = 0;
                foreach( var r in newObjects.rows )
                {
                    var originalId = r.GetFieldValue( "id" );
                    var pair = idMap[currentEntry];
                    idMap[currentEntry] = new KeyValuePair<string, string>( pair.Key, originalId );

                    currentEntry++;
                }

                for( int i = 0; i < idMap.Count; ++i )
                {
                    var entry = idMap[i];
                    sql = "insert  into object_attributes (object_id,name,value) select " + entry.Value +
                          ",name,value from object_attributes where object_id = " + entry.Key;
                    executeQuery( sql );
                }

                return int.Parse( maxID ) + 1;
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }

            return -1;
        }

        public json.query executeQuery( String sql )
        {
            json.query result = new json.query();

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        foreach( var attachedDatabase in AttachedDatabases )
                        {
                            string SQL = "ATTACH '" + attachedDatabase.Key + "' AS " +
                                         attachedDatabase.Value + "";
                            SQLiteCommand cmd = new SQLiteCommand( SQL, con );
                            int retval = 0;
                            try
                            {
                                retval = cmd.ExecuteNonQuery();
                            }
                            catch( Exception )
                            {
                                MessageBox.Show( "An error occurred, your import was not completed." );
                            }
                            finally
                            {
                                cmd.Dispose();
                            }
                        }

                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var rows = new json.rows();

                                var count = read.FieldCount;
                                for( int i = 0; i < count; ++i )
                                {
                                    var field = new json.field();
                                    field.name = read.GetName( i );
                                    field.value = Convert.ToString( read.GetValue( i ) );

                                    rows.fieldPairs.Add( field );
                                }

                                result.rows.Add( rows );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }

            return result;
        }

        public int executeDML( String sql )
        {
            json.query result = new json.query();

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        return comm.ExecuteNonQuery();
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }

            return 0;
        }

        public void exportModel( string path, int epxModelID )
        {
            try
            {
                int count = 0;

                string sql = "select name from configured_actors where id = " + epxModelID + ";";
                var rs = executeQuery( sql );
                if( rs == null )
                {
                    return;
                }

                string model_name = "";
                if( rs.rows.Count > 0 )
                {
                    model_name = rs.GetFieldValue( 0 );
                }

                var exp_db = new Database();
                if( model_name != "" )
                {
                    string wsPath = path;
                    string fileName = wsPath + model_name + ".mxp";
                    SQLiteConnection.CreateFile( fileName );

                    exp_db.ConnectionString = @"Data Source = " + fileName + "; Version = 3";
                    exp_db.open();

                    model_name = wsPath + model_name + ".mxp";
                }

                string expDB = "export";
                var attachSql = "attach database '" + model_name + "' as 'export'";
                executeDML( attachSql );
                AttachedDatabases.Add( new KeyValuePair<string, string>( model_name, "export" ) );

                sql = "drop table if exists " + expDB + ".settings;";
                executeQuery( sql );
                sql = "drop table if exists " + expDB + ".configured_actors;";
                executeQuery( sql );
                sql = "drop table if exists " + expDB + ".virtual_tx;";
                executeQuery( sql );
                sql = "drop table if exists " + expDB + ".attribs;";
                executeQuery( sql );

                sql = "create table " + expDB +
                      ".settings as select * from settings where param = 'db_version';";
                executeQuery( sql );

                sql = "create table " + expDB + ".configured_actors as " +
                      "select * from main.configured_actors " +
                      "where lft <= ( select rght from main.configured_actors where id = " + epxModelID +
                      " ) " +
                      "and rght >= (select lft from main.configured_actors where id = " + epxModelID +
                      " ) " + "order by lft; ";

                executeQuery( sql );

                sql = "create table " + expDB + ".virtual_tx as " +
                      "select * from main.virtual_tx where actor_id = " + epxModelID + ";";
                executeQuery( sql );

                sql = "create table " + expDB +
                      ".attribs as select * from main.attribs where actor_id = " + epxModelID + ";";
                executeQuery( sql );

                sql =
                    "create table " + expDB +
                    ".actor_objects as select * from main.actor_objects where actor_id = " + epxModelID +
                    ";";
                executeQuery( sql );

                List<ModelObject> modelObjects = new List<ModelObject>();
                var modelObjectsSql = "select * from actor_objects where actor_id = " + epxModelID;
                var modelObjectsResult = executeQuery( modelObjectsSql );
                foreach( var r in modelObjectsResult.rows )
                {
                    var original_id = r.GetFieldValueAsInt( "original_id" );
                    var id = r.GetFieldValueAsInt( "id" );
                    var modelId = r.GetFieldValueAsInt( "actor_id" );
                    var className = r.GetFieldValue( "class" );
                    var ident = r.GetFieldValue( "ident" );
                    var groupType = r.GetFieldValue( "group_type" );
                    var originalId = 0;  // original_id != null ? Convert.ToInt32(original_id) : 0;

                    modelObjects.Add( new ModelObject() { Id = id, ModelId = modelId, Class = className,
                                                          Ident = ident, GroupType = groupType,
                                                          OriginalId = originalId } );
                }

                sql = "create table " + expDB +
                      ".object_attributes as select * from main.object_attributes where object_id = ";
                int modelObjectCount = 0;
                foreach( var m in modelObjects )
                {
                    if( modelObjectCount > 0 )
                    {
                        sql += " or object_id = ";
                    }

                    sql += m.Id;

                    modelObjectCount++;
                }

                executeQuery( sql );

                sql = "select * from configured_actors where id = " + epxModelID + ";";
                rs = executeQuery( sql );
                model_name = "";
                if( rs.rows.Count > 0 )
                {
                    model_name = rs.rows[0].GetFieldValue( "name" );
                }

                executeQuery( "DETACH 'export'" );

                AttachedDatabases.Clear();
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        public int importModel( string filePathUTF8, string modelName )
        {
            try
            {
                string refid = "";

                //StringW userPath = systemSettings->getUserDataFolder();

                string filePath = filePathUTF8;

                string empDB = "import";
                executeQuery( "ATTACH DATABASE '" + filePath + "' as 'import';" );

                AttachedDatabases.Add( new KeyValuePair<string, string>( filePath, "import" ) );

                //executeDML(L"attach database '" + filePath + L"' as 'import'; ");

                int id = 0;

                //  check db level
                string sql = "select refdb.settings.value - " + empDB +
                             ".settings.value from refdb.settings, " + empDB +
                             ".settings where refdb.settings.param = " + empDB +
                             ".settings.param and refdb.settings.param = 'db_version';";
                var rs = executeQuery( sql );
                if( rs != null )
                {
                    if( rs.rows.Count > 0 )
                    {
                        int dbDelta = rs.rows[0].GetFieldValueAsInt( 0 );
                        if( dbDelta < 0 )
                        {
                            // refdb is lower level than import.
                            executeQuery( "detach database 'import'; " );
                            MessageBox.Show( "Error cannot import." );
                            return 0;
                        }
                    }
                }
                else
                {
                    // refdb is lower level than import.
                    executeQuery( "detach database 'import'; " );
                    MessageBox.Show( "Error cannot import." );
                    return 0;
                }

                sql = "select id, parent_id from " + empDB + ".configured_actors where standard = 1 ;";
                rs = executeQuery( sql );

                string parent_id = "";
                string maxID = "";
                string maxLft = "";
                string maxRght = "";

                if( rs != null && rs.rows.Count > 0 )
                {
                    refid = rs.GetFieldValue( 0 );
                    parent_id = rs.GetFieldValue( 1 );
                }

                sql = "select max(id) from main.configured_actors";
                rs = executeQuery( sql );
                if( rs != null && rs.rows.Count > 0 )
                {
                    maxID = rs.GetFieldValue( 0 );
                }

                int newModelId = int.Parse( maxID ) + 1;

                // where to put copyied model
                sql =
                    "select lft, rght from main.configured_actors where id = (select max(id) from main.configured_actors where parent_id = " +
                    parent_id + ")";
                rs = executeQuery( sql );
                if( rs != null && rs.rows.Count > 0 )
                {
                    maxLft = rs.GetFieldValue( 0 );
                    maxRght = rs.GetFieldValue( 1 );
                }

                int containerID = 0;
                if( rs != null )
                {
                    string containerIDStr = rs.GetFieldValue( 0 );
                    containerID = int.Parse( containerIDStr );
                }

                // 1. Make room to copy the branch for model refid from refdb
                sql = "update configured_actors set " + "lft = lft + (select (rght - lft) + 1 from " +
                      empDB + ".configured_actors where id = " + refid + ") " + "where lft > " + maxRght;
                executeQuery( sql );
                sql = "update configured_actors set " + "rght = rght + (select (rght - lft) + 1 from " +
                      empDB + ".configured_actors where id = " + refid + ") " + "where rght > " +
                      maxRght;
                executeQuery( sql );

                // 2. Copy the tree with offsets
                sql =
                    "insert into configured_actors " +
                    "(parent_id, name, ref_component_id, scale, nitro, standard, lft, rght, scenenode, crud, config_type, model_image, camera_data, ref_actor_row_id, resource_name, channel ) " +
                    "select 0, name, ref_component_id, scale, nitro, standard, " + "lft + " + maxRght +
                    " - (select lft from " + empDB + ".configured_actors where id = " + refid +
                    ") + 1, " + "rght + " + maxRght + " - (select lft from " + empDB +
                    ".configured_actors where id = " + refid + ") + 1, " +
                    "scenenode, crud, config_type, model_image, camera_data , ref_actor_row_id, resource_name, channel  " +
                    "from " + empDB + ".configured_actors " + "where lft between (select lft from " +
                    empDB + ".configured_actors where id = " + refid + ") " + "and (select rght from " +
                    empDB + ".configured_actors where id = " + refid + ") " + "order by lft asc ";
                executeQuery( sql );

                // 3. now update parent id's
                sql = "update configured_actors set parent_id = " +
                      " ( select cm.id from configured_actors as cm " +
                      " where configured_actors.lft between cm.lft and cm.rght " +
                      " and configured_actors.lft != cm.lft " + " order by cm.lft desc limit 1 );";
                executeQuery( sql );

                sql = "update configured_actors set name = '" + modelName + "' where id = 1 + " + maxID;
                executeQuery( sql );

                sql =
                    "insert into main.attribs (title, value, ref_component_id, actor_id, configured_actor_id, applied) " +
                    "select a.title, a.value, a.ref_component_id, 1+" + maxID +
                    ", mcm.id, a.applied   from " + empDB + ".configured_actors as cm, " + empDB +
                    ".attribs as a, main.configured_actors as mcm " +
                    "where a.configured_actor_id = cm.id " +
                    "and mcm.ref_actor_row_id = cm.ref_actor_row_id " +
                    "and mcm.lft between (select lft from main.configured_actors where id = 1 + " +
                    maxID + ") and (select rght from main.configured_actors where id = 1 + " + maxID +
                    "); ";
                executeQuery( sql );
                // copy  virtual tx rows
                sql = "insert into main.virtual_tx (actor_id, param, value) select 1 + " + maxID +
                      ", param, value from " + empDB + ".virtual_tx where actor_id = " + refid;
                rs = executeQuery( sql );

                sql = "select * from " + empDB +
                      ".virtual_tx where param = 'ch5switch' and actor_id = " + refid;
                rs = executeQuery( sql );
                if( rs != null && rs.rows.Count > 0 )
                {
                    sql = "insert into main.virtual_tx (actor_id, param, value) values (1 + " + maxID +
                          ", 'ch5mode', '0')";
                    rs = executeQuery( sql );
                    sql = "insert into main.virtual_tx (actor_id, param, value) values (1 + " + maxID +
                          ", 'ch5switch', '1')";
                    rs = executeQuery( sql );
                }

                sql = "insert into actor_objects (actor_id, class, ident, group_type ) select " +
                      newModelId + ",class, ident, group_type from  " + empDB +
                      ".actor_objects where actor_id = " + refid;
                executeQuery( sql );

                List<ModelObject> modelObjects = new List<ModelObject>();
                var modelObjectsSql =
                    "select * from " + empDB + ".actor_objects where actor_id = " + refid;
                var modelObjectsResult = executeQuery( modelObjectsSql );
                foreach( var r in modelObjectsResult.rows )
                {
                    var original_id = r.GetFieldValueAsInt( "original_id" );
                    var modelObjectId = r.GetFieldValueAsInt( "id" );
                    var modelId = r.GetFieldValueAsInt( "actor_id" );
                    var className = r.GetFieldValue( "class" );
                    var ident = r.GetFieldValue( "ident" );
                    var groupType = r.GetFieldValue( "group_type" );
                    var originalId = 0;  // original_id != null ? Convert.ToInt32(original_id) : 0;

                    modelObjects.Add( new ModelObject() { Id = modelObjectId, ModelId = modelId,
                                                          Class = className, Ident = ident,
                                                          GroupType = groupType,
                                                          OriginalId = originalId } );
                }

                List<ModelObject> newModelObjects = new List<ModelObject>();
                var newModelObjectsSql = "select * from actor_objects where actor_id = " + newModelId;
                var newModelObjectsResult = executeQuery( newModelObjectsSql );
                foreach( var r in newModelObjectsResult.rows )
                {
                    var original_id = r.GetFieldValueAsInt( "original_id" );
                    var modelObjectId = r.GetFieldValueAsInt( "id" );
                    var modelId = r.GetFieldValueAsInt( "actor_id" );
                    var className = r.GetFieldValue( "class" );
                    var ident = r.GetFieldValue( "ident" );
                    var groupType = r.GetFieldValue( "group_type" );
                    var originalId = 0;  // original_id != null ? Convert.ToInt32(original_id) : 0;

                    newModelObjects.Add( new ModelObject() { Id = modelObjectId, ModelId = modelId,
                                                             Class = className, Ident = ident,
                                                             GroupType = groupType,
                                                             OriginalId = originalId } );
                }

                int modelObjectCount = 0;
                foreach( var m in modelObjects )
                {
                    sql = "insert into object_attributes (object_id,name,value) select " +
                          newModelObjects[modelObjectCount].Id + ",name,value from " + empDB +
                          ".object_attributes where object_id = " + m.Id;
                    executeQuery( sql );

                    modelObjectCount++;
                }

                sql = "UPDATE `configured_actors` SET `name`= '" +
                      Path.GetFileNameWithoutExtension( filePath ) + "' WHERE `id`= " + newModelId;
                executeQuery( sql );

                executeQuery( "detach database 'import'; " );
                MessageBox.Show( "importSuccess" );

                AttachedDatabases.Clear();

                return int.Parse( maxID ) + 1;
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }

            return 0;
        }

        private int executeNonQuery( String sql )
        {
            int rowsUpdated = -1;
            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        rowsUpdated = comm.ExecuteNonQuery();
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            return rowsUpdated;
        }

        public int GetChildCount( int parentId )
        {
            int left = 0;

            String sql = "select * from configured_actors where id = " + parentId;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "select * from configured_actors where lft > " + left + " and parent_id = " + parentId;
            var result = executeQuery( sql );

            return result.rows.Count;
        }

        public List<int> GetNodeChildren( int parentId )
        {
            var children = new List<int>();

            int left = 0;

            String sql = "select * from configured_actors where id = " + parentId;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "select * from configured_actors where lft > " + left + " and parent_id = " + parentId;
            var result = executeQuery( sql );
            foreach( var r in result.rows )
            {
                var iId = r.GetFieldValueAsInt( "id" );
                children.Add( iId );
            }

            return children;
        }

        public int GetNodeId( int parentId, string name )
        {
            int left = 0;

            String sql = "select * from configured_actors where id = " + parentId;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "select * from configured_actors where lft > " + left + " and parent_id = " + parentId;
            var result = executeQuery( sql );
            foreach( var r in result.rows )
            {
                var rowName = r.GetFieldValue( "name" );
                if( name == rowName )
                {
                    return r.GetFieldValueAsInt( "id" );
                }
            }

            return -1;
        }

        public bool IsExistingNode( int parentId, string name )
        {
            int left = 0;

            String sql = "select * from configured_actors where id = " + parentId;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "select * from configured_actors where lft > " + left + " and parent_id = " + parentId;
            var result = executeQuery( sql );
            foreach( var r in result.rows )
            {
                var rowName = r.GetFieldValue( "name" );
                if( name == rowName )
                {
                    return true;
                }
            }

            return false;
        }

        public int AddChildNode( int parentId, string name )
        {
            int left = 0;
            int result = 0;

            String sql = "select * from configured_actors where id = " + parentId;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "update configured_actors set rght = rght + 2 where rght > " + left;
            result = executeNonQuery( sql );

            sql = "update configured_actors set lft = lft + 2 where lft > " + left;
            result = executeNonQuery( sql );

            sql = "insert into configured_actors (name, parent_id, lft, rght) values ('" + name + "'," +
                  parentId + ", " + left + "+1," + left + "+2)";
            var query = executeQuery( sql );
            foreach( var r in query.rows )
            {
                return r.GetFieldValueAsInt( "id" );
            }

            return -1;
        }

        private void RemoveNode( int node_id )
        {
            int left = 0;
            int right = 0;
            int offset = 0;
            int result = 0;

            String sql = "select * from configured_actors where id = " + node_id;

            using( SQLiteConnection con = new SQLiteConnection( ConnectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                left = read.GetInt32( read.GetOrdinal( "lft" ) );
                                right = read.GetInt32( read.GetOrdinal( "rght" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            sql = "delete from configured_actors where lft between " + left + " and " + right;
            result = executeNonQuery( sql );

            offset = 2;

            sql = "update configured_actors set lft = lft - " + offset + "  where lft > " + left;
            result = executeNonQuery( sql );

            sql = "update configured_actors set rght = rght - " + offset + "  where rght > " + right;
            result = executeNonQuery( sql );

            sql = "delete from attribs where configured_actor_id = " + node_id;
            result = executeNonQuery( sql );
        }

        public void SetupFW( int modelId )
        {
            int wingComponentId = 902;
            int controlSurfaceComponentId = 903;

            if( !IsExistingNode( modelId, "Wings" ) )
            {
                AddChildNode( modelId, "Wings" );
            }

            int wingsNodeId = GetNodeId( modelId, "Wings" );
            if( wingsNodeId != -1 )
            {
                if( !IsExistingNode( wingsNodeId, "Main Wings" ) )
                {
                    AddChildNode( wingsNodeId, "Main Wings" );
                }

                if( !IsExistingNode( wingsNodeId, "Tail Wings" ) )
                {
                    AddChildNode( wingsNodeId, "Tail Wings" );
                }

                int mainWingNodeId = GetNodeId( wingsNodeId, "Main Wings" );
                if( mainWingNodeId != -1 )
                {
                    if( GetChildCount( mainWingNodeId ) == 0 )
                    {
                        AddChildNode( mainWingNodeId, "Wing Component" );
                    }

                    var mainChildren = GetNodeChildren( mainWingNodeId );
                    foreach( var child in mainChildren )
                    {
                        AddComponent( modelId, child, wingComponentId, "Main Wing" );
                    }

                    int controlSurfacesNodeId = GetNodeId( mainWingNodeId, "Control Surfaces" );
                    if( controlSurfacesNodeId == -1 )
                    {
                        controlSurfacesNodeId = AddChildNode( mainWingNodeId, "Control Surfaces" );
                    }

                    if( GetChildCount( mainWingNodeId ) == 0 )
                    {
                        AddChildNode( mainWingNodeId, "Wing Component" );
                    }

                    if( controlSurfacesNodeId != -1 )
                    {
                        //AddComponent(modelId, child, controlSurfaceComponentId, "Main Wing");
                    }
                }

                int tailWingNodeId = GetNodeId( wingsNodeId, "Tail Wings" );
                if( tailWingNodeId != -1 )
                {
                    if( GetChildCount( tailWingNodeId ) == 0 )
                    {
                        AddChildNode( tailWingNodeId, "Wing Component" );
                    }

                    var tailChildren = GetNodeChildren( tailWingNodeId );
                    foreach( var child in tailChildren )
                    {
                        AddComponent( modelId, child, wingComponentId, "Tail Wing" );
                    }

                    int controlSurfacesNodeId = GetNodeId( tailWingNodeId, "Control Surfaces" );
                    if( controlSurfacesNodeId == -1 )
                    {
                        controlSurfacesNodeId = AddChildNode( tailWingNodeId, "Control Surfaces" );
                    }
                }
            }
        }

        public void SetupAllFW()
        {
            var query = executeQuery( "select * from configured_actors where parent_id = 220" );
            foreach( var r in query.rows )
            {
                var id = r.GetFieldValueAsInt( "id" );
                SetupFW( id );
            }
        }

        public void AddComponent( int actor_id, int row_id, int component_id, string componentName )
        {
            var title = componentName;
            var scene = "";
            var chan = "";
            var extras = "";

            // if (row_id != null && row_id != "" && component_id != null && component_id != "" && title != null && title != "" && actor_id != null && actor_id != "")
            {
                if( chan != null && chan != "" )
                {
                    extras = extras + ", channel = '" + chan + "'";
                }

                if( scene != null && scene != "" )
                {
                    extras = extras + ", scenenode = '" + scene + "'";
                }

                String sql = "update configured_actors set name = '" + title +
                             "', ref_component_id = " + component_id + " " + extras +
                             " where id = " + row_id;
                int result = executeNonQuery( sql );
                if( result == -1 )
                {
                    MessageBox.Show( "Component add failed" );
                    return;
                }
                sql = "delete from attribs where configured_actor_id = " + row_id;
                result = executeNonQuery( sql );
                if( result == -1 )
                {
                    MessageBox.Show( "Attribs delete failed" );
                    return;
                }
                sql =
                    "insert into attribs (title,value, ref_component_id, actor_id, configured_actor_id, applied) SELECT title, value, ref_component_id, " +
                    actor_id + "," + row_id +
                    ", 'true' FROM ref_attribs where  ref_component_id = " + component_id;
                result = executeNonQuery( sql );
                if( result == -1 )
                {
                    MessageBox.Show( "Attribs add failed" );
                    return;
                }
            }
        }

        public void DeleteComponent( int object_id )
        {
            var sql = "delete from object_attributes where object_id = " + object_id;
            executeQuery( sql );

            var sqlObject = "delete from actor_objects where id = " + object_id;
            executeQuery( sqlObject );
        }

        public void DeleteRefComponent( int ref_component_id )
        {
            var sql = "delete from ref_attribs where ref_component_id = " + ref_component_id;
            executeQuery( sql );

            var sqlObject = "delete from ref_components where id = " + ref_component_id;
            executeQuery( sqlObject );

            var sqlGroup = "delete from component_groups_ref_components where ref_component_id = " +
                           ref_component_id;
            executeQuery( sqlGroup );
        }

        public void CloneComponent( int object_id )
        {
            var sqlAttribs = "select * from object_attributes where object_id = " + object_id;
            var attribResult = executeQuery( sqlAttribs );

            var sqlObject = "select * from actor_objects where id = " + object_id;
            var objectResult = executeQuery( sqlObject );

            foreach( var r in objectResult.rows )
            {
                var modelId = r.GetFieldValue( "actor_id" );
                var className = r.GetFieldValue( "class" );
                var ident = r.GetFieldValue( "ident" );
                var groupType = r.GetFieldValue( "group_type" );
                var originalId = r.GetFieldValue( "original_id" );

                var sql = "INSERT INTO `actor_objects` VALUES(NULL, " + modelId + ", '" + className +
                          "', '" + ident + "', '" + groupType + "', '" + originalId + "');";
                executeQuery( sql );
            }

            foreach( var r in attribResult.rows )
            {
                var name = r.GetFieldValue( "name" );
                var value = r.GetFieldValue( "value" );

                var sql =
                    "INSERT INTO `object_attributes` VALUES(NULL, (select max(id) from actor_objects), '" +
                    name + "', '" + value + "');";
                executeQuery( sql );
            }
        }

        public void CloneRefComponent( int ref_component_id )
        {
            var sqlAttribs = "select * from ref_attribs where ref_component_id = " + ref_component_id;
            var attribResult = executeQuery( sqlAttribs );

            var sqlObject = "select * from ref_components where id = " + ref_component_id;
            var objectResult = executeQuery( sqlObject );

            foreach( var r in objectResult.rows )
            {
                var id = r.GetFieldValueAsInt( "id" );
                var title = r.GetFieldValue( "title" );

                var sql = "INSERT INTO `ref_components` VALUES(NULL, " + id + ", '" + title +
                          "', (select max(id) from ref_components), 'true');";
                executeQuery( sql );
            }

            foreach( var r in attribResult.rows )
            {
                var name = r.GetFieldValue( "title" );
                var value = r.GetFieldValue( "value" );

                var sql = "INSERT INTO `ref_attribs` VALUES(NULL, '" + name + "', '" + value +
                          "', (select max(id) from ref_components), '');";
                executeQuery( sql );
            }
        }

        public void CloneObjectAttrib( int id )
        {
            var sqlAttribs = "select * from object_attributes where id = " + id;
            var attribResult = executeQuery( sqlAttribs );

            foreach( var r in attribResult.rows )
            {
                var name = r.GetFieldValue( "name" );
                var value = r.GetFieldValue( "value" );

                var sql =
                    "INSERT INTO `object_attributes` VALUES(NULL, (select max(id) from actor_objects), '" +
                    name + "', '" + value + "');";
                executeQuery( sql );
            }
        }

        public void CloneRefAttrib( int id )
        {
            var sqlAttribs = "select * from ref_attribs where id = " + id;
            var attribResult = executeQuery( sqlAttribs );

            foreach( var r in attribResult.rows )
            {
                var name = r.GetFieldValue( "title" );
                var value = r.GetFieldValue( "value" );
                var refId = r.GetFieldValueAsInt( "ref_component_id" );

                var sql = "INSERT INTO `ref_attribs` VALUES(NULL, '" + name + "', '" + value + "', " +
                          refId + ", '');";
                executeQuery( sql );
            }
        }

        public void CloneAttrib( int id )
        {
            var sqlAttribs = "select * from attribs where id = " + id;
            var attribResult = executeQuery( sqlAttribs );

            foreach( var r in attribResult.rows )
            {
                var name = r.GetFieldValue( "title" );
                var value = r.GetFieldValue( "value" );
                var refId = r.GetFieldValueAsInt( "ref_component_id" );
                var actor_id = r.GetFieldValueAsInt( "actor_id" );
                var configured_actor_id = r.GetFieldValueAsInt( "configured_actor_id" );

                var sql = "INSERT INTO `attribs` VALUES(NULL, '" + name + "', '" + value + "', " +
                          refId + ", '" + actor_id + "', '" + configured_actor_id + "', 'true');";
                executeQuery( sql );
            }
        }

        public void DeleteObjectAttrib( int id )
        {
            var sql = "delete from object_attributes where id = " + id;
            executeQuery( sql );
        }

        public void DeleteRefAttrib( int id )
        {
            var sql = "delete from ref_attribs where id = " + id;
            executeQuery( sql );
        }

        public void DeleteAttrib( int id )
        {
            var sql = "delete from attribs where id = " + id;
            executeQuery( sql );
        }
    }
}
