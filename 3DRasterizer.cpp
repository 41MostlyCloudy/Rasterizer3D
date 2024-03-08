#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION // Image loading library made by Sean Barrett.
#include "stb_image.h"
#include "OBJ_Loader.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono> // Deals with time

using namespace std;



// Color structure
struct RGBColor
{
    uint8_t r = 0;	uint8_t g = 0;	uint8_t b = 0;
};


// Color structure
struct RGBFloat
{
    float r = 0;	float g = 0;	float b = 0;
};


// Texture
struct Texture
{
    // 128 by 128 texture
    RGBColor px[16384];
};


// Bloom Texture
struct BloomTexture
{
    // 64 by 64 texture
    RGBFloat px[1024];
};


// 2D structure
struct UV
{
    float u = 0;
    float v = 0;
};


// 3D structure
struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;
};


// A point on a mesh with usual point data
struct Point
{
    Vector3 coord;
    RGBColor light;
    UV uv;
};


// a triangle structure
struct Triangle
{
	Point p[3]; // The position of each vertex
    uint8_t lighting = 255;
};


// A 3d object structure
struct Mesh
{
	vector<Triangle> tris; // List of triangles that make up the mesh. A vector is a resizable array.
};


// A 3d object structure
struct MeshInstance
{
    Mesh instanceMesh;
    Vector3 position;
    Vector3 rotation;
};


// flags
bool fillTris = true;
bool vertexColorEnabled = false;
bool shadeFlat = false;
bool spinModel = true;
bool faceLighting = true;
bool globalLightingFacingCamera = false;
bool fog = false;
bool applyTextureFilter = true;
bool wireframe = false;
bool bloom = false;
bool dofBlur = false;

// Important variables
RGBColor* screenColorData; // Screen data for player camera
float* depthBuffer;
float deltaT;// Multiply to get frame-independent speed.
float fov = 1;
float cameraNear = 1;
int screenResolution = 512;
Texture loadedTexture;
BloomTexture bloomTexture;
Vector3 globalLightPosition = { 4000, -1000, 1000 };
int fogDepth = 20;
int blurSize = 3;

// Resources
vector<Mesh> loadedMeshes;
vector<MeshInstance> loadedMeshInstances;

// Movement
Vector3 cameraPosition = { 0, -2, 30 };
Vector3 cameraRotation = { 0, 0, 0 };
Vector3 cameraVelocity = { 0, 0, 0 };
float cameraRotVelocity = 0;
float cameraRotXVelocity = 0;
float camRotX = 0;
    

// Runs everything
void RunEngine();
// Takes user input
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
// Loads objects and textures
void  LoadAssets();
// Updates physics, called every frame
void UpdatePhysics(float delta);
// Move a point
Vector3 Translate(Vector3 vect, Vector3 vect2);
// Clip a triangle
void ClipAndDraw(Triangle tri);
// Draw a triangle
void DrawTriangle(Triangle tri);
// Rotate a point
Vector3 Rotate(Vector3 vect, Vector3 rot);
// Find the normal of a triangle
float CalculateNormal(Triangle tri);
// Apply a bilinear filter
RGBColor Filter(float x, float y);
// Apply filter to bloom texture
RGBColor FilterBloom(float x, float y);
// Blur
void Blur(int x, int y);



// The actions performed when starting and running the engine
void RunEngine()
{
    
    // Load the meshes
    LoadAssets();


    glDisable(GL_DEPTH_TEST);


    // Initialize the library
    glfwInit();

    // Create a windowed mode window and its OpenGL context
    float width = glfwGetVideoMode(glfwGetPrimaryMonitor())->width;
    float height = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;

    // Sets to perfect screen resolution, (probably not the best performance)
    //screenResolution = height;
    screenResolution = 1024;
    //screenResolution = 256;
    
    // Update the screen data to the screen size
    screenColorData = new RGBColor[screenResolution * screenResolution];
    depthBuffer = new float[screenResolution * screenResolution];


    float windowRatio = height / width;

    GLFWwindow* window = glfwCreateWindow(width, height, "", glfwGetPrimaryMonitor(), nullptr);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Create Screen Texture
    unsigned int screenTex;
    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Start the delta timer
        std::chrono::high_resolution_clock time;
        auto start = time.now();

        for (int y = 0; y < (screenResolution * screenResolution); y++)
        {
            screenColorData[y] = { 0, 0, 0 };
            depthBuffer[y] = 0;
        }
        
        // Update game physics
        UpdatePhysics(deltaT);

        /////////////////////////////////////////////////////////////////////////// Drawing


        // Draw the triangles for each loaded mesh
        for (int i = 0; i < loadedMeshInstances.size(); i++)
        {
            for (int j = 0; j < loadedMeshInstances.at(i).instanceMesh.tris.size(); j++)
            {
                Triangle worldPoint = loadedMeshes.at(i).tris[j];

                

                worldPoint.p[0].coord = Rotate(worldPoint.p[0].coord, loadedMeshInstances.at(i).rotation);
                worldPoint.p[1].coord = Rotate(worldPoint.p[1].coord, loadedMeshInstances.at(i).rotation);
                worldPoint.p[2].coord = Rotate(worldPoint.p[2].coord, loadedMeshInstances.at(i).rotation);


                // Apply object transformations and rotations
                worldPoint.p[0].coord = Translate(worldPoint.p[0].coord, loadedMeshInstances.at(i).position);
                worldPoint.p[1].coord = Translate(worldPoint.p[1].coord, loadedMeshInstances.at(i).position);
                worldPoint.p[2].coord = Translate(worldPoint.p[2].coord, loadedMeshInstances.at(i).position);

                ////////////////////////////////////////////////////////////////////////////////////////////////////////
                worldPoint.p[0].coord = Translate(worldPoint.p[0].coord, globalLightPosition);
                worldPoint.p[1].coord = Translate(worldPoint.p[1].coord, globalLightPosition);
                worldPoint.p[2].coord = Translate(worldPoint.p[2].coord, globalLightPosition);

                if (!globalLightingFacingCamera)
                {
                    float lightingNormal = CalculateNormal(worldPoint);

                    worldPoint.lighting = (lightingNormal + 1) * 100;
                }

                worldPoint.p[0].coord = Translate(worldPoint.p[0].coord, { -globalLightPosition.x, -globalLightPosition.y, -globalLightPosition.z });
                worldPoint.p[1].coord = Translate(worldPoint.p[1].coord, { -globalLightPosition.x, -globalLightPosition.y, -globalLightPosition.z });
                worldPoint.p[2].coord = Translate(worldPoint.p[2].coord, { -globalLightPosition.x, -globalLightPosition.y, -globalLightPosition.z });
                /////////////////////////////////////////////////////////////////////////////////////////////////////////

                

                worldPoint.p[0].coord = Translate(worldPoint.p[0].coord, cameraPosition);
                worldPoint.p[1].coord = Translate(worldPoint.p[1].coord, cameraPosition);
                worldPoint.p[2].coord = Translate(worldPoint.p[2].coord, cameraPosition);

                

                float dotProduct = CalculateNormal(worldPoint);


                // Draw the projected triangle.
                if (dotProduct < 0)
                {     
                    // Rotate each triangle to match camera space, and then project it.

                    Vector3 rot = Rotate(cameraRotation, { 0, 0, camRotX });

                    worldPoint.p[0].coord = Rotate(worldPoint.p[0].coord, cameraRotation);
                    worldPoint.p[1].coord = Rotate(worldPoint.p[1].coord, cameraRotation);
                    worldPoint.p[2].coord = Rotate(worldPoint.p[2].coord, cameraRotation);

                    worldPoint.p[0].coord = Rotate(worldPoint.p[0].coord, { 0, 0, camRotX });
                    worldPoint.p[1].coord = Rotate(worldPoint.p[1].coord, { 0, 0, camRotX });
                    worldPoint.p[2].coord = Rotate(worldPoint.p[2].coord, { 0, 0, camRotX });
                    

                    if (faceLighting)
                    {
                        if (globalLightingFacingCamera)
                        {
                            worldPoint.lighting = (dotProduct + 1) * 100;
                        }
                    }

                    ClipAndDraw(worldPoint);
                }
            }
        }


        if (bloom)
        {
            for (int i = 0; i < 1024; i++)
            {
                bloomTexture.px[i] = { 0, 0, 0 };
            }

            for (int y = 0; y < screenResolution; y++)
            {
                for (int x = 0; x < screenResolution; x++)
                {
                    bloomTexture.px[(int(float(y) / screenResolution * 32) * 32) + int(float(x) / screenResolution * 32)].r += float(screenColorData[x + y * screenResolution].r) * 0.001;
                    bloomTexture.px[(int(float(y) / screenResolution * 32) * 32) + int(float(x) / screenResolution * 32)].g += float(screenColorData[x + y * screenResolution].g) * 0.001;
                    bloomTexture.px[(int(float(y) / screenResolution * 32) * 32) + int(float(x) / screenResolution * 32)].b += float(screenColorData[x + y * screenResolution].b) * 0.001;
                }
            }

            for (int y = 0; y < screenResolution; y++)
            {
                for (int x = 0; x < screenResolution; x++)
                {
                    RGBColor blur = FilterBloom((float(x) / screenResolution) * 32, (float(y) / screenResolution) * 32);
                    
                    if (screenColorData[x + y * screenResolution].r + blur.r < 255)
                        screenColorData[x + y * screenResolution].r += blur.r;
                    else
                        screenColorData[x + y * screenResolution].r = 255;
                    if (screenColorData[x + y * screenResolution].g + blur.g < 255)
                        screenColorData[x + y * screenResolution].g += blur.g;
                    else
                        screenColorData[x + y * screenResolution].g = 255;
                    if (screenColorData[x + y * screenResolution].b + blur.b < 255)
                        screenColorData[x + y * screenResolution].b += blur.b;
                    else
                        screenColorData[x + y * screenResolution].b = 255;
                }
            }
        }

        // Apply depth of field blur
        if (dofBlur)
        {
            for (int y = 0; y < screenResolution; y++)
            {
                for (int x = 0; x < screenResolution; x++)
                {
                    Blur(x, y);
                }
            }
        }
        
        
        ///////////////////////////////////////////////////////////////////////////

        // Create window quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 1.0); glVertex3f(-windowRatio, -1.0f, 0.0f);
        glTexCoord2f(0.0, 0.0); glVertex3f(-windowRatio, 1.0f, 0.0f);
        glTexCoord2f(1.0, 0.0); glVertex3f(windowRatio, 1.0f, 0.0f);
        glTexCoord2f(1.0, 1.0); glVertex3f(windowRatio, -1.0f, 0.0f);
        glEnd();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenResolution, screenResolution, 0, GL_RGB, GL_UNSIGNED_BYTE, screenColorData);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();


        // Process player input
        processInput(window);

        

        // Find the frame time
        auto end = time.now();
        using ms = std::chrono::duration<float, std::milli>;
        deltaT = std::chrono::duration_cast<ms>(end - start).count();
    }

    glfwTerminate();
}



void UpdatePhysics(float delta)
{
    Vector3 rotatedVelocity = cameraVelocity;

    rotatedVelocity = Rotate(rotatedVelocity, { 0, -cameraRotation.y, 0 });

    cameraPosition.x += rotatedVelocity.x * 0.03 * delta;
    cameraPosition.y += rotatedVelocity.y * 0.03 * delta;
    cameraPosition.z += rotatedVelocity.z * 0.03 * delta;


    cameraRotation.y += cameraRotVelocity * 0.08 * delta;
    camRotX += cameraRotXVelocity * 0.08 * delta;
    

    // Set rotation to wrap
    if (cameraRotation.x > 6.283185)
        cameraRotation.x -= 6.283185;
    if (cameraRotation.x < 0)
        cameraRotation.x += 6.283185;
    if (cameraRotation.y > 6.283185)
        cameraRotation.y -= 6.283185;
    if (cameraRotation.y < 0)
        cameraRotation.y += 6.283185;


    // Spin the model
    if (spinModel)
    {
        loadedMeshInstances[0].rotation.y += 0.0005 * delta;
        if (loadedMeshInstances[0].rotation.y > 6.283185)
            loadedMeshInstances[0].rotation.y -= 6.283185;
    }
}



void Blur(int x, int y)
{
    // Only blur far pixels
    if (depthBuffer[x + y * screenResolution] < 0.037)
    {
        // The number of pixels that will be blurred together
        float blurredPixels = 0;
        float combinedR = 0;
        float combinedG = 0;
        float combinedB = 0;

        for (int i = -blurSize; i <= blurSize; i++)
        {
            for (int j = -blurSize; j <= blurSize; j++)
            {
                if ((i + y >= 0) && (i + y < screenResolution) && (j + x >= 0) && (j + x < screenResolution) && (depthBuffer[x + j + (y + i) * screenResolution] < 0.037))
                {

                    float blurAmount = (blurSize - abs(float(i) / blurSize)) * (blurSize - abs(float(j) / blurSize));
                    combinedR += screenColorData[x + j + (y + i) * screenResolution].r * blurAmount;
                    combinedG += screenColorData[x + j + (y + i) * screenResolution].g * blurAmount;
                    combinedB += screenColorData[x + j + (y + i) * screenResolution].b * blurAmount;

                    blurredPixels += blurAmount;
                }
            }
        }

        float blurDiv = 1 / blurredPixels;

        combinedR *= blurDiv;
        combinedG *= blurDiv;
        combinedB *= blurDiv;

        // Add the colors together
        screenColorData[x + y * screenResolution] = { uint8_t(combinedR), uint8_t(combinedG), uint8_t(combinedB) };

        return;
    }
    else
        return;
}



RGBColor Filter(float x, float y)
{
    // Wrap the coordinates on the texture for tiling


    // find the distance the point is between pixels so add weight to each sample for filtering
    // Use the wrapped coordinates to find the correct pixel on the texture

    RGBColor centerSample = loadedTexture.px[int(x) + (int(y) * 128)];
    if (centerSample.r == 255 && centerSample.g == 0 && centerSample.b == 255)
        return centerSample;

    // Find the pixels around the sampled pixel
    x -= 0.5;
    y -= 0.5;

    RGBColor sample1 = loadedTexture.px[int(x) + (int(y) * 128)];
    RGBColor sample2 = loadedTexture.px[int(x+1) + (int(y) * 128)];
    RGBColor sample3 = loadedTexture.px[int(x) + (int(y+1) * 128)];
    RGBColor sample4 = loadedTexture.px[int(x+1) + (int(y+1) * 128)];

    float offset_x = (x - int(x));
    float offset_y = (y - int(y));
    float totalOffset = offset_x * offset_y;

    if (sample1.r == 255 && sample1.g == 0 && sample1.b == 255)
        sample1 = centerSample;
    if (sample2.r == 255 && sample2.g == 0 && sample2.b == 255)
        sample2 = centerSample;
    if (sample3.r == 255 && sample3.g == 0 && sample3.b == 255)
        sample3 = centerSample;
    if (sample4.r == 255 && sample4.g == 0 && sample4.b == 255)
        sample4 = centerSample;

    sample1.r *= (1 - offset_x - offset_y + totalOffset);
    sample1.g *= (1 - offset_x - offset_y + totalOffset);
    sample1.b *= (1 - offset_x - offset_y + totalOffset);

    sample2.r *= offset_x - totalOffset;
    sample2.g *= offset_x - totalOffset;
    sample2.b *= offset_x - totalOffset;

    sample3.r *= offset_y - totalOffset;
    sample3.g *= offset_y - totalOffset;
    sample3.b *= offset_y - totalOffset;

    sample4.r *= totalOffset;
    sample4.g *= totalOffset;
    sample4.b *= totalOffset;

    // Add the colors together
    return { uint8_t(sample1.r + sample2.r + sample3.r + sample4.r), uint8_t(sample1.g + sample2.g + sample3.g + sample4.g), uint8_t(sample1.b + sample2.b + sample3.b + sample4.b) };
}


RGBColor FilterBloom(float x, float y)
{
    // Wrap the coordinates on the texture for tiling


    // find the distance the point is between pixels so add weight to each sample for filtering
    // Use the wrapped coordinates to find the correct pixel on the texture


    // Find the pixels around the sampled pixel
    x -= 0.5;
    y -= 0.5;

    int roundDownX = (x);
    int roundDownY = (y);
    int roundUpX = (x + 1);
    int roundUpY = (y + 1);

    if (roundUpX >= 32)
        roundUpX = 31;
    if (roundUpY >= 32)
        roundUpY = 31;
    if (roundDownX < 0)
        roundDownX = 0;
    if (roundDownY < 0)
        roundDownY = 0;

    RGBFloat sample1 = bloomTexture.px[roundDownX + (roundDownY * 32)];
    RGBFloat sample2 = bloomTexture.px[roundUpX + (roundDownY * 32)];
    RGBFloat sample3 = bloomTexture.px[roundDownX + (roundUpY * 32)];
    RGBFloat sample4 = bloomTexture.px[roundUpX + (roundUpY * 32)];

    float offset_x = (x - roundDownX);
    float offset_y = (y - roundDownY);
    float totalOffset = offset_x * offset_y;

    sample1.r *= (1 - offset_x - offset_y + totalOffset);
    sample1.g *= (1 - offset_x - offset_y + totalOffset);
    sample1.b *= (1 - offset_x - offset_y + totalOffset);

    sample2.r *= offset_x - totalOffset;
    sample2.g *= offset_x - totalOffset;
    sample2.b *= offset_x - totalOffset;

    sample3.r *= offset_y - totalOffset;
    sample3.g *= offset_y - totalOffset;
    sample3.b *= offset_y - totalOffset;

    sample4.r *= totalOffset;
    sample4.g *= totalOffset;
    sample4.b *= totalOffset;

    // Add the colors together
    return { uint8_t(sample1.r + sample2.r + sample3.r + sample4.r), uint8_t(sample1.g + sample2.g + sample3.g + sample4.g), uint8_t(sample1.b + sample2.b + sample3.b + sample4.b) };
}


float CalculateNormal(Triangle tri)
{
    // Use Cross Product formula to find the normal of the triangle. Only draw triangles with a normal facing the camera

    Vector3 normal;
    normal.x = ((tri.p[1].coord.y - tri.p[0].coord.y) * (tri.p[2].coord.z - tri.p[0].coord.z)) - ((tri.p[1].coord.z - tri.p[0].coord.z) * (tri.p[2].coord.y - tri.p[0].coord.y));
    normal.y = ((tri.p[1].coord.z - tri.p[0].coord.z) * (tri.p[2].coord.x - tri.p[0].coord.x)) - ((tri.p[1].coord.x - tri.p[0].coord.x) * (tri.p[2].coord.z - tri.p[0].coord.z));
    normal.z = ((tri.p[1].coord.x - tri.p[0].coord.x) * (tri.p[2].coord.y - tri.p[0].coord.y)) - ((tri.p[1].coord.y - tri.p[0].coord.y) * (tri.p[2].coord.x - tri.p[0].coord.x));

    // Normalize the face's normal vector

    float vecLength = 1 / sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    normal.x *= vecLength;
    normal.y *= vecLength;
    normal.z *= vecLength;

    Vector3 vec3;
    float vecLength2 = 1 / sqrt(tri.p[0].coord.x * tri.p[0].coord.x + tri.p[0].coord.y * tri.p[0].coord.y + tri.p[0].coord.z * tri.p[0].coord.z);
    vec3.x = tri.p[0].coord.x * vecLength2;
    vec3.y = tri.p[0].coord.y * vecLength2;
    vec3.z = tri.p[0].coord.z * vecLength2;

    // Return the dot product
    return (normal.x * vec3.x) + (normal.y * vec3.y) + (normal.z * vec3.z);
}



void LoadAssets()
{
    // Load textures
    int x, y, comps;
    unsigned char* texData = stbi_load("testTexture.png", &x, &y, &comps, 3);

    //Texture addTexture;

    if (texData)
    {
        for (int i = 0; i < 16384; i++)
        {
            unsigned char* pixelOffset = texData + (i) * 3;

            RGBColor pixel = { pixelOffset[0], pixelOffset[1], pixelOffset[2] };

            loadedTexture.px[(127 - i / 128) * 128 + (i % 128)] = pixel;
        }
    }
    
    stbi_image_free(texData);


    // Initialize Loader
    objl::Loader Loader;

    // Load .obj File
    bool isLoaded = Loader.LoadFile("testModel.obj");

    if (isLoaded)
    {
        for (int i = 0; i < Loader.LoadedMeshes.size(); i++)
        {
            objl::MeshData currentMesh = Loader.LoadedMeshes[i];
            Mesh newMesh;

            for (int j = 0; j < currentMesh.Indices.size(); j += 3)
            {
                Triangle newTri;
                newTri.p[0].coord.x = currentMesh.Vertices[currentMesh.Indices[j]].Position.X;
                newTri.p[0].coord.y = currentMesh.Vertices[currentMesh.Indices[j]].Position.Y;
                newTri.p[0].coord.z = currentMesh.Vertices[currentMesh.Indices[j]].Position.Z;
                newTri.p[1].coord.x = currentMesh.Vertices[currentMesh.Indices[j + 1]].Position.X;
                newTri.p[1].coord.y = currentMesh.Vertices[currentMesh.Indices[j + 1]].Position.Y;
                newTri.p[1].coord.z = currentMesh.Vertices[currentMesh.Indices[j + 1]].Position.Z;
                newTri.p[2].coord.x = currentMesh.Vertices[currentMesh.Indices[j + 2]].Position.X;
                newTri.p[2].coord.y = currentMesh.Vertices[currentMesh.Indices[j + 2]].Position.Y;
                newTri.p[2].coord.z = currentMesh.Vertices[currentMesh.Indices[j + 2]].Position.Z;

                newTri.p[0].uv.u = currentMesh.Vertices[currentMesh.Indices[j]].TextureCoordinate.X;
                newTri.p[0].uv.v = currentMesh.Vertices[currentMesh.Indices[j]].TextureCoordinate.Y;
                newTri.p[1].uv.u = currentMesh.Vertices[currentMesh.Indices[j + 1]].TextureCoordinate.X;
                newTri.p[1].uv.v = currentMesh.Vertices[currentMesh.Indices[j + 1]].TextureCoordinate.Y;
                newTri.p[2].uv.u = currentMesh.Vertices[currentMesh.Indices[j + 2]].TextureCoordinate.X;
                newTri.p[2].uv.v = currentMesh.Vertices[currentMesh.Indices[j + 2]].TextureCoordinate.Y;

                newTri.p[0].light.r = 255 - currentMesh.Vertices[currentMesh.Indices[j]].Color.X * 255;
                newTri.p[0].light.g = 255 - currentMesh.Vertices[currentMesh.Indices[j]].Color.Y * 255;
                newTri.p[0].light.b = 255 - currentMesh.Vertices[currentMesh.Indices[j]].Color.Z * 255;
                newTri.p[1].light.r = 255 - currentMesh.Vertices[currentMesh.Indices[j + 1]].Color.X * 255;
                newTri.p[1].light.g = 255 - currentMesh.Vertices[currentMesh.Indices[j + 1]].Color.Y * 255;
                newTri.p[1].light.b = 255 - currentMesh.Vertices[currentMesh.Indices[j + 1]].Color.Z * 255;
                newTri.p[2].light.r = 255 - currentMesh.Vertices[currentMesh.Indices[j + 2]].Color.X * 255;
                newTri.p[2].light.g = 255 - currentMesh.Vertices[currentMesh.Indices[j + 2]].Color.Y * 255;
                newTri.p[2].light.b = 255 - currentMesh.Vertices[currentMesh.Indices[j + 2]].Color.Z * 255;

                newMesh.tris.emplace_back(newTri);
            }

            loadedMeshes.emplace_back(newMesh);
        }
    }
    
    // Make one instance of each mesh for now
    for (int i = 0; i < 1; i++)
    {
        MeshInstance newInstance;
        newInstance.instanceMesh = loadedMeshes[i];
        loadedMeshInstances.emplace_back(newInstance);
    }
}


Vector3 Translate(Vector3 vect, Vector3 vect2)
{
    vect.x += vect2.x;
    vect.y += vect2.y;
    vect.z += vect2.z;

    return vect;
}


Vector3 Rotate(Vector3 vect, Vector3 rot)
{
    Vector3 returnVect;

    returnVect.x = vect.x * (cos(rot.y) * cos(rot.x)) +
        vect.y * (sin(rot.z) * sin(rot.y) * cos(rot.x) - cos(rot.z) * sin(rot.x)) +
        vect.z * (cos(rot.z) * sin(rot.y) * cos(rot.x) + sin(rot.z) * sin((rot.x)));
    
    returnVect.y = vect.x * (cos(rot.y) * sin(rot.x)) +
        vect.y * (sin(rot.z) * sin(rot.y) * sin(rot.x) + cos(rot.z) * cos(rot.x)) +
        vect.z * (cos(rot.z) * sin(rot.y) * sin(rot.x) - sin(rot.z) * cos((rot.x)));

    returnVect.z = vect.x * (-sin(rot.y)) +
        vect.y * (sin(rot.z) * cos(rot.y)) +
        vect.z * (cos(rot.z) * cos(rot.y));

    return returnVect;
}


void ClipAndDraw(Triangle tri)
{
    // Clips triangles against one plane at a time.
    // Each time it is clipped, clip each of the resulting triangles against the next plane.


    // Clip near
    vector <Triangle> newTris1; // Create and the new triangles generated from clipping
    vector <Point> newPoints1; // Clipped points from which new triangles will be constructed

    for (int p1 = 0; p1 < 3; p1++)
    {
        // If the point is inside screen, include it in newPoints.
        if (tri.p[p1].coord.z >= cameraNear)
            newPoints1.emplace_back(tri.p[p1]);

        int p2 = p1 + 1;
        if (p2 > 2)
            p2 = 0;

        Point point1 = tri.p[p1];
        Point point2 = tri.p[p2];

        if (point1.coord.z > point2.coord.z) // p1 has the smaller y.
            swap(point1, point2);

        // Z intersection
        if (point1.coord.z < cameraNear && point2.coord.z > cameraNear) // Line has points on either side of edge
        {
            float a = -(point1.coord.z - cameraNear) / ((point2.coord.z - cameraNear) - (point1.coord.z - cameraNear));

            Point newP;

            newP.coord.x = point2.coord.x * a + point1.coord.x * (1 - a);
            newP.coord.y = point2.coord.y * a + point1.coord.y * (1 - a);
            newP.coord.z = cameraNear;

            newP.uv.u = point2.uv.u * a + point1.uv.u * (1 - a);
            newP.uv.v = point2.uv.v * a + point1.uv.v * (1 - a);

            newP.light.r = point2.light.r * a + point1.light.r * (1 - a);
            newP.light.g = point2.light.g * a + point1.light.g * (1 - a);
            newP.light.b = point2.light.b * a + point1.light.b * (1 - a);

            newPoints1.emplace_back(newP);
        }
    }

    for (int i = 0; i < newPoints1.size(); i++)
    {
        newPoints1[i].coord.x = newPoints1[i].coord.x / (newPoints1[i].coord.z * fov) + 0.5;
        newPoints1[i].coord.y = -newPoints1[i].coord.y / (newPoints1[i].coord.z * fov) + 0.5;
        newPoints1[i].coord.z = 1 / newPoints1[i].coord.z;
    }

    if (newPoints1.size() > 2)
    {
        for (int i = 0; i < newPoints1.size() - 2; i++)
        {
            if (!((newPoints1[0].coord.x < 0 && newPoints1[i + 1].coord.x < 0 && newPoints1[i + 2].coord.x < 0) ||
                (newPoints1[0].coord.x > 1 && newPoints1[i + 1].coord.x > 1 && newPoints1[i + 2].coord.x > 1) ||
                (newPoints1[0].coord.y < 0 && newPoints1[i + 1].coord.y < 0 && newPoints1[i + 2].coord.y < 0) ||
                (newPoints1[0].coord.y > 1 && newPoints1[i + 1].coord.y > 1 && newPoints1[i + 2].coord.y > 1)))
            {
                Triangle newTri = { newPoints1[0], newPoints1[i + 1], newPoints1[i + 2] };
                newTri.lighting = tri.lighting;
                newTris1.emplace_back(newTri);
            }
        }
    }
    else
        return; // Draw nothing
    

    for (int i = 0; i < newTris1.size(); i++)
    {
        DrawTriangle(newTris1[i]);
    }
}


void DrawTriangle(Triangle tri)
{
    // The three points for the triangle
    Point p1 = tri.p[0];
    Point p2 = tri.p[1];
    Point p3 = tri.p[2];

    // Project the points into 2d space
    p1.coord.x *= screenResolution;
    p1.coord.y *= screenResolution;
    p2.coord.x *= screenResolution;
    p2.coord.y *= screenResolution;
    p3.coord.x *= screenResolution;
    p3.coord.y *= screenResolution;


    // Sort the points. The topmost point is first, then middle, then bottommost.
    if (p3.coord.y < p1.coord.y)
        swap(p3, p1);
    if (p3.coord.y < p2.coord.y)
        swap(p3, p2);
    if (p2.coord.y < p1.coord.y)
        swap(p2, p1);   


    // The change in x for every change in y.
    // Left slope is from 0-1, right is from 1-2, other is from 0-2
    float leftSlope = 1000;
    float rightSlope = 1000;

    if (int(p3.coord.y) - int(p1.coord.y) != 0)
        leftSlope = (p1.coord.x - p3.coord.x) / (int(p3.coord.y) - int(p1.coord.y));
    if (int(p2.coord.y) - int(p1.coord.y) != 0)
        rightSlope = (p1.coord.x - p2.coord.x) / float(int(p2.coord.y) - int(p1.coord.y));

    // Set the start and end of the scan lines
    float scanStart = p1.coord.x;
    float scanEnd = scanStart;


    if (leftSlope < rightSlope)
        swap(leftSlope, rightSlope);


    // Draw the pixels along the scanline from top to middle of the triangle.
    // Change the start and end by the left and right slopes.
    for (int i = p1.coord.y; i < int(p2.coord.y); i++)
    {
        if (i < screenResolution && i >= 0)
        {
            for (int j = scanStart; j < scanEnd; j++)
            {
                if (j < screenResolution && j >= 0)
                {
                    ////////////////////////////////////////////////////////////////////////////////////////////////// PERSPECTIVE CORRECTION
                    // Move the points into a normalized space to make multiplying them for depth simpler.

                    Point a1;
                    Point a2;
                    Point a3;

                    a1.coord.x = (p1.coord.x - float(j)) / p1.coord.z;
                    a1.coord.y = (p1.coord.y - float(i)) / p1.coord.z;
                    a2.coord.x = (p2.coord.x - float(j)) / p2.coord.z;
                    a2.coord.y = (p2.coord.y - float(i)) / p2.coord.z;
                    a3.coord.x = (p3.coord.x - float(j)) / p3.coord.z;
                    a3.coord.y = (p3.coord.y - float(i)) / p3.coord.z;


                    // vertex weights must be more the further they are from the camera.
                    // This means that they pull the colors and textures towards themselves.
                    // This is done by making them closer to the center (0, 0).
                    // These modified points are only used to find the interpolated texture coordinates, the screen space ones are used in deciding where to place the pixel.

                    float denominator = ((a2.coord.y - a3.coord.y) * (a1.coord.x - a3.coord.x) + (a3.coord.x - a2.coord.x) * (a1.coord.y - a3.coord.y));
                    if (denominator != 0)
                        denominator = 1 / denominator;

                    float p1Weight = ((a2.coord.y - a3.coord.y) * -a3.coord.x + (a3.coord.x - a2.coord.x) * -a3.coord.y) * denominator;
                    float p2Weight = ((a3.coord.y - a1.coord.y) * -a3.coord.x + (a1.coord.x - a3.coord.x) * -a3.coord.y) * denominator;
                    float p3Weight = 1 - p1Weight - p2Weight;


                    
                    //////////////////////////////////////////////////////////////////////////////////////////////////

                    float depth = (p1.coord.z * p1Weight) + (p2.coord.z * p2Weight) + (p3.coord.z * p3Weight);

                    

                    if (depth > depthBuffer[(i * screenResolution) + j])
                    {
                        float weightedU = ((p1.uv.u * p1Weight) + (p2.uv.u * p2Weight) + (p3.uv.u * p3Weight)) * 128;
                        float weightedV = ((p1.uv.v * p1Weight) + (p2.uv.v * p2Weight) + (p3.uv.v * p3Weight)) * 128;

                        float colWeightR = (p1.light.r * p1Weight) + (p2.light.r * p2Weight) + (p3.light.r * p3Weight);
                        float colWeightG = (p1.light.g * p1Weight) + (p2.light.g * p2Weight) + (p3.light.g * p3Weight);
                        float colWeightB = (p1.light.b * p1Weight) + (p2.light.b * p2Weight) + (p3.light.b * p3Weight);


                        RGBColor vertexWeightedCol;

                        bool dontDraw = true;
                        
                        if (fillTris)
                        {
                            dontDraw = false;
                            if (shadeFlat)
                            {
                                vertexWeightedCol = { 255, 255, 255 };
                            }
                            else
                            {
                                if (weightedU > 127)
                                    weightedU = 127;
                                if (weightedU < 0)
                                    weightedU = 0;
                                if (weightedV > 127)
                                    weightedV = 127;
                                if (weightedV < 0)
                                    weightedV = 0;

                                if (applyTextureFilter)
                                {
                                    vertexWeightedCol = Filter(weightedU, weightedV);

                                    if (vertexWeightedCol.r == 255 && vertexWeightedCol.g == 0 && vertexWeightedCol.b == 255)
                                        dontDraw = true;
                                }
                                else
                                {
                                    vertexWeightedCol = loadedTexture.px[int(weightedU) + (int(weightedV) * 128)];

                                    if (vertexWeightedCol.r == 255 && vertexWeightedCol.g == 0 && vertexWeightedCol.b == 255)
                                        dontDraw = true;
                                }
                            }
                            if (vertexColorEnabled)
                            {
                                if (vertexWeightedCol.r - colWeightR > 0)
                                    vertexWeightedCol.r -= colWeightR;
                                else
                                    vertexWeightedCol.r = 0;
                                if (vertexWeightedCol.g - colWeightG > 0)
                                    vertexWeightedCol.g -= colWeightG;
                                else
                                    vertexWeightedCol.g = 0;
                                if (vertexWeightedCol.b - colWeightB > 0)
                                    vertexWeightedCol.b -= colWeightB;
                                else
                                    vertexWeightedCol.b = 0;
                            }
                            if (faceLighting)
                            {
                                if (vertexWeightedCol.r - tri.lighting > 0)
                                    vertexWeightedCol.r -= tri.lighting;
                                else
                                    vertexWeightedCol.r = 0;
                                if (vertexWeightedCol.g - tri.lighting > 0)
                                    vertexWeightedCol.g -= tri.lighting;
                                else
                                    vertexWeightedCol.g = 0;
                                if (vertexWeightedCol.b - tri.lighting > 0)
                                    vertexWeightedCol.b -= tri.lighting;
                                else
                                    vertexWeightedCol.b = 0;
                            }
                            if (fog)
                            {
                                if (1 / depth > 20)
                                {
                                    if (vertexWeightedCol.r - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.r -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.r = 0;
                                    if (vertexWeightedCol.g - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.g -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.g = 0;
                                    if (vertexWeightedCol.b - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.b -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.b = 0;
                                }
                            }
                        }

                        if (wireframe)
                        {
                            if (i - 4 < p1.coord.y || j - 2 < scanStart || j + 2 > scanEnd)
                            {
                                vertexWeightedCol = { 190, 190, 190 };
                                dontDraw = false;
                            }
                        }

                        if (!dontDraw)
                        {
                            screenColorData[(i * screenResolution) + j] = vertexWeightedCol;
                            depthBuffer[(i * screenResolution) + j] = depth;
                        }
                    }
                }
                else if (j < 0)
                    j = -1;
                else if (j > screenResolution)
                    j = scanEnd;
            }
            scanStart -= leftSlope;
            scanEnd -= rightSlope;
        }
        else if (i < 0)
        {
            if (p2.coord.y < 0)
            {
                i = p2.coord.y;
                scanStart -= leftSlope * -(int(p1.coord.y) - int(p2.coord.y));
                scanEnd -= rightSlope * -(int(p1.coord.y) - int(p2.coord.y));
            }
            else
            {
                i = -1;
                scanStart -= leftSlope * -int(p1.coord.y);
                scanEnd -= rightSlope * -int(p1.coord.y);
            }
        }
        else
        {
            return;
        }
        
    }

    if ((p3.coord.y - p1.coord.y) != 0)
        leftSlope = (p1.coord.x - p3.coord.x) / (p3.coord.y - p1.coord.y);
    else
        leftSlope = 1000;
    if ((p3.coord.y - p2.coord.y) != 0)
        rightSlope = (p2.coord.x - p3.coord.x) / (p3.coord.y - p2.coord.y);
    else
        leftSlope = 1000;
    

    if (leftSlope > rightSlope)
        swap(leftSlope, rightSlope);

    if (int(p1.coord.y) == int(p2.coord.y))
    {
        if (p1.coord.x > p2.coord.x)
            scanStart = p2.coord.x;
        else
            scanEnd = p2.coord.x;
    }

    // Draw the pixels along the scanline from top to middle of the triangle.
    // Change the start and end by the left and right slopes.
    for (int i = p2.coord.y; i < int(p3.coord.y); i++)
    {
        if (i < screenResolution && i >= 0)
        {
            for (int j = scanStart; j < scanEnd; j++)
            {
                if (j < screenResolution && j >= 0)
                {
                    ////////////////////////////////////////////////////////////////////////////////////////////////// PERSPECTIVE CORRECTION
                    // Move the points into a normalized space to make multiplying them for depth simpler.

                    Point a1;
                    Point a2;
                    Point a3;

                    a1.coord.x = (p1.coord.x - float(j)) / p1.coord.z;
                    a1.coord.y = (p1.coord.y - float(i)) / p1.coord.z;
                    a2.coord.x = (p2.coord.x - float(j)) / p2.coord.z;
                    a2.coord.y = (p2.coord.y - float(i)) / p2.coord.z;
                    a3.coord.x = (p3.coord.x - float(j)) / p3.coord.z;
                    a3.coord.y = (p3.coord.y - float(i)) / p3.coord.z;


                    // vertex weights must be more the further they are from the camera.
                    // This means that they pull the colors and textures towards themselves.
                    // This is done by making them closer to the center (0, 0).
                    // These modified points are only used to find the interpolated texture coordinates, the screen space ones are used in deciding where to place the pixel.

                    float denominator = ((a2.coord.y - a3.coord.y) * (a1.coord.x - a3.coord.x) + (a3.coord.x - a2.coord.x) * (a1.coord.y - a3.coord.y));
                    if (denominator != 0)
                        denominator = 1 / denominator;

                    float p1Weight = ((a2.coord.y - a3.coord.y) * -a3.coord.x + (a3.coord.x - a2.coord.x) * -a3.coord.y) * denominator;
                    float p2Weight = ((a3.coord.y - a1.coord.y) * -a3.coord.x + (a1.coord.x - a3.coord.x) * -a3.coord.y) * denominator;
                    float p3Weight = 1 - p1Weight - p2Weight;



                    //////////////////////////////////////////////////////////////////////////////////////////////////

                    float depth = (p1.coord.z * p1Weight) + (p2.coord.z * p2Weight) + (p3.coord.z * p3Weight);


                    if (depth > depthBuffer[(i * screenResolution) + j])
                    {
                        float colWeightR = (p1.light.r * p1Weight) + (p2.light.r * p2Weight) + (p3.light.r * p3Weight);
                        float colWeightG = (p1.light.g * p1Weight) + (p2.light.g * p2Weight) + (p3.light.g * p3Weight);
                        float colWeightB = (p1.light.b * p1Weight) + (p2.light.b * p2Weight) + (p3.light.b * p3Weight);

                        float weightedU = ((p1.uv.u * p1Weight) + (p2.uv.u * p2Weight) + (p3.uv.u * p3Weight)) * 128;
                        float weightedV = ((p1.uv.v * p1Weight) + (p2.uv.v * p2Weight) + (p3.uv.v * p3Weight)) * 128;

                        if (weightedU > 127)
                            weightedU = 127;
                        if (weightedU < 0)
                            weightedU = 0;
                        if (weightedV > 127)
                            weightedV = 127;
                        if (weightedV < 0)
                            weightedV = 0;

                        RGBColor vertexWeightedCol;

                        bool dontDraw = true;

                        if (fillTris)
                        {
                            dontDraw = false;
                            if (shadeFlat)
                            {
                                vertexWeightedCol = { 255, 255, 255 };
                            }
                            else
                            {
                                if (weightedU > 127)
                                    weightedU = 127;
                                if (weightedU < 0)
                                    weightedU = 0;
                                if (weightedV > 127)
                                    weightedV = 127;
                                if (weightedV < 0)
                                    weightedV = 0;

                                if (applyTextureFilter)
                                {
                                    vertexWeightedCol = Filter(weightedU, weightedV);

                                    if (vertexWeightedCol.r == 255 && vertexWeightedCol.g == 0 && vertexWeightedCol.b == 255)
                                        dontDraw = true;
                                }
                                else
                                {
                                    vertexWeightedCol = loadedTexture.px[int(weightedU) + (int(weightedV) * 128)];

                                    if (vertexWeightedCol.r == 255 && vertexWeightedCol.g == 0 && vertexWeightedCol.b == 255)
                                        dontDraw = true;
                                }
                            }
                            if (vertexColorEnabled)
                            {
                                if (vertexWeightedCol.r - colWeightR > 0)
                                    vertexWeightedCol.r -= colWeightR;
                                else
                                    vertexWeightedCol.r = 0;
                                if (vertexWeightedCol.g - colWeightG > 0)
                                    vertexWeightedCol.g -= colWeightG;
                                else
                                    vertexWeightedCol.g = 0;
                                if (vertexWeightedCol.b - colWeightB > 0)
                                    vertexWeightedCol.b -= colWeightB;
                                else
                                    vertexWeightedCol.b = 0;
                            }
                            if (faceLighting)
                            {
                                if (vertexWeightedCol.r - tri.lighting > 0)
                                    vertexWeightedCol.r -= tri.lighting;
                                else
                                    vertexWeightedCol.r = 0;
                                if (vertexWeightedCol.g - tri.lighting > 0)
                                    vertexWeightedCol.g -= tri.lighting;
                                else
                                    vertexWeightedCol.g = 0;
                                if (vertexWeightedCol.b - tri.lighting > 0)
                                    vertexWeightedCol.b -= tri.lighting;
                                else
                                    vertexWeightedCol.b = 0;
                            }
                            if (fog)
                            {
                                if (1 / depth > 20)
                                {
                                    if (vertexWeightedCol.r - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.r -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.r = 0;
                                    if (vertexWeightedCol.g - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.g -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.g = 0;
                                    if (vertexWeightedCol.b - ((1 / depth) - 20) * fogDepth > 0)
                                        vertexWeightedCol.b -= ((1 / depth) - 20) * fogDepth;
                                    else
                                        vertexWeightedCol.b = 0;
                                }
                            }
                        }

                        if (wireframe)
                        {
                            if (i - 4 < p1.coord.y || j - 2 < scanStart || j + 2 > scanEnd)
                            {
                                vertexWeightedCol = { 190, 190, 190 };
                                dontDraw = false;
                            }
                        }

                        if (!dontDraw)
                        {
                            screenColorData[(i * screenResolution) + j] = vertexWeightedCol;
                            depthBuffer[(i * screenResolution) + j] = depth;
                        }
                    }
                }
                else if (j < 0)
                    j = -1;
                else if (j > screenResolution)
                    j = scanEnd;
            }

            scanStart -= leftSlope;
            scanEnd -= rightSlope;
        }
        else if (i < 0)
        {
            if (p3.coord.y < 0)
                return;
            else
            {
                i = -1;
                scanStart -= leftSlope * -p2.coord.y;
                scanEnd -= rightSlope * -p2.coord.y;
            }
        }
        else
        {
            return;
        }
    }
}


void processInput(GLFWwindow* window)
{
    cameraVelocity.x = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
    cameraVelocity.y = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
    cameraVelocity.z = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    
    fov += 0.01 * ((glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS));

    cameraRotVelocity = 0.01 * ((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS));

    cameraRotXVelocity = 0.01 * ((glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS));

    glfwSetKeyCallback(window, key_callback);
}


// Checks for input the moment a key is pressed
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
           glfwTerminate();
        }

        if (key == GLFW_KEY_SPACE)
        {
            spinModel = !spinModel;
        }

        if (key == GLFW_KEY_1)
        {
            fillTris = !fillTris;
        }

        if (key == GLFW_KEY_2)
        {
            wireframe = !wireframe;
        }

        if (key == GLFW_KEY_3)
        {
            fog = !fog;
        }

        if (key == GLFW_KEY_4)
        {
            faceLighting = !faceLighting;
        }

        if (key == GLFW_KEY_5)
        {
            globalLightingFacingCamera = !globalLightingFacingCamera;
        }

        if (key == GLFW_KEY_6)
        {
            vertexColorEnabled = !vertexColorEnabled;
        }

        if (key == GLFW_KEY_7)
        {
            shadeFlat = !shadeFlat;
        }

        if (key == GLFW_KEY_8)
        {
           applyTextureFilter = !applyTextureFilter;
        }

        if (key == GLFW_KEY_9)
        {
            bloom = !bloom;
        }

        if (key == GLFW_KEY_0)
        {
            dofBlur = !dofBlur;
        }
    }
}


int main(void)
{
    RunEngine();

    return 0;
}
