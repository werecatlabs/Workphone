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
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.IO;



namespace FolderStructure
{


    public class folder
    {
        public folder() { }

        public folder(string name)
        {
            folderName = name;
        }

        public folder(string name, List<folder> folders)
        {
            folderName = name;
            subFolders = folders;
        }

        public string folderName = "";
        public List<folder> subFolders = new List<folder>();
    }


    public class preset
    {
        public string name = "";
        public folder root = new folder();
    }


    public class Item
    {
        public string Name { get; set; }
        public string Path { get; set; }
    }

    public class FileItem : Item
    {

    }


    public class DirectoryItem : Item
    {
        public List<Item> Items { get; set; }

        public DirectoryItem()
        {
            Items = new List<Item>();
        }
    }


    public class ItemProvider
    {
        public List<Item> GetItems(string path)
        {
            var items = new List<Item>();

            var dirInfo = new DirectoryInfo(path);

            foreach (var directory in dirInfo.GetDirectories())
            {
                var item = new DirectoryItem
                {
                    Name = directory.Name,
                    Path = directory.FullName,
                    Items = GetItems(directory.FullName)
                };

                items.Add(item);
            }

            foreach (var file in dirInfo.GetFiles())
            {
                var item = new FileItem
                {
                    Name = file.Name,
                    Path = file.FullName
                };

                items.Add(item);
            }

            return items;
        }

        public List<Item> GetItems(folder f)
        {
            List<Item> items = new List<Item>();

            foreach (var directory in f.subFolders)
            {
                var item = new DirectoryItem
                {
                    Name = directory.folderName,
                    Path = directory.folderName,
                    Items = GetItems(directory)
                };

                items.Add(item);
            }

            return items;

        }

        public List<Item> LoadItemsFromPreset(preset json)
        {
            var items = new List<Item>();
            if (json != null)
            {
                var item = new DirectoryItem
                {
                    Name = json.root.folderName,
                    Path = json.root.folderName,
                    Items = GetItems(json.root)
                };

                items.Add(item);
            }

            return items;
        }

        public List<Item> LoadItemsFromPreset(string path)
        {
            var items = new List<Item>();

            string data = File.ReadAllText(path);
            var json = new JavaScriptSerializer().Deserialize<preset>(data);
            if (json != null)
            {
                var item = new DirectoryItem
                {
                    Name = json.root.folderName,
                    Path = json.root.folderName,
                    Items = GetItems(json.root)
                };

                items.Add(item);
            }

            return items;
        }
    }


    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        preset m_Preset = new preset();

        public MainWindow()
        {
            InitializeComponent();
        }

        void CreateDirectory(string path, folder f)
        {
            string newPath = path + "/" + f.folderName;
            Directory.CreateDirectory(newPath);

            foreach(var subFolder in f.subFolders)
            {
                CreateDirectory(newPath, subFolder);
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                System.Windows.Forms.DialogResult result = dialog.ShowDialog();
                //if (result == true)
                {
                    CreateDirectory(dialog.SelectedPath, m_Preset.root);
                }
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();
            var newDestination = Environment.CurrentDirectory;

            if (dialog.ShowDialog() == true)
            {
                var itemProvider = new ItemProvider();

                var filePath = dialog.FileName;
                string data = File.ReadAllText(filePath);
                var json = new JavaScriptSerializer().Deserialize<preset>(data);
                m_Preset = json;

                var items = itemProvider.LoadItemsFromPreset(json);

                DataContext = items;
            }
        }

        private void Button_Click_2(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.SaveFileDialog();
            var newDestination = Environment.CurrentDirectory;
            if (dialog.ShowDialog() == true)
            {
                var obj = new preset
                {
                    name = "cubase",
                    root = new folder
                    {
                        folderName = "Root",
                        subFolders = new List<folder>()
                        {
                            new folder("Cubase Projects", new List<folder>(){ new folder("Mix") }),
                             new folder("Pro Tools", new List<folder>(){ new folder("Projects"), new folder("Exports") }),
                             new folder("Presets", new List<folder>()),
                             new folder("Output", new List<folder>())
                        }
                    }
                };

                var json = new JavaScriptSerializer().Serialize(obj);
                Console.WriteLine(json);

                var filePath = dialog.FileName;
                File.WriteAllText(filePath, json);
            }
        }
    }
}
