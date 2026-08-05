# References: reserved for a future module

`REFR` and `LIGH` output are intentionally disabled in the current MVP.

The compiler rejects a World IR containing objects or lights before it opens an
output file. It exports terrain only: `TES4`, `WRLD`, exterior `CELL` records,
their GRUP hierarchy, and `LAND` records.

The worldspace group plan still reserves placement collections for the future
REFR architecture, but the group serializer rejects any non-empty reference
collection. No empty or placeholder REFR record is emitted.
