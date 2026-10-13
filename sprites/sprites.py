#!/usr/bin/env python3
"""
PNG to ARGB8888 C Array Converter
Converts all 24x24 PNG files in the current directory to C arrays
"""

import os
import sys
from pathlib import Path
from PIL import Image

def sanitize_name(filename):
    """Convert filename to valid C identifier"""
    # Remove extension and replace non-alphanumeric with underscore
    name = Path(filename).stem
    # Ensure it starts with a letter or underscore
    if not name[0].isalpha() and name[0] != '_':
        name = '_' + name
    # Replace invalid characters
    import re
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    return name

def convert_png_to_argb8888(png_path, name):
    """Convert PNG to ARGB8888 byte array"""
    try:
        img = Image.open(png_path).convert('RGBA')
    except Exception as e:
        print(f"Error opening {png_path}: {e}")
        return None
    
    # Verify image size
    if img.size != (24, 24):
        print(f"Warning: {png_path} is not 24x24 (actual: {img.size}). Converting anyway...")
    
    width, height = img.size
    pixels = list(img.getdata())
    
    # Convert to ARGB8888 (Alpha, R, G, B) - each component 8 bits
    argb_bytes = bytearray()
    
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            # ARGB format: Alpha, Red, Green, Blue
            argb_bytes.append(a)  # Alpha
            argb_bytes.append(r)  # Red
            argb_bytes.append(g)  # Green
            argb_bytes.append(b)  # Blue
    
    return {
        'name': name,
        'width': width,
        'height': height,
        'data': argb_bytes,
        'size': len(argb_bytes)
    }

def format_c_array(image_data):
    """Format image data as C array"""
    name = image_data['name']
    data = image_data['data']
    
    # Format bytes for C array
    hex_bytes = []
    for i, byte in enumerate(data):
        hex_bytes.append(f'0x{byte:02X}')
        if i < len(data) - 1:
            hex_bytes.append(',')
            if (i + 1) % 12 == 0:  # 12 bytes per line for readability
                hex_bytes.append('\n    ')
    
    return f"""const uint32_t {name}_width = {image_data['width']};
const uint32_t {name}_height = {image_data['height']};
const uint8_t {name}_bitmap[] = {{
    {''.join(hex_bytes)}
}};"""

def create_header_file(arrays):
    """Create complete header file content"""
    header = """/*
 * ARGB8888 Bitmaps
 * Auto-generated from PNG files
 * Image size: 24x24 pixels
 * Format: ARGB8888 (Alpha, Red, Green, Blue - 8 bits each)
 */

#ifndef BITMAPS_H
#define BITMAPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

"""
    
    # Add all arrays
    for array in arrays:
        header += array + "\n\n"
    
    header += """#ifdef __cplusplus
}
#endif

#endif /* BITMAPS_H */"""
    
    return header

def main():
    print("PNG to ARGB8888 C Array Converter")
    print("==================================\n")
    
    # Get current directory
    current_dir = Path.cwd()
    print(f"Searching for PNG files in: {current_dir}")
    
    # Find all PNG files
    png_files = list(current_dir.glob("*.png")) + list(current_dir.glob("*.PNG"))
    
    if not png_files:
        print("No PNG files found in the current directory!")
        return 1
    
    print(f"Found {len(png_files)} PNG file(s):")
    for png in png_files:
        print(f"  - {png.name}")
    print()
    
    # Convert each PNG
    image_data_list = []
    c_arrays = []
    
    for png_file in png_files:
        print(f"Processing: {png_file.name}")
        name = sanitize_name(png_file.name)
        image_data = convert_png_to_argb8888(png_file, name)
        
        if image_data:
            image_data_list.append(image_data)
            c_array = format_c_array(image_data)
            c_arrays.append(c_array)
            
            # Print to console
            print(f"  Name: {name}")
            print(f"  Size: {image_data['width']}x{image_data['height']}")
            print(f"  Data size: {image_data['size']} bytes")
            print()
    
    if not c_arrays:
        print("No images were successfully converted!")
        return 1
    
    # Generate header file content
    header_content = create_header_file(c_arrays)
    
    # Save to file
    output_file = "bitmaps.h"
    with open(output_file, 'w') as f:
        f.write(header_content)
    
    print(f"\nGenerated header file: {output_file}")
    print(f"Total images converted: {len(c_arrays)}")
    
    # Print complete C arrays to console
    print("\n" + "="*50)
    print("C ARRAYS OUTPUT:")
    print("="*50)
    print(header_content)
    
    return 0

if __name__ == "__main__":
    # Check for PIL installation
    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow library is required!")
        print("Install it with: pip install Pillow")
        sys.exit(1)
    
    sys.exit(main())