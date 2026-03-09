#include "scene.h"
#include <algorithm>

SceneObject* Scene::add(SceneObject obj)
{
    m_objects.push_back(std::move(obj));
    return &m_objects.back();
}

SceneObject* Scene::find(const std::string& name)
{
    for (auto& obj : m_objects)
        if (obj.name == name)
            return &obj;
    return nullptr;
}

void Scene::remove(const std::string& name)
{
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [&name](const SceneObject& o) { return o.name == name; }),
        m_objects.end());
}
