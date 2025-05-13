// ---- Entity.cpp ----

#include "Entity.h"

Entity::Entity(const std::string& name)
    : m_Name(name)
{
}

void Entity::Initialize() {
    for (auto& component : m_Components) {
        component->Initialize();
    }
}

void Entity::Update(float deltaTime) {
    for (auto& component : m_Components) {
        if (component->IsEnabled()) {
            component->Update(deltaTime);
        }
    }
}

void Entity::Render() {
    for (auto& component : m_Components) {
        if (component->IsEnabled()) {
            component->Render();
        }
    }
}