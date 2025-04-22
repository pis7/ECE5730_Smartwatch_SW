import argparse
import os
from PIL import Image

# Set up command-line argument parsing
parser = argparse.ArgumentParser(description="Extract non-blank pixels from an image and output a C array.")
parser.add_argument("image_file", type=str, help="Path to the image file")
parser.add_argument("output_file", type=str, help="Path to the output C file")
parser.add_argument("x_offset", type=int, help="X offset for pixel coordinates")
parser.add_argument("y_offset", type=int, help="Y offset for pixel coordinates")
args = parser.parse_args()

# Load the image
image = Image.open(args.image_file).convert("RGBA")

# Scale the image to 48x48
scaled_image = image.resize((48, 48), Image.NEAREST)  # Maintain pixelated style
pixels = scaled_image.load()

# Extract non-blank (non-transparent) pixel coordinates
non_blank_pixels = []
for y in range(scaled_image.height):
    for x in range(scaled_image.width):
        r, g, b, a = pixels[x, y]  # Extract alpha channel
        if a != 0:
            non_blank_pixels.append((x+args.x_offset, y+args.y_offset))  # Add x and y coordinates

# Format the output as a C-style array
output_file_name = os.path.splitext(os.path.basename(args.output_file))[0]
output = f"const uint8_t {output_file_name}[][2] = {{\n"
output += ",\n".join(f"    {{{x}, {y}}}" for x, y in non_blank_pixels)
output += "\n};\n"

# Write the output to the specified file
with open(args.output_file, "w") as file:
    file.write(output)

print(f"C-array written to '{args.output_file}'")