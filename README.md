# Unreal Engine -> ESM worldspace compiler

ESM Compiler is an experimental, open-source compiler that converts Unreal Engine 5 worlds into .esm files that can be imported into Creation Kit.

Long term goal of this project is to provide modern level design pipeline (Gaea, Houdini, SpeedTree and other procedural tools) for modding Skyrim

**Project Status**: Early development

---
# Features

- Reverse Engineering of Creation Engine LAND.DATA formats.
- World Intermediate Representation.
- Unreal Engine 5.8.1 Frontend.
- Backend for Creation Engine .esm generation.
- Binary serialization of WRLD, CELL, LAND records.
- FormID translation.

---
# Repository Structure

docs/ -> Project Documentation
src/ -> Source code
tests/ -> Unit and integration tests
samples/ -> Example projects
tools/ -> Development tools

---
# Current Status:

Currently implemented:
- Repository structure
- Documentation framework
- Reverse engineering through LAND analysis
- Binary Writer backend
- UE5 frontend
- World IR
- WRLD Serializer
- CELL Serializer
- LAND Serializer (experimental)

Work in progress:

- 0x10 LAND.DATA byte semantics reverse engineering
- 0x08 precise byte semantics
- REFR Serializer (concept phase)
