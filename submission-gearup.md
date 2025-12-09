# Project 6: Gear Up — Testing

## Gameplay + Physics + Integration Tests

| Feature | How to produce | Expected Output | Your Output |
|--------|--------------|----------------|------------|
| 1. Physics – Movement (Acceleration and Velocity) | Press **W** for ~5 seconds | Snake moves smoothly with increasing speed due to acceleration | Video below |
| 2. Physics – Collision / Death Animation | Steer snake into a wall or a tree | Snake squashes horizontally and respawns | Video below |
| 3. Growth from Food | Eat 3 food cubes in a row | New body segments spawn one-by-one and follow smoothly | Video below |
| 4. Camera Follow | Move the snake | Camera locks behind snake head with smooth motion | Video below |
| 5. L-System Trees – Randomness | Start scene → observe 3 trees | Same overall structure but slightly different branches | <img width="148" height="218" alt="lsystem_random" src="https://github.com/user-attachments/assets/90770b37-f4bf-4dfc-9ec3-d374a06b9a67" /> |
| 6. L-System Path Strip | Walk halfway down the path | Even spacing of trees along edges | <img width="1094" height="795" alt="lsystem_path" src="https://github.com/user-attachments/assets/3b423ebb-0838-43c3-9679-71a1a05324e6" /> |
| 7. Physics - Full Gameplay Integration Test | Move around, eat food, die, respawn | All mechanics function together with stable framerate | Video below |

---

### Physics Videos

#### 1. Physics - Movement 
https://github.com/user-attachments/assets/360aca19-1c20-4719-a4ce-d1eb2e853634


#### 2. Physics - Collision 
https://github.com/user-attachments/assets/22213884-ddb0-42cb-9b4a-1b3f49e64848

#### 3. Growth from Food
https://github.com/user-attachments/assets/0b946ba0-0914-44c7-b65c-e011f19a5220

#### 4. Camera Follow
https://github.com/user-attachments/assets/71b0febc-b343-4791-9228-e621d3828025

#### 7. Physics - Full Gameplay
https://github.com/user-attachments/assets/cfc5ec84-2907-487b-9be0-92a68ab7454e



---

## Visual Feature Tests 

> Load the provided JSON test scenes and save rendered images.  
> Drag and drop the saved images into the “Your Output” column.

| Input Scene / Setting | How to Produce Output | Expected Output | Your Output |
|---|---|---|---|
| Normal Mapping ON – Brick Cube | Load `brick_normalmap_test.json` → Save `brick_nm_on.png` | Bricks show visible micro-surface detail under lighting | <img width="611" height="600" alt="normal_map" src="https://github.com/user-attachments/assets/df473fb2-63ec-4081-8fa3-91dca62b28f8" /> |
| Normal Mapping in Game – Brick Path | N/A | N/A | <img width="252" height="154" alt="Normal Mapping Game" src="https://github.com/user-attachments/assets/3ae59a38-f1a0-4df6-8df3-b6cdf6b74b5e" />|
| Grass Bump Mapping | Load `grass_bump_test.json` → Save `grass_bump.png` | Grass shows subtle height changes (not flat) | <img width="798" height="597" alt="bump mapping from top" src="https://github.com/user-attachments/assets/51105783-4d3d-4f64-8d26-a146bb9b921c" />|
| L-system Tall tree | N/A | Tall Blocky L-system tree | <img width="745" height="567" alt="larger L-system" src="https://github.com/user-attachments/assets/ee1f326c-050b-4876-a851-4bc7c3295bf4" />|
| L-system Wide and Tall tree | N/A | Tall and wide blocky L-system tree |  <img width="793" height="597" alt="tall-wide l-system" src="https://github.com/user-attachments/assets/c030ae49-a594-4a87-af98-20dffdffa783" />|

---


### Design Choices
- Realtime Snake Arena: I designed the project as a 3D, Minecraft-style snake game to demonstrate procedural content generation and physically-inspired motion. The player controls a rigid-body cube snake moving inside an arena with collision-driven gameplay (walls, terrain, food).
- Procedural Terrain & World Building: The arena and long path beyond the opening gate are generated algorithmically using noise-based heightfields and cube instancing.
    - L-system logic produces tree-like structures along the path to create visual life and variation without manual modeling.
- Per-pixel lighting and advanced shading: Path bricks use tangent-space normal mapping to bring out surface depth from texture detail.
    - The grassy floor uses bump mapping (height-based normals) to simulate uneven ground and break the flat polygon look.
- Dynamic Gameplay Systems: Snake movement is implemented as a rigid-body controller with direction forces and collision response.    
    - Eating food dynamically grows the snake using instanced body cubes
- Cinematic Camera Behavior: A high third-person fixed camera gives a dramatic reveal of the arena.
    - A follow-camera mode was added to track the player during movement


## Collaboration/References
- Relied heavily on online content on L-systems, physics, normal mapping, and bump mapping
- Used code mainly from project 5 for this project.
- I used ChatGPT (OpenAI, GPT-5, 2025) as a debugging/explanation/boilerplate tool for this project

### Known Bugs
N/A

### Extra Credit
N/A
