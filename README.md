# Quin Engine
Quin is a game engine, primarily supporting real-time 3D graphics. Currently it's going through a major refactor, so some implementations are left empty.

# Platforms
- Currently only supports Windows. Plans are to later support linux and macOS. 
- Currently only supports Vulkan. 
  - I've made an abstraction so I can implement another API for the renderer to use while application logic can stay the same. 

# Getting Started
1. Clone the repo by running `clone --recurse-submodules https://github.com/daniel10015/QUIN.git`. If submodules aren't cloned then run `git submodule update --init --recursive`
2. Install external dependencies (Vulkan and other APIs) if needed (TODO: automate this: shell scripts calling python scripts to install dependencies)

# Features
- **Multithreading:** Decoupled Renderer and Application logic  
- **Particle System:** Custom math expressions parsed and evaluated during runtime with an [expression evaluator](https://github.com/daniel10015/Math-Expression-Evaluator)

# Demos
TODO

# Future Goals
- Create an Editor 
- Manage vulkan memory with 3rd party memory manager 
- GPU Particle System
- More sohpisticated multithreading system reacting during runtime
  - e.g. Assigning more worker threads to the slower system
- Skeletal animation
- Keyframe animation 


# Notes
This is my first attempt at creating a game engine, as such I've used plenty of resources. Here is a list of them and their keypoints:
- vulkan-tutorial.com
  - How vulkan works and many of its features
- vkguide.dev
  - Rendering architecture and optimization
- TheCherno.com
  - Game engine architecture (longer explanations)
- Unreal Engine and Unity docs and talks
  - Editor workflow

While my main focus is on rendering, I plan to work on other features as needed to make future development or testing easier (e.g. physics, particle systems, editors).

- 9/15/2024 I pushed a major refactor decoupling engine components and abstracting the renderer, which makes many features unusable in its current state. I'm looking to build it back from the ground up with the editor at the forefront of it. 