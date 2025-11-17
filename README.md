# CAD Visualizer Tool
A CAD editor built using OpenGL and Win32.

# CAD Visualizer Tool – Features Overview

## Core Features

### 3D Scene Editor
- Create, select, transform and delete 3D objects in real-time object.
- Object properties panel.

<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/a37972f2-b346-4618-926c-b7365d221437" />

### Rendering
- Grid rendering on the XZ plane for scene orientation.
- Bounding box and vertex visualization for selected objects.
- Object list with selection and delete functionality.
- Dialog for creating new objects with type and parameters.

### Camera Controls
- Free-look camera: WASD movement and mouse look.
- Provides Orthographic and Perspective Views.
- Orbit camera: rotate around a target, zoom in/out.
- Seamless switching between camera modes.

#### Orthographic View
<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/3515265d-21dd-4700-869f-b325e8108191" />

#### Perspective View
<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/83b56aee-ca21-4982-8174-6789c927d82d" />

### File I/O
- Import 3D models from STL files.
- Support for both binary and ASCII STL formats.

Modular MVC (Model-View-Controller) design for maintainability.

# Technical Functionality

## Geometry & 3D Modeling

### Parametric Primitives
The engine supports mathematically defined primitives (cube, sphere, cylinder, torus, cone, plane, pyramid) with fully editable parameters and transform operations.  

### Face-Level Selection
Each mesh contains a list of triangular faces, each defined by:
- 3 vertex positions  
- Precomputed normal  
- Per-face AABB bounds

This enables precise selection and ray-intersection down to the triangle level.

### Multi-Mode Bounding Volumes
- **Object-Space AABB:** Original dimensions before transforms  
- **Scaled Bounds:** Size after applying scale factors  
- **Tight Bounds (Scale Only):** Pre-rotation bounding box  
- **World-Space AABB:** Full transformation applied  

## Bounding Volume Hierarchy (BVH)
A memory-optimized BVH implementation provides fast ray queries and intersection testing.

Key features:
- **SAH (Surface Area Heuristic):** Optimal split evaluation  
- **In-place triangle sorting:** Improves cache locality  
- **O(n) SAH evaluation:** Using precomputed left/right surface area arrays  

## KD-Tree Accelerator

Optimized for balanced space partitioning.

### Key Features
- Axis cycling (X → Y → Z)  
- Median-based splitting  
- Triangle overlap handling  
- Non-recursive stack traversal  

### Ray-AABB Rejection
The engine performs fast AABB tests before expensive triangle checks.  
This rejects over **90%** of rays early.

### Möller–Trumbore Triangle Intersection
The engine implements a high-precision triangle intersection method for face and object selection.

**Note:**  
This application is written in C++14, uses OpenGL for rendering, and the Win32 API for its user interface.
