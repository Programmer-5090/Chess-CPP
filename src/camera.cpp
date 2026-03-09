#include "camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM),
      ProjectionMode(CameraProjection::Perspective),
      OrthoHeight(ORTHO_HEIGHT), NearPlane(NEAR_PLANE), FarPlane(FAR_PLANE)
{
    Position = position;
    WorldUp  = up;
    Yaw      = yaw;
    Pitch    = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ,
               float upX,  float upY,  float upZ,
               float yaw,  float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM),
      ProjectionMode(CameraProjection::Perspective),
      OrthoHeight(ORTHO_HEIGHT), NearPlane(NEAR_PLANE), FarPlane(FAR_PLANE)
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp  = glm::vec3(upX, upY, upZ);
    Yaw      = yaw;
    Pitch    = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const
{
    if (ProjectionMode == CameraProjection::Orthographic)
    {
        float halfH = OrthoHeight * 0.5f;
        float halfW = halfH * aspect;
        return glm::ortho(-halfW, halfW, -halfH, halfH, NearPlane, FarPlane);
    }
    return glm::perspective(glm::radians(Zoom), aspect, NearPlane, FarPlane);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)  Position += Front * velocity;
    if (direction == BACKWARD) Position -= Front * velocity;
    if (direction == LEFT)     Position -= Right * velocity;
    if (direction == RIGHT)    Position += Right * velocity;
    if (direction == UP)       Position += Up    * velocity;
    if (direction == DOWN)     Position -= Up    * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch >  89.0f) Pitch =  89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    if (ProjectionMode == CameraProjection::Orthographic)
    {
        OrthoHeight -= yoffset;
        if (OrthoHeight <   1.0f) OrthoHeight =   1.0f;
        if (OrthoHeight > 500.0f) OrthoHeight = 500.0f;
        return;
    }

    Zoom -= yoffset;
    if (Zoom <  1.0f) Zoom =  1.0f;
    if (Zoom > 90.0f) Zoom = 90.0f;
}

void Camera::ProcessInput(const Input& input, float deltaTime)
{
    if (input.keyHeld("W")) ProcessKeyboard(FORWARD,  deltaTime);
    if (input.keyHeld("S")) ProcessKeyboard(BACKWARD, deltaTime);
    if (input.keyHeld("A")) ProcessKeyboard(LEFT,     deltaTime);
    if (input.keyHeld("D")) ProcessKeyboard(RIGHT,    deltaTime);
    if (input.keyHeld("E")) ProcessKeyboard(UP,       deltaTime);
    if (input.keyHeld("Q")) ProcessKeyboard(DOWN,     deltaTime);

    int dx = input.getMouseDeltaX();
    int dy = input.getMouseDeltaY();
    if (dx != 0 || dy != 0)
        ProcessMouseMovement(static_cast<float>(dx), static_cast<float>(-dy));
}

void Camera::setPosition(glm::vec3 pos)
{
    Position = pos;
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
