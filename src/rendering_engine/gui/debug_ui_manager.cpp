#include "debug_ui_manager.h"
#include <algorithm>
#include <imgui.h>

DebugUIManager::DebugUIManager()
{
    m_fpsHistory.reserve(m_maxSamples);
    m_frameTimeHistory.reserve(m_maxSamples);
}

void DebugUIManager::init(SDL_Window* window, Camera& camera)
{
    m_window = window;
    m_camera = &camera;
    m_selector = std::make_unique<MouseSelector>(camera);
    setActive(false);
}

void DebugUIManager::handleInput(const Input& input)
{
    if (input.keyDown("Escape"))
        setActive(!m_active);
}

void DebugUIManager::setActive(bool active)
{
    m_active = active;
    if (m_window) {
        SDL_SetWindowRelativeMouseMode(m_window, !m_active);
        if (m_active) {
            SDL_ShowCursor();
        } else {
            SDL_HideCursor();
        }
    }
}

void DebugUIManager::updatePerformanceHistory(float deltaTime)
{
    float frameTimeMs = deltaTime * 1000.0f;
    float fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

    if (m_fpsHistory.size() >= m_maxSamples) {
        m_fpsHistory.erase(m_fpsHistory.begin());
        m_frameTimeHistory.erase(m_frameTimeHistory.begin());
    }

    m_fpsHistory.push_back(fps);
    m_frameTimeHistory.push_back(frameTimeMs);
}

glm::vec3 DebugUIManager::extractPosition(const glm::mat4& transform)
{
    return glm::vec3(transform[3]);
}

static glm::mat4 buildInstanceBaseTransform(const SceneObject& obj)
{
    glm::mat4 base = obj.transform.matrix();
    base[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return base;
}

static glm::mat4 combineInstanceTransform(const glm::mat4& baseTransform, const glm::mat4& instanceTransform)
{
    glm::mat4 combined = baseTransform;
    combined[3] = instanceTransform[3];
    return combined;
}

void DebugUIManager::rebuildSelectables(Scene& scene)
{
    if (!m_camera)
        return;

    m_selector = std::make_unique<MouseSelector>(*m_camera);
    m_idToInfo.clear();
    m_objectToIds.clear();
    m_selectedId.reset();

    for (auto& obj : scene.objects()) {
        std::vector<ID> ids;
        if (obj.model) {
            glm::mat4 transform = obj.transform.matrix();
            ID id = m_selector->addSelectable(*obj.model, transform);
            ids.push_back(id);
            m_idToInfo[id] = { &obj, extractPosition(transform), transform, false };
        } else if (obj.mesh) {
            if (obj.mesh->isInstanced() && !obj.mesh->getInstanceTransforms().empty()) {
                const auto& instanceTransforms = obj.mesh->getInstanceTransforms();
                glm::mat4 baseTransform = buildInstanceBaseTransform(obj);
                ids.reserve(instanceTransforms.size());
                for (const auto& instanceTransform : instanceTransforms) {
                    glm::mat4 combined = combineInstanceTransform(baseTransform, instanceTransform);
                    ID id = m_selector->addSelectable(*obj.mesh, combined);
                    ids.push_back(id);
                    m_idToInfo[id] = { &obj, extractPosition(combined), combined, true };
                }
            } else {
                glm::mat4 transform = obj.transform.matrix();
                ID id = m_selector->addSelectable(*obj.mesh, transform);
                ids.push_back(id);
                m_idToInfo[id] = { &obj, extractPosition(transform), transform, false };
            }
        }
        if (!ids.empty())
            m_objectToIds[&obj] = std::move(ids);
    }

    m_lastSceneSize = scene.objects().size();
    applySelection(std::nullopt);
}

void DebugUIManager::applySelection(std::optional<ID> selectionId)
{
    m_selectedId = selectionId;

    SceneObject* nextSelected = nullptr;
    glm::vec3 nextPosition{ 0.0f, 0.0f, 0.0f };
    bool hasPosition = false;
    glm::mat4 nextTransform{ 1.0f };
    bool hasTransform = false;

    if (selectionId) {
        auto it = m_idToInfo.find(*selectionId);
        if (it != m_idToInfo.end()) {
            nextSelected = it->second.object;
            nextPosition = it->second.position;
            hasPosition = true;
            if (it->second.useTransform) {
                nextTransform = it->second.transform;
                hasTransform = true;
            }
        }
    }

    if (nextSelected != m_selected) {
        if (m_selected) {
            m_selected->selected = false;
            m_selected->hasSelectedTransform = false;
        }
        m_selected = nextSelected;
        if (m_selected)
            m_selected->selected = true;
    }

    if (m_selected) {
        m_selected->hasSelectedTransform = hasTransform;
        if (hasTransform)
            m_selected->selectedTransform = nextTransform;
    }

    m_selectedPosition = nextPosition;
    m_hasSelectedPosition = hasPosition;
}

void DebugUIManager::updateSelection(const Input& input, Scene& scene)
{
    if (!m_selector)
        return;

    bool needsRebuild = scene.objects().size() != m_lastSceneSize;

    if (!needsRebuild) {
        for (const auto& obj : scene.objects()) {
            auto it = m_objectToIds.find(&obj);
            if (it == m_objectToIds.end()) {
                needsRebuild = true;
                break;
            }
            if (obj.mesh && obj.mesh->isInstanced()) {
                if (it->second.size() != obj.mesh->getInstanceTransforms().size()) {
                    needsRebuild = true;
                    break;
                }
            }
        }
    }

    if (needsRebuild) {
        rebuildSelectables(scene);
    }

    for (auto& obj : scene.objects()) {
        auto it = m_objectToIds.find(&obj);
        if (it == m_objectToIds.end())
            continue;

        if (obj.model || !obj.mesh || !obj.mesh->isInstanced()) {
            if (!it->second.empty()) {
                ID id = it->second.front();
                glm::mat4 transform = obj.transform.matrix();
                m_selector->updateSelectableTransform(id, transform);
                m_idToInfo[id] = { &obj, extractPosition(transform), transform, false };
            }
            continue;
        }

        const auto& instanceTransforms = obj.mesh->getInstanceTransforms();
        const auto& ids = it->second;
        size_t count = std::min(ids.size(), instanceTransforms.size());
        glm::mat4 baseTransform = buildInstanceBaseTransform(obj);

        for (size_t i = 0; i < count; ++i) {
            glm::mat4 combined = combineInstanceTransform(baseTransform, instanceTransforms[i]);
            ID id = ids[i];
            m_selector->updateSelectableTransform(id, combined);
            m_idToInfo[id] = { &obj, extractPosition(combined), combined, true };
        }
    }

    if (!m_active) {
        if (m_selectedId)
            applySelection(m_selectedId);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(m_window, &width, &height);

    m_selector->handleSelection(input, { width, height });
    applySelection(m_selector->getSelection());
}

void DebugUIManager::draw(const Input& input, Scene& scene, float deltaTime)
{
    updatePerformanceHistory(deltaTime);
    updateSelection(input, scene);

    if (!m_active)
        return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360, 420), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug UI")) {
        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Performance")) {
                if (!m_fpsHistory.empty()) {
                    ImGui::PlotLines("FPS", m_fpsHistory.data(), static_cast<int>(m_fpsHistory.size()), 0, nullptr, 0.0f, 240.0f, ImVec2(0, 80));
                }
                if (!m_frameTimeHistory.empty()) {
                    ImGui::PlotLines("Frame Time (ms)", m_frameTimeHistory.data(), static_cast<int>(m_frameTimeHistory.size()), 0, nullptr, 0.0f, 40.0f, ImVec2(0, 80));
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Data")) {
                ImGui::DragFloat("Player Speed", &m_playerSpeed, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Gravity", &m_gravity, 0.1f, -100.0f, 100.0f);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Object Inspector")) {
                if (m_selected) {
                    ImGui::Text("Name: %s", m_selected->name.c_str());
                    if (m_hasSelectedPosition) {
                        ImGui::Text("Position: %.2f, %.2f, %.2f",
                                    m_selectedPosition.x,
                                    m_selectedPosition.y,
                                    m_selectedPosition.z);
                    } else {
                        ImGui::Text("Position: %.2f, %.2f, %.2f",
                                    m_selected->transform.position.x,
                                    m_selected->transform.position.y,
                                    m_selected->transform.position.z);
                    }
                } else {
                    ImGui::Text("No object selected.");
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
