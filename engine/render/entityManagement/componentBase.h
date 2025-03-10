#pragma once
#include <cstdint>

class ComponentBase
{
public:
	uint32_t ownerId;
	uint32_t componentMask;  // Bitmask to track components
	virtual ~ComponentBase() {};

	virtual void SetOwner(uint32_t entityId);
	uint32_t GetOwner() const;


};

inline void ComponentBase::SetOwner(uint32_t entityId)
{
	ownerId = entityId;
}

inline uint32_t ComponentBase::GetOwner() const
{ 
	return ownerId;
}
