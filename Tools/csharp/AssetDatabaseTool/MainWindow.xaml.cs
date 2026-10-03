using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Data;
using System.Data.SQLite;
using System.IO;
using Microsoft.Win32;
using System.Globalization;
using System.Runtime.Serialization;
using Newtonsoft.Json;
using System.Numerics;
using fb.json;
using Path = System.IO.Path;
using System.Diagnostics;
using System.Windows.Markup;
//using InteractiveDataDisplay.WPF;

namespace fb
{

    public class Attrib
    {
        public int Id { get; set; }

        public string Name { get; set; }

        public string Value { get; set; }

        public string GroupType { get; set; }

        public int RefComponentId { get; set; }

        public int ModelId { get; set; }

        public int ConfiguredModelId { get; set; }

        public bool Applied { get; set; }

        public int ObjectId { get; set; }

        public int Channel { get; set; }

        public string Type { get; set; }

        public string Map { get; set; }

        public bool Reverse { get; set; }

        public string Emulation { get; set; }

        public float Offset { get; set; }

        public float Multiplier { get; set; }

        public string Tx { get; set; }

        public float LowMultiplier { get; set; }

        public float HighMultiplier { get; set; }

        public string Details
        {
            get {
                return String.Format(
                    "{0} was born on {1} and this is a long description of the person.",
                    this.Id.ToString(), this.Name );
            }
        }
    }

    public class ConfiguredModelRow
    {
        public int id { get; set; }
        public int parent_id { get; set; }

        public string name { get; set; }

        public int ref_component_id { get; set; }

        public int lft { get; set; }

        public int rgt { get; set; }

        public string data { get; set; }

        public string type { get; set; }

        public int index { get; set; }
    }

    public class RefComponent
    {
        public int Id { get; set; }

        public string Type { get; set; }

        public string Title { get; set; }

        public int RefComponentId { get; set; }

        public bool Enabled { get; set; }
    }

    public class ModelObject
    {
        public int Id { get; set; }

        public int ModelId { get; set; }

        public string Class { get; set; }

        public string Ident { get; set; }

        public string GroupType { get; set; }

        public int OriginalId { get; set; }

        public string Details
        {
            get {
                return String.Format(
                    "{0} was born on {1} and this is a long description of the person.",
                    this.Id.ToString(), this.Ident );
            }
        }
    }

    public class Model
    {
        public int Id { get; set; }

        public int ParentId { get; set; }

        public string Name { get; set; }

        public string Resource { get; set; }

        public string Details
        {
            get {
                return String.Format(
                    "{0} was born on {1} and this is a long description of the person.",
                    this.Id.ToString(), this.Name );
            }
        }
    }

    public class CopyData
    {
    }

    public class PasteData
    {
    }

    public class Fields
    {
        public string name = "";
        public string value = "";
    }

    public class VisibilityToCheckedConverter : IValueConverter
    {
        public object Convert( object value, Type targetType, object parameter,
                               System.Globalization.CultureInfo culture )
        {
            return ( (Visibility)value ) == Visibility.Visible;
        }

        public object ConvertBack( object value, Type targetType, object parameter,
                                   System.Globalization.CultureInfo culture )
        {
            return ( (bool)value ) ? Visibility.Visible : Visibility.Collapsed;
        }
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private string dbFileName;
        private string connectionString;
        private string CachePath { get; set; }
        private json.aircraft_airfoil_data Airfoil = null;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Apply_Click( object sender, RoutedEventArgs e )
        {
        }

        int CloneModel( int refid )
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

        int CloneModel( int refid, string srcConnection, string dstConnection )
        {
            try
            {
                Database srcDatabase = new Database();
                Database dstDatabase = new Database();

                srcDatabase.ConnectionString = srcConnection;
                dstDatabase.ConnectionString = dstConnection;

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

                rs = dstDatabase.executeQuery( sql );
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
                dstDatabase.executeQuery( sql );

                sql =
                    "update configured_actors set " +
                    "rght = rght + (select (rght - lft) + 1 from configured_actors where id = " + refid +
                    ") " + "where rght > " + maxRght;
                dstDatabase.executeQuery( sql );

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
                dstDatabase.executeQuery( sql );

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

        private void ImportModel_Click( object sender, RoutedEventArgs e )
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "mxp|*.mxp";

            var filePath = Convert.ToString( Settings.Default["ModelImport"] );
            if( !string.IsNullOrEmpty( filePath ) )
            {
                try
                {
                    dialog.InitialDirectory = Path.GetDirectoryName( filePath );
                    dialog.FileName = filePath;
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            if( dialog.ShowDialog() == true )
            {
                dbFileName = dialog.FileName;
                Settings.Default["ModelImport"] = dbFileName;

                if( MessageBox.Show( "Import " + dbFileName, "Question", MessageBoxButton.YesNo,
                                     MessageBoxImage.Warning ) == MessageBoxResult.Yes )
                {
                    var database = new Database();
                    database.ConnectionString = connectionString;
                    database.importModel( dbFileName, dbFileName );
                }
            }
        }

        private void ExportModel_Click( object sender, RoutedEventArgs e )
        {
            var model = modelsData.SelectedItem as Model;
            if( model != null )
            {
                if( MessageBox.Show( "Export " + model.Name, "Question", MessageBoxButton.YesNo,
                                     MessageBoxImage.Warning ) == MessageBoxResult.Yes )
                {
                    var database = new Database();
                    database.ConnectionString = connectionString;

                    //database.exportModel(@"H:\dev\ref_db_tool\WP_db_tool\bin\Debug\", model.Id);
                    database.exportModel( @"", model.Id );

                    updateModelsTab();
                }
            }
            else
            {
                MessageBox.Show( "No model selected." );
            }
        }

        private void CopyModelFromOther_Click( object sender, RoutedEventArgs e )
        {
            var srcDatabase = "";

            try
            {
                OpenFileDialog dialog = new OpenFileDialog();
                dialog.Filter = "DB|*.db";

                var filePath = Convert.ToString( Settings.Default["FBDatabaseOtherDB"] );
                if( !string.IsNullOrEmpty( filePath ) )
                {
                    try
                    {
                        dialog.InitialDirectory = Path.GetDirectoryName( filePath );
                        dialog.FileName = filePath;
                    }
                    catch( Exception ex )
                    {
                        MessageBox.Show( ex.Message );
                    }
                }

                if( dialog.ShowDialog() == true )
                {
                    dbFileName = dialog.FileName;
                    Settings.Default["FBDatabaseOtherDB"] = dbFileName;
                }

                srcDatabase = @"Data Source = " + dbFileName + "; Version = 3";

                CopyFixedWingDataWindow popup = new CopyFixedWingDataWindow();
                popup.ConnectionSrc = srcDatabase;
                popup.ConnectionDst = connectionString;
                popup.ShowDialog();
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        private void CloneModelFromOther_Click( object sender, RoutedEventArgs e )
        {
        }

        private void CloneModel_Click( object sender, RoutedEventArgs e )
        {
            var model = modelsData.SelectedItem as Model;
            if( model != null )
            {
                if( MessageBox.Show( "Clone " + model.Name, "Question", MessageBoxButton.YesNo,
                                     MessageBoxImage.Warning ) == MessageBoxResult.Yes )
                {
                    CloneModel( model.Id );
                    updateModelsTab();
                }
            }
            else
            {
                MessageBox.Show( "No model selected." );
            }
        }

        void GenerateCamberData()
        {
            if( Airfoil != null )
            {
                var value = 0.1f;

                if( Airfoil.clPlus10.values.Count == 0 )
                {
                    Airfoil.clPlus10.values = new List<aircraft_curve_value>( Airfoil.cl.values );

                    for( int i = 0; i < Airfoil.clPlus10.values.Count; ++i )
                    {
                        var val = Airfoil.clPlus10.values[i];
                        Airfoil.clPlus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value + value );
                    }
                }

                if( Airfoil.cdPlus10.values.Count == 0 )
                {
                    Airfoil.cdPlus10.values = new List<aircraft_curve_value>( Airfoil.cd.values );

                    for( int i = 0; i < Airfoil.cdPlus10.values.Count; ++i )
                    {
                        var val = Airfoil.cdPlus10.values[i];
                        Airfoil.cdPlus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value + value );
                    }
                }

                if( Airfoil.cmPlus10.values.Count == 0 )
                {
                    Airfoil.cmPlus10.values = new List<aircraft_curve_value>( Airfoil.cm.values );

                    for( int i = 0; i < Airfoil.cmPlus10.values.Count; ++i )
                    {
                        var val = Airfoil.cmPlus10.values[i];
                        Airfoil.cmPlus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value + value );
                    }
                }

                if( Airfoil.clMinus10.values.Count == 0 )
                {
                    Airfoil.clMinus10.values = new List<aircraft_curve_value>( Airfoil.cl.values );

                    for( int i = 0; i < Airfoil.clMinus10.values.Count; ++i )
                    {
                        var val = Airfoil.clMinus10.values[i];
                        Airfoil.clMinus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value - value );
                    }
                }

                if( Airfoil.cdMinus10.values.Count == 0 )
                {
                    Airfoil.cdMinus10.values = new List<aircraft_curve_value>( Airfoil.cd.values );

                    for( int i = 0; i < Airfoil.cdMinus10.values.Count; ++i )
                    {
                        var val = Airfoil.cdMinus10.values[i];
                        Airfoil.cdMinus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value - value );
                    }
                }

                if( Airfoil.cmMinus10.values.Count == 0 )
                {
                    Airfoil.cmMinus10.values = new List<aircraft_curve_value>( Airfoil.cm.values );

                    for( int i = 0; i < Airfoil.cmMinus10.values.Count; ++i )
                    {
                        var val = Airfoil.cmMinus10.values[i];
                        Airfoil.cmMinus10.values[i] =
                            new json.aircraft_curve_value( val.time, val.value - value );
                    }
                }
            }
        }

        private void SaveAirfoil_Click( object sender, RoutedEventArgs e )
        {
            if( Airfoil != null )
            {
                var fileName = "";
                SaveFileDialog dialog = new SaveFileDialog();
                dialog.Filter = "airfoil|*.airfoil";

                var filePath = Convert.ToString( Settings.Default["Airfoil"] );
                if( !string.IsNullOrEmpty( filePath ) )
                {
                    try
                    {
                        dialog.InitialDirectory = Path.GetDirectoryName( filePath );
                        dialog.FileName = filePath;
                    }
                    catch( Exception ex )
                    {
                        MessageBox.Show( ex.Message );
                    }
                }

                if( dialog.ShowDialog() == true )
                {
                    fileName = dialog.FileName;
                    Settings.Default["Airfoil"] = fileName;

                    var settings = new JsonSerializerSettings();
                    settings.Culture = new CultureInfo( "en-GB" );
                    settings.Formatting = Formatting.Indented;

                    var data = JsonConvert.SerializeObject( Airfoil, settings );
                    File.WriteAllText( fileName, data );
                }
            }
        }

        private void FWSetupAllConfig_Click( object sender, RoutedEventArgs e )
        {
            Database database = new Database();
            database.ConnectionString = this.connectionString;

            database.SetupAllFW();
        }

        private void FWSetupConfig_Click( object sender, RoutedEventArgs e )
        {
            Database database = new Database();
            database.ConnectionString = this.connectionString;

            var model = ConfiguredModelsData.SelectedItem as Model;
            if( model != null )
            {
                database.SetupFW( model.Id );
            }
        }

        private double DegreeToRadian( double angle )
        {
            return Math.PI * angle / 180.0;
        }

        private double RadianToDegree( double angle )
        {
            return angle * ( 180.0 / Math.PI );
        }

        private void ExecuteSql_Click( object sender, RoutedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> models = new List<Attrib>();

                        var textRange =
                            new TextRange( SqlText.Document.ContentStart, SqlText.Document.ContentEnd );
                        var sql = textRange.Text;

                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var count = read.FieldCount;

                                for( int i = 0; i < count; ++i )
                                {
                                    var name = Convert.ToString( read.GetName( i ) );
                                    var value = Convert.ToString( read.GetValue( i ) );

                                    models.Add( new Attrib() { Name = name, Value = value } );
                                }
                            }

                            SqlResults.ItemsSource = models;
                            SqlTextResults.Text = "Query executed successfully.";
                        }
                    }
                }
                catch( Exception ex )
                {
                    SqlTextResults.Text = ex.Message;
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }
        }

        private void updateModelsTab()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Model> models = new List<Model>();
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        var sql = "Select id,name From configured_actors where parent_id = 1";
                        SQLiteCommand comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                string modelName =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "Name" ) ) );

                                models.Add( new Model() { Id = id, Name = modelName } );
                            }
                        }

                        modelsData.ItemsSource = models;
                        modelObjectModels.ItemsSource = models;
                        modelComponentsModels.ItemsSource = models;
                        ConfiguredModelsData.ItemsSource = models;
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
        }

        private void updateComponentsTab()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<RefComponent> models = new List<RefComponent>();
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm = new SQLiteCommand( "Select * from ref_components", con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                string modelName =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );

                                models.Add( new RefComponent() { Id = id, Title = modelName } );
                            }
                        }

                        refComponents.ItemsSource = models;
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }
        }

        private void updateComponentGroups()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> models = new List<Attrib>();
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm = new SQLiteCommand( "Select * from component_groups", con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                string modelName =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );
                                string groupType =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "group_type" ) ) );

                                models.Add(
                                    new Attrib() { Id = id, Name = modelName, GroupType = groupType } );
                            }
                        }

                        allComponentGroups.ItemsSource = models;
                        ConfiguredModelsAllGroups.ItemsSource = models;
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }
        }

        void AddResourceAttrib( int id, String name, String value )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        var attribSql =
                            "insert into ref_attribs (title,value,ref_component_id, ref_component_title) values ('" +
                            name + "', '" + value + "', '" + id + "', '')";
                        executeQuery( attribSql );
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }
        }

        void AddMesh_Click( object sender, RoutedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        var meshName = "New Mesh";
                        var sql =
                            "insert into ref_components (type,title,ref_component,enabled) values ('Default', '" +
                            meshName + "', '0','true')";
                        executeQuery( sql );
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            var id = GetNewResourceId();
            AddResourceAttrib( id, "path", "none" );

            updateComponentsTab();
        }

        private void AddMaterial_Click( object sender, RoutedEventArgs e )
        {
            int result = 0;
            int parent_id = 0;
            int max_id = 0;
            int maxLft = 0;
            int maxRght = 0;
            int ref_id = 0;

            var newObjectId = 217;  // GetNewId();
            var materialNodeId = AddChildNode( newObjectId, true, "New Material" );
            var schemeNodeId = AddChildNode( materialNodeId, false, "Scheme" );
            var passNodeId = AddChildNode( schemeNodeId, false, "Pass" );
            var textureId = AddChildNode( passNodeId, false, "Texture" );

            updateModelsTab();
        }

        private void AddTexture_Click( object sender, RoutedEventArgs e )
        {
            int result = 0;
            int parent_id = 0;
            int max_id = 0;
            int maxLft = 0;
            int maxRght = 0;
            int ref_id = 0;

            // get parent_id in tree where new model needs to be added
            String sql =
                "select id from configured_actors where lft in (SELECT lft+1 from configured_actors where name = 'Multirotors' )";

            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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
                                parent_id = read.GetInt32( read.GetOrdinal( "id" ) );
                            }
                        }

                        // get the id of the model template to be copied
                        sql = "select id from template_models where name = 'MultiRotor Name'";

                        comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                ref_id = read.GetInt32( read.GetOrdinal( "id" ) );
                            }
                        }

                        // get the new id for the model to be added
                        sql = "select max(id) as id from configured_actors";

                        comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                max_id = read.GetInt32( read.GetOrdinal( "id" ) );
                            }
                        }

                        // work out the bounds of the tree where the model is to be added
                        sql =
                            "select lft, rght from configured_actors where id = (select max(id) from configured_actors where parent_id = " +
                            parent_id + ")";
                        comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                maxLft = read.GetInt32( read.GetOrdinal( "lft" ) );
                                maxRght = read.GetInt32( read.GetOrdinal( "rght" ) );
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            // make room for new template model
            sql =
                "update configured_actors set lft = lft + (select (rght - lft) + 1 from template_models where id = " +
                ref_id + ") where lft > " + maxRght;
            result = executeNonQuery( sql );

            sql =
                "update configured_actors set rght = rght + (select (rght - lft) + 1 from template_models where id = " +
                ref_id + ") where rght > " + maxRght;
            result = executeNonQuery( sql );

            // 2. Copy the tree with offsets
            sql =
                "insert into configured_actors (parent_id, name, ref_component_id, scale, nitro, standard, lft, rght, scenenode, crud, config_type, model_image, camera_data, ref_actor_row_id, resource_name, channel ) select 0, name, ref_component_id, scale, nitro, standard, lft + " +
                maxRght + " - (select lft from template_models where id = " + ref_id + ") + 1, rght + " +
                maxRght + " - (select lft from template_models where id = " + ref_id +
                ") + 1, scenenode, crud, config_type, model_image, camera_data , ref_actor_row_id, resource_name, channel from template_models where lft between (select lft from template_models where id = " +
                ref_id + ") and (select rght from template_models where id = " + ref_id +
                ") order by lft asc ";
            result = executeNonQuery( sql );

            // 3. now update parent id's
            sql =
                "update configured_actors set parent_id = ( select cm.id from configured_actors as cm where configured_actors.lft between cm.lft and cm.rght and configured_actors.lft != cm.lft order by cm.lft desc limit 1 )";
            result = executeNonQuery( sql );

            updateModelsTab();
        }

        int GetNewResourceId()
        {
            try
            {
                using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
                {
                    con.Open();

                    if( con.State == ConnectionState.Open )
                    {
                        var max_id = 0;
                        var sql = "select max(id) as id from ref_components";

                        var comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                max_id = read.GetInt32( read.GetOrdinal( "id" ) );
                            }
                        }

                        return max_id;
                    }
                }
            }
            catch( System.Exception ex )
            {
                MessageBox.Show( ex.Message );
            }

            return 0;
        }

        int GetNewObjectId()
        {
            try
            {
                using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
                {
                    con.Open();

                    if( con.State == ConnectionState.Open )
                    {
                        var max_id = 0;
                        var sql = "select max(id) as id from configured_actors";

                        var comm = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                max_id = read.GetInt32( read.GetOrdinal( "id" ) );
                            }
                        }

                        return max_id;
                    }
                }
            }
            catch( System.Exception ex )
            {
                MessageBox.Show( ex.Message );
            }

            return 0;
        }

        int AddChildNode( int node_id, bool isRootNode, String name )
        {
            try
            {
                int left = 0;
                int result = 0;

                String sql = "select * from configured_actors where id = " + node_id;

                using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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

                if( isRootNode )
                {
                    sql = "insert into configured_actors (name, parent_id, lft, rght) values ('" + name +
                          "'," + node_id + ", " + left + "+1," + left + "+2)";
                    result = executeNonQuery( sql );
                }
                else
                {
                    sql = "insert into configured_actors (name, parent_id, lft, rght) values ('" + name +
                          "'," + node_id + ", " + left + "+1," + left + "+2)";
                    result = executeNonQuery( sql );
                }

                SetupConfigModelAttribs();
                updateModelsTab();

                return GetNewObjectId();
            }
            catch( System.Exception ex )
            {
                MessageBox.Show( ex.Message );
            }

            return 0;
        }

        private void AddChildNode_Click( object sender, RoutedEventArgs e )
        {
            ConfiguredModelRow row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;

            int node_id = row.id;
            int left = 0;
            int result = 0;

            String sql = "select * from configured_actors where id = " + node_id;

            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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

            sql = "insert into configured_actors (name, parent_id, lft, rght) values ('empty'," +
                  node_id + ", " + left + "+1," + left + "+2)";
            result = executeNonQuery( sql );

            SetupConfigModelAttribs();
        }

        private json.query executeQuery( String sql )
        {
            json.query result = new json.query();

            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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

        private int executeNonQuery( String sql )
        {
            int rowsUpdated = -1;
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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

        private void DeleteNode_Click( object sender, RoutedEventArgs e )
        {
            ConfiguredModelRow row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;

            int node_id = row.id;
            int left = 0;
            int right = 0;
            int offset = 0;
            int result = 0;

            String sql = "select * from configured_actors where id = " + node_id;

            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
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

            updateModelsTab();
            SetupConfigModelAttribs();
        }

        private void AddComponent_Click( object sender, RoutedEventArgs e )
        {
            Model model = ConfiguredModelsData.SelectedItem as Model;
            ConfiguredModelRow row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
            Attrib component = ConfiguredModelsComponents.SelectedItem as Attrib;

            var actor_id = model.Id;
            var row_id = row.id;
            var component_id = component.Id;
            var title = component.Name;
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

                SetupConfigModelAttribs();
            }
        }

        private void RemoveComponent_Click( object sender, RoutedEventArgs e )
        {
            ConfiguredModelRow row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;

            var row_id = row.id;
            //  if (row_id != null && row_id != "")
            {
                String sql =
                    "update configured_actors set name = 'Empty', ref_component_id = NULL where id = " +
                    row_id;
                int result = executeNonQuery( sql );
                if( result == -1 )
                {
                    MessageBox.Show( "Component remove failed" );
                    return;
                }
                sql = "delete from attribs where configured_actor_id = " + row_id;
                result = executeNonQuery( sql );
                if( result == -1 )
                {
                    MessageBox.Show( "Attribs delete failed" );
                    return;
                }

                SetupConfigModelAttribs();
            }
        }

        void AddGroup_Click( object sender, RoutedEventArgs e )
        {
            var row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
            var group = ConfiguredModelsAllGroups.SelectedItem as Attrib;
            if( row != null && group != null )
            {
                var row_id = row.id;
                var group_id = group.Id;

                //if (row_id != null && row_id != "" && group_id != null && group_id != "")
                {
                    String sql =
                        "insert into configured_actors_component_groups (configured_actor_id, component_group_id) values (" +
                        row_id + ", " + group_id + ")";
                    int result = executeNonQuery( sql );
                    if( result == -1 )
                    {
                        MessageBox.Show( "Group add failed" );
                    }
                }

                SetupConfigComponentGroups();
                SetupConfigComponents();
            }
        }

        void RemoveGroup_Click( object sender, RoutedEventArgs e )
        {
            // delete group button
            var row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
            var group = ConfiguredModelsGroups.SelectedItem as Attrib;
            if( row != null && group != null )
            {
                var row_id = row.id;
                var group_id = group.Id;

                //  if (row_id != null && row_id != "" && group_id != null && group_id != "")
                {
                    String sql =
                        "delete from configured_actors_component_groups where configured_actor_id = " +
                        row_id + " and  component_group_id =" + group_id;
                    int result = executeNonQuery( sql );
                    if( result == -1 )
                    {
                        MessageBox.Show( "Group delete failed" );
                    }
                }

                SetupConfigComponentGroups();
                SetupConfigComponents();
            }
        }

        private void SetComponentType_Click( object sender, RoutedEventArgs e )
        {
            ConfiguredModelRow row = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;

            var row_id = row.id;

            //if (row_id != null && row_id != "")
            //{
            //    if (componentTypes.SelectedIndex != -1)
            //    {
            //        String compType = null;
            //        compType = componentTypes.SelectedItem.ToString();

            //        String sql = "update configured_actors set config_type = '" + compType + "' where id = " + row_id;
            //        int result = executeNonQuery(sql);
            //        if (result == -1)
            //        {
            //            MessageBox.Show("Component Type update failed");
            //            return;
            //        }

            //    }
            //    else
            //    {
            //        MessageBox.Show("No component type selected");
            //        return;
            //    }
            //}
            //else
            //{
            //    MessageBox.Show("Problem with selected row");
            //    return;
            //}

            SetupConfigModelAttribs();
        }

        public enum ModelType
        {
            E_MODEL_TYPE_BASIC,
            E_MODEL_TYPE_HELICOPTER,
            E_MODEL_TYPE_PLANE,
            E_MODEL_TYPE_GLIDER,
            E_MODEL_TYPE_CAR,
            E_MODEL_TYPE_BASIC_RIG,
            E_MODEL_TYPE_MULTI_ROTOR,
            E_MODEL_TYPE_MISC,
            E_MODEL_TYPE_TRUCK,
            E_MODEL_TYPE_BOAT,
            E_MODEL_TYPE_MOTOR_CYCLE,
            E_MODEL_TYPE_UNKNOWN,
            E_MODEL_TYPE_EXTERNAL,

            E_MODEL_TYPE_COUNT
        }
        ;

        string GetModelResource( int id )
        {
            var resourceName = "";
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm =
                            new SQLiteCommand( "Select * From configured_actors where id = " + id, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                resourceName = Convert.ToString(
                                    read.GetValue( read.GetOrdinal( "resource_name" ) ) );
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

            return resourceName;
        }

        ModelType GetModelType( int id )
        {
            var parent_id = -1;
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm =
                            new SQLiteCommand( "Select * From configured_actors where id = " + id, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                parent_id =
                                    Convert.ToInt32( read.GetValue( read.GetOrdinal( "parent_id" ) ) );
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

            if( parent_id == 217 )
            {
                return ModelType.E_MODEL_TYPE_HELICOPTER;
            }
            else if( parent_id == 220 )
            {
                return ModelType.E_MODEL_TYPE_PLANE;
            }

            return ModelType.E_MODEL_TYPE_UNKNOWN;
        }

        public void CreateRigFromDB( int modelId )
        {
            string resourceName = GetModelResource( modelId );
            var modelType = GetModelType( modelId );
            switch( modelType )
            {
            case ModelType.E_MODEL_TYPE_PLANE:
            {
                var aircraftData = new json.aircraft_data();

                var modelDataSql = "select * from actor_objects where actor_id = " + modelId +
                                   "  and class = 'model_data'";
                var modelDataResult = executeQuery( modelDataSql );
                foreach( var r in modelDataResult.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "cg_position" )
                            aircraftData.cgPosition = JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "drag" )
                            aircraftData.drag = JsonUtil.StringToVector3( attribValue, ',' );

                        if( attribName == "rollwiseDamping" )
                            aircraftData.rollwiseDamping = float.Parse( attribValue );
                        if( attribName == "sectionMultiplier" )
                            aircraftData.sectionMultiplier = float.Parse( attribValue );
                    }
                }

                var propwashDataSql = "select * from actor_objects where actor_id = " + modelId +
                                      "  and class = 'prop_wash_data'";
                var propwashDataResult = executeQuery( propwashDataSql );
                foreach( var r in propwashDataResult.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var propwashData = new json.aircraft_prop_wash_data();
                    propwashData.name = r.GetFieldValue( "ident" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "propwashSource" )
                            propwashData.propwashSource = attribValue;
                        if( attribName == "strength" )
                            propwashData.strength = float.Parse( attribValue );

                        if( attribName == "affected_sections" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = val == "True" || val == "1" ? 1 : 0;
                                propwashData.affectedSections.Add( iVal );
                            }
                        }

                        if( attribName == "section_multipliers" )
                        {
                            var s = attribValue.Split( ',' );
                            propwashData.sectionMultipliers =
                                new List<float>( Array.ConvertAll( s, float.Parse ) );
                        }
                    }

                    aircraftData.propWashData.Add( propwashData );
                }

                var sql = "select * from actor_objects where actor_id = " + modelId +
                          " and class = 'wing_data'";
                var result = executeQuery( sql );

                foreach( var r in result.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var wingData = new json.aircraft_wing_data();
                    wingData.name = r.GetFieldValue( "ident" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "position" )
                            wingData.localTransform.position =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "rotation" )
                            wingData.localTransform.rotation =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "scale" )
                            wingData.localTransform.scale = JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "sections" )
                            wingData.sectionCount = int.Parse( attribValue );

                        if( attribName == "liftLineChordPosition" )
                            wingData.liftLineChordPosition = float.Parse( attribValue );
                        if( attribName == "wingTipWidthZeroToOne" )
                            wingData.wingTipWidthZeroToOne = float.Parse( attribValue );
                        if( attribName == "wingTipSweep" )
                            wingData.wingTipSweep = float.Parse( attribValue );
                        if( attribName == "wingTipAngle" )
                            wingData.wingTipAngle = float.Parse( attribValue );

                        if( attribName == "aoaMultiplier" )
                            wingData.aoaMultiplier = float.Parse( attribValue );

                        if( attribName == "cdMultiplier" )
                            wingData.cdMultiplier = float.Parse( attribValue );

                        if( attribName == "clMultiplier" )
                            wingData.clMultiplier = float.Parse( attribValue );

                        if( attribName == "cmMultiplier" )
                            wingData.cmMultiplier = float.Parse( attribValue );

                        if( attribName == "stallControlCL" )
                            wingData.stallControlCL = float.Parse( attribValue );

                        if( attribName == "stallControlCD" )
                            wingData.stallControlCD = float.Parse( attribValue );

                        if( attribName == "stallControlCM" )
                            wingData.stallControlCM = float.Parse( attribValue );

                        if( attribName == "stallThreshold" )
                            wingData.stallThreshold = float.Parse( attribValue );

                        if( attribName == "aerofoilName" )
                            wingData.aerofoilName = attribValue;
                        if( attribName == "subDivision" )
                        {
                            wingData.subDivision = JsonUtil.StringToVector3( attribValue, ',' );
                        }
                    }

                    Quaternion orientation = Quaternion.CreateFromYawPitchRoll(
                        (float)DegreeToRadian( wingData.localTransform.rotation.y ),
                        (float)DegreeToRadian( wingData.localTransform.rotation.x ),
                        (float)DegreeToRadian( wingData.localTransform.rotation.z ) );

                    wingData.localTransform.orientation.x = orientation.X;
                    wingData.localTransform.orientation.y = orientation.Y;
                    wingData.localTransform.orientation.z = orientation.Z;
                    wingData.localTransform.orientation.w = orientation.W;

                    wingData.localTransform.scale.x *= 2.0f;
                    wingData.localTransform.scale.y *= 2.0f;
                    wingData.localTransform.scale.z *= 2.0f;

                    aircraftData.wingData.Add( wingData );
                }

                var clLookup = new LinkedList<float>();
                var cdLookup = new LinkedList<float>();
                var cmLookup = new LinkedList<float>();

                var controlSurfaceSql = "select * from actor_objects where actor_id = " + modelId +
                                        "  and class = 'control_surface_data'";
                var resultControlSurface = executeQuery( controlSurfaceSql );
                foreach( var r in resultControlSurface.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var controlSurfaceData = new aircraft_control_surface_data();
                    controlSurfaceData.name = r.GetFieldValue( "ident" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "invert" )
                            controlSurfaceData.reverse = bool.Parse( attribValue );
                        if( attribName == "maxDeflectionNeg" )
                        {
                            controlSurfaceData.minDeflectionDegrees = float.Parse( attribValue );
                        }

                        if( attribName == "maxDeflectionPos" )
                        {
                            controlSurfaceData.maxDeflectionDegrees = float.Parse( attribValue );
                        }

                        if( attribName == "rootHingeDistanceFromTrailingEdge" )
                            controlSurfaceData.rootHingeDistanceFromTrailingEdge =
                                float.Parse( attribValue );
                        if( attribName == "tipHingeDistanceFromTrailingEdge" )
                            controlSurfaceData.tipHingeDistanceFromTrailingEdge =
                                float.Parse( attribValue );

                        if( attribName == "modelRotationAxis" )
                        {
                            var axis = JsonUtil.StringToVector3( attribValue, ',' );
                            controlSurfaceData.modelRotationAxis = new vec4( axis.x, axis.y, axis.z );
                        }

                        if( attribName == "surfaceId" )
                            controlSurfaceData.surfaceId = int.Parse( attribValue );
                        if( attribName == "aoaMultiplier" )
                            controlSurfaceData.aoaMultiplier = float.Parse( attribValue );

                        if( attribName == "cdMultiplier" )
                            controlSurfaceData.cdMultiplier = float.Parse( attribValue );

                        if( attribName == "clMultiplier" )
                            controlSurfaceData.clMultiplier = float.Parse( attribValue );

                        if( attribName == "cmMultiplier" )
                            controlSurfaceData.cmMultiplier = float.Parse( attribValue );

                        if( attribName == "stallControlCL" )
                            controlSurfaceData.stallControlCL = float.Parse( attribValue );

                        if( attribName == "stallControlCD" )
                            controlSurfaceData.stallControlCD = float.Parse( attribValue );

                        if( attribName == "stallControlCM" )
                            controlSurfaceData.stallControlCM = float.Parse( attribValue );

                        if( attribName == "stallThreshold" )
                            controlSurfaceData.stallThreshold = float.Parse( attribValue );

                        if( attribName == "reverse" )
                            controlSurfaceData.reverse = bool.Parse( attribValue );

                        if( attribName == "affected_sections" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = val == "True" || val == "1" ? 1 : 0;
                                controlSurfaceData.affectedSections.Add( iVal );
                            }
                        }

                        if( attribName == "clLookup" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                clLookup.AddLast( iVal );
                            }
                        }

                        if( attribName == "clLookupNegative" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                clLookup.AddFirst( iVal );
                            }
                        }

                        if( attribName == "cdLookup" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                cdLookup.AddLast( iVal );
                            }
                        }

                        if( attribName == "cdLookupNegative" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                cdLookup.AddFirst( iVal );
                            }
                        }

                        if( attribName == "cmLookup" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                cmLookup.AddLast( iVal );
                            }
                        }

                        if( attribName == "cmLookupNegative" )
                        {
                            var s = attribValue.Split( ',' );

                            for( int i = 0; i < s.Length; ++i )
                            {
                                var val = s[i];
                                var iVal = float.Parse( val );
                                cmLookup.AddFirst( iVal );
                            }
                        }
                    }

                    controlSurfaceData.clLookup.AddRange( clLookup );
                    controlSurfaceData.cdLookup.AddRange( cdLookup );
                    controlSurfaceData.cmLookup.AddRange( cmLookup );

                    aircraftData.controlSurfaceData.Add( controlSurfaceData );
                }

                var engineDataSql = "select * from actor_objects where actor_id = " + modelId +
                                    "  and class = 'engine_data'";
                var resultEngineData = executeQuery( engineDataSql );
                foreach( var r in resultEngineData.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var engineData = new aircraft_engine_data();
                    engineData.name = r.GetFieldValue( "ident" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "position" )
                            engineData.localTransform.position =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "rotation" )
                            engineData.localTransform.rotation =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "scale" )
                            engineData.localTransform.scale =
                                JsonUtil.StringToVector3( attribValue, ',' );

                        if( attribName == "thrustMultiplier" )
                            engineData.thrustMultiplier = float.Parse( attribValue );

                        if( attribName == "torqueMultiplier" )
                            engineData.torqueMultiplier = float.Parse( attribValue );
                    }

                    aircraftData.engineData.Add( engineData );
                }

                var wheelSql = "select * from actor_objects where actor_id = " + modelId +
                               "  and class = 'wheel_data'";
                var resultWheel = executeQuery( wheelSql );
                foreach( var r in resultWheel.rows )
                {
                    var id = r.GetFieldValueAsInt( "id" );

                    var wheelData = new aircraft_wheel_data();
                    wheelData.name = r.GetFieldValue( "ident" );

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = executeQuery( attributesSql );
                    foreach( var a in attributeResults.rows )
                    {
                        var attribName = a.GetFieldValue( "name" );
                        var attribValue = a.GetFieldValue( "value" );

                        if( attribName == "position" )
                            wheelData.localTransform.position =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "rotation" )
                            wheelData.localTransform.rotation =
                                JsonUtil.StringToVector3( attribValue, ',' );
                        if( attribName == "scale" )
                            wheelData.localTransform.scale =
                                JsonUtil.StringToVector3( attribValue, ',' );

                        if( attribName == "radius" )
                            wheelData.radius = float.Parse( attribValue );
                        if( attribName == "wheelDamping" )
                            wheelData.wheelDamping = float.Parse( attribValue );
                        if( attribName == "mass" )
                            wheelData.mass = float.Parse( attribValue );

                        if( attribName == "suspensionDistance" )
                            wheelData.suspensionDistance = float.Parse( attribValue );
                        if( attribName == "springRate" )
                            wheelData.springRate = float.Parse( attribValue );
                        if( attribName == "suspensionDamper" )
                            wheelData.suspensionDamper = float.Parse( attribValue );

                        if( attribName == "forwardExtremumSlip" )
                            wheelData.forwardExtremumSlip = float.Parse( attribValue );
                        if( attribName == "forwardExtrememValue" )
                            wheelData.forwardExtrememValue = float.Parse( attribValue );
                        if( attribName == "forwardAsymptoteSlip" )
                            wheelData.forwardAsymptoteSlip = float.Parse( attribValue );

                        if( attribName == "forwardAsymptoteValue" )
                            wheelData.forwardAsymptoteValue = float.Parse( attribValue );
                        if( attribName == "forwardStiffness" )
                            wheelData.forwardStiffness = float.Parse( attribValue );
                        if( attribName == "sidewaysExtremumSlip" )
                            wheelData.sidewaysExtremumSlip = float.Parse( attribValue );

                        if( attribName == "sidewaysAsymptoteSlip" )
                            wheelData.sidewaysAsymptoteSlip = float.Parse( attribValue );
                        if( attribName == "sidewaysAsymptoteValue" )
                            wheelData.sidewaysAsymptoteValue = float.Parse( attribValue );
                        if( attribName == "sidewaysStiffness" )
                            wheelData.sidewaysStiffness = float.Parse( attribValue );
                    }

                    aircraftData.wheelData.Add( wheelData );
                }

                var settings = new JsonSerializerSettings();
                settings.Culture = new CultureInfo( "en-GB" );
                settings.Formatting = Formatting.Indented;
                settings.FloatFormatHandling = FloatFormatHandling.DefaultValue;
                var jsonStr = JsonConvert.SerializeObject( aircraftData, settings );

                var fileName = Path.GetFileNameWithoutExtension( GetResourceValue( resourceName ) );
                var filePath = CachePath + "/" + fileName + ".modelrig";
                File.WriteAllText( filePath, jsonStr );
            }
            break;
            case ModelType.E_MODEL_TYPE_TRUCK:
            {
                /*
                var dynamicsGO = gameObject.FindChildByName("Dynamics");
                if (dynamicsGO == null)
                {
                    dynamicsGO = new GameObject("Dynamics");
                    dynamicsGO.transform.SetParent(gameObject.transform);
                }

                dynamicsGO.DestroyAllChildren();

                var wheelSql = "select * from actor_objects where actor_id = " + modelId +
                               "  and class = 'wheel_data'";
                var resultWheel = DatabaseManager.ExecuteQueryEditorRefDB(wheelSql);
                foreach (var r in resultWheel.rows)
                {
                    var id = r.GetFieldValueAsInt("id");

                    var wheelData = new vehicle_wheel_data();
                    wheelData.name = r.GetFieldValue("ident");

                    var position = Vector3.zero;
                    var rotation = Vector3.zero;
                    var scale = Vector3.zero;

                    var attributesSql = "select * from object_attributes where object_id = " + id;
                    var attributeResults = DatabaseManager.ExecuteQueryEditorRefDB(attributesSql);
                    foreach (var a in attributeResults.rows)
                    {
                        var attribName = a.GetFieldValue("name");
                        var attribValue = a.GetFieldValue("value");

                        if (attribName == "position") position = Util.StringToVector3(attribValue, ',');
                        if (attribName == "rotation") rotation = Util.StringToVector3(attribValue, ',');
                        if (attribName == "scale") scale = Util.StringToVector3(attribValue, ',');
                    }

                    var child = new GameObject(wheelData.name);
                    child.transform.SetParent(dynamicsGO.transform, false);

                    child.transform.localPosition = position;
                    child.transform.localRotation = Quaternion.Euler(rotation);
                    child.transform.localScale = scale;

                    wheelData.localTransform = GetTransform(position, Quaternion.Euler(rotation), scale);

                    //Util.Destroy(child);

                    m_TruckData.wheelData.Add(wheelData);
                }

                //Util.Destroy(dynamicsGO);


                var jsonStr = Serializer.Serialize(m_TruckData, true);

                var fileName = gameObject.name.Replace("(Clone)", "");
                var filePath = Application.streamingAssetsPath + "/models/" + fileName + ".modelrig";
                File.WriteAllText(filePath, jsonStr);
                */
            }
            break;
            }
        }

        void CacheModelData_Click( object sender, RoutedEventArgs e )
        {
            System.Windows.Forms.FolderBrowserDialog dialog =
                new System.Windows.Forms.FolderBrowserDialog();

            var filePath = Convert.ToString( Settings.Default["CacheModelData"] );
            if( !string.IsNullOrEmpty( filePath ) )
            {
                try
                {
                    dialog.SelectedPath = Path.GetDirectoryName( filePath );
                }
                catch( Exception ex )
                {
                    MessageBox.Show( ex.Message );
                }
            }

            if( dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK )
            {
                CachePath = dialog.SelectedPath;
                Settings.Default["CacheModelData"] = CachePath;

                List<Model> models = new List<Model>();

                using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
                {
                    try
                    {
                        con.Open();
                        if( con.State == ConnectionState.Open )
                        {
                            // models.Add(new Model() { Id = 1, Name = "John Doe" });

                            SQLiteCommand comm = new SQLiteCommand(
                                "Select * From configured_actors where standard = 1", con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    var parent_id = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "parent_id" ) ) );
                                    string modelName =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "name" ) ) );
                                    string resource_name = Convert.ToString(
                                        read.GetValue( read.GetOrdinal( "resource_name" ) ) );

                                    models.Add( new Model() { Id = id, ParentId = parent_id,
                                                              Name = modelName,
                                                              Resource = resource_name } );
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

                foreach( var m in models )
                {
                    CreateRigFromDB( m.Id );
                }
            }
        }

        string GetResourceValue( string name )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm = new SQLiteCommand(
                            "select * from resourcemap where name = '" + name + "'", con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                string modelName =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "name" ) ) );
                                return modelName;
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

            return name;
        }

        void AddTransmitter_Click( object sender, RoutedEventArgs e )
        {
            var filePath = "";
            OpenFileDialog refDBFile = new OpenFileDialog();
            //refDBFile.Filter = "DB|*.db";
            if( refDBFile.ShowDialog() == true )
            {
                filePath = refDBFile.FileName;
            }

            if( !string.IsNullOrEmpty( filePath ) )
            {
                string text = System.IO.File.ReadAllText( filePath );
                if( !string.IsNullOrEmpty( text ) )
                {
                    string txMake = System.IO.Path.GetFileNameWithoutExtension( filePath );

                    json.TransmitterProfile profile =
                        JsonConvert.DeserializeObject<json.TransmitterProfile>( text );

                    var sql = "DELETE FROM rx_map where emulation = '" + txMake + "'";
                    executeNonQuery( sql );

                    foreach( var channel in profile.functions )
                    {
                        string sReverse = channel.reverse ? "true" : "false";
                        var insertQueryStr =
                            "INSERT INTO `rx_map`(`id`,`channel`,`value`,`type`,`cmap`,`reverse`,`emulation`,`offset`,`multiplier`,`low_multiplier`,`high_multiplier` ) " +
                            "VALUES(NULL, '" + channel.channel + "', '" + channel.value + "', '" +
                            channel.type + "', '" + channel.cmap + "', '" + sReverse + "', '" + txMake +
                            "' , '" + channel.offset + "' , '" + channel.multiplier + "' , '" +
                            channel.low_multiplier + "' , '" + channel.high_multiplier + "');";

                        executeNonQuery( insertQueryStr );
                    }
                }
            }
        }

        private void EditUndo_Click( object sender, RoutedEventArgs e )
        {
        }

        private void EditRedo_Click( object sender, RoutedEventArgs e )
        {
        }

        private void EditClone_Click( object sender, RoutedEventArgs e )
        {
            var tab = this.MainTab.SelectedIndex;
            switch( tab )
            {
            case 1:
            {
                var database = new Database();
                database.ConnectionString = this.connectionString;

                var selected = this.modelObjects.SelectedItems;
                foreach( var s in selected )
                {
                    var component = s as ModelObject;
                    database.CloneComponent( component.Id );
                }

                PopulateModelObjects();
            }
            break;
            default:
            {
            }
            break;
            }
        }

        private void EditCloneObject_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.modelObjects.SelectedItems;
            foreach( var s in selected )
            {
                var component = s as ModelObject;
                database.CloneComponent( component.Id );
            }

            PopulateModelObjects();
        }

        private void EditCloneObjectAttrib_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.objectAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var component = s as Attrib;
                database.CloneObjectAttrib( component.Id );
            }

            PopulateObjectAttribs();
        }

        private void EditCut_Click( object sender, RoutedEventArgs e )
        {
        }

        private void EditCopy_Click( object sender, RoutedEventArgs e )
        {
        }

        private void EditPaste_Click( object sender, RoutedEventArgs e )
        {
        }

        private void ModelAttribsClone_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.modelAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var attrib = s as Attrib;
                database.CloneAttrib( attrib.Id );
            }

            UpdateModelAttribs();
        }

        private void ModelAttribsDelete_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.modelAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var attrib = s as Attrib;
                database.DeleteAttrib( attrib.Id );
            }

            UpdateModelAttribs();
        }

        private void RefComponentsClone_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.refComponents.SelectedItems;
            foreach( var s in selected )
            {
                var component = s as RefComponent;
                database.CloneRefComponent( component.Id );
            }

            updateComponentsTab();
            SetupRefComponents();
        }

        private void RefComponentsDelete_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.refComponents.SelectedItems;
            foreach( var s in selected )
            {
                var component = s as RefComponent;
                database.DeleteRefComponent( component.Id );
            }

            updateComponentsTab();
            SetupRefComponents();
        }

        private void RefAttribsClone_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.refAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var attrib = s as Attrib;
                database.CloneRefAttrib( attrib.Id );
            }

            SetupRefComponents();
        }

        private void RefAttribsDelete_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.refAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var attrib = s as Attrib;
                database.DeleteRefAttrib( attrib.Id );
            }

            SetupRefComponents();
        }

        private void EditDelete_Click( object sender, RoutedEventArgs e )
        {
            var tab = this.MainTab.SelectedIndex;
            switch( tab )
            {
            case 1:
            {
                var database = new Database();
                database.ConnectionString = this.connectionString;

                var selected = this.modelObjects.SelectedItems;
                foreach( var s in selected )
                {
                    var component = s as ModelObject;
                    database.DeleteComponent( component.Id );
                }

                PopulateModelObjects();
            }
            break;
            default:
            {
            }
            break;
            }
        }

        private void EditDeleteObject_Click( object sender, RoutedEventArgs e )
        {
            try
            {
                var database = new Database();
                database.ConnectionString = this.connectionString;

                var selected = this.modelObjects.SelectedItems;
                foreach( var s in selected )
                {
                    var component = s as ModelObject;
                    database.DeleteComponent( component.Id );
                }

                PopulateModelObjects();
            }
            catch( System.Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        private void EditDeleteObjectAttrib_Click( object sender, RoutedEventArgs e )
        {
            var database = new Database();
            database.ConnectionString = this.connectionString;

            var selected = this.objectAttribs.SelectedItems;
            foreach( var s in selected )
            {
                var component = s as Attrib;
                database.DeleteObjectAttrib( component.Id );
            }

            PopulateObjectAttribs();
        }

        private void Copy_Click( object sender, RoutedEventArgs e )
        {
            WingDataWindow popup = new WingDataWindow();
            popup.ShowDialog();
        }

        private void CopyWingData_Click( object sender, RoutedEventArgs e )
        {
            CopyDataWindow popup = new CopyDataWindow();
            popup.ConnectionSrc = this.connectionString;
            popup.ShowDialog();
        }

        private void CopyAllFixedWingData_Click( object sender, RoutedEventArgs e )
        {
            CopyFixedWingDataWindow popup = new CopyFixedWingDataWindow();
            popup.ConnectionSrc = this.connectionString;
            popup.ShowDialog();
        }

        private void MirrorWingData_Click( object sender, RoutedEventArgs e )
        {
            MirrorWingDataWindow mirrorPopup = new MirrorWingDataWindow();
            mirrorPopup.connectionString = this.connectionString;
            mirrorPopup.ShowDialog();
        }

        private void AddComponentGroup_Click( object sender, RoutedEventArgs e )
        {
            AddComponentGroupWindow popup = new AddComponentGroupWindow();
            popup.connectionString = this.connectionString;
            popup.ShowDialog();

            updateComponentGroups();
        }

        private void AddGroupToComponent_Click( object sender, RoutedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        RefComponent model = refComponents.SelectedItem as RefComponent;
                        Attrib groupAttrib = allComponentGroups.SelectedItem as Attrib;
                        if( model != null && groupAttrib != null )
                        {
                            var component = model.Id;
                            var group = groupAttrib.Id;

                            string sql =
                                "insert  into component_groups_ref_components (component_group_id, ref_component_id ) values (" +
                                group + "," + component + " )";
                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                            }
                        }
                    }

                    SetupRefComponentGroups();
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

        private void RemoveGroupToComponent_Click( object sender, RoutedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        RefComponent model = refComponents.SelectedItem as RefComponent;
                        Attrib groupAttrib = componentGroups.SelectedItem as Attrib;
                        if( model != null && groupAttrib != null )
                        {
                            var component = model.Id;
                            var group = groupAttrib.Id;

                            string sql =
                                "delete from component_groups_ref_components where component_group_id = " +
                                group + " and ref_component_id =" + component;

                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                            }
                        }
                    }

                    SetupRefComponentGroups();
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

        private void HelpSupport_Click( object sender, RoutedEventArgs e )
        {
            var d = new HelpSupportWindow();
            d.ShowDialog();
        }

        private void Paste_Click( object sender, RoutedEventArgs e )
        {
        }

        private void Exit_Click( object sender, RoutedEventArgs e )
        {
            Close();
        }

        private void Open_Click( object sender, RoutedEventArgs e )
        {
            try
            {
                OpenFileDialog dialog = new OpenFileDialog();
                dialog.Filter = "DB|*.db";

                var filePath = Convert.ToString( Settings.Default["FBDatabasePath"] );
                if( !string.IsNullOrEmpty( filePath ) )
                {
                    try
                    {
                        dialog.InitialDirectory = Path.GetDirectoryName( filePath );
                        dialog.FileName = filePath;
                    }
                    catch( Exception ex )
                    {
                        MessageBox.Show( ex.Message );
                    }
                }

                if( dialog.ShowDialog() == true )
                {
                    dbFileName = dialog.FileName;
                    Settings.Default["FBDatabasePath"] = dbFileName;
                }

                connectionString = @"Data Source = " + dbFileName + "; Version = 3";
                using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
                {
                    try
                    {
                        con.Open();
                        if( con.State == ConnectionState.Open )
                        {
                            updateModelsTab();
                            updateComponentsTab();
                            updateComponentGroups();
                        }
                    }
                    catch( Exception ex )
                    {
                        MessageBox.Show( ex.Message );
                    }
                }
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.Message );
            }
        }

        void SetupRefComponents()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    List<Attrib> attribs = new List<Attrib>();

                    RefComponent model = refComponents.SelectedItem as RefComponent;
                    if( model != null )
                    {
                        SQLiteCommand comm = new SQLiteCommand(
                            "Select * From ref_attribs where ref_component_id = " + model.Id, con );
                        using( SQLiteDataReader read = comm.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                var objectId = Convert.ToInt32(
                                    read.GetValue( read.GetOrdinal( "ref_component_id" ) ) );
                                string title =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );
                                string value =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "value" ) ) );

                                attribs.Add( new Attrib() { Id = id, ObjectId = objectId, Name = title,
                                                            Value = value, RefComponentId = objectId } );
                            }
                        }

                        this.refAttribs.ItemsSource = attribs;
                    }
                }

                con.Close();
            }
        }

        void SetupRefComponentGroups()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    RefComponent model = refComponents.SelectedItem as RefComponent;
                    if( model != null )
                    {
                        var attribs = new List<Attrib>();

                        string sql =
                            "Select * from component_groups where id in (Select component_group_id from component_groups_ref_components where ref_component_id = " +
                            model.Id + ")";
                        SQLiteCommand commGroup = new SQLiteCommand( sql, con );
                        using( SQLiteDataReader read = commGroup.ExecuteReader() )
                        {
                            while( read.Read() )
                            {
                                var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                string title =
                                    Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );

                                attribs.Add( new Attrib() { Id = id, Name = title } );
                            }
                        }

                        this.componentGroups.ItemsSource = attribs;
                    }
                }

                con.Close();
            }
        }

        private void RefComponents_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            SetupRefComponents();
            SetupRefComponentGroups();
        }

        private void RefAttribs_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
        }

        private void modelAttribs_CellEditEnding( object sender, DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = modelAttribs.SelectedItem as Attrib;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Ident" )
                            {
                                fieldName = "ident";
                            }
                            else if( bindingPath == "Class" )
                            {
                                fieldName = "class";
                            }
                            else if( bindingPath == "ModelId" )
                            {
                                fieldName = "actor_id";
                            }
                            else if( bindingPath == "GroupType" )
                            {
                                fieldName = "group_type";
                            }
                            else if( bindingPath == "OriginalId" )
                            {
                                fieldName = "original_id";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "title";
                            }
                            else if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else
                            {
                                fieldName = bindingPath;
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm =
                                new SQLiteCommand( "UPDATE `attribs` SET `" + fieldName + "`= '" +
                                                       el.Text + "' WHERE `id`= " + attrib.Id + ";",
                                                   con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void RefAttribs_CellEditEnding( object sender, DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = refAttribs.SelectedItem as Attrib;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "title";
                            }
                            else if( bindingPath == "RefComponentId" )
                            {
                                fieldName = "ref_component_id";
                            }
                            else if( bindingPath == "Id" )
                            {
                                //fieldName = "id";
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm =
                                new SQLiteCommand( "UPDATE `ref_attribs` SET `" + fieldName + "`= '" +
                                                       el.Text + "' WHERE `id`= " + attrib.Id + ";",
                                                   con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjectsAttribs_CellEditEnding( object sender,
                                                         DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = objectAttribs.SelectedItem as Attrib;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "name";
                            }
                            else if( bindingPath == "ObjectId" )
                            {
                                fieldName = "object_id";
                            }
                            else if( bindingPath == "Id" )
                            {
                                //fieldName = "id";
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm = new SQLiteCommand(
                                "UPDATE `object_attributes` SET `" + fieldName + "`= '" + el.Text +
                                    "' WHERE `id`= " + attrib.Id + ";",
                                con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjectsModels_CellEditEnding( object sender,
                                                        DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = objectAttribs.SelectedItem as Attrib;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "name";
                            }
                            else if( bindingPath == "ObjectId" )
                            {
                                fieldName = "object_id";
                            }
                            else if( bindingPath == "Id" )
                            {
                                //fieldName = "id";
                            }

                            //int rowIndex = e.Row.GetIndex();
                            //var el = e.EditingElement as TextBox;

                            //SQLiteCommand comm =
                            //    new SQLiteCommand(
                            //        "UPDATE `object_attributes` SET `" + fieldName + "`= '" + el.Text +
                            //        "' WHERE `id`= " + attrib.Id + ";", con);
                            //using (var writer = comm.ExecuteReader())
                            //{

                            //}
                        }
                    }
                }

                con.Close();
            }
        }

        private void ConfiguredModelsAttribs_CellEditEnding( object sender,
                                                             DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Ident" )
                            {
                                fieldName = "ident";
                            }
                            else if( bindingPath == "Class" )
                            {
                                fieldName = "class";
                            }
                            else if( bindingPath == "ModelId" )
                            {
                                fieldName = "actor_id";
                            }
                            else if( bindingPath == "GroupType" )
                            {
                                fieldName = "group_type";
                            }
                            else if( bindingPath == "OriginalId" )
                            {
                                fieldName = "original_id";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "title";
                            }
                            else if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else
                            {
                                fieldName = bindingPath;
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm = new SQLiteCommand(
                                "UPDATE `configured_actors` SET `" + fieldName + "`= '" + el.Text +
                                    "' WHERE `id`= " + attrib.id + ";",
                                con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelConfigAttribs_CellEditEnding( object sender,
                                                        DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = modelConfigAttribs.SelectedItem as Attrib;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Ident" )
                            {
                                fieldName = "ident";
                            }
                            else if( bindingPath == "Class" )
                            {
                                fieldName = "class";
                            }
                            else if( bindingPath == "ModelId" )
                            {
                                fieldName = "actor_id";
                            }
                            else if( bindingPath == "GroupType" )
                            {
                                fieldName = "group_type";
                            }
                            else if( bindingPath == "OriginalId" )
                            {
                                fieldName = "original_id";
                            }
                            else if( bindingPath == "Name" )
                            {
                                fieldName = "title";
                            }
                            else if( bindingPath == "Value" )
                            {
                                fieldName = "value";
                            }
                            else
                            {
                                fieldName = bindingPath;
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm =
                                new SQLiteCommand( "UPDATE `attribs` SET `" + fieldName + "`= '" +
                                                       el.Text + "' WHERE `id`= " + attrib.Id + ";",
                                                   con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelConfigComponents_CellEditEnding( object sender,
                                                           DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = modelObjects.SelectedItem as ModelObject;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Ident" )
                            {
                                fieldName = "ident";
                            }
                            else if( bindingPath == "Class" )
                            {
                                fieldName = "class";
                            }
                            else if( bindingPath == "ModelId" )
                            {
                                fieldName = "actor_id";
                            }
                            else if( bindingPath == "GroupType" )
                            {
                                fieldName = "group_type";
                            }
                            else if( bindingPath == "OriginalId" )
                            {
                                fieldName = "original_id";
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm =
                                new SQLiteCommand( "UPDATE `actor_objects` SET `" + fieldName + "`= '" +
                                                       el.Text + "' WHERE `id`= " + attrib.Id + ";",
                                                   con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjects_CellEditEnding( object sender, DataGridCellEditEndingEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    var attrib = modelObjects.SelectedItem as ModelObject;
                    if( attrib != null )
                    {
                        var column = e.Column as DataGridBoundColumn;
                        if( column != null )
                        {
                            var fieldName = "";

                            var bindingPath = ( column.Binding as Binding ).Path.Path;
                            if( bindingPath == "Ident" )
                            {
                                fieldName = "ident";
                            }
                            else if( bindingPath == "Class" )
                            {
                                fieldName = "class";
                            }
                            else if( bindingPath == "ModelId" )
                            {
                                fieldName = "actor_id";
                            }
                            else if( bindingPath == "GroupType" )
                            {
                                fieldName = "group_type";
                            }
                            else if( bindingPath == "OriginalId" )
                            {
                                fieldName = "original_id";
                            }

                            int rowIndex = e.Row.GetIndex();
                            var el = e.EditingElement as TextBox;

                            SQLiteCommand comm =
                                new SQLiteCommand( "UPDATE `actor_objects` SET `" + fieldName + "`= '" +
                                                       el.Text + "' WHERE `id`= " + attrib.Id + ";",
                                                   con );
                            using( var writer = comm.ExecuteReader() )
                            {
                            }
                        }
                    }
                }

                con.Close();
            }
        }

        private void PopulateObjectAttribs()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    List<Attrib> attribs = new List<Attrib>();

                    //if (e.Source != null)
                    {
                        ModelObject model = modelObjects.SelectedItem as ModelObject;
                        if( model != null )
                        {
                            SQLiteCommand comm = new SQLiteCommand(
                                "Select * From object_attributes where object_id = " + model.Id, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    var objectId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "object_id" ) ) );
                                    string title =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "name" ) ) );
                                    string value =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "value" ) ) );

                                    attribs.Add( new Attrib() { Id = id, ObjectId = objectId,
                                                                Name = title, Value = value } );
                                }
                            }

                            this.objectAttribs.ItemsSource = attribs;
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjects_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            PopulateObjectAttribs();
        }

        private void PopulateModelObjects()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    List<ModelObject> attribs = new List<ModelObject>();

                    //if (e.Source != null)
                    {
                        Model model = modelObjectModels.SelectedItem as Model;
                        if( model != null )
                        {
                            SQLiteCommand comm = new SQLiteCommand(
                                "Select * From actor_objects where actor_id = " + model.Id, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var original_id = read.GetValue( read.GetOrdinal( "original_id" ) );

                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    var modelId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "actor_id" ) ) );
                                    var className =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "class" ) ) );
                                    var ident =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "ident" ) ) );
                                    var groupType = Convert.ToString(
                                        read.GetValue( read.GetOrdinal( "group_type" ) ) );
                                    var originalId =
                                        0;  // original_id != null ? Convert.ToInt32(original_id) : 0;

                                    attribs.Add( new ModelObject() { Id = id, ModelId = modelId,
                                                                     Class = className, Ident = ident,
                                                                     GroupType = groupType,
                                                                     OriginalId = originalId } );
                                }
                            }

                            this.modelObjects.ItemsSource = attribs;
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjectsModels_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            if( e.Source != null )
            {
                PopulateModelObjects();
            }
        }

        private void UpdateModelAttribs()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                con.Open();
                if( con.State == ConnectionState.Open )
                {
                    List<Attrib> attribs = new List<Attrib>();

                    //if (e.Source != null)
                    {
                        Model model = modelsData.SelectedItem as Model;
                        if( model != null )
                        {
                            SQLiteCommand comm = new SQLiteCommand(
                                "Select * From attribs where configured_actor_id = " + model.Id, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    string title =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );
                                    string value =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "value" ) ) );
                                    var refComponentId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "ref_component_id" ) ) );
                                    var modelId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "actor_id" ) ) );
                                    var configuredModelId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "configured_actor_id" ) ) );
                                    var applied = Convert.ToBoolean(
                                        read.GetValue( read.GetOrdinal( "applied" ) ) );

                                    attribs.Add( new Attrib() { Id = id, Name = title, Value = value,
                                                                RefComponentId = refComponentId,
                                                                ModelId = modelId,
                                                                ConfiguredModelId = configuredModelId,
                                                                Applied = applied } );
                                }
                            }

                            this.modelAttribs.ItemsSource = attribs;
                        }
                    }
                }
            }
        }

        private void ModelsData_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            UpdateModelAttribs();
        }

        void SetupConfigComponentGroups()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> attribs = new List<Attrib>();

                        ConfiguredModelRow model =
                            ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
                        if( model != null )
                        {
                            string sql =
                                "select id,title from component_groups where id in (select component_group_id from configured_actors_component_groups where configured_actor_id = " +
                                model.id + ")";

                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    string modelName =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );

                                    attribs.Add( new Attrib() { Id = id, Name = modelName } );
                                }
                            }

                            ConfiguredModelsGroups.ItemsSource = attribs;
                        }
                    }
                }
                catch( Exception ex )
                {
                    con.Close();
                    MessageBox.Show( ex.Message );
                }
            }
        }

        void SetupConfigComponents()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> attribs = new List<Attrib>();

                        ConfiguredModelRow model =
                            ConfiguredModelsModelAttribs.SelectedItem as ConfiguredModelRow;
                        if( model != null )
                        {
                            string sql =
                                "select id, title from ref_components where id in (select ref_component_id from  component_groups_ref_components where component_group_id in (select component_group_id from configured_actors_component_groups where configured_actor_id = (select parent_id from configured_actors where id = " +
                                model.id + ")))";

                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    string modelName =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );

                                    attribs.Add( new Attrib() { Id = id, Name = modelName } );
                                }
                            }

                            ConfiguredModelsComponents.ItemsSource = attribs;
                        }
                    }
                }
                catch( Exception ex )
                {
                    con.Close();
                    MessageBox.Show( ex.Message );
                }
            }
        }

        private void ConfiguredModelsAttribs_SelectionChanged( object sender,
                                                               SelectionChangedEventArgs e )
        {
            SetupConfigComponentGroups();
            SetupConfigComponents();
        }

        private void TransmittersAttribs_CellEditEnding( object sender,
                                                         DataGridCellEditEndingEventArgs e )
        {
        }

        private void ModelConfigComponents_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> attribs = new List<Attrib>();

                        Attrib model = modelConfigComponents.SelectedItem as Attrib;
                        if( model != null )
                        {
                            // string sql = "select id,title from component_groups where id in (select component_group_id from configured_actors_component_groups where configured_actor_id = " + model.Id + ")";
                            string sql = "select * from attribs where configured_actor_id = " + model.Id;

                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    string modelName =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "title" ) ) );
                                    string value =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "value" ) ) );
                                    var modelId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "actor_id" ) ) );
                                    var configuredModelId = Convert.ToInt32(
                                        read.GetValue( read.GetOrdinal( "configured_actor_id" ) ) );
                                    bool applied = Convert.ToBoolean(
                                        read.GetValue( read.GetOrdinal( "applied" ) ) );

                                    attribs.Add( new Attrib() { Id = id, Name = modelName, Value = value,
                                                                ModelId = modelId,
                                                                ConfiguredModelId = configuredModelId,
                                                                Applied = applied } );
                                }
                            }

                            modelConfigAttribs.ItemsSource = attribs;
                        }
                    }
                }
                catch( Exception ex )
                {
                    con.Close();
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }
        }

        private void ModelComponentsModels_SelectionChanged( object sender, SelectionChangedEventArgs e )
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<Attrib> attribs = new List<Attrib>();

                        Model model = modelComponentsModels.SelectedItem as Model;
                        if( model != null )
                        {
                            // string sql = "select id,title from component_groups where id in (select component_group_id from configured_actors_component_groups where configured_actor_id = " + model.Id + ")";
                            string sql =
                                "select distinct id,name from configured_actors as cm where cm.lft between (select lft from configured_actors where id =  " +
                                model.Id +
                                ") and ( select rght from configured_actors where id = " + model.Id +
                                ") and ref_component_id not null order by cm.lft asc";

                            SQLiteCommand comm = new SQLiteCommand( sql, con );
                            using( SQLiteDataReader read = comm.ExecuteReader() )
                            {
                                while( read.Read() )
                                {
                                    var id = Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );
                                    string modelName =
                                        Convert.ToString( read.GetValue( read.GetOrdinal( "name" ) ) );

                                    attribs.Add( new Attrib() { Id = id, Name = modelName } );
                                }
                            }

                            modelConfigComponents.ItemsSource = attribs;
                        }
                    }
                }
                catch( Exception ex )
                {
                    con.Close();
                    MessageBox.Show( ex.Message );
                }
                finally
                {
                    con.Close();
                }
            }
        }

        void SetupConfigModelAttribs()
        {
            using( SQLiteConnection con = new SQLiteConnection( connectionString ) )
            {
                try
                {
                    con.Open();
                    if( con.State == ConnectionState.Open )
                    {
                        List<ConfiguredModelRow> attribs = new List<ConfiguredModelRow>();

                        //if (e.Source != null)
                        {
                            Model model = ConfiguredModelsData.SelectedItem as Model;
                            if( model != null )
                            {
                                string sql =
                                    "select * from configured_actors where lft between (select lft from configured_actors where id = " +
                                    model.Id +
                                    ") and (select rght from configured_actors where id = " + model.Id +
                                    ") order by lft asc";

                                SQLiteCommand comm = new SQLiteCommand( sql, con );
                                using( SQLiteDataReader read = comm.ExecuteReader() )
                                {
                                    while( read.Read() )
                                    {
                                        var id =
                                            Convert.ToInt32( read.GetValue( read.GetOrdinal( "id" ) ) );

                                        
                                        var parentIdIndex = read.GetOrdinal( "parent_id" );
                                        var parent_id = !read.IsDBNull(parentIdIndex) ? read.GetInt32(parentIdIndex) : 0;

                                        string name = Convert.ToString(
                                            read.GetValue( read.GetOrdinal( "name" ) ) );
                                        var lft =
                                            Convert.ToInt32( read.GetValue( read.GetOrdinal( "lft" ) ) );
                                        var rgt = Convert.ToInt32(
                                            read.GetValue( read.GetOrdinal( "rght" ) ) );

                                        var ref_component_idObject = read.GetOrdinal( "ref_component_id" );
                                        var ref_component_id = !read.IsDBNull(ref_component_idObject) ? read.GetInt32(ref_component_idObject) : 0;


                                        var dataObject = read.GetOrdinal( "data" );
                                        string data = Convert.ToString( !read.IsDBNull(dataObject) ? read.GetString(dataObject) : "" );

                                        var typeObject = read.GetOrdinal( "type" );
                                        string type = Convert.ToString( !read.IsDBNull(typeObject) ? read.GetString(typeObject) : "" );

                                        var indexObject = read.GetOrdinal( "index" );
                                        var channel = !read.IsDBNull(indexObject) ? read.GetInt32(indexObject) : 0;

                                        attribs.Add( new ConfiguredModelRow() {
                                            id = id, parent_id = parent_id, name = name, ref_component_id = ref_component_id, lft = lft,
                                            rgt = rgt, data = data, type = type, index = channel
                                        } );
                                    }
                                }

                                this.ConfiguredModelsModelAttribs.ItemsSource = attribs;
                            }
                        }
                    }
                }
                catch( Exception ex )
                {
                    con.Close();
                    MessageBox.Show( ex.Message );
                }
            }
        }

        private void ConfiguredModelsModelsData_SelectionChanged( object sender,
                                                                  SelectionChangedEventArgs e )
        {
            SetupConfigModelAttribs();
        }
    }
}
