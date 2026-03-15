#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "input.h"
#include "scene.h"
#include "camera.h"
#include "mouse_selector.h"

class DebugUIManager {
public:
    DebugUIManager();

    void init(SDL_Window* window, Camera& camera);
    void handleInput(const Input& input);
    void draw(const Input& input, Scene& scene, float deltaTime);

    bool isActive() const { return m_active; }

    float getPlayerSpeed() const { return m_playerSpeed; }
    float getGravity() const { return m_gravity; }

private:
    struct SelectionInfo {
        SceneObject* object = nullptr;
        glm::vec3 position { 0.0f, 0.0f, 0.0f };
        glm::mat4 transform { 1.0f };
        bool useTransform = false;
    };

    SDL_Window* m_window = nullptr;
    Camera* m_camera = nullptr;
    bool m_active = false;

    SceneObject* m_selected = nullptr;
    glm::vec3 m_selectedPosition { 0.0f, 0.0f, 0.0f };
    bool m_hasSelectedPosition = false;

    float m_playerSpeed = 5.0f;
    float m_gravity = -9.81f;

    std::vector<float> m_fpsHistory;
    std::vector<float> m_frameTimeHistory;
    size_t m_maxSamples = 120;

    std::unique_ptr<MouseSelector> m_selector;
    std::unordered_map<ID, SelectionInfo> m_idToInfo;
    std::unordered_map<const SceneObject*, std::vector<ID>> m_objectToIds;
    size_t m_lastSceneSize = 0;
    std::optional<ID> m_selectedId;

    void setActive(bool active);
    void updatePerformanceHistory(float deltaTime);
    void updateSelection(const Input& input, Scene& scene);
    void rebuildSelectables(Scene& scene);
    void applySelection(std::optional<ID> selectionId);
    static glm::vec3 extractPosition(const glm::mat4& transform);
};
