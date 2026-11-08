#!/usr/bin/env python3
"""Generate original synthetic PPC machine code, no game assets.
0x10000: cmpwi r3,0; beq +8; addi r3,r3,7; blr
0x10020: stw r3,0(r4); lwz r5,0(r4); addi r3,r5,2; blr
"""
from pathlib import Path
import json,struct
out=Path(__file__).resolve().parent
branch=[0x2c030000,0x41820008,0x38630007,0x4e800020]
mem=[0x90640000,0x80a40000,0x38650002,0x4e800020]
words=branch+[0x60000000]*4+mem # use NOPs as padding in original test image
assert len(words)==12
assert words[8]==0x90640000
(out/'tiny.ppc.bin').write_bytes(struct.pack('>'+('I'*len(words)),*words))
funcs=[{'start':'0x10000','end':'0x10010'}, {'start':'0x10020','end':'0x10030'}]
(out/'functions.json').write_text(json.dumps(funcs)+'\n')
(out/'opcodes.txt').write_text('\n'.join(f'{0x10000+i*4:08x}: {w:08x}' for i,w in enumerate(words))+'\n')
print('FUNCTION_1=cmpwi r3,0; beq +8; addi r3,r3,7; blr')
print('FUNCTION_2=stw r3,0(r4); lwz r5,0(r4); addi r3,r5,2; blr')
print('RAW_PPC_IMAGE_BYTES='+str(len(words)*4))
