# Quin Engine
Quin is an acronym for $(\mathbb{Q}\cup \mathbb{I})^n$ or just $\mathbb{R}^n$. 
This project is primarily a learning experience for me to practice designing and implementing APIs. As such I will list out several resources I've used along the way:
- vulkan-tutorial.com
  - How vulkan works and many of its features
- vkguide.dev
  - Rendering architecture and optimization
- TheCherno.com
  - Game engine architecture (longer explanations)

While my main focus is on rendering, I plan to work on other features as needed to make future development or testing easier (e.g. physics, particle systems, editors).

# Platforms
- Currently only supports Windows. Plans are to later support linux and macOS. 
- Currently only supports Vulkan. Might support other APIs later. 

# Getting Started
1. Clone the repo by running `clone --recurse-submodules https://github.com/daniel10015/QUIN.git`. If submodules aren't cloned then run `git submodule update --init --recursive`
2. Install dependencies if needed (todo) probably a shell script that calls a python script. We'll do this on the next update

# Features
- **Sprite loading:** Add visual data into a file (`Sandbox/Assets/Data/VertexData.json`) 
- **Render ordering:** Placing backgrounds behind characters with the `layer` field in `VertexData.json`
- **Reloading scene:** Reload during engine runtime to save developer time with `ctrl+R`
- **Particle System:** Custom math expressions parsed and evaluated with an [evaluator](https://github.com/daniel10015/Math-Expression-Evaluator)

# Demos
todo

# Future Goals
- Keyframe animation for 2D
- manage vulkan memory with 3rd party memory manager 
- Abstract graphics APIs to support multiple during runtime
- Create an Editor 
- 3D renderer  
- GPU Particle System