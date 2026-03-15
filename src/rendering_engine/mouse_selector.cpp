#include "mouse_selector.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace {
constexpr float kEpsilon = 1e-6f;
}

MouseSelector::MouseSelector(Camera& cam, float nearPlaneDistance, float farPlaneDistance)
    : camera(&cam), nearPlane(nearPlaneDistance), farPlane(farPlaneDistance) {}

ID MouseSelector::addSelectable(const Model& model, const glm::mat4& transform) {
    Selectable selectable{ nextId++, &model, nullptr, transform };
    selectables.push_back(selectable);
    return selectable.id;
}

ID MouseSelector::addSelectable(const Mesh& mesh, const glm::mat4& transform) {
    Selectable selectable{ nextId++, nullptr, &mesh, transform };
    selectables.push_back(selectable);
    return selectable.id;
}

bool MouseSelector::removeSelectable(ID id) {
    auto it = std::remove_if(selectables.begin(), selectables.end(), [id](const Selectable& selectable) {
        return selectable.id == id;
    });
    if (it != selectables.end()) {
        selectables.erase(it, selectables.end());
        if (selectedId && *selectedId == id) {
            selectedId.reset();
        }
        return true;
    }
    return false;
}

void MouseSelector::updateSelectableTransform(ID id, const glm::mat4& transform) {
    for (auto& selectable : selectables) {
        if (selectable.id == id) {
            selectable.transform = transform;
            break;
        }
    }
}

void MouseSelector::clearSelection() {
    selectedId.reset();
}

std::optional<ID> MouseSelector::getSelection() const {
    return selectedId;
}

void MouseSelector::handleSelection(const Input& input, const std::pair<int, int>& screenSize) {
    bool leftPressed = input.isMouseButtonHeld(SDL_BUTTON_LEFT);
    bool middlePressed = input.isMouseButtonHeld(SDL_BUTTON_MIDDLE);

    // Only process selection on left-button rising edge, ignore during camera drag
    if (leftPressed && !mousePressedLastFrame && !middlePressed) {
        auto mousePos = input.getMousePos();

        auto [rayOrigin, rayDir] = calculateRay(screenSize, mousePos);

        std::optional<std::pair<ID, float>> closest;

        for (const auto& selectable : selectables) {
            std::optional<std::pair<ID, float>> result;
            if (selectable.model) {
                result = testModel(rayOrigin, rayDir, selectable);
            } else if (selectable.mesh) {
                result = testMesh(rayOrigin, rayDir, selectable);
            }

            if (!result) continue;
            
            if (!closest || result->second < closest->second) {
                closest = result;
            }
        }

        if (closest) {
            selectedId = closest->first;
        } else {
            selectedId.reset();
        }
    }

    mousePressedLastFrame = leftPressed;
}


std::pair<glm::vec3, glm::vec3> MouseSelector::calculateRay(const std::pair<int, int>& screenSize,
                                                            const std::pair<int, int>& mousePos) const {
    float ndcX = (2.0f * mousePos.first) / screenSize.first - 1.0f;
    float ndcY = 1.0f - (2.0f * mousePos.second) / screenSize.second;

    glm::mat4 projection = camera->GetProjectionMatrix(
        static_cast<float>(screenSize.first) / static_cast<float>(screenSize.second));
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 invViewProj = glm::inverse(projection * view);

    glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    if (std::abs(nearPoint.w) > kEpsilon) {
        nearPoint /= nearPoint.w;
    }
    if (std::abs(farPoint.w) > kEpsilon) {
        farPoint /= farPoint.w;
    }

    glm::vec3 rayOrigin = glm::vec3(nearPoint);
    glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint - nearPoint));

    return {rayOrigin, rayDir};
}


std::pair<bool, float> MouseSelector::rayIntersectsTriangle(const glm::vec3& rayOrigin,
                                                            const glm::vec3& rayDir,
                                                            const glm::vec3& v0,
                                                            const glm::vec3& v1,
                                                            const glm::vec3& v2) const {
    // Compute edge vectors from v0
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    
    // Begin calculating determinant (also used to calculate u parameter)
    glm::vec3 h = glm::cross(rayDir, edge2);
    float det = glm::dot(edge1, h);
    
    // If determinant is near zero, ray lies in plane of triangle
    if (std::abs(det) < kEpsilon) {
        return {false, 0.0f};
    }

    float invDet = 1.0f / det;
    
    // Calculate distance from v0 to ray origin
    glm::vec3 s = rayOrigin - v0;
    
    // Calculate u parameter and test bounds
    float u = invDet * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) {
        return {false, 0.0f};
    }

    // Prepare to test v parameter
    glm::vec3 q = glm::cross(s, edge1);
    
    // Calculate v parameter and test bounds
    float v = invDet * glm::dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f) {
        return {false, 0.0f};
    }

    // Calculate t (distance along ray to intersection)
    float t = invDet * glm::dot(edge2, q);
    
    // Ray intersection (t must be positive - in front of camera)
    if (t > kEpsilon) {
        return {true, t};
    }
    
    return {false, 0.0f};
}


std::optional<std::pair<ID, float>> MouseSelector::testModel(const glm::vec3& rayOrigin,
                                                             const glm::vec3& rayDir,
                                                             const Selectable& selectable) const {
    if (!selectable.model) {
        return std::nullopt;
    }

    std::optional<std::pair<ID, float>> closest;
    for (const auto& mesh : selectable.model->meshes) {
        Selectable temp{selectable.id, nullptr, &mesh, selectable.transform};
        auto result = testMesh(rayOrigin, rayDir, temp);
        if (result && (!closest || result->second < closest->second)) {
            closest = result;
        }
    }
    return closest;
}


std::optional<std::pair<ID, float>> MouseSelector::testMesh(const glm::vec3& rayOrigin,
                                                            const glm::vec3& rayDir,
                                                            const Selectable& selectable) const {
    if (!selectable.mesh) {
        return std::nullopt;
    }

    const auto& mesh = *selectable.mesh;
    float closestDistance = std::numeric_limits<float>::max();
    bool hit = false;

    // Iterate through triangles (3 indices per triangle)
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        // Get vertex data
        Vertex v0 = mesh.vertices[mesh.indices[i]];
        Vertex v1 = mesh.vertices[mesh.indices[i + 1]];
        Vertex v2 = mesh.vertices[mesh.indices[i + 2]];

        // Transform vertices to world space
        glm::vec3 p0 = glm::vec3(selectable.transform * glm::vec4(v0.Position, 1.0f));
        glm::vec3 p1 = glm::vec3(selectable.transform * glm::vec4(v1.Position, 1.0f));
        glm::vec3 p2 = glm::vec3(selectable.transform * glm::vec4(v2.Position, 1.0f));

        // Test intersection
        auto [intersects, distance] = rayIntersectsTriangle(rayOrigin, rayDir, p0, p1, p2);
        if (intersects && distance < closestDistance) {
            closestDistance = distance;
            hit = true;
            lastIntersectionPoint = rayOrigin + rayDir * distance;
        }
    }

    if (hit) {
        return std::make_pair(selectable.id, closestDistance);
    }
    return std::nullopt;
}

std::vector<std::pair<ID, float>> MouseSelector::raycast(const glm::vec3& direction) const {
    return raycast(camera->Position, direction);
}

std::vector<std::pair<ID, float>> MouseSelector::raycast(const glm::vec3& origin, const glm::vec3& direction) const {
    glm::vec3 rayOrigin = origin;
    glm::vec3 rayDir = glm::normalize(direction);

    std::vector<std::pair<ID, float>> hits;

    for (const auto& selectable : selectables) {
        std::optional<std::pair<ID, float>> result;
        if (selectable.model) {
            result = testModel(rayOrigin, rayDir, selectable);
        } else if (selectable.mesh) {
            result = testMesh(rayOrigin, rayDir, selectable);
        }

        if (result) {
            hits.push_back(*result);
        }
    }

    std::sort(hits.begin(), hits.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    return hits;
}

std::optional<ID> MouseSelector::selectFromCameraCenter() {
    // Shoot a ray from camera position in the direction the camera is looking
    auto hits = raycast(camera->Front);
    
    if (!hits.empty()) {
        selectedId = hits.front().first;  // Select the closest
        return selectedId;
    }
    
    selectedId.reset();
    return std::nullopt;
}

std::vector<ID> MouseSelector::raycastGrid(int gridSize, float spreadAngle, const Camera& cam) const {
    std::vector<ID> uniqueHits;
    std::unordered_set<ID> hitSet;
    
    glm::vec3 forward = glm::normalize(cam.Front);
    glm::vec3 right = glm::normalize(cam.Right);
    glm::vec3 up = glm::normalize(cam.Up);
    glm::vec3 origin = cam.Position;
    
    float halfSpread = glm::radians(spreadAngle * 0.5f);
    float step = (gridSize > 1) ? (2.0f * halfSpread) / (gridSize - 1) : 0.0f;
    
    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            float yawOffset = -halfSpread + col * step;
            float pitchOffset = -halfSpread + row * step;
            
            glm::vec3 rayDir = forward;
            rayDir = glm::vec3(glm::rotate(glm::mat4(1.0f), yawOffset, up) * glm::vec4(rayDir, 0.0f));
            rayDir = glm::vec3(glm::rotate(glm::mat4(1.0f), pitchOffset, right) * glm::vec4(rayDir, 0.0f));
            rayDir = glm::normalize(rayDir);
            
            auto hits = raycast(origin, rayDir);
            if (!hits.empty()) {
                ID hitId = hits.front().first;
                if (hitSet.find(hitId) == hitSet.end()) {
                    hitSet.insert(hitId);
                    uniqueHits.push_back(hitId);
                }
            }
        }
    }
    
    return uniqueHits;
}