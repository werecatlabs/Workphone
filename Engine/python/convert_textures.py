import os
import subprocess
from PIL import Image

def convert_to_dds(input_directory, output_directory, texconv_path='texconv'):
    # Ensure output directory exists
    os.makedirs(output_directory, exist_ok=True)

    # Supported image formats (you can expand this list)
    supported_formats = ('.png', '.jpg', '.jpeg', '.bmp', '.tga')

    for root, dirs, files in os.walk(input_directory):
        for file in files:
            if file.lower().endswith(supported_formats):
                input_file = os.path.join(root, file)
                output_file = os.path.join(output_directory, os.path.splitext(file)[0] + '.dds')

                try:
                    # Convert image to DDS using texconv
                    subprocess.run([texconv_path, '-o', output_directory, input_file], check=True)

                    print(f"Converted: {input_file} -> {output_file}")
                except subprocess.CalledProcessError as e:
                    print(f"Error converting {input_file}: {e}")
                except Exception as e:
                    print(f"Unexpected error with {input_file}: {e}")

# Example usage
input_directory = '/path/to/your/input_images'
output_directory = '/path/to/your/output_dds'
texconv_path = '/path/to/texconv/executable'  # Or leave as 'texconv' if it's in your system's PATH

convert_to_dds(input_directory, output_directory, texconv_path)
