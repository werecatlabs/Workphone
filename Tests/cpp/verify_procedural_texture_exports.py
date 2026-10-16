"""Independent decoder checks after ProceduralTextureExportTests <output-directory>.

Usage: python Tests/verify_procedural_texture_exports.py <output-directory>
Requires Pillow; checks signatures, alpha, orientation, clamping, and PNG blocks.
"""
from pathlib import Path
import sys
from PIL import Image

directory = Path(sys.argv[1])
expected = [(0, 128, 255, 64), (255, 0, 0, 255), (0, 255, 0, 0), (0, 0, 255, 255)]
for extension in ("png", "tga"):
    with Image.open(directory / f"texture-export-test.{extension}") as image:
        assert image.size == (2, 2)
        assert list(image.convert("RGBA").getdata()) == expected

with Image.open(directory / "texture-export-blocks.png") as image:
    image.load()
    assert image.size == (257, 257)
    assert image.getpixel((0, 0)) == (128, 64, 255, 128)
    assert image.getpixel((256, 256)) == (128, 64, 255, 128)

pam = (directory / "texture-export-test.pam").read_bytes()
header, data = pam.split(b"ENDHDR\n", 1)
assert header == b"P7\nWIDTH 2\nHEIGHT 2\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\n"
assert data == bytes(channel for pixel in expected for channel in pixel)
print("Procedural PNG/TGA/PAM independent round-trip verification passed")
