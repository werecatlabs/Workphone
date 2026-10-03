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
    /// Interaction logic for ModelObjectWindow.xaml
    /// </summary>
    public partial class ModelObjectWindow : Window
    {
        public string connectionString { get; set; }
        public string ModelObjectType { get; set; }
        public Model SelectedModel { get; set; }
        public ModelObject SelectedModelObject { get; set; }

        public ModelObjectWindow()
        {
            InitializeComponent();
        }

        private void Apply_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        

        public void SetupModels()
        {
            using (SQLiteConnection con = new SQLiteConnection(connectionString))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        List<Model> models = new List<Model>();
                        // models.Add(new Model() { Id = 1, Name = "John Doe" });

                        SQLiteCommand comm = new SQLiteCommand("Select id,name From configured_actors where standard = 1", con);
                        using (SQLiteDataReader read = comm.ExecuteReader())
                        {
                            while (read.Read())
                            {
                                var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                string modelName = Convert.ToString(read.GetValue(read.GetOrdinal("Name")));

                                models.Add(new Model()
                                {
                                    Id = id,
                                    Name = modelName
                                });
                            }
                        }

                        modelObjectModels.ItemsSource = models;
                    }
                }
                catch (Exception ex)
                {
                    con.Close();
                    MessageBox.Show(ex.Message);
                }
            }
        }

        private void ModelObjectsModels_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            using (SQLiteConnection con = new SQLiteConnection(connectionString))
            {
                con.Open();
                if (con.State == ConnectionState.Open)
                {
                    List<ModelObject> attribs = new List<ModelObject>();

                    if (e.Source != null)
                    {
                        Model model = modelObjectModels.SelectedItem as Model;
                        if (model != null)
                        {
                            SelectedModel = model;

                            SQLiteCommand comm =
                                new SQLiteCommand("Select * From actor_objects where actor_id = " + model.Id, con);
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
                                    var originalId = 0;// original_id != null ? Convert.ToInt32(original_id) : 0;

                                    if (ModelObjectType == "" || ModelObjectType == className)
                                    {
                                        attribs.Add(new ModelObject()
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

                            this.modelObjects.ItemsSource = attribs;
                        }
                    }
                }

                con.Close();
            }
        }

        private void ModelObjects_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            using (SQLiteConnection con = new SQLiteConnection(connectionString))
            {
                con.Open();
                if (con.State == ConnectionState.Open)
                {
                    List<Attrib> attribs = new List<Attrib>();

                    if (e.Source != null)
                    {
                        ModelObject model = modelObjects.SelectedItem as ModelObject;
                        if (model != null)
                        {
                            SelectedModelObject = model;

                            SQLiteCommand comm =
                                new SQLiteCommand("Select * From object_attributes where object_id = " + model.Id, con);
                            using (SQLiteDataReader read = comm.ExecuteReader())
                            {
                                while (read.Read())
                                {
                                    var id = Convert.ToInt32(read.GetValue(read.GetOrdinal("id")));
                                    var objectId = Convert.ToInt32(read.GetValue(read.GetOrdinal("object_id")));
                                    string title = Convert.ToString(read.GetValue(read.GetOrdinal("name")));
                                    string value = Convert.ToString(read.GetValue(read.GetOrdinal("value")));

                                    attribs.Add(new Attrib()
                                    {
                                        Id = id,
                                        ObjectId = objectId,
                                        Name = title,
                                        Value = value
                                    });
                                }
                            }

                            //this.objectAttribs.ItemsSource = attribs;
                        }
                    }
                }

                con.Close();
            }
        }
    }
}
