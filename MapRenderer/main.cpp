#include <glad/glad.h>

#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "GeoJsonLoader.hpp"
#include "Geometry.hpp"
#include "RoutePlanner.hpp"

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
    PlannedRoute planned;

    try
    {
        mapData = GeoJsonLoader::load("../data/sample_map.geojson");

        cout << "Buildings: " << mapData.buildings.size() << '\n';
        cout << "Roads: " << mapData.roads.size() << '\n';

        tileManager.build(mapData);

        planned = RoutePlanner().plan(mapData, mapData.roads.front().points.front(), mapData.buildings.size() - 1);

        cout << "Route points: " << planned.roadPath.size() << '\n';
        cout << "Destination: " << planned.destinationBuilding.x << ", " << planned.destinationBuilding.y << '\n';

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
        renderer.uploadRoute(planned.roadPath, 3.2f);
        renderer.uploadDestination(planned.destinationBuilding, planned.destinationHeight);

        float lastFrame = 0.0f;

        bool cullingEnabled = true;
        bool fPressed = false;

        double statTime = glfwGetTime();
        double accumulatedTime = 0.0;
        int frameCount = 0;
        double averageMs = 0.0;

        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = (float)glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fPressed)
            {
                cullingEnabled = !cullingEnabled;
                fPressed = true;
            }

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
                fPressed = false;

            camera.processInput(window, deltaTime);

            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            float aspect = height == 0 ? 1.0f : (float)width / (float)height;

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), aspect, 0.1f, 1000.0f);

            FrameStats stats = renderer.draw(tileManager.getTiles(), view, projection, cullingEnabled);

            accumulatedTime += deltaTime;
            frameCount++;

            double now = glfwGetTime();

            if (now - statTime >= 0.5)
            {
                averageMs = accumulatedTime / frameCount * 1000.0;
                accumulatedTime = 0.0;
                frameCount = 0;
                statTime = now;
            }

            ostringstream title;

            title << "Map Renderer"
                << " | Culling " << (cullingEnabled ? "ON" : "OFF")
                << " | Tiles " << tileManager.getTiles().size()
                << " | Visible " << stats.visibleTiles
                << " | Draws " << stats.drawCalls
                << " | Vertices " << stats.renderedVertices
                << " | FPS " << (averageMs > 0.0 ? 1000.0 / averageMs : 0.0)
                << " | " << averageMs << " ms";

            glfwSetWindowTitle(window, title.str().c_str());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwTerminate();

    return 0;
}