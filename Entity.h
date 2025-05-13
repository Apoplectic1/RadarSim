// ---- Entity.h ----

#pragma once

#include "Component.h"
#include <string>
#include <memory>
#include <vector>
#include <typeindex>
#include <unordered_map>

class Entity : public std::enable_shared_from_this<Entity> {
public:
    Entity(const std::string& name = "Entity");
    ~Entity() = default;

    // Core entity lifecycle methods
    void Initialize();
    void Update(float deltaTime);
    void Render();

    // Component management
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        component->SetOwner(shared_from_this());

        // Store by type and in main collection
        m_ComponentsByType[typeid(T)] = component;
        m_Components.push_back(component);

        return component;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto it = m_ComponentsByType.find(typeid(T));
        if (it != m_ComponentsByType.end()) {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    // Entity identification and management
    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

private:
    std::string m_Name;
    std::vector<std::shared_ptr<Component>> m_Components;
    std::unordered_map<std::type_index, std::shared_ptr<Component>> m_ComponentsByType;
};