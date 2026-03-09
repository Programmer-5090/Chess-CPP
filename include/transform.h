#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale    { 1.0f, 1.0f, 1.0f };

    // Rotation stored as quaternion (no gimbal lock)
    glm::quat rotation { glm::identity<glm::quat>() };

    // Translation
    void setPosition(float x, float y, float z)      { position = { x, y, z }; }
    void setPosition(const glm::vec3& p)             { position = p; }

    // Move by a delta in world space
    void translate(const glm::vec3& delta)           { position += delta; }
    void translate(float x, float y, float z)        { position += glm::vec3(x, y, z); }

    // Scale
    void setScale(float uniform)                     { scale = glm::vec3(uniform); }
    void setScale(float x, float y, float z)         { scale = { x, y, z }; }
    void setScale(const glm::vec3& s)                { scale = s; }

    // Multiply current scale by a factor
    void scaleBy(float factor)                       { scale *= factor; }
    void scaleBy(const glm::vec3& factors)           { scale *= factors; }

    // Rotation
    // Convenience: set rotation from Euler angles in radians (pitch=X, yaw=Y, roll=Z)
    void setEuler(float pitch, float yaw, float roll)
    {
        rotation = glm::quat(glm::vec3(pitch, yaw, roll));
    }
    void setEuler(const glm::vec3& pitchYawRoll)
    {
        rotation = glm::quat(pitchYawRoll);
    }

    // Apply an additional rotation on top of the current one
    void rotate(float angleDegrees, const glm::vec3& axis)
    {
        rotation = glm::normalize(
            glm::angleAxis(glm::radians(angleDegrees), glm::normalize(axis)) * rotation);
    }

    // Set rotation directly from a quaternion
    void setQuaternion(const glm::quat& q)           { rotation = glm::normalize(q); }

    // Smoothly interpolate rotation toward `target` by factor t ? [0,1]
    void slerpTo(const glm::quat& target, float t)
    {
        rotation = glm::slerp(rotation, target, t);
    }

    // Convenience: look toward a direction (world up = Y)
    void lookAt(const glm::vec3& direction,
                const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        glm::vec3 dir = glm::normalize(direction);
        if (glm::length(dir) < 1e-6f) return;
        rotation = glm::quatLookAt(dir, up);
    }

    // Reset
    void reset()
    {
        position = glm::vec3(0.0f);
        scale    = glm::vec3(1.0f);
        rotation = glm::identity<glm::quat>();
    }

    // Build the final model matrix: T * R * S 
    glm::mat4 matrix() const
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::toMat4(rotation);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
};
