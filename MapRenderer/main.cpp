#include <glad/glad.h>

#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "GeoJsonLoader.hpp"
#include "Geometry.hpp"
#include "RoutePlanner.hpp"

#include <iostream>
#include <filesystem>
#include <sstream>
#include <cmath>

using namespace std;

void framebufferCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

struct RouteSample {
    glm::vec2 position{};
    float heading = 0.0f;
};

RouteSample sampleRoute(const vector<glm::vec2>& route, float distance)
{
    float totalLength = 0.0f;
    for (size_t i = 1; i < route.size(); ++i) 
        totalLength += glm::length(route[i] - route[i - 1]);

    if (route.size() < 2 || totalLength <= 0.0f) 
        return {};

    float remaining = fmod(distance, totalLength * 2.0f);
    bool reverse = remaining > totalLength;
    if (reverse) 
        remaining = totalLength * 2.0f - remaining;

    for (size_t i = 1; i < route.size(); ++i) 
    {
        glm::vec2 delta = route[i] - route[i - 1];
        float segmentLength = glm::length(delta);

        if (remaining <= segmentLength) 
        {
            float t = segmentLength > 0.0f ? remaining / segmentLength : 0.0f;
            float heading = atan2(delta.y, delta.x) + (reverse ? glm::pi<float>() : 0.0f);
            return { glm::mix(route[i - 1], route[i], t), heading };
        }

        remaining -= segmentLength;
    }

    glm::vec2 delta = route.back() - route[route.size() - 2];
    return { route.back(), atan2(delta.y, delta.x) };
}

bool pointInPolygon(glm::vec2 p, const vector<glm::vec2>& polygon)
{
    bool inside = false;

    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        glm::vec2 a = polygon[i];
        glm::vec2 b = polygon[j];

        bool cross = (a.y > p.y) != (b.y > p.y);
        if (cross && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }

    return inside;
}

glm::vec2 mouseToGround(double mouseX, double mouseY, int width, int height, const glm::mat4& view, const glm::mat4& projection)
{
    float x = 2.0f * static_cast<float>(mouseX) / width - 1.0f;
    float y = 1.0f - 2.0f * static_cast<float>(mouseY) / height;

    glm::mat4 inverse = glm::inverse(projection * view);

    glm::vec4 nearPoint = inverse * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint = inverse * glm::vec4(x, y, 1.0f, 1.0f);

    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    glm::vec3 origin(nearPoint);
    glm::vec3 direction = glm::normalize(glm::vec3(farPoint - nearPoint));

    if (abs(direction.y) < 0.0001f) return { 0, 0 };

    float t = -origin.y / direction.y;
    glm::vec3 hit = origin + direction * t;

    return { hit.x, hit.z };
}

int pickBuilding(const MapData& mapData, glm::vec2 point)
{
    for (size_t i = 0; i < mapData.buildings.size(); ++i) 
    {
        if (pointInPolygon(point, mapData.buildings[i].polygon)) return static_cast<int>(i);
    }

    return -1;
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
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) return;

        Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        camera->processMouse(x, y);
        });

    glfwSetScrollCallback(window, [](GLFWwindow* window, double, double y)
        {
            Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
            camera->processScroll(y);
        });

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    cout << "Current path: " << filesystem::current_path() << '\n';
    cout << "GeoJSON exists: " << filesystem::exists("data/sample_map.geojson") << '\n';



    MapData mapData;
    TileManager tileManager;
    PlannedRoute planned;
    glm::vec2 startPosition(0.0f);

    try
    {
        mapData = GeoJsonLoader::load("../data/sample_map.geojson");

        cout << "Buildings: " << mapData.buildings.size() << '\n';
        cout << "Roads: " << mapData.roads.size() << '\n';

        tileManager.build(mapData);

        planned = RoutePlanner().plan(mapData, startPosition, mapData.buildings.size() - 1);

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
        float vehicleDistance = 0.0f;

        bool leftPressed = false;
        bool rightPressed = false;

        bool cullingEnabled = true;
        bool fPressed = false;

        bool followVehicle = true;
        bool tPressed = false;

        double statTime = glfwGetTime();
        double accumulatedTime = 0.0;
        int frameCount = 0;
        double averageMs = 0.0;

        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = (float)glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            vehicleDistance += 16.0f * deltaTime;
            RouteSample vehicle = sampleRoute(planned.roadPath, vehicleDistance);


            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            // KEY_F
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fPressed)
            {
                cullingEnabled = !cullingEnabled;
                fPressed = true;
            }

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
                fPressed = false;

            // KEY_T
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) 
            {
                followVehicle = !followVehicle;
                tPressed = true;
            }

            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) tPressed = false;

            bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

            if (rightDown && !rightPressed) 
            {
                camera.resetMouse();
                followVehicle = false;
            }

            rightPressed = rightDown;

            if (followVehicle) camera.follow(vehicle.position);
            else camera.processInput(window, deltaTime);

            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            float aspect = height == 0 ? 1.0f : (float)width / (float)height;

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), aspect, 0.1f, 1000.0f);

            FrameStats stats = renderer.draw(tileManager.getTiles(), view, projection, cullingEnabled, vehicle.position, vehicle.heading);

            bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (leftDown && !leftPressed) {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);

                glm::vec2 point = mouseToGround(mouseX, mouseY, width, height, view, projection);
                int building = pickBuilding(mapData, point);

                if (building >= 0) {
                    try {
                        planned = RoutePlanner().plan(mapData, startPosition, building);

                        renderer.uploadRoute(planned.roadPath, 3.2f);
                        renderer.uploadDestination(planned.destinationBuilding, planned.destinationHeight);

                        vehicleDistance = 0.0f;

                        cout << "Destination building: " << building << '\n';
                        cout << "Route points: " << planned.roadPath.size() << '\n';
                    }
                    catch (const exception& e) {
                        cout << "Route failed: " << e.what() << '\n';
                    }
                }
            }

            leftPressed = leftDown;

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
                << " | " << averageMs << " ms"
                << " | Follow " << (followVehicle ? "ON" : "OFF");

            glfwSetWindowTitle(window, title.str().c_str());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwTerminate();

    return 0;
}