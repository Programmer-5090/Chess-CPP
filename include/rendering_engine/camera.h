#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "input.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum class CameraProjection { Perspective, Orthographic };

// Default camera values
inline constexpr float YAW         = -90.0f;
inline constexpr float PITCH        =   0.0f;
inline constexpr float SPEED        =  10.5f;
inline constexpr float SENSITIVITY  =   0.1f;
inline constexpr float ZOOM         =  45.0f;
inline constexpr float ORTHO_HEIGHT =  50.0f;
inline constexpr float NEAR_PLANE   =   0.1f;
inline constexpr float FAR_PLANE    = 1000.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    CameraProjection ProjectionMode;
    float OrthoHeight;
    float NearPlane;
    float FarPlane;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);
    void ProcessInput(const Input& input, float deltaTime);
    void setPosition(glm::vec3 pos);

    void SetProjectionMode(CameraProjection mode) { ProjectionMode = mode; }
    void SetClipPlanes(float nearPlane, float farPlane) { NearPlane = nearPlane; FarPlane = farPlane; }
    void SetOrthoHeight(float height)                   { OrthoHeight = height; }

    CameraProjection GetProjectionMode() const { return ProjectionMode; }
    float GetNearPlane()   const { return NearPlane; }
    float GetFarPlane()    const { return FarPlane; }
    float GetOrthoHeight() const { return OrthoHeight; }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};

#endif