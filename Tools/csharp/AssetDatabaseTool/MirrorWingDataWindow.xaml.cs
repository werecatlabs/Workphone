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
using System.Globalization;

namespace fb
{
    /// <summary>
    /// Interaction logic for MirrorWingDataWindow.xaml
    /// </summary>
    public partial class MirrorWingDataWindow : Window
    {

        public string connectionString { get; set; }

        public int ModelSourceId { get; set; }
        public int ModelDestinationId { get; set; }

        public int ModelObjectSourceId { get; set; }
        public int ModelObjectDestinationId { get; set; }

        public Model ModelSource { get; set; }
        public Model ModelDestination { get; set; }

        public ModelObject ModelObjectSource { get; set; }
        public ModelObject ModelObjectDestination { get; set; }

        public MirrorWingDataWindow()
        {
            InitializeComponent();
        }



        private void Button_Click(object sender, RoutedEventArgs e)
        {
            ModelObjectWindow popup = new ModelObjectWindow();
            popup.connectionString = this.connectionString;
            popup.ModelObjectType = "wing_data";
            popup.SetupModels();
            popup.ShowDialog();

            ModelSource = popup.SelectedModel;
            ModelObjectSource = popup.SelectedModelObject;

            if (ModelSource != null && ModelObjectSource != null)
            {
                SourceText.Text = ModelSource.Name + " " + ModelObjectSource.Ident;
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            ModelObjectWindow popup = new ModelObjectWindow();
            popup.connectionString = this.connectionString;
            popup.ModelObjectType = "wing_data";
            popup.SetupModels();
            popup.ShowDialog();

            ModelDestination = popup.SelectedModel;
            ModelObjectDestination = popup.SelectedModelObject;

            if (ModelDestination != null && ModelObjectDestination != null)
            {
                DestinationText.Text = ModelDestination.Name + " " + ModelObjectDestination.Ident;
            }
        }

        private void Button_Click_2(object sender, RoutedEventArgs e)
        {
            using (SQLiteConnection con = new SQLiteConnection(connectionString))
            {
                CultureInfo us = new CultureInfo("en-GB");

                con.Open();
                if (con.State == ConnectionState.Open)
                {
                    List<Attrib> srcAttribs = new List<Attrib>();
                    List<Attrib> dstAttribs = new List<Attrib>();
                    List<Attrib> newAttribs = new List<Attrib>();

                    if (e.Source != null)
                    {
                        if (ModelObjectSource != null)
                        {
                            SQLiteCommand comm =
                                new SQLiteCommand("Select * From object_attributes where object_id = " + ModelObjectSource.Id, con);
                            using (SQLiteDataReader read = comm.ExecuteReader())
                            {
                                while (read.Read())
                                {
                                    var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                    var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                    string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                    string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

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

                        if (ModelObjectDestination != null)
                        {
                            SQLiteCommand comm =
                                new SQLiteCommand("Select * From object_attributes where object_id = " + ModelObjectDestination.Id, con);
                            using (SQLiteDataReader read = comm.ExecuteReader())
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
                                    ObjectId = ModelObjectDestination.Id,
                                    Name = srcAttrib.Name,
                                    Value = srcAttrib.Value
                                });
                            }
                        }

                        foreach (var attrib in newAttribs)
                        {
                            if (attrib.Name == "position")
                            {
                                var values = attrib.Value.Split(',');
                                if (values.Length >= 3)
                                {
                                    float x = float.Parse(values[0], CultureInfo.GetCultureInfo("en-GB"));
                                    float y = float.Parse(values[1], CultureInfo.GetCultureInfo("en-GB"));
                                    float z = float.Parse(values[2], CultureInfo.GetCultureInfo("en-GB"));

                                    x = -x;

                                    attrib.Value = x.ToString(CultureInfo.GetCultureInfo("en-GB")) +
                                                   ", " + y.ToString(CultureInfo.GetCultureInfo("en-GB")) +
                                                   ", " + z.ToString(CultureInfo.GetCultureInfo("en-GB"));
                                }
                            }
                            else if (attrib.Name == "scale")
                            {
                                var values = attrib.Value.Split(',');
                                if (values.Length >= 3)
                                {
                                    float x = float.Parse(values[0], CultureInfo.GetCultureInfo("en-GB"));
                                    float y = float.Parse(values[1], CultureInfo.GetCultureInfo("en-GB"));
                                    float z = float.Parse(values[2], CultureInfo.GetCultureInfo("en-GB"));

                                    x = -x;

                                    attrib.Value = x.ToString(CultureInfo.GetCultureInfo("en-GB")) + 
                                                   ", " + y.ToString(CultureInfo.GetCultureInfo("en-GB")) + 
                                                   ", " + z.ToString(CultureInfo.GetCultureInfo("en-GB"));
                                }
                            }
                        }

                        foreach (var attrib in newAttribs)
                        {
                            var fieldName = "value";
                            //SQLiteCommand comm =
                            //    new SQLiteCommand(
                            //        "UPDATE `object_attributes` SET `" + fieldName + "`= '" + attrib.Value +
                            //        "' WHERE `id`= " + attrib.Id + ";", con);

                            var id = attrib.Id != -1 ? attrib.Id.ToString() : "NULL";
                            string sql = "insert or replace into `object_attributes`(id, object_id, name, value) values(" +
                                id + "," +
                                attrib.ObjectId + ",'" +
                                attrib.Name + "','" +
                                attrib.Value + "');";
                            SQLiteCommand comm =
                                new SQLiteCommand(sql, con);

                            using (SQLiteDataReader read = comm.ExecuteReader())
                            {
                            }
                        }
                    }
                }

                con.Close();
            }

            Close();
        }
    }
}
