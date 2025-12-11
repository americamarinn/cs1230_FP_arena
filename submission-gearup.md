# Neon Snake Arena

A 3D neon “snake” arena built in realtime.  
You control a physics-based snake inside a glowing stadium, eat power-up food, dodge maze walls, and try to survive a chasing boss cube with a spring–mass “hair” cloth.

---

## How to Run the Project

All third-party libraries (GLM, GLEW, etc.) are already integrated through the starter code / project setup.

### Run from Qt Creator

1. **Open the project**
   - Open `CMakeLists.txt` in Qt Creator.
   - Configure the project using your desired Qt kit (e.g. *Qt 6.x for macOS*).

2. **Configure CMake if needed**
   - Make sure `resources/` is in the correct place relative to the executable (Qt should handle this if you’re in the default build tree).
   - Verify that shaders are under `resources/shaders` and textures under `resources/textures`.

3. **Build & Run**
   - Hit **Build** -> **Run** (or the green play button).
   - The start screen should appear; press the appropriate key (see controls below) to start the game.

## Gameplay Highlights

### Neon Arena
- Layered stadium walls with glowing textures and bouncing point lights.
- Floor tiles with checkered brightness and bloom for a “club” / arcade vibe.

### Physics-Based Snake
- Head moves using simple force + friction physics.
- Body segments follow a sampled trail path for smooth motion.
- Jumping affects each body segment with a trailing jump offset, so the body “follows” the head’s jump instead of teleporting.

### Food & Power-Ups
- **Base food:** grows the snake when eaten.
- **Rare power-up foods:**
  - Speed boost (temporary faster movement).
  - High jump boost (temporarily higher jumps).
- Food is spawned in valid, walkable positions inside the arena.

### Boss Cube + “Hair” Cloth
- A boss cube activates after a countdown and chases the snake.
- Basic steering AI that follows the snake on the XZ plane, with simple wall blocking.
- A spring–mass cloth sheet attached to the boss to act as flowing “hair”, simulated with:
  - Structural springs (left/right/up/down).
  - Gravity and damping.
  - Simple wind forces.
  - Sphere collision to drape over an invisible “head”.
  
### Teleportation
- Snake can teleport from purple spots at the sides of the arena
- Simulates the pacman arena

### Visual Effects
- **Deferred shading pipeline** with:
  - G-buffer (position, normal, albedo, emissive).
  - Lighting pass with multiple point lights.
  - Bloom via ping-pong Gaussian blur.
- **Fog around the outer arena**, blended in screen space based on distance from center and slight hue shifts for a party-fog vibe.

## Resources

Physics (FABIO):
- Movement and Gravity: Physics
- spring-mass systems: https://www.cs.cmu.edu/~barbic/jellocube_bw.pdf
- Cloth: https://graphics.stanford.edu/~mdfisher/cloth.html 

Lighting (AMERICA):
- https://learnopengl.com/Advanced-Lighting/Deferred-Shading
- https://www.3dgep.com/forward-plus/

Portals (DAVID):
- https://learnopengl.com/Advanced-OpenGL/Stencil-testing
- https://th0mas.nl/2013/05/19/rendering-recursive-portals-with-opengl/
- https://terathon.com/lengyel/Lengyel-Oblique.pdf

Other:
- LLMs like ChatGPT and Gemini halped with boilerplate and/or repetitive code as to how to create the arena patterns and set up key controls! 

