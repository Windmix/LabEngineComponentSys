#pragma once

enum class EntityType
{
	Unknown,
	SpaceShip,
	EnemyShip,
	Asteroid,
	Node,
	// Add other types as needed
};

enum class BehaviorType
{
	Aggressive,
	Neutral,
	Defensive
};

enum class AIState 
{
	Idle,
	ChasingEnemy,
	Roaming,
	Fleeing
};
