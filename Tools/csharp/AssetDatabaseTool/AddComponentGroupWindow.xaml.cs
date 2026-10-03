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
    /// Interaction logic for AddComponentGroupWindow.xaml
    /// </summary>
    public partial class AddComponentGroupWindow : Window
    {
        public string connectionString { get; set; }

        public AddComponentGroupWindow()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            using (SQLiteConnection con = new SQLiteConnection(connectionString))
            {
                try
                {
                    con.Open();
                    if (con.State == ConnectionState.Open)
                    {
                        string sql = "insert into component_groups (title, group_type) values ('" + GroupName.Text +
                                     "','Group Type')";
                        SQLiteCommand comm = new SQLiteCommand(sql, con);
                        using (SQLiteDataReader read = comm.ExecuteReader())
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

            Close();
        }
    }
}
