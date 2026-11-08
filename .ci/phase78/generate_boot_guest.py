#!/usr/bin/env python3
"""Original non-game PPC diagnostics. Phase78 adds an ELF startup entry writing a memory marker.
New 0x100a0: mflr/std/stdu/mtctr r12/bctrl/stack+LR restore.
New 0x100e0: addi r3,r3,11; blr.
Existing 4 functions from phase68 are preserved as regressions.
"""
from pathlib import Path
import json,struct
p=Path(__file__).resolve().parent
base=0x10000
end=0x10110
image=[0x60000000]*((end-base)//4)
def insert(addr,ops):
    assert addr >= base and (addr-base)%4==0
    ix=(addr-base)//4
    image[ix:ix+len(ops)]=ops
insert(0x10000,[0x2c030000,0x41820008,0x38630007,0x4e800020])
insert(0x10020,[0x90640000,0x80a40000,0x38650002,0x4e800020])
insert(0x10040,[
    0x7c0802a6, # mflr r0
    0xf8010010, # std r0,16(r1)
    0xf821ff71, # stdu r1,-144(r1)
    0x48000035, # bl 0x10080 from 0x1004c
    0x38630005, # addi r3,r3,5
    0x38210090, # addi r1,r1,144
    0xe8010010, # ld r0,16(r1)
    0x7c0803a6, # mtlr r0
    0x4e800020, # blr
])
insert(0x10080,[0x38630007,0x4e800020])
insert(0x100a0,[
    0x7c0802a6, # mflr r0
    0xf8010010, # std r0,16(r1)
    0xf821ff71, # stdu r1,-144(r1)
    0x7d8903a6, # mtctr r12 (target guest effective address)
    0x4e800421, # bctrl (indirect call through CTR; updates LR)
    0x38630005, # addi r3,r3,5
    0x38210090, # addi r1,r1,144
    0xe8010010, # ld r0,16(r1)
    0x7c0803a6, # mtlr r0
    0x4e800020, # blr
])
insert(0x100e0,[0x3863000b,0x4e800020])
# New Phase 78 entry function: r4=0x50000; r3=42; stw r3,0x60(r4); blr.
# It leaves a guest-memory proof that ppu_run really invoked this function.
insert(0x10100,[0x3c800005,0x3860002a,0x90640060,0x4e800020])
assert image[(0x100b0-base)//4]==0x4e800421
assert image[(0x100ac-base)//4]==0x7d8903a6
assert image[(0x1004c-base)//4]==0x48000035
assert len(image)*4==end-base
(p/'tiny.ppc.bin').write_bytes(struct.pack('>'+('I'*len(image)),*image))
funcs=[
 {'start':'0x10000','end':'0x10010'},
 {'start':'0x10020','end':'0x10030'},
 {'start':'0x10040','end':'0x10064'},
 {'start':'0x10080','end':'0x10088'},
 {'start':'0x100a0','end':'0x100c8'},
 {'start':'0x100e0','end':'0x100e8'},
 {'start':'0x10100','end':'0x10110'},
]
(p/'functions.json').write_text(json.dumps(funcs)+'\n')
(p/'opcodes.txt').write_text('\n'.join(f'{base+i*4:08x}: {w:08x}' for i,w in enumerate(image))+'\n')
print('PPC_FUNCTIONS=7')
print('BOOT_ENTRY=0x10100 EXPECTED_MARKER=0x2A MEMORY=0x50060')
print('NEW_INDIRECT_CALLER=0x100a0 MTCTR_R12=0x7d8903a6 BCTRL=0x4e800421')
print('INDIRECT_CALLEE_A=0x10080 ADDS=7')
print('INDIRECT_CALLEE_B=0x100e0 ADDS=11')
print('RAW_PPC_IMAGE_BYTES='+str(len(image)*4))
