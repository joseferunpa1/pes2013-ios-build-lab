#!/usr/bin/env python3
"""Generate a synthetic PPC64 big-endian ELF with code, OPD, data/BSS.
The lifted host code is compiled independently from the SAME machine-code bytes.
No commercial game assets are used.
"""
from pathlib import Path
import struct,sys
if len(sys.argv)!=3: raise SystemExit('usage: make_boot_elf.py OUT.elf IN.ppc.bin')
out=Path(sys.argv[1]); raw=Path(sys.argv[2]).read_bytes(); out.parent.mkdir(parents=True,exist_ok=True)
assert len(raw)==0x110, ('unexpected PPC size',len(raw))
file=bytearray(0x3000+len(raw))
file[:16]=b'\x7fELF'+bytes([2,2,1,0,0])+bytes(7)
struct.pack_into('>HHIQQQIHHHHHH',file,16, 2,21,1,0x40000,64,0,0,64,56,3,0,0,0)
# PT_LOAD descriptor image, static game data with zero-filled BSS, and executable PPC bytes.
struct.pack_into('>IIQQQQQQ',file,64,1,4,0x1000,0x40000,0x40000,16,0x40,0x1000)
struct.pack_into('>IIQQQQQQ',file,64+56,1,6,0x2000,0x50000,0x50000,16,0x80,0x1000)
struct.pack_into('>IIQQQQQQ',file,64+112,1,5,0x3000,0x10000,0x10000,len(raw),len(raw),0x1000)
struct.pack_into('>IIII',file,0x1000,0x10100,0x60000,0x10080,0x60000)
struct.pack_into('>IIII',file,0x2000,0x12345678,0xAABBCCDD,0x13579BDF,0x2468ACE0)
file[0x3000:]=raw
out.write_bytes(file)
print('ELF_GENERATED=YES')
print('ELF_PPC64_MSB=YES')
print('ELF_ENTRY_OPD=0x00040000')
print('ELF_ENTRY_CODE=0x00010100')
print('ELF_PT_LOAD_COUNT=3')
print('ELF_CODE_MATCHES_LIFTER_INPUT=YES')
print('ELF_BYTES='+str(len(file)))
