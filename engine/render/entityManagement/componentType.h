#pragma once
#include <cstdint>
enum class ComponentType : uint32_t
{
	NONE = 0,
	TRANSFORM = 1 << 0,			// 00000001
	RIGIDBODY = 1 << 1,			// 00000010
	COLLIDER = 1 << 2,			// 00000100
	CAMERA = 1 << 3,			// 00001000
	FORCE = 1 << 4,				// 00010000
	RENDERABLE = 1 << 5,		// 00100000
	INPUT = 1 << 6,				// 01000000 
	PARTICLE_EMITTER = 1 << 7,	// 10000000
	NAVNODE = 1 << 8,           // 00000001 00000000
	AI_CONTROLLER = 1 << 9      // 00000001 00000000 00000000
};