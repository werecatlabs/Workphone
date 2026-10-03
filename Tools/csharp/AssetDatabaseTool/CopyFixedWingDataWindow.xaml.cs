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
using System.Windows.Shapes;
using System.Data;
using System.Data.SQLite;
using System.IO;
using Microsoft.Win32;

namespace fb
{
    /// <summary>
    /// Interaction logic for CopyWingDataWindow.xaml
    /// </summary>
    public partial class CopyFixedWingDataWindow : Window
    {

        public string ConnectionSrc { get; set; }
        public string ConnectionDst { get; set; }

        //public int ModelSourceId { get; set; }
        //public int ModelDestinationId { get; set; }

        //public int ModelObjectSourceId { get; set; }
        // public int ModelObjectDestinationId { get; set; }

        public Model ModelSource { get; set; }
        public Model ModelDestination { get; set; }

        //public ModelObject ModelObjectSource { get; set; }
        //public ModelObject ModelObjectDestination { get; set; }



        public CopyFixedWingDataWindow()
        {
            InitializeComponent();
        }



        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(ConnectionDst))
            {
                ConnectionDst = ConnectionSrc;
            }

            ModelBrowserWindow popup = new ModelBrowserWindow();
            popup.connectionString = this.ConnectionSrc;
            popup.ModelObjectType = "wing_data";
            popup.SetupModels();
            popup.ShowDialog();

            ModelSource = popup.SelectedModel;

            if (ModelSource != null)
            {
                SourceText.Text = ModelSource.Name;
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(ConnectionDst))
            {
                ConnectionDst = ConnectionSrc;
            }

            ModelBrowserWindow popup = new ModelBrowserWindow();
            popup.connectionString = this.ConnectionDst;
            popup.ModelObjectType = "wing_data";
            popup.SetupModels();
            popup.ShowDialog();

            ModelDestination = popup.SelectedModel;

            if (ModelDestination != null)
            {
                DestinationText.Text = ModelDestination.Name;
            }
        }

        void CopyModelObjects()
        {
            if (string.IsNullOrEmpty(ConnectionDst))
            {
                ConnectionDst = ConnectionSrc;
            }

            List<string> classNames = new List<string>();

            if (WingShapeData_Checkbox.IsChecked == true)
            {
                classNames.Add("wing_data");
            }

            if (WheelData_Checkbox.IsChecked == true)
            {
                classNames.Add("wheel_data");
            }

            if (Engine_Checkbox.IsChecked == true)
            {
                classNames.Add("engine_data");
            }

            if (Propwash_Checkbox.IsChecked == true)
            {
                classNames.Add("prop_wash_data");
            }

            if (Model_Checkbox.IsChecked == true)
            {
                classNames.Add("model_data");
                classNames.Add("car_data");
                classNames.Add("truck_data");
                classNames.Add("plane_data");
            }

            if (ControlSurfaceProperties_Checkbox.IsChecked == true)
            {
                classNames.Add("control_surface_data");
                classNames.Add("controlsurface");
            }

            foreach (var currentClass in classNames)
            {
                List<ModelObject> srcAttribs = new List<ModelObject>();
                List<ModelObject> dstAttribs = new List<ModelObject>();
                List<ModelObject> newAttribs = new List<ModelObject>();

                using (SQLiteConnection con = new SQLiteConnection(ConnectionSrc))
                {
                    try
                    {
                        con.Open();
                        if (con.State == ConnectionState.Open)
                        {
                            // foreach (var currentClass in classNames)
                            {
                                var sql = "Select * From actor_objects where class = '" + currentClass +
                                          "' and actor_id = '" + ModelSource.Id + "'";
                                SQLiteCommand comm = new SQLiteCommand(sql, con);
                                using (SQLiteDataReader read = comm.ExecuteReader())
                                {
                                    while (read.Read())
                                    {
                                        var original_id = read.GetValue(read.GetOrdinal("original_id"));

                                        var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                        var modelId = Convert.ToInt32(read.GetValue(read.GetOrdinal("actor_id")));
                                        var className = Convert.ToString(read.GetValue(read.GetOrdinal("class")));
                                        var ident = Convert.ToString(read.GetValue(read.GetOrdinal("ident")));
                                        var groupType = Convert.ToString(read.GetValue(read.GetOrdinal("group_type")));
                                        var originalId = 0; // original_id != null ? Convert.ToInt32(original_id) : 0;

                                            srcAttribs.Add(new ModelObject()
                                            {
                                                Id = id,
                                                ModelId = modelId,
                                                Class = className,
                                                Ident = ident,
                                                GroupType = groupType,
                                                OriginalId = originalId
                                            });
                                    }
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                    finally
                    {
                        con.Close();
                    }
                }


                using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
                {
                    try
                    {
                        con.Open();
                        if (con.State == ConnectionState.Open)
                        {
                            // foreach (var currentClass in classNames)
                            {
                                SQLiteCommand comm2 = new SQLiteCommand(
                                    "Select * From actor_objects where class = '" + currentClass +
                                    "' and actor_id = '" +
                                    ModelDestination.Id + "'", con);
                                using (SQLiteDataReader read = comm2.ExecuteReader())
                                {
                                    while (read.Read())
                                    {
                                        var original_id = read.GetValue(read.GetOrdinal("original_id"));

                                        var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                        var modelId = Convert.ToInt32(read.GetValue(read.GetOrdinal("actor_id")));
                                        var className = Convert.ToString(read.GetValue(read.GetOrdinal("class")));
                                        var ident = Convert.ToString(read.GetValue(read.GetOrdinal("ident")));
                                        var groupType = Convert.ToString(read.GetValue(read.GetOrdinal("group_type")));
                                        var originalId = 0; // original_id != null ? Convert.ToInt32(original_id) : 0;

                                        dstAttribs.Add(new ModelObject()
                                        {
                                            Id = id,
                                            ModelId = modelId,
                                            Class = className,
                                            Ident = ident,
                                            GroupType = groupType,
                                            OriginalId = originalId
                                        });
                                    }
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                    finally
                    {
                        con.Close();
                    }
                }

                using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
                {
                    try
                    {
                        con.Open();
                        if (con.State == ConnectionState.Open)
                        {
                            foreach (var srcAttrib in srcAttribs)
                            {
                                bool added = false;
                                foreach (var dstAttrib in dstAttribs)
                                {
                                    if (srcAttrib.Ident == dstAttrib.Ident)
                                    {
                                        CopyObjectAttribs(srcAttrib, dstAttrib);
                                        added = true;
                                    }
                                }

                                if (added == false)
                                {
                                    string insertModelObjectSql =
                                        "INSERT INTO `actor_objects` VALUES(NULL, " + ModelDestination.Id + ", '" +
                                        srcAttrib.Class +
                                        "', '" + srcAttrib.Ident + "', 'Control', NULL);";
                                    SQLiteCommand commInsert = new SQLiteCommand(insertModelObjectSql, con);
                                    using (SQLiteDataReader read = commInsert.ExecuteReader())
                                    {
                                        while (read.Read())
                                        {
                                        }
                                    }

                                    int newObjectId = 0;
                                    SQLiteCommand commMaxIdSql =
                                        new SQLiteCommand("select max(id) from actor_objects", con);
                                    using (SQLiteDataReader read = commMaxIdSql.ExecuteReader())
                                    {
                                        while (read.Read())
                                        {
                                            newObjectId = Convert.ToInt32(read.GetValue(0));
                                        }
                                    }

                                    CopyObjectAttribs(srcAttrib.Id, newObjectId);
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                    finally
                    {
                        con.Close();
                    }
                }
            }


        }

        void CopyObjectAttribs(ModelObject source, ModelObject destination)
        {
            List<Attrib> srcAttribs = new List<Attrib>();
            List<Attrib> dstAttribs = new List<Attrib>();
            List<Attrib> newAttribs = new List<Attrib>();

            List<string> aeroProperties = new List<string>();
            aeroProperties.Add("aoaMultiplier");
            aeroProperties.Add("cdMultiplier");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("cmMultiplier");
            aeroProperties.Add("stallControlCL");
            aeroProperties.Add("stallControlCD");
            aeroProperties.Add("stallControlCM");
            aeroProperties.Add("stallThreshold");

            aeroProperties.Add("strength");
            aeroProperties.Add("affected_sections");
            aeroProperties.Add("section_multipliers");

            aeroProperties.Add("aoaMultiplier");
            aeroProperties.Add("affected_sections");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("cmMultiplier");

            aeroProperties.Add("clLookup");
            aeroProperties.Add("cdLookup");
            aeroProperties.Add("cmLookup");

            aeroProperties.Add("clLookupNegative");
            aeroProperties.Add("cdLookupNegative");
            aeroProperties.Add("cmLookupNegative");

            using (SQLiteConnection con = new SQLiteConnection(ConnectionSrc))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand comm =
                            new SQLiteCommand("Select * From object_attributes where object_id = " + source.Id, con);
                        using (SQLiteDataReader read = comm.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

                                
                                if ((source.Class == "control_surface_data" || source.Class == "controlsurface") && ControlSurfaceProperties_Checkbox.IsChecked == true)
                                {
                                    srcAttribs.Add(new Attrib()
                                    {
                                        Id = id,
                                        ObjectId = objectId,
                                        Name = title,
                                        Value = value
                                    });
                                }
                                else if (source.Class == "wheel_data" && WheelData_Checkbox.IsChecked == true)
                                {
                                    if (WheelPositionData_Checkbox.IsChecked == true)
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                    else if (title != "position")
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                }
                                else if (WingShapeData_Checkbox.IsChecked == true &&
                                    WingAerodynamicProperties_Checkbox.IsChecked == true)
                                {
                                    srcAttribs.Add(new Attrib()
                                    {
                                        Id = id,
                                        ObjectId = objectId,
                                        Name = title,
                                        Value = value
                                    });
                                }
                                else if (WingShapeData_Checkbox.IsChecked == true)
                                {
                                    if (title == "position" || title == "rotation" || title == "scale" ||
                                        title == "wingTipSweep" || title == "wingTipAngle" ||
                                        title == "liftLineChordPosition" || title == "wingTipWidthZeroToOne")
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                }
                                else if (WingAerodynamicProperties_Checkbox.IsChecked == true)
                                {
                                    if (aeroProperties.Contains(title))
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand commDst =
                            new SQLiteCommand("Select * From object_attributes where object_id = " + destination.Id,
                                con);
                        using (SQLiteDataReader read = commDst.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

                                dstAttribs.Add(new Attrib()
                                {
                                    Id = id,
                                    ObjectId = objectId,
                                    Name = title,
                                    Value = value
                                });
                            }
                        }


                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    foreach (var srcAttrib in srcAttribs)
                    {
                        bool added = false;
                        foreach (var dstAttrib in dstAttribs)
                        {
                            if (srcAttrib.Name == dstAttrib.Name)
                            {
                                newAttribs.Add(new Attrib()
                                {
                                    Id = dstAttrib.Id,
                                    ObjectId = dstAttrib.ObjectId,
                                    Name = dstAttrib.Name,
                                    Value = srcAttrib.Value
                                });

                                added = true;
                            }
                        }

                        if (added == false)
                        {
                            newAttribs.Add(new Attrib()
                            {
                                Id = -1,
                                ObjectId = destination.Id,
                                Name = srcAttrib.Name,
                                Value = srcAttrib.Value
                            });
                        }
                    }

                    con.Open();
                    foreach (var attrib in newAttribs)
                    {
                        var fieldName = "value";
                        //SQLiteCommand comm =
                        //    new SQLiteCommand(
                        //        "UPDATE `object_attributes` SET `" + fieldName + "`= '" + attrib.Value +
                        //        "' WHERE `id`= " + attrib.Id + ";", con);

                        var id = attrib.Id != -1 ? attrib.Id.ToString() : "NULL";
                        if (id == "NULL")
                        {
                            int stop = 0;
                            stop = 0;
                        }


                        string sql = "insert or replace into `object_attributes`(id, object_id, name, value) values(" +
                                     id + "," +
                                     attrib.ObjectId + ",'" +
                                     attrib.Name + "','" +
                                     attrib.Value + "');";
                        SQLiteCommand commInsert =
                            new SQLiteCommand(sql, con);

                        using (SQLiteDataReader read = commInsert.ExecuteReader())
                        {
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }
        }

        void CopyObjectAttribs(int sourceId, int destinationId)
        {
            List<Attrib> srcAttribs = new List<Attrib>();
            List<Attrib> dstAttribs = new List<Attrib>();
            List<Attrib> newAttribs = new List<Attrib>();

            List<string> aeroProperties = new List<string>();
            aeroProperties.Add("aoaMultiplier");
            aeroProperties.Add("cdMultiplier");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("cmMultiplier");
            aeroProperties.Add("stallControlCL");
            aeroProperties.Add("stallControlCD");
            aeroProperties.Add("stallControlCM");
            aeroProperties.Add("stallThreshold");

            aeroProperties.Add("strength");
            aeroProperties.Add("affected_sections");
            aeroProperties.Add("section_multipliers");

            aeroProperties.Add("aoaMultiplier");
            aeroProperties.Add("affected_sections");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("clMultiplier");
            aeroProperties.Add("cmMultiplier");

            aeroProperties.Add("clLookup");
            aeroProperties.Add("cdLookup");
            aeroProperties.Add("cmLookup");

            aeroProperties.Add("clLookupNegative");
            aeroProperties.Add("cdLookupNegative");
            aeroProperties.Add("cmLookupNegative");

            using (SQLiteConnection con = new SQLiteConnection(ConnectionSrc))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand comm =
                            new SQLiteCommand("Select * From object_attributes where object_id = " + sourceId, con);
                        using (SQLiteDataReader read = comm.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

                                if (WingShapeData_Checkbox.IsChecked == true &&
                                    WingAerodynamicProperties_Checkbox.IsChecked == true)
                                {
                                    srcAttribs.Add(new Attrib()
                                    {
                                        Id = id,
                                        ObjectId = objectId,
                                        Name = title,
                                        Value = value
                                    });
                                }
                                else if (WingShapeData_Checkbox.IsChecked == true)
                                {
                                    if (title == "position" || title == "rotation" || title == "scale" ||
                                        title == "wingTipSweep" || title == "wingTipAngle" ||
                                        title == "liftLineChordPosition" || title == "wingTipWidthZeroToOne")
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                }
                                else if (WingAerodynamicProperties_Checkbox.IsChecked == true)
                                {
                                    if (aeroProperties.Contains(title))
                                    {
                                        srcAttribs.Add(new Attrib()
                                        {
                                            Id = id,
                                            ObjectId = objectId,
                                            Name = title,
                                            Value = value
                                        });
                                    }
                                }
                                else
                                {
                                    srcAttribs.Add(new Attrib()
                                    {
                                        Id = id,
                                        ObjectId = objectId,
                                        Name = title,
                                        Value = value
                                    });
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand commDst =
                            new SQLiteCommand("Select * From object_attributes where object_id = " + destinationId,
                                con);
                        using (SQLiteDataReader read = commDst.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

                                dstAttribs.Add(new Attrib()
                                {
                                    Id = id,
                                    ObjectId = objectId,
                                    Name = title,
                                    Value = value
                                });
                            }
                        }


                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    foreach (var srcAttrib in srcAttribs)
                    {
                        bool added = false;
                        foreach (var dstAttrib in dstAttribs)
                        {
                            if (srcAttrib.Name == dstAttrib.Name)
                            {
                                newAttribs.Add(new Attrib()
                                {
                                    Id = dstAttrib.Id,
                                    ObjectId = dstAttrib.ObjectId,
                                    Name = dstAttrib.Name,
                                    Value = srcAttrib.Value
                                });

                                added = true;
                            }
                        }

                        if (added == false)
                        {
                            newAttribs.Add(new Attrib()
                            {
                                Id = -1,
                                ObjectId = destinationId,
                                Name = srcAttrib.Name,
                                Value = srcAttrib.Value
                            });
                        }
                    }

                    con.Open();
                    foreach (var attrib in newAttribs)
                    {
                        var fieldName = "value";
                        //SQLiteCommand comm =
                        //    new SQLiteCommand(
                        //        "UPDATE `object_attributes` SET `" + fieldName + "`= '" + attrib.Value +
                        //        "' WHERE `id`= " + attrib.Id + ";", con);

                        var id = attrib.Id != -1 ? attrib.Id.ToString() : "NULL";
                        if (id == "NULL")
                        {
                            int stop = 0;
                            stop = 0;
                        }


                        string sql = "insert or replace into `object_attributes`(id, object_id, name, value) values(" +
                                     id + "," +
                                     attrib.ObjectId + ",'" +
                                     attrib.Name + "','" +
                                     attrib.Value + "');";
                        SQLiteCommand commInsert =
                            new SQLiteCommand(sql, con);

                        using (SQLiteDataReader read = commInsert.ExecuteReader())
                        {
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }
        }

        void CopyModelAttribs()
        {

        }
        
        void CopyAttribs(int sourceId, int destinationId)
        {
            List<Attrib> srcAttribs = new List<Attrib>();
            List<Attrib> dstAttribs = new List<Attrib>();
            List<Attrib> newAttribs = new List<Attrib>();

            using (SQLiteConnection con = new SQLiteConnection(ConnectionSrc))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand comm =
                            new SQLiteCommand("Select * From attribs where actor_id = " + sourceId, con);
                        using (SQLiteDataReader read = comm.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("title")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));
                                var configured_actor_id = Convert.ToInt32(read.GetValue(read.GetOrdinal("configured_actor_id")));

                                srcAttribs.Add(new Attrib()
                                {
                                    Id = id,
                                    Name = title,
                                    Value = value,
                                    ConfiguredModelId = configured_actor_id
                                });
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        SQLiteCommand commDst =
                            new SQLiteCommand("Select * From attribs where actor_id = " + destinationId,
                                con);
                        using (SQLiteDataReader read = commDst.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                string title = Convert.ToString(read.GetValue(read.GetOrdinal("title")));
                                string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));
                                var configured_actor_id = Convert.ToInt32(read.GetValue(read.GetOrdinal("configured_actor_id")));

                                dstAttribs.Add(new Attrib()
                                {
                                    Id = id,
                                    Name = title,
                                    Value = value,
                                    ConfiguredModelId = configured_actor_id
                                });
                            }
                        }


                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }

            using (SQLiteConnection con = new SQLiteConnection(ConnectionDst))
            {
                try
                {
                    foreach (var srcAttrib in srcAttribs)
                    {
                        bool added = false;
                        foreach (var dstAttrib in dstAttribs)
                        {
                            if (srcAttrib.Name == dstAttrib.Name &&
                                srcAttrib.ConfiguredModelId == dstAttrib.ConfiguredModelId)
                            {
                                newAttribs.Add(new Attrib()
                                {
                                    Id = dstAttrib.Id,
                                    Name = dstAttrib.Name,
                                    Value = srcAttrib.Value,
                                    ConfiguredModelId = srcAttrib.ConfiguredModelId
                                });

                                added = true;
                            }
                        }

                        if (added == false)
                        {
                            newAttribs.Add(new Attrib()
                            {
                                Id = -1,
                                Name = srcAttrib.Name,
                                Value = srcAttrib.Value,
                                ConfiguredModelId = srcAttrib.ConfiguredModelId
                            });
                        }
                    }

                    con.Open();
                    foreach (var attrib in newAttribs)
                    {
                        var fieldName = "value";
                        //SQLiteCommand comm =
                        //    new SQLiteCommand(
                        //        "UPDATE `object_attributes` SET `" + fieldName + "`= '" + attrib.Value +
                        //        "' WHERE `id`= " + attrib.Id + ";", con);

                        var id = attrib.Id != -1 ? attrib.Id.ToString() : "NULL";
                        if (id == "NULL")
                        {
                            int stop = 0;
                            stop = 0;
                        }

                        if (attrib.Name == "FlEqGyroSenseReverse")
                        {
                            int stop = 0;
                            stop = 0;
                        }
                        if (attrib.Value == "Rail")
                        {
                            int stop = 0;
                            stop = 0;
                        }

                        string sql = "update `attribs` set `value`= '" + attrib.Value + "'" + " where `configured_actor_id` = " + attrib.ConfiguredModelId + " and `title` = '" + attrib.Name + "'";
                        SQLiteCommand commInsert =
                            new SQLiteCommand(sql, con);

                        using (SQLiteDataReader read = commInsert.ExecuteReader())
                        {
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
                finally
                {
                    con.Close();
                }
            }
        }

        public void DeleteDstComponents(int sourceId, int dstId)
        {
            Database srcDatabase = new Database();
            Database dstDatabase = new Database();

            srcDatabase.ConnectionString = this.ConnectionSrc;
            dstDatabase.ConnectionString = this.ConnectionDst;

            var srcObjectsSql = "select * from actor_objects where actor_id = " + sourceId;
            var srcObjectsResult = srcDatabase.executeQuery(srcObjectsSql);

            var dstObjectsSql = "select * from actor_objects where actor_id = " + dstId;
            var dstObjectsResult = dstDatabase.executeQuery(dstObjectsSql);

            foreach (var dstRow in dstObjectsResult.rows)
            {
                var dstRowId = dstRow.GetFieldValueAsInt("id");

                bool bContains = false;
                foreach (var srcRow in srcObjectsResult.rows)
                {
                    var srcRowId = srcRow.GetFieldValueAsInt("id");

                    if (dstRowId == srcRowId)
                    {
                        bContains = true;
                        break;
                    }
                }

                if (!bContains)
                {
                    dstDatabase.DeleteComponent(dstRowId);
                }
            }
        }


        private void Button_Click_2(object sender, RoutedEventArgs e)
        {
            if (WingShapeData_Checkbox.IsChecked == true || 
                WingAerodynamicProperties_Checkbox.IsChecked == true ||
                WheelData_Checkbox.IsChecked == true ||
                ControlSurfaceProperties_Checkbox.IsChecked == true ||
                Propwash_Checkbox.IsChecked == true ||
                Engine_Checkbox.IsChecked == true ||
                Model_Checkbox.IsChecked == true)
            {
                CopyModelObjects();
            }

            if (Attribs_Checkbox.IsChecked == true)
            {
                CopyAttribs(ModelSource.Id, ModelDestination.Id);
            }

            if (DeleteModelObjects_Checkbox.IsChecked == true)
            {
                DeleteDstComponents(ModelSource.Id, ModelDestination.Id);
            }

            Close();
        }
    }
}

