#pragma once
#include <vector>
#include "entityid.h"
#include "components.h"
#include "ComponentBase.h"
#include "componentType.h"
#include "entityType.h"


class Entity
{
public:
	EntityId id;
	EntityType eType;
	std::vector<ComponentBase*> components;

	

	Entity() {}
	int CalculateHCost(glm::vec3 goal);
	Entity(uint32_t entityID);
	~Entity();

	void AddComponent(ComponentBase* component, ComponentType type, EntityType eType);
	void RemoveComponent(ComponentType type);
	bool HasComponent(ComponentType type) const;
	template<typename T>
	T* GetComponent();

	//ai stuff
	Entity* parentNode = nullptr;
	std::vector<Entity*> path;
	Entity* closestNodeFromShip;
	int gCost = 0;  // Cost from start to current node
	int hCost = 0;  // Heuristic cost from current node to end
	int FCost() const { return gCost + hCost;}

	int pathIndex = 0;
	float nodeArrivalTimer = 0.0f;
	bool hasReachedTheStartNode = false;
	bool closestNodeCalled = false;
	bool isAvoidingAsteroids = false;

	float avoidanceCooldown = 0.01f;
	float avoidanceTime = 0.0f;
};
//--------------------------------------------------------------------------------------------

int Entity::CalculateHCost(glm::vec3 goal)
{
	auto transFormComponent = GetComponent<Components::TransformComponent>();
	int dx = std::abs(transFormComponent->transform[3].x - goal.x);
	int dy = std::abs(transFormComponent->transform[3].y - goal.y);
	int dz = std::abs(transFormComponent->transform[3].z - goal.z);
	return (dx + dy + dz) * 10;
}

inline Entity::Entity(uint32_t entityID)
{
	id = entityID;
	eType = EntityType::Unknown;
}

inline Entity::~Entity()
{

}

inline void Entity::AddComponent(ComponentBase* component, ComponentType type, EntityType eType)
{
	
	component->SetOwner(this->id);
	component->componentMask |= static_cast<uint32_t>(type);  // Set the bit for this component
	components.push_back(component);
}
inline void Entity::RemoveComponent(ComponentType type)
{
	// Iterate through the components to find the one of the specified type
	for (auto it = components.begin(); it != components.end(); ++it)
	{
		ComponentBase* component = *it;

		// Check if the component is of the type we want to remove
		if (component->componentMask & static_cast<uint32_t>(type))
		{
			// Reset the component mask (optional, if needed)
			component->componentMask &= ~static_cast<uint32_t>(type);

			// Remove the component from the vector
			components.erase(it);

			// Optionally, delete the component if necessary
			delete component;

			break;  // Exit after removing the first match (if components are unique)
		}
	}
}

inline bool Entity::HasComponent(ComponentType type) const
{
	for (auto component : components)
	{
		return (component->componentMask & static_cast<uint32_t>(type)) != 0;
	}
}
	

template<typename T>
T* Entity::GetComponent()
{
	// Iterate through the component list to Check if the component exists using the bitmask
	for (ComponentBase* comp : components)
	{
		if (T* casted = dynamic_cast<T*>(comp)) // slow I know but who cares
		{
			return casted;
		}
		
	}
	return nullptr;
	

}