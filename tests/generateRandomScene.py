import random
import json

# Define your initial template
template_data = [
    {
        "id": 1,
        "position": [-10.0, -10.0],
        "size": [20.0, 20.0],
        "texture": "Assets/Textures/Sprites/grass_1.png",
        "textureDimensions": [0.0, 0.0, 20.0, 20.0],
        "color": [255.0, 255.0, 255.0, 0.0],
        "rotation": 0.0,
        "layer": 0
    },
    {
        "id": 2,
        "position": [3.0, 3.0],
        "size": [2.0, 2.0],
        "texture": "Assets/Textures/Sprites/D_Walk.png",
        "textureDimensions": [0.0, 0.0, 1.0, 1.0],
        "color": [255.0, 255.0, 255.0, 0.0],
        "rotation": 45.0,
        "layer": 1
    },
    {
        "id": 3,
        "position": [-3.0, -8.0],
        "size": [5.0, 5.0],
        "texture": "Assets/Textures/Sprites/S_Walk.png",
        "textureDimensions": [0.0, 0.0, 1.0, 1.0],
        "color": [255.0, 255.0, 255.0, 0.0],
        "rotation": 90.0,
        "layer": 2
    }
]

# Create the 10,000 quads
quads = []
quad_count = 500
grid_size = int(quad_count ** 0.5)
spacing = 1.0  # Spacing between quads in the grid

for i in range(quad_count):
    template = template_data[i % len(template_data)].copy()
    template['id'] = i + 1
    template['position'] = [
        (i % grid_size) * spacing,
        (i // grid_size) * spacing
    ]
    template['rotation'] = random.uniform(0, 360)
    quads.append(template)
with open('quads.json', 'w') as file:
    json.dump(quads, file, indent=2)

print("Quads have been written to 'quads.json'")
# Now `quads` contains the 10,000 quads with varied positions and rotations.
