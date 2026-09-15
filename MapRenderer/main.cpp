#include <glad/glad.h>

#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "GeoJsonLoader.hpp"
#include "Geometry.hpp"

#include <iostream>
#include <filesystem>
#include <sstream>

using namespace std;

void framebufferCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Map Renderer", nullptr, nullptr);

    if (!window)
    {
        cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    Camera camera;

    glfwSetWindowUserPointer(window, &camera);

    glfwSetFramebufferSizeCallback(window, framebufferCallback);

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y)
        {
            Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
            camera->processMouse(x, y);
        });

    glfwSetScrollCallback(window, [](GLFWwindow* window, double, double y)
        {
            Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
            camera->processScroll(y);
        });

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    cout << "Current path: " << filesystem::current_path() << '\n';
    cout << "GeoJSON exists: " << filesystem::exists("data/map.geojson") << '\n';



    MapData mapData;
    TileManager tileManager;

    try
    {
        mapData = GeoJsonLoader::load("../data/sample_map.geojson");

        cout << "Buildings: " << mapData.buildings.size() << '\n';
        cout << "Roads: " << mapData.roads.size() << '\n';

        tileManager.build(mapData);

        for (const MapTile& tile : tileManager.getTiles())
        {
            cout << "Tile: " << tile.coordinate.x << ", " << tile.coordinate.y
                << " Buildings: " << tile.buildingCount
                << " Roads: " << tile.roadSegmentCount << '\n';
        }
    }
    catch (const exception& e)
    {
        cout << e.what() << '\n';
    }

    {
        Renderer renderer(tileManager.getTiles());

        float lastFrame = 0.0f;

        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = (float)glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            camera.processInput(window, deltaTime);

            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            float aspect = height == 0 ? 1.0f : (float)width / (float)height;

            glm::mat4 view = camera.getViewMatrix();

            glm::mat4 projection = glm::perspective(
                glm::radians(camera.getFov()),
                aspect,
                0.1f,
                1000.0f
            );

            FrameStats stats = renderer.draw(tileManager.getTiles(), view, projection);

            ostringstream title;
            title << "Map Renderer"
                << " | Tiles " << tileManager.getTiles().size()
                << " | Visible " << stats.visibleTiles
                << " | Draws " << stats.drawCalls
                << " | Vertices " << stats.renderedVertices;

            glfwSetWindowTitle(window, title.str().c_str());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwTerminate();

    return 0;
}