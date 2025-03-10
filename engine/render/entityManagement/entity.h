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
	Entity(uint32_t entityID);
	~Entity();

	void AddComponent(ComponentBase* component, ComponentType type, EntityType eType);
	bool HasComponent(ComponentType type) const;
	template<typename T>
	T* GetComponent();

};
//--------------------------------------------------------------------------------------------

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