#!/usr/bin/env python3
"""
extract_disc.py — Extract CTR NTSC-U PS1 retail assets from a .bin CD image
directly into ~/Library/Application Support/CTR-Native/assets and ./assets.
"""

import os
import sys
import struct

SECTOR_SIZE = 2352
DATA_OFFSET = 24
DATA_SIZE = 2048

def read_sector_data(f, lba):
    f.seek(lba * SECTOR_SIZE + DATA_OFFSET)
    return f.read(DATA_SIZE)

def parse_iso(f):
    # Read Primary Volume Descriptor at LBA 16
    pvd = read_sector_data(f, 16)
    if b"CD001" not in pvd[1:6]:
        raise ValueError("Invalid ISO9660 volume descriptor (CD001 not found)")
        
    root_record = pvd[156:156+34]
    root_lba = struct.unpack("<I", root_record[2:6])[0]
    root_len = struct.unpack("<I", root_record[10:14])[0]
    
    def list_dir(dir_lba, dir_len, current_path=""):
        dir_data = b""
        sectors = (dir_len + DATA_SIZE - 1) // DATA_SIZE
        for i in range(sectors):
            dir_data += read_sector_data(f, dir_lba + i)
            
        offset = 0
        items = []
        while offset < len(dir_data):
            rec_len = dir_data[offset]
            if rec_len == 0:
                offset += 1
                continue
            record = dir_data[offset:offset+rec_len]
            lba = struct.unpack("<I", record[2:6])[0]
            size = struct.unpack("<I", record[10:14])[0]
            flags = record[25]
            name_len = record[32]
            name = record[33:33+name_len].decode("latin1", errors="replace")
            offset += rec_len
            
            if name in ("\x00", "\x01"):
                continue
                
            clean_name = name.split(";")[0]
            full_path = f"{current_path}/{clean_name}" if current_path else clean_name
            is_dir = (flags & 2) != 0
            items.append((is_dir, full_path, lba, size))
            
            if is_dir:
                items.extend(list_dir(lba, size, full_path))
                
        return items
        
    return list_dir(root_lba, root_len)

def extract_file_iso_data(f_in, lba, size, out_path):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    sectors = (size + DATA_SIZE - 1) // DATA_SIZE
    bytes_remaining = size
    
    with open(out_path, "wb") as f_out:
        for i in range(sectors):
            f_in.seek((lba + i) * SECTOR_SIZE + DATA_OFFSET)
            chunk_size = min(bytes_remaining, DATA_SIZE)
            data = f_in.read(chunk_size)
            f_out.write(data)
            bytes_remaining -= len(data)

def extract_file_raw_xa(f_in, lba, size, out_path):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    # XA files must be extracted as 2352-byte raw sectors
    sectors = (size + DATA_SIZE - 1) // DATA_SIZE
    with open(out_path, "wb") as f_out:
        f_in.seek(lba * SECTOR_SIZE)
        for i in range(sectors):
            data = f_in.read(SECTOR_SIZE)
            f_out.write(data)

def main():
    bin_path = os.path.expanduser("~/Downloads/CTR - Crash Team Racing (USA)/CTR - Crash Team Racing (USA).bin")
    if len(sys.argv) > 1:
        bin_path = sys.argv[1]
        
    if not os.path.isfile(bin_path):
        print(f"Error: {bin_path} not found.")
        sys.exit(1)
        
    target_lib_dir = os.path.expanduser("~/Library/Application Support/CTR-Native/assets")
    local_dir = os.path.abspath("assets")
    
    os.makedirs(target_lib_dir, exist_ok=True)
    
    print(f"Opening CD image: {bin_path}")
    with open(bin_path, "rb") as f:
        entries = parse_iso(f)
        
        # We need:
        # BIGFILE.BIG
        # SOUNDS/KART.HWL
        # TEST.STR
        # XA/ENG.XNF
        # All XA/*.XA files
        needed_entries = []
        for is_dir, path, lba, size in entries:
            if is_dir:
                continue
            if path in ("BIGFILE.BIG", "SOUNDS/KART.HWL", "TEST.STR", "XA/ENG.XNF") or path.endswith(".XA"):
                needed_entries.append((path, lba, size))
                
        print(f"Found {len(needed_entries)} required asset files to extract.")
        
        for idx, (rel_path, lba, size) in enumerate(needed_entries, 1):
            out_file = os.path.join(target_lib_dir, rel_path)
            is_xa = rel_path.endswith(".XA")
            size_mb = size / (1024 * 1024)
            print(f"[{idx}/{len(needed_entries)}] Extracting {rel_path} ({size_mb:.2f} MB, raw_xa={is_xa})...")
            
            if is_xa:
                extract_file_raw_xa(f, lba, size, out_file)
            else:
                extract_file_iso_data(f, lba, size, out_file)
                
    # Also create/update symlink at ./assets pointing to Application Support
    if os.path.islink(local_dir):
        os.remove(local_dir)
    elif os.path.isdir(local_dir) and not os.listdir(local_dir):
        os.rmdir(local_dir)
        
    if not os.path.exists(local_dir):
        os.symlink(target_lib_dir, local_dir)
        print(f"Created symlink: {local_dir} -> {target_lib_dir}")
        
    print(f"\nAll assets extracted successfully to:\n  {target_lib_dir}\n")

if __name__ == "__main__":
    main()
