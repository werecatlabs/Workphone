import os

def remove_h_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(".h"):
                file_path = os.path.join(root, file)
                try:
                    os.remove(file_path)
                    print(f"Deleted: {file_path}")
                except Exception as e:
                    print(f"Error deleting {file_path}: {e}")

# Provide the path to your project directory
project_directory = "../cpp/Include/FBGraphicsOgreNext"
remove_h_files(project_directory)
