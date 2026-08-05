After analyzing 52,302 LAND records from Skyrim.esm, Dawnguard.esm, Dragonborn.esm these observations been made:

0x01 bit is strongly correlated with presence of VHGT + VNML subrecords. Additional observation: VHGT and VNML never appeared alone, always in same pair.

0x02 bit is strongly correlated with presence of VCLR subrecords.

0x04 is strongly correlated with presence of BTXT, ATXT and VTXT subrecords.
  
0x08 appeared in every LAND record analyzed, current working hypothesis is that it's base flag, more precise semantics unknown for now.

0x10 bit is currently unknown for me, it's semantics isn't fully Reverse Engineered yet. Through analysis 51 921 from 52 302 LAND records have 0x10, for compatybility issues compiler adds it to every cell

Hypothesis:

- 0x01 indicates the presence of LAND geometry. Confidecne: Strong
- 0x02 indicates the presence of VCLR subrecords. Confidence: Strong
- 0x04 indicates the presence of texture payload. Confidence: Strong
- 0x08 UNKNOWN. All analyzes proved, that this flag exists within every LAND record analyzed. Current implementation: put it in every LAND record compiled.
- 0x010 is currently unknown, requires more tests and analyses.

xEdit:

xEdit labels are consistent with the corpus observations:

- 0x01: Has Vertex Normals/Height Map
- 0x02: Has Vertex Colors
- 0x04: Has Layers
- 0x08: Unknown 4
- 0x10: Auto-Calc Normals

Current Compiler Policy:

The compiler builds one flag value and writes it once as a four-byte, little-endian DATA payload. Flags are not written in a temporal order.

`std::uint32_t flags = 0x08;`

if (hasVhgt && hasVnml) {
    flags |= 0x01;
}
if (hasVclr) {
    flags |= 0x02;
}
if (hasTextureLayers) {
    flags |= 0x04;
}
if (experimentalAutoCalcNormalsEnabled) {
    flags |= 0x10;
}
