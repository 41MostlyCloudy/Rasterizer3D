# Rasterizer3D
A program to display 3D graphics

# About

#### - This is a project that I have been working on starting in the fall of 2023 when I wanted to learn how computer graphics work.
#### - Rasterizer3D displays a 3D model with various settings that can be enabled using the number keys at the top of the keyboard.
#### - The image still contains some artifacts around edges, and the general efficiency and organization of the code is something that I hope to improve later on down the line.
#### - Currently, the bloom and blur post-processing effects lower the framerate significantly.
#### - I eventually plan to add support for different models and textures, as well as more control over the enviroment, such as changing the direction of the lighting.
#### - I also made the model of the castle with the very well-textured rock.

# How to use

#### - Space to stop the model from spinning
#### - Up and down arrow keys to move up and down
#### - W and S keys to move forewards and backwards
#### - A and D keys to move to the side
#### - Left and right arrow keys to turn
#### - 8 and 2 on the key pad to look up or down
#### - 1: Toggle model visibility
#### - 2: Toggle wireframe overlay
#### - 3: Toggle Fog
#### - 4: Toggle directional lighting
#### - 5: Toggle directional light fixed position / fixed to camera
#### - 6: Toggle vertex colors
#### - 7: Toggle Texture visibility
#### - 8: Toggle bilinear texture filtering
#### - 9: Toggle bloom effect
#### - 0: Toggle depth of field blur

# Dependencies:
#### - stb_image.h
#### - OBJ_LOADER.h (modified to support vertex colors)
#### - OpenGL
#### - GLFW

# Requirements

#### - Windows computer
#### - Ability to instal OpenGL and GLFW
#### - To run the project, you must download the testImage.png and the testModel.png to the file the project is in.

# Copyright and Licensing

#### - All code is licensed under the MIT license unless otherwise specified. Please check the LICENCE.md file for more information.
