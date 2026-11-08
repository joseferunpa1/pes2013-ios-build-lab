#!/usr/bin/env python3
"""Generate a tiny, original PowerPC64 big-endian ELF for testing ps3recomp's ELF loader.
No PES 2013 data, firmware, keys, or copied game assets are included.
This intentionally tests PT_LOAD/BSS/OPD; lifted code remains in the host app.
"""
from pathlib import Path
import struct, sys

if len(sys.argv) != 2:
    raise SystemExit('usage: make_test_elf.py OUTPUT')
out=Path(sys.argv[1]); out.parent.mkdir(parents=True,exist_ok=True)
file=bytearray(0x2010)
file[:16]=b'\x7fELF'+bytes([2,2,1,0,0])+bytes(7)   # ELF64, MSB, version 1
struct.pack_into('>HHIQQQIHHHHHH',file,16,
    2,21,1, 0x00040000,64,0,0, 64,56,2, 0,0,0)
# ELF64 program headers: p_type, p_flags, p_offset, p_vaddr, p_paddr,
# p_filesz, p_memsz, p_align.
struct.pack_into('>IIQQQQQQ',file,64,
    1,4,0x1000,0x40000,0x40000,16,0x40,0x1000)
struct.pack_into('>IIQQQQQQ',file,64+56,
    1,6,0x2000,0x50000,0x50000,16,0x80,0x1000)
# 2 PowerPC OPD descriptors: {code address, TOC} in big endian.
struct.pack_into('>IIII',file,0x1000, 0x100A0,0x60000, 0x10080,0x60000)
struct.pack_into('>IIII',file,0x2000, 0x12345678,0xAABBCCDD,0x13579BDF,0x2468ACE0)
out.write_bytes(file)
assert len(file)==0x2010 and file[0x1000:0x1004]==bytes.fromhex('000100a0')
print('ELF_GENERATED=YES')
print('ELF_PPC64_MSB=YES')
print('ELF_ET_EXEC=YES')
print('ELF_ENTRY_OPD=0x00040000')
print('ELF_PT_LOAD_COUNT=2')
print('ELF_OPD_VADDR=0x00040000')
print('ELF_DATA_VADDR=0x00050000')
print('ELF_BSS_AT=0x00050010')
print('ELF_BYTES='+str(len(file)))
