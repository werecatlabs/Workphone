# AAA Procedural City Engine - ThreeJS Demo

## Overview
This demo showcases a complete AAA-quality procedural city matching Call of Duty visual quality. Built with techniques from the Claude-of-Duty reference codebase.

## Features Implemented

### 1. Procedural Noise System
- Perlin gradient noise with FBM (Fractional Brownian Motion)
- Worley (Voronoi) noise for cellular patterns
- Ridged noise for cracks and terrain
- Domain warping support
- Deterministic seeding for reproducible generation

### 2. Procedural PBR Textures
- **Concrete**: Multi-octave noise, aggregate bumps, formwork lines, pores
- **Brick**: Proper brick pattern with mortar joints, color variation, spalled corners
- **Plaster**: Base texture with crack patterns, water stains
- **Asphalt**: Road texture with wheel ruts, aggregate detail
- **Sand**: Wind-blown sand with grain detail

### 3. Building Generation
- Chamfered box geometry (not simple boxes)
- Wall warp for non-perfect surfaces
- Window states: glazed, open, boarded, shuttered, lit, curtain
- Roof systems: flat, parapet, terrace
- Balcony generation
- AC units on roofs
- Material variety: plaster, concrete, brick

### 4. Vehicle Generation
- Sedan, truck, wreck variants
- Procedural body geometry
- Wheel systems
- Damage modeling for wrecks

### 5. Particle Atlas System
- 8 procedural particle types:
  - Smoke (billowing, wispy)
  - Fire (hot core, glowing edges)
  - Dust (fine grain, grit specks)
  - Spark (point with bloom)
  - Debris (angular chunks)
  - Splash (ring patterns)
  - Flash (muzzle flash)
- All particles procedurally generated using noise functions

### 6. Explosion System
- Phased explosion composition:
  1. Core flash (immediate)
  2. Fireball expansion
  3. Smoke column
  4. Debris physics
  5. Light flash
  6. Scorch mark decal
- Realistic particle count and timing
- Additive blending for fire

### 7. Sky System
- Time-of-day transitions:
  - Night (dark blue)
  - Sunrise (orange to yellow)
  - Day (warm white)
  - Sunset (orange)
- Dynamic fog color
- Sun position calculation
- Atmospheric fog (exponential)

### 8. Ground System
- FBM terrain heightmap
- Road with camber (crown)
- Wheel rut patterns
- Sidewalk generation
- Road markings
- Scattered debris

### 9. Props
- Wooden crates
- Metal barrels
- Sandbags (stacked)
- All with shadows

## Controls
- **Orbit**: Left mouse drag
- **Zoom**: Scroll wheel
- **Pan**: Right mouse drag
- **Space/Double-click**: Trigger explosion
- **P**: Pause time
- **R**: Reset to noon

## Running the Demo
1. Create an HTML file that imports this module
2. Include Three.js from CDN
3. Import the AAAProceduralCity class
4. Initialize with 
ew AAAProceduralCity()

## Future Enhancements
- [ ] GTAO (Ground-Truth Ambient Occlusion)
- [ ] SSR (Screen Space Reflections)
- [ ] TAA (Temporal Anti-Aliasing)
- [ ] Bloom post-processing
- [ ] Volumetric fog
- [ ] Procedural sky dome with Rayleigh scattering
- [ ] Nav mesh generation
- [ ] LOD system for buildings
- [ ] Interior generation
- [ ] AI pathfinding
- [ ] Weapon systems
- [ ] Character models

## References
- Claude-of-Duty: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\
  - buildings.js - Building generation
  - kit.js - Building kit components
  - dressing.js - World dressing
  - ground.js - Ground/road generation
  - props.js - Prop library
  - explosions.js - Explosion effects
  - atlas.js - Particle/decal atlases
  - sky/index.js - Complete sky system

## License
Part of the Workphone/Lioncat game engine project.
