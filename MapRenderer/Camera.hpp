#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera
{
public:
    Camera();

    void processInput(GLFWwindow* window, float deltaTime);
    void processMouse(double xpos, double ypos);
    void processScroll(double yoffset);

    glm::mat4 getViewMatrix() const;
    float getFov() const;
    void follow(glm::vec2 target);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;
    float fov;

    float lastX;
    float lastY;
    bool firstMouse;
};