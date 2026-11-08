#!/usr/bin/env python3
"""Four original diagnostic PPU functions, no copyrighted game instructions.
0x10000: conditional branch + arithmetic (phase 67 regression)
0x10020: big-endian 32-bit store/load (phase 67 regression)
0x10040: caller saves LR and adjusts stack, bl to 0x10080, restores LR
0x10080: callee adds 7 and returns; caller then adds 5
"""
from pathlib import Path
import json, struct
p=Path(__file__).resolve().parent
base=0x10000
image=[0x60000000]*((0x10088-base)//4)
def insert(addr, ops):
    assert addr>=base and (addr-base)%4==0
    ix=(addr-base)//4
    image[ix:ix+len(ops)]=ops
insert(0x10000,[0x2c030000,0x41820008,0x38630007,0x4e800020])
insert(0x10020,[0x90640000,0x80a40000,0x38650002,0x4e800020])
insert(0x10040,[
    0x7c0802a6, # mflr r0
    0xf8010010, # std r0,16(r1)
    0xf821ff71, # stdu r1,-144(r1)
    0x48000035, # bl 0x10080 from 0x1004c (displacement +0x34)
    0x38630005, # addi r3,r3,5
    0x38210090, # addi r1,r1,144
    0xe8010010, # ld r0,16(r1)
    0x7c0803a6, # mtlr r0
    0x4e800020, # blr
])
insert(0x10080,[0x38630007,0x4e800020])
assert image[(0x1004c-base)//4] == 0x48000035
assert len(image)==34
(p/'tiny.ppc.bin').write_bytes(struct.pack('>'+('I'*len(image)),*image))
funcs=[
 {'start':'0x10000','end':'0x10010'},
 {'start':'0x10020','end':'0x10030'},
 {'start':'0x10040','end':'0x10064'},
 {'start':'0x10080','end':'0x10088'},
]
(p/'functions.json').write_text(json.dumps(funcs)+'\n')
(p/'opcodes.txt').write_text('\n'.join(f'{base+i*4:08x}: {w:08x}' for i,w in enumerate(image))+'\n')
print('PPC_FUNCTIONS=4')
print('NEW_CALLER=0x10040 BL_TARGET=0x10080 STACK_FRAME=144')
print('NEW_CALLEE=0x10080 ADDS=7 CALLER_ADDS=5')
print('RAW_PPC_IMAGE_BYTES='+str(len(image)*4))
