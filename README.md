# UE5 → Creation Kit World Import Framework

> **Build the world in Unreal Engine 5. Build the game in Creation Kit.**

The UE5 → Creation Kit World Import Framework is an open-source project that enables Unreal Engine 5 to act as a modern world authoring platform for Creation Engine projects.

Instead of building large worlds directly in Creation Kit, world authoring is performed in Unreal Engine 5. The framework compiles the UE5 project into a native Creation Engine plugin (`.esm`), which can be opened directly in Creation Kit for gameplay authoring and final project assembly.

The framework synchronizes **world data only**.

Gameplay systems (including quests, AI, dialogue, Papyrus, NavMesh, lighting, and final polish) remain authored inside Creation Kit.

---

## Features

- Unreal Engine 5 as the primary world editor
    
- Native `.esm` world compilation
    
- Deterministic world generation
    
- Native Landscape → LAND conversion
    
- Automatic Cell generation
    
- AssetID-based asset mapping
    
- Placeholder workflow
    
- Asset-agnostic design
    
- Persistent object identifiers
    
- Continuous world regeneration
    

---

## Philosophy

- Unreal Engine 5 is the world editor.
    
- Creation Kit is the gameplay editor.
    
- UE5 is the single source of truth for the world.
    
- Generated worldspaces are disposable derived artifacts.
    
- The framework synchronizes world data only.
    
- The framework compiles UE5 projects directly into native Creation Engine plugin files (`.esm`).
    

---

## Workflow

```text
Build World
        │
        ▼
Unreal Engine 5
        │
        ▼
Export to Creation Engine
        │
        ▼
Compile World
        │
        ▼
MyWorld.esm
        │
        ▼
Open in Creation Kit
        │
        ▼
Gameplay Authoring
```

---

## Documentation

Project vision and architectural principles are documented in:

- **RFC-0001 – Vision** (`docs/RFC-0001-Vision.md`)
    

---

## Project Status

The project is currently in the design phase.

RFC-0001 defines the vision, goals, and architectural principles of the framework.

Implementation has not yet started.

Contributions, design discussions, and architecture proposals are welcome.
