# Cloth and Physics simulation
---
## how to run
Make sure that [glm](https://github.com/g-truc/glm) and [SDL2](https://www.libsdl.org/) are installed. Ideally, these should be installed by your package manager rather than manually (at least, if you are on Linux or Mac).

Then compile the code using the standard CMake procedure:

- The first time, run `cmake -B build` from the project root to create a `build/` directory and initialize a build system there.
- Then, every time you want to compile the code, run `cmake --build build` (again from the project root). Then the example programs will be created under `build/`.
## Implementation

Uses constrain solving method to simulate cloth inextensibility, and shear and bending springs to simulate shear and bending resistance. 
We use discrete collision detection and semi-implicit Euler for updates.

## sample images
![image](https://github.com/user-attachments/assets/4d4aeb8b-3fda-4c58-a8e6-686f55227447)
![image](https://github.com/user-attachments/assets/22f030fa-a00d-4688-8f0c-0f25083c6165)
