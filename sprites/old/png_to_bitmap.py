#!/usr/bin/env python3
"""
PNG to C ARGB Bitmap Array Converter
Converts PNG images to ARGB8888 format (32-bit per pixel) for embedded systems
Handles transparency correctly
"""

import os
import argparse
from PIL import Image
import numpy as np

def convert_image_to_argb(image_path, bg_color=(0, 0, 0)):
    """
    Convert a PNG image to ARGB8888 format
    
    Args:
        image_path: Path to the PNG file
        bg_color: Background color (R,G,B) for transparent pixels (0-255)
    
    Returns:
        tuple: (width, height, argb_array, c_array_string)
    """
    # Open image and convert to RGBA
    img = Image.open(image_path)
    
    # Ensure image has alpha channel
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    
    width, height = img.size
    
    # Convert to numpy array
    rgba_array = np.array(img)
    
    # Convert RGBA to ARGB format and handle transparency
    # Format: 0xAARRGGBB (8 bits each)
    argb_array = bytearray(width * height * 4)
    
    # Get background color
    bg_r, bg_g, bg_b = bg_color
    
    for y in range(height):
        for x in range(width):
            r, g, b, a = rgba_array[y, x]
            
            # Alpha blending with background for semi-transparent pixels
            if a < 255:
                # Convert to float for blending
                alpha = a / 255.0
                inv_alpha = 1.0 - alpha
                
                # Blend with background color
                r = int(r * alpha + bg_r * inv_alpha)
                g = int(g * alpha + bg_g * inv_alpha)
                b = int(b * alpha + bg_b * inv_alpha)
                a = 255  # Make fully opaque after blending
            
            # Pack into ARGB format: 0xAARRGGBB
            idx = (y * width + x) * 4
            argb_array[idx] = a          # Alpha
            argb_array[idx + 1] = r      # Red
            argb_array[idx + 2] = g      # Green
            argb_array[idx + 3] = b      # Blue
    
    # Generate C array string
    array_name = os.path.splitext(os.path.basename(image_path))[0]
    array_name = "".join(c if c.isalnum() else "_" for c in array_name)
    
    # Function to convert 4 bytes to 32-bit hex value
    def bytes_to_uint32(a, r, g, b):
        return (a << 24) | (r << 16) | (g << 8) | b
    
    # Create C array
    c_array = f"// {os.path.basename(image_path)} - {width}x{height} pixels\n"
    c_array += f"// Format: ARGB8888 (0xAARRGGBB, 32-bit per pixel)\n"
    c_array += f"const uint32_t argb_{array_name}[] = {{\n"
    
    # Format as 32-bit values (more readable than bytes)
    pixels_per_line = 8
    for i in range(0, len(argb_array), 4 * pixels_per_line):
        line = "    "
        for j in range(0, min(4 * pixels_per_line, len(argb_array) - i), 4):
            idx = i + j
            a = argb_array[idx]
            r = argb_array[idx + 1]
            g = argb_array[idx + 2]
            b = argb_array[idx + 3]
            value = bytes_to_uint32(a, r, g, b)
            line += f"0x{value:08X}, "
        
        c_array += line.rstrip(", ") + "\n"
    
    c_array = c_array.rstrip(", \n") + "\n};\n"
    
    return width, height, argb_array, c_array

def convert_image_to_rgb565(image_path, bg_color=(0, 0, 0)):
    """
    Convert a PNG image to RGB565 format (16-bit per pixel)
    
    Args:
        image_path: Path to the PNG file
        bg_color: Background color (R,G,B) for transparent pixels
    
    Returns:
        tuple: (width, height, rgb565_array, c_array_string)
    """
    # Open image and convert to RGBA
    img = Image.open(image_path)
    
    # Ensure image has alpha channel
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    
    width, height = img.size
    
    # Convert to numpy array
    rgba_array = np.array(img)
    
    # Convert RGBA to RGB565 format and handle transparency
    # Format: 16-bit RGB565 (5-6-5 bits)
    rgb565_array = bytearray(width * height * 2)
    
    # Get background color
    bg_r, bg_g, bg_b = bg_color
    
    def rgb_to_565(r, g, b):
        # Convert 8-bit RGB to 5-6-5 bits
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        return (r5 << 11) | (g6 << 5) | b5
    
    for y in range(height):
        for x in range(width):
            r, g, b, a = rgba_array[y, x]
            
            # Alpha blending with background
            if a < 255:
                alpha = a / 255.0
                inv_alpha = 1.0 - alpha
                r = int(r * alpha + bg_r * inv_alpha)
                g = int(g * alpha + bg_g * inv_alpha)
                b = int(b * alpha + bg_b * inv_alpha)
            
            # Convert to RGB565
            rgb565 = rgb_to_565(r, g, b)
            
            # Store as little-endian (LSB first)
            idx = (y * width + x) * 2
            rgb565_array[idx] = rgb565 & 0xFF
            rgb565_array[idx + 1] = (rgb565 >> 8) & 0xFF
    
    # Generate C array string
    array_name = os.path.splitext(os.path.basename(image_path))[0]
    array_name = "".join(c if c.isalnum() else "_" for c in array_name)
    
    c_array = f"// {os.path.basename(image_path)} - {width}x{height} pixels\n"
    c_array += f"// Format: RGB565 (16-bit per pixel, little-endian)\n"
    c_array += f"const uint16_t rgb565_{array_name}[] = {{\n"
    
    # Format as 16-bit values
    pixels_per_line = 12
    total_pixels = width * height
    
    for i in range(0, total_pixels, pixels_per_line):
        line = "    "
        for j in range(min(pixels_per_line, total_pixels - i)):
            idx = (i + j) * 2
            low = rgb565_array[idx]
            high = rgb565_array[idx + 1]
            value = (high << 8) | low
            line += f"0x{value:04X}, "
        
        c_array += line.rstrip(", ") + "\n"
    
    c_array = c_array.rstrip(", \n") + "\n};\n"
    
    return width, height, rgb565_array, c_array

def process_folder(folder_path, output_file=None, format='argb8888', bg_color=(0, 0, 0)):
    """
    Process all PNG files in a folder
    
    Args:
        folder_path: Path to folder containing PNG files
        output_file: Output file path (None for console output)
        format: Output format ('argb8888', 'rgb565', or 'mono1')
        bg_color: Background color for transparent pixels
    """
    # Find all PNG files
    png_files = [f for f in os.listdir(folder_path) 
                 if f.lower().endswith('.png')]
    
    if not png_files:
        print(f"No PNG files found in {folder_path}")
        return
    
    print(f"Found {len(png_files)} PNG file(s)")
    print(f"Output format: {format}")
    print(f"Background color for transparency: RGB{bg_color}")
    
    # Prepare output
    output_lines = []
    output_lines.append("// Generated by PNG to C Bitmap Converter")
    output_lines.append(f"// Format: {format.upper()}")
    output_lines.append(f"// Background color for transparency: RGB{bg_color}")
    output_lines.append("")
    
    # Include headers based on format
    if format == 'argb8888':
        output_lines.append("#include <stdint.h>")
        output_lines.append("")
    elif format == 'rgb565':
        output_lines.append("#include <stdint.h>")
        output_lines.append("")
    
    # Process each file
    for png_file in sorted(png_files):
        file_path = os.path.join(folder_path, png_file)
        try:
            if format == 'argb8888':
                width, height, data_array, c_array = convert_image_to_argb(
                    file_path, bg_color
                )
                bytes_per_pixel = 4
            elif format == 'rgb565':
                width, height, data_array, c_array = convert_image_to_rgb565(
                    file_path, bg_color
                )
                bytes_per_pixel = 2
            else:
                # Fall back to mono (from previous script)
                from mono_converter import convert_image_to_bitmap
                width, height, data_array, c_array = convert_image_to_bitmap(
                    file_path, threshold=128, invert=False
                )
                bytes_per_pixel = 1
            
            total_bytes = len(data_array)
            
            print(f"Converted: {png_file} ({width}x{height}, "
                  f"{total_bytes} bytes, {bytes_per_pixel}B/px)")
            
            output_lines.append(c_array)
            output_lines.append("")  # Empty line between arrays
            
            # Save individual binary files
            binary_filename = f"{os.path.splitext(png_file)[0]}_{format}.bin"
            with open(binary_filename, "wb") as f:
                f.write(data_array)
            print(f"  Binary saved: {binary_filename}")
            
        except Exception as e:
            print(f"Error processing {png_file}: {e}")
            import traceback
            traceback.print_exc()
    
    # Write output
    output_content = "\n".join(output_lines)
    
    if output_file:
        with open(output_file, "w") as f:
            f.write(output_content)
        print(f"\nC arrays saved to: {output_file}")
    else:
        print("\n" + "="*60)
        print(output_content)

def hex_to_rgb(hex_color):
    """Convert hex color #RRGGBB to tuple (R,G,B)"""
    hex_color = hex_color.lstrip('#')
    if len(hex_color) != 6:
        raise ValueError("Color must be in format #RRGGBB")
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def main():
    parser = argparse.ArgumentParser(
        description="Convert PNG images to C bitmap arrays for embedded systems"
    )
    parser.add_argument("folder", help="Folder containing PNG files")
    parser.add_argument("-o", "--output", help="Output C header file")
    parser.add_argument("-f", "--format", default="argb8888",
                       choices=["argb8888", "rgb565", "mono1"],
                       help="Output format: argb8888 (32-bit), rgb565 (16-bit), mono1 (1-bit)")
    parser.add_argument("-b", "--background", default="#000000",
                       help="Background color for transparent pixels (format: #RRGGBB)")
    
    args = parser.parse_args()
    
    # Convert background color
    try:
        bg_color = hex_to_rgb(args.background)
    except ValueError as e:
        print(f"Error: Invalid background color: {e}")
        print("Use format like #RRGGBB (e.g., #FFFFFF for white)")
        return
    
    # Process folder
    process_folder(
        folder_path=args.folder,
        output_file=args.output,
        format=args.format,
        bg_color=bg_color
    )

if __name__ == "__main__":
    main()