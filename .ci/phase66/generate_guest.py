#!/usr/bin/env python3
"""Generate two original PPC instructions: addi r3,r3,7; blr.
Uses raw 32-bit big-endian PPU opcodes, not game copyrighted binaries."""
from pathlib import Path
import json,struct
out=Path(__file__).resolve().parent
(out/'tiny.ppc.bin').write_bytes(struct.pack('>II',0x38630007,0x4e800020))
(out/'functions.json').write_text(json.dumps([{'start':'0x10000','end':'0x10008'}])+'\n')
print('RAW_PPC_OPCODES=38630007 4e800020')
print('EXPECTED_TRANSLATION=addi r3,r3,7; blr')
