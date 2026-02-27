#!/usr/bin/env python3
import struct
import sys
import os

def pad(f, size):
    cur_pos = f.tell()
    if cur_pos < size:
        f.write(b'\0' * (size - cur_pos))

def build_iso(floppy_img_path, iso_out_path):
    # Sector size is 2048 bytes
    with open(floppy_img_path, 'rb') as f:
        floppy_data = f.read()
    
    with open(iso_out_path, 'wb') as f:
        # System Area (16 sectors = 32768 bytes) + Primary Volume Descriptor (sector 16)
        pad(f, 32768)
        
        # Sector 16: Primary Volume Descriptor (Type 1)
        f.write(b'\x01CD001\x01\x00') # Type 1, Identifier, Version
        f.write(b' ' * 32)            # System Identifier
        f.write(b'VLSMC_OS'.ljust(32, b' ')) # Volume Identifier
        f.write(b'\0' * 8)
        f.write(struct.pack('<I', 50)) # Volume Space Size: little endian (just a dummy size)
        f.write(struct.pack('>I', 50)) # Volume Space Size: big endian
        f.write(b'\0' * (2048 - f.tell() % 2048))
        
        # Sector 17: Boot Record Volume Descriptor (Type 0)
        pad(f, 17 * 2048)
        f.write(b'\x00CD001\x01')    # Type 0, Identifier, Version
        f.write(b'EL TORITO SPECIFICATION'.ljust(32, b'\0')) # Boot System Identifier
        f.write(b'\0' * 32)
        f.write(struct.pack('<I', 19)) # Boot Catalog Sector number (Sector 19)
        f.write(b'\0' * (2048 - f.tell() % 2048))
        
        # Sector 18: Volume Descriptor Set Terminator (Type 255)
        pad(f, 18 * 2048)
        f.write(b'\xffCD001\x01')
        f.write(b'\0' * (2048 - f.tell() % 2048))
        
        # Sector 19: Boot Catalog
        pad(f, 19 * 2048)
        # Validation Entry
        f.write(b'\x01\x00\x00\x00') # Header ID (1), Platform ID (0 = 80x86), Reserved
        f.write(b'VLSMC'.ljust(24, b'\0')) # ID string
        f.write(b'\x00\x00') # Checksum (dummy here)
        f.write(b'\xaa\x55') # Signature
        
        # Default Entry
        f.write(b'\x88\x02') # Boot indicator (Bootable), Boot media type (1.44MB Flopy)
        f.write(b'\x00\x00') # Load Segment (0 = default 07C0)
        f.write(b'\x00')     # System Type
        f.write(b'\x00')     # Unused
        f.write(struct.pack('<H', 1)) # Sector count (1 virtual sector to load)
        f.write(struct.pack('<I', 20)) # Load RBA (Sector 20)
        f.write(b'\0' * (2048 - f.tell() % 2048))
        
        # Sector 20+: Floppy Image (1.44MB = 1474560 bytes = 720 sectors of 2048 bytes)
        pad(f, 20 * 2048)
        f.write(floppy_data)
        
        print(f"ISO Image '{iso_out_path}' created successfully.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python build_iso.py <disk.img> <out.iso>")
        sys.exit(1)
    build_iso(sys.argv[1], sys.argv[2])
