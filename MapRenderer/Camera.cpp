#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera() : 
    position(0.0f, 0.0f, 3.0f),
    front(0.0f, 0.0f, -1.0f),
    up(0.0f, 1.0f, 0.0f),
    yaw(-90.0f),
    pitch(0.0f),
    fov(45.0f),
    lastX(400.0f),
    lastY(300.0f),
    firstMouse(true)
{
}

void Camera::processInput(GLFWwindow* window, float deltaTime)
{
    float speed = 3.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * speed;

    glm::vec3 right = glm::normalize(glm::cross(front, up));

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * speed;
}

void Camera::processMouse(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xOffset = (float)xpos - lastX;
    float yOffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;

    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(direction);
}

void Camera::processScroll(double yoffset)
{
    fov -= (float)yoffset;

    if (fov < 20.0f) fov = 20.0f;
    if (fov > 80.0f) fov = 80.0f;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

float Camera::getFov() const
{
    return fov;
}