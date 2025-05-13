// ---- Component.h ----

#pragma once

#include <string>
#include <memory>

class Entity; // Forward declaration

class Component {
public:
    Component(const std::string& name = "Component");
    virtual ~Component() = default;

    // Core component lifecycle methods
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() {}  // Optional, not all components will render

    // Component identification and management
    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    // Entity relationship
    void SetOwner(std::shared_ptr<Entity> owner) { m_Owner = owner; }
    std::shared_ptr<Entity> GetOwner() const { return m_Owner.lock(); }

    // Enable/disable functionality
    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

protected:
    std::string m_Name;
    std::weak_ptr<Entity> m_Owner;  // Weak reference to avoid circular dependencies
    bool m_Enabled = true;
};