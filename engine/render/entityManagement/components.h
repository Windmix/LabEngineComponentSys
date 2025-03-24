#pragma once
// External libraries
#include "glm.hpp"
#include <vector>

// Project headers
#include "ComponentBase.h"
#include "../physics.h"
#include "entityType.h"
#include "componentType.h"
#include "render/input/inputserver.h"
#include "render/input/mouse.h"
#include "render/input/keyboard.h"

// Implementation files (only include if strictly necessary)
#include <render/physics.cc>
#include <render/cameramanager.h>
#include "render/particlesystem.h"



namespace Components
{
	class TransformComponent : public ComponentBase
	{

	public:
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat orientation = glm::identity<glm::quat>();
		glm::mat4x4 transform = glm::mat4(1.0f);
		glm::vec3 linearVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);      // For rotating the object
		float rotationSpeed = 0.0f;            // For controlling the speed of rotation

		EntityType entityType = EntityType::Unknown;
		static constexpr ComponentType TYPE = ComponentType::TRANSFORM;
		
	};



	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------

	class ColliderComponent : public ComponentBase
	{

	public:
		static constexpr ComponentType TYPE = ComponentType::COLLIDER;
		EntityType UsingEntityType = EntityType::Unknown;

		Physics::ColliderMeshId collidermeshId;
		Physics::ColliderId colliderID;
		glm::vec3 EndPointsNodes[6];
		std::vector<glm::vec3> colliderEndPoints;  // Reserve space for 17 elements
		std::vector<glm::vec3> rayCastPoints;


		Core::CVar* r_Raycasts = Core::CVarCreate(Core::CVarType::CVar_Int, "r_Raycasts", "0");
		ColliderComponent() {}
		ColliderComponent(Physics::ColliderMeshId colliderId) : collidermeshId(colliderId) {}
	

		

	};	

	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

	class RigidBodyComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::RIGIDBODY;

		glm::vec3 velocity;
		glm::vec3 angularVelocity;
		float mass;



	};
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

	class CameraComponent : public ComponentBase
	{
	public:

		Core::CVar* r_camera = Core::CVarCreate(Core::CVarType::CVar_Int, "r_Camera", "0");
		Core::CVar* r_camera_id = Core::CVarCreate(Core::CVarType::CVar_Int, "r_Camera_id", "0");

		static constexpr ComponentType TYPE = ComponentType::CAMERA;
		glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
		const float accelerationFactor = 1.0f;
		const float camOffsetY = 1.0f;
		const float cameraSmoothFactor = 10.0f;
		Render::Camera* theCam = nullptr;
		
		
	};
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

	class ContinuousForceComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::FORCE;
		


	};
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	class RenderableComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::RENDERABLE;
		Render::ModelId modelId;

		RenderableComponent() {}
		RenderableComponent(Render::ModelId model) : modelId(model) {}

	};

	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	class ParticleEmitterComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::PARTICLE_EMITTER;
		uint32_t numParticles = 2048;
		Render::ParticleEmitter* particleEmitterLeft = nullptr;
		Render::ParticleEmitter* particleEmitterRight = nullptr;

		Render::ParticleEmitter* particleCanonLeft = nullptr;
		Render::ParticleEmitter* particleCanonRight = nullptr;
		float emitterOffset = -0.5f;
		float canonEmitterOffset = 0.5f;
	
		// Allow external access to the emitter data (either as public member or getter method)
	
	};
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	class PlayerInputComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::INPUT;
		//ship speed variables
		float normalSpeed = 1.0f;
		float boostSpeed = normalSpeed * 10.0f;
		float accelerationFactor = 1.0f;
		float currentSpeed = 0.0f;

		float rotationZ = 0;
		float rotXSmooth = 0;
		float rotYSmooth = 0;
		float rotZSmooth = 0;

		float canonDefaultSpeed = 1.0f;
		float canonLaunchSpeed = canonDefaultSpeed * 10.0f;

		Input::Mouse* mouse = Input::GetDefaultMouse();
		Input::Keyboard* kbd = Input::GetDefaultKeyboard();
		PlayerInputComponent() {}
	};
	
	class AINavNodeComponent : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::NAVNODE;
		Core::CVar* r_draw_path = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_path", "0");

		Core::CVar* r_draw_Node_Axis = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_Node_Axis", "0");
		Core::CVar* r_draw_Node_Axis_id = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_Node_Axis_id", " - 1");

		glm::vec3 EndPoints[6];
		bool isCollidedAsteroids = false;
		

	};

	class AIinputController : public ComponentBase
	{
	public:
		static constexpr ComponentType TYPE = ComponentType::AI_CONTROLLER;
		//Ai path
		

		//ship speed variables
		float normalSpeed = 1.0f;
		float boostSpeed = normalSpeed * 10.0f;
		float accelerationFactor = 1.0f;
		float currentSpeed = 0.0f;

		float rotationZ = 0;
		float rotXSmooth = 0;
		float rotYSmooth = 0;
		float rotZSmooth = 0;

		float canonDefaultSpeed = 1.0f;
		float canonLaunchSpeed = canonDefaultSpeed * 10.0f;

		// AiController inputs
		float rotationInputX = 0;
		float rotationInputY = 0;
		float rotationInputZ = 0;

		bool isForward = false;
		bool isBoosting = false;
		bool isShooting = false;

		


		AIinputController() {}

	};

}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

