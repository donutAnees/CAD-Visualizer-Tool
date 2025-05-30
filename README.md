# CAD Visualizer Tool
A CAD editor built using OpenGL and Win32.

# CAD Visualizer Tool – Features Overview

![Screenshot 2025-05-30 100020](https://github.com/user-attachments/assets/7d343a05-6b65-4678-aaf1-9431a372bb47)

## Core Features

- **3D Scene Editor**
  - Create, select, and delete 3D objects.
  - Real-time object transformation: move, rotate, and scale objects.
  - Sidebar for editing object properties.

- **Rendering**
  - Grid rendering on the XZ plane for scene orientation.
  - Bounding box and vertex visualization for selected objects.
  - Object list with selection and delete functionality.
  - Dialog for creating new objects with type and parameters.
    
- **Camera Controls**
  - Free-look camera: WASD movement and mouse look.
  - Orbit camera: rotate around a target, zoom in/out.
  - Fit camera to selected object or bounding box.
  - Seamless switching between camera modes.

- **File I/O**
  - Import 3D models from STL files.
  - Support for both binary and ASCII STL formats.

- **Performance**
  - Multi-threaded rendering for responsive UI and smooth interaction.
  - Efficient scene updates and redraws on object or camera changes.

- **Architecture**
  - Modular MVC (Model-View-Controller) design for maintainability.

---

![Screenshot 2025-05-30 095757](https://github.com/user-attachments/assets/334c8c8c-f716-4aa0-bd45-297693c6b61d)

**Note:**  
This application is written in C++14, uses OpenGL for rendering, and the Win32 API for its user interface.
