After analyzing 52,302 LAND records from Skyrim.esm, Dawnguard.esm, Dragonborn.esm these observations been made:

0x01 byte is strongly correlated with presence of VGHT + VNML subrecords. Additional observation: VGHT and VNML never appeared alone, always in same pair.

0x02 byte is strongly correlated with presence of VCLR subrecords.

0x04 is strongly correlated with presence of BTXT, ATXT and VTXT subrecords.
  
0x08 appeared in every LAND record analyzed, current working hypothesis is that it's base flag, more precise semantics unknown for now.

0x10 byte is currently unknown, it's semantics isn't fully Reverse Engineered yet.

Hypothesis:

0x01 indicates the presence of LAND geometry. Confidecne: Strong
0x02 indicates the presence of VCLR subrecords. Confidence: Strong
0x04 indicates the presence of texture payload. Confidence: Strong
0x08 UNKOWN. All analyzes proved, that this flag exists within every LAND record analyzed. Current implementation: put it in every LAND record compiled.
0x10 is currently unknown, requires more tests and analyzes.
