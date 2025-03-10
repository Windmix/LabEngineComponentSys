#pragma once
#include <cstdint>
#include <climits>

class PhysicsId
{
public:
	uint32_t index;
	uint32_t generation;

	bool operator==(const PhysicsId& other) const;
	bool operator!=(const PhysicsId& other) const;
};

inline bool PhysicsId::operator==(const PhysicsId& other) const
{
	return index == other.index && generation == other.generation;
}

inline bool PhysicsId::operator!=(const PhysicsId& other) const
{
	return !(*this == other);
}
