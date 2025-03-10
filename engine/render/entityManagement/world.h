#pragma once
#include <vector>
#include "entity.h"
#include "chunkAllocator.h"
#include "components.h"  // Assuming components are declared here
#include "render/renderdevice.h"




class World
{
public:
    // Chunk allocators for each component type, pre-allocate 1 chunk for each containing 64 elements
    ChunkAllocator<Entity, 64> entityChunk;
    ChunkAllocator<Components::TransformComponent, 64> transformChunk;
    ChunkAllocator<Components::RenderableComponent, 64> renderableChunk;
    ChunkAllocator<Components::CameraComponent, 64> cameraChunk;
    ChunkAllocator<Components::ColliderComponent, 64> colliderChunk;
    ChunkAllocator<Components::RigidBodyComponent, 64> rigidBodyChunk;
    ChunkAllocator<Components::PlayerInputComponent, 64> controlInputChunk;
    ChunkAllocator<Components::ParticleEmitterComponent, 64> particleEmitterChunk;
    ChunkAllocator<Render::ParticleEmitter, 64> ChunkOfPartcles;


    // Vector to store active entities (to track them)
    std::vector<Entity*> entities;
    uint32_t nextEntityId = 0;

    //saving for destroyed ships
    uint32_t savedID = 0;
    

    World();
    ~World();

    static World* instance();
    static void destroy();

    // Register a new entity
    Entity* createEntity(EntityType etype, bool isRespawning);

    // Attach a component to an entity
    void AttachComponentToEntity(uint32_t entityId, ComponentBase* component, ComponentType compType, EntityType eType);

    // Get an entity by its ID
    Entity* GetEntity(uint32_t entityId);

    // Update all entities
    void Update(float dt);


    // Destroy an entity and deallocate all components
    void DestroyEntity(uint32_t entityId);
    void Cleanup();
    void DestroyWorld();

    void CreatePlayerShip(bool isRespawning);
    void CreateAsteroid(float spread);
private:

    void UpdateShip(Entity* entity, float dt);
    void UpdateAsteroid(Entity* entity, float dt);

    void draw(Entity* entity);

};


static World* _instance; // singleton
inline World::World() {}

inline World::~World()
{

}
inline World* World::instance()
{
    if (!_instance) 
    {
        _instance = new World();
    }
    return _instance;
}
inline void World::destroy() 
{
    if (_instance)
    {
        delete _instance;
        _instance = nullptr;
    }
    
}

inline Entity* World::createEntity(EntityType etype, bool isRespawning)
{

    // Allocate an entity from the chunk allocator
    Entity* entity = entityChunk.Allocate();
    if (!isRespawning)
    {
        entity->id = nextEntityId++;
    }
    else
    {
        entity->id = savedID;
    }
    entity->eType = etype;


    entities.push_back(entity);

    return entity;
}

inline void World::AttachComponentToEntity(uint32_t entityId, ComponentBase* component, ComponentType compType, EntityType eType)
{
    Entity* entity = GetEntity(entityId);
    if (entity)
    {
        entity->AddComponent(component, compType, eType);
    }
}

inline Entity* World::GetEntity(uint32_t entityId)
{
    auto it = std::find_if(entities.begin(), entities.end(), [entityId](const Entity* entity) { return entity->id == entityId; });
    if (it != entities.end())
    {
        return *it;
    }
    // Return nullptr if not found
    return nullptr;
}

inline void World::Update(float dt)
{

    //updates everything
    for (Entity* entity : entities)
    {
        UpdateAsteroid(entity, dt);
        UpdateShip(entity, dt);
        draw(entity);
       
    }
}


inline void World::DestroyEntity(uint32_t entityId)
{
    auto it = std::find_if(entities.begin(), entities.end(), [entityId](Entity* entity) { return entity->id == entityId; });

    if (it != entities.end())
    {
        Entity* entityToDelete = *it;

        // Deallocate all components using the appropriate chunk allocators
        for (auto* component : entityToDelete->components)
        {
            // Handle component deallocation
            if (auto* transformComp = dynamic_cast<Components::TransformComponent*>(component))
            {
                transformChunk.Deallocate(transformComp);
            }
            else if (auto* renderableComp = dynamic_cast<Components::RenderableComponent*>(component))
            {
                renderableChunk.Deallocate(renderableComp);
            }
            else if (auto* colliderComp = dynamic_cast<Components::ColliderComponent*>(component))
            {
                colliderChunk.Deallocate(colliderComp);
            }
            else if (auto* rigidBodyComp = dynamic_cast<Components::RigidBodyComponent*>(component))
            {
                rigidBodyChunk.Deallocate(rigidBodyComp);
            }
            else if (auto* playerInputComp = dynamic_cast<Components::PlayerInputComponent*>(component))
            {
                controlInputChunk.Deallocate(playerInputComp);
            }
            else if (auto* particleEmitterComp = dynamic_cast<Components::ParticleEmitterComponent*>(component))
            {
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterRight);
                particleEmitterChunk.Deallocate(particleEmitterComp);

            }
          
        }
        //// Deallocate the entity from the Chunk
        entityChunk.Deallocate(entityToDelete);

        // Remove the entity from the entity vector
        entities.erase(it);
    }

}

inline void World::Cleanup()
{
    // Deallocate all entities and their components
    for (Entity* entity : entities) {
        for (auto* component : entity->components) {
            if (component) {
                // Handle component deallocation
                if (auto* transformComp = dynamic_cast<Components::TransformComponent*>(component)) 
                {
                    transformChunk.Deallocate(transformComp);
                }
                else if (auto* renderableComp = dynamic_cast<Components::RenderableComponent*>(component))
                {
                    renderableChunk.Deallocate(renderableComp);
                }
                else if (auto* colliderComp = dynamic_cast<Components::ColliderComponent*>(component))
                {
                    colliderChunk.Deallocate(colliderComp);
                }
                else if (auto* rigidBodyComp = dynamic_cast<Components::RigidBodyComponent*>(component))
                {
                    rigidBodyChunk.Deallocate(rigidBodyComp);
                }
                else if (auto* playerInputComp = dynamic_cast<Components::PlayerInputComponent*>(component)) 
                {
                    controlInputChunk.Deallocate(playerInputComp);
                }
                else if (auto* particleEmitterComp = dynamic_cast<Components::ParticleEmitterComponent*>(component))
                {
                    particleEmitterChunk.Deallocate(particleEmitterComp);
                }
            }
        }

        // Deallocate the entity itself
        entityChunk.Deallocate(entity);
    }
    entities.clear();
}

inline void World::DestroyWorld()
{
    if (_instance)
    {
        _instance->Cleanup();  // Clean up the world before destroying the instance
        delete _instance;
        _instance = nullptr;
    }
}
inline void World::CreatePlayerShip(bool isRespawning)
{
    auto shipModel = Render::LoadModel("assets/space/spaceship.glb");

    const glm::vec3 colliderEndPoints[17] =
    {
        glm::vec3(1.40173, 0.0, -0.225342),  // left wing back
        glm::vec3(1.33578, 0.0, 0.088893),  // left wing front
        glm::vec3(0.227107, -0.200232, -0.588618),  // left back engine bottom
        glm::vec3(0.227107, 0.228809, -0.588618),  // left back engine top
        glm::vec3(0.391073, -0.130853, 1.28339),  // left weapon
        glm::vec3(0.134787, 0.0, 1.68965),  // left front
        glm::vec3(0.134787, 0.250728, 0.647422),  // left wind shield

        glm::vec3(-1.40173, 0.0, -0.225342),  // right wing back
        glm::vec3(-1.33578, 0.0, 0.088893),  // right wing front
        glm::vec3(-0.227107, -0.200232, -0.588618),  // right back engine bottom
        glm::vec3(-0.227107, 0.228809, -0.588618),  // right back engine top
        glm::vec3(-0.391073, -0.130853, 1.28339),  // right weapon
        glm::vec3(-0.134787, 0.0, 1.68965),  // right front
        glm::vec3(-0.134787, 0.250728, 0.647422),  // right wind shield

        glm::vec3(0.0, 0.525049, -0.392836),  // top back
        glm::vec3(0.0, 0.739624, 0.102582),  // top fin
        glm::vec3(0.0, -0.244758, 0.284825),  // bottom
    };
   
   

    if (!isRespawning)
    {
        Entity* spaceship = createEntity(EntityType::SpaceShip, isRespawning);

        Components::TransformComponent* newTransform = transformChunk.Allocate();
        spaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::SpaceShip);
        /* newTransform->transform[3] = glm::vec4(0.0f + i*2, 0.0f + i*2, 0.0f + i*2, 0.0f);*/

        Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
        spaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::SpaceShip);

        Components::ColliderComponent* collider = colliderChunk.Allocate();
        spaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::SpaceShip);
        for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
        {
            collider->colliderEndPoints[i] = colliderEndPoints[i];
        }
        Components::CameraComponent* camera = cameraChunk.Allocate();
        spaceship->AddComponent(camera, ComponentType::CAMERA, EntityType::SpaceShip);

        Components::PlayerInputComponent* controllinput = controlInputChunk.Allocate();
        spaceship->AddComponent(controllinput, ComponentType::INPUT, EntityType::SpaceShip);

        Components::ParticleEmitterComponent* particleEmitter = particleEmitterChunk.Allocate();
        spaceship->AddComponent(particleEmitter, ComponentType::PARTICLE_EMITTER, EntityType::SpaceShip);


        particleEmitter->particleEmitterLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
        particleEmitter->particleEmitterRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);

        particleEmitter->particleCanonLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
        particleEmitter->particleCanonRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);

        particleEmitter->particleEmitterLeft->data = {
            .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->emitterOffset), 1.0f),
            .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
            .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };


        particleEmitter->particleEmitterRight->data = particleEmitter->particleEmitterLeft->data;

        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterLeft);
        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterRight);


        particleEmitter->particleCanonLeft->data = {
            .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[3]) * particleEmitter->canonEmitterOffset), 1.0f),
            .dir = glm::vec4(glm::vec3(newTransform->transform[3]), 0),
            .startColor = glm::vec4(0.0f, 0.76f, 0.00f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(1.0),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.1f,
            .endScale = 0.2f,
            .decayTime = 0.6f,
            .randomTimeOffsetDist = 0.01f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };

        particleEmitter->particleCanonRight->data = particleEmitter->particleCanonLeft->data;

        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonLeft);
        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonRight);

    }
    else
    {
        Entity* spaceship = createEntity(EntityType::SpaceShip, isRespawning);

        Components::TransformComponent* newTransform = transformChunk.Allocate();
        spaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::SpaceShip);
        /* newTransform->transform[3] = glm::vec4(0.0f + i*2, 0.0f + i*2, 0.0f + i*2, 0.0f);*/

        Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
        spaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::SpaceShip);

        Components::ColliderComponent* collider = colliderChunk.Allocate();
        spaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::SpaceShip);
        for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
        {
            collider->colliderEndPoints[i] = colliderEndPoints[i];
        }
        Components::CameraComponent* camera = cameraChunk.Allocate();
        spaceship->AddComponent(camera, ComponentType::CAMERA, EntityType::SpaceShip);

        Components::PlayerInputComponent* controllinput = controlInputChunk.Allocate();
        spaceship->AddComponent(controllinput, ComponentType::INPUT, EntityType::SpaceShip);

        Components::ParticleEmitterComponent* particleEmitter = particleEmitterChunk.Allocate();
        spaceship->AddComponent(particleEmitter, ComponentType::PARTICLE_EMITTER, EntityType::SpaceShip);

        particleEmitter->particleEmitterLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
        particleEmitter->particleEmitterRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);

        particleEmitter->particleCanonLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
        particleEmitter->particleCanonRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);

        particleEmitter->particleEmitterLeft->data = {
            .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->emitterOffset), 1.0f),
            .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
            .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };


        particleEmitter->particleEmitterRight->data = particleEmitter->particleEmitterLeft->data;

        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterLeft);
        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterRight);

        particleEmitter->particleCanonLeft->data = {
            .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->canonEmitterOffset), 1.0f),
            .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
            .startColor = glm::vec4(0.0f, 0.76f, 0.00f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 100.0f,
            .endSpeed = 30.0f,
            .startScale = 0.02f,
            .endScale = 0.0f,
            .decayTime = 0.3f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };

        particleEmitter->particleCanonRight->data = particleEmitter->particleCanonLeft->data;

        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonLeft);
        Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonRight);
    }

}
inline void World::CreateAsteroid(float spread)
{
    Render::ModelId models[6] = {
    Render::LoadModel("assets/space/Asteroid_1.glb"),
    Render::LoadModel("assets/space/Asteroid_2.glb"),
    Render::LoadModel("assets/space/Asteroid_3.glb"),
    Render::LoadModel("assets/space/Asteroid_4.glb"),
    Render::LoadModel("assets/space/Asteroid_5.glb"),
    Render::LoadModel("assets/space/Asteroid_6.glb")
    };

    Physics::ColliderMeshId colliderMeshes[6] = {
       Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
       Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
       Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
       Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
       Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
       Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };

    Entity* asteroidEntity = createEntity(EntityType::Asteroid, false);
    if (asteroidEntity->eType == EntityType::Asteroid)

    {

        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        // Allocating stuff to the chunkAllocator
        Components::TransformComponent* newTransform = transformChunk.Allocate();
        asteroidEntity->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::Asteroid);


        Components::RenderableComponent* renderable = renderableChunk.Allocate(models[resourceIndex]);
        asteroidEntity->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::Asteroid);


        Components::ColliderComponent* collider = colliderChunk.Allocate(colliderMeshes[resourceIndex]);
        asteroidEntity->AddComponent(collider, ComponentType::COLLIDER, EntityType::Asteroid);




        //transform Setup

        newTransform->entityType = EntityType::Asteroid;
        // Randomize position
        glm::vec3 randomPosition(Core::RandomFloatNTP() * spread, Core::RandomFloatNTP() * spread, Core::RandomFloatNTP() * spread);
        newTransform->transform = glm::translate(newTransform->transform, randomPosition);
        // Randomize rotation axis
        glm::vec3 rotation_Axis = glm::vec3
        (static_cast<float>(rand() % 2 ? 1 : -1) * 2.0f,
            static_cast<float>(rand() % 2 ? 1 : -1) * 2.0f,
            static_cast<float>(rand() % 2 ? 1 : -1) * 2.0f);
        //GIVE THE TRANSFORM ROTATIONAXIS
        newTransform->rotationAxis = glm::normalize(rotation_Axis);

        //GIVE THE TRANSFORM RANDOM VELOCITY IF TRANSLATION IS ENABLED
        glm::vec3 velocity = glm::vec3(Core::RandomFloatNTP() * 2.0f - 1.0f, Core::RandomFloatNTP() * 2.0f - 1.0f, Core::RandomFloatNTP() * 2.0f - 1.0f);

        newTransform->linearVelocity = glm::normalize(velocity);  // Normalize to get a consistent direction
        //GIVE THE TRANSFORM ROTATIONSPEED
        float rotationSpeed = Core::RandomFloat() * 90.0f + 10.0f;  // Random speed between 10 and 100
        newTransform->rotationSpeed = rotationSpeed;
        collider->colliderID = Physics::CreateCollider(colliderMeshes[resourceIndex], newTransform->transform);

    }
}
inline void World::UpdateShip(Entity* entity, float dt)
{
    if (entity->eType == EntityType::SpaceShip) //playerShip
    {
        auto playerInputComponent = entity->GetComponent<Components::PlayerInputComponent>();
        auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        auto cameraComponent = entity->GetComponent<Components::CameraComponent>();
        auto colliderComponent = entity->GetComponent< Components::ColliderComponent>();
        auto particleComponent = entity->GetComponent<Components::ParticleEmitterComponent>();

        if (playerInputComponent && transformComponent && cameraComponent)
        {

            if (playerInputComponent->kbd->held[Input::Key::W])
            {
                if (playerInputComponent->kbd->held[Input::Key::Shift])
                    playerInputComponent->currentSpeed = glm::mix(playerInputComponent->currentSpeed, playerInputComponent->boostSpeed, std::min(1.0f, dt * 30.0f));
                else
                    playerInputComponent->currentSpeed = glm::mix(playerInputComponent->currentSpeed, playerInputComponent->normalSpeed, std::min(1.0f, dt * 90.0f));
            }
            else
            {
                playerInputComponent->currentSpeed = 0.0f;
            }

            glm::vec3 desiredVelocity = glm::vec3(0, 0, playerInputComponent->currentSpeed);

            desiredVelocity = transformComponent->transform * glm::vec4(desiredVelocity, 0.0f);



            transformComponent->linearVelocity = glm::mix(transformComponent->linearVelocity, desiredVelocity, playerInputComponent->accelerationFactor);

            float rotX = playerInputComponent->kbd->held[Input::Key::Left] ? 1.0f : playerInputComponent->kbd->held[Input::Key::Right] ? -1.0f : 0.0f;
            float rotY = playerInputComponent->kbd->held[Input::Key::Up] ? -1.0f : playerInputComponent->kbd->held[Input::Key::Down] ? 1.0f : 0.0f;
            float rotZ = playerInputComponent->kbd->held[Input::Key::A] ? -1.0f : playerInputComponent->kbd->held[Input::Key::D] ? 1.0f : 0.0f;

            transformComponent->transform[3] += glm::vec4(transformComponent->linearVelocity * dt * 10.0f, 0.0f);

            const float rotationSpeed = 1.8f * dt;
            playerInputComponent->rotXSmooth = glm::mix(playerInputComponent->rotXSmooth, rotX * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);
            playerInputComponent->rotYSmooth = glm::mix(playerInputComponent->rotYSmooth, rotY * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);
            playerInputComponent->rotZSmooth = glm::mix(playerInputComponent->rotZSmooth, rotZ * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);

            glm::quat localOrientation = glm::quat(glm::vec3(-playerInputComponent->rotYSmooth, playerInputComponent->rotXSmooth, playerInputComponent->rotZSmooth));
            transformComponent->orientation = transformComponent->orientation * localOrientation;
            playerInputComponent->rotationZ -= playerInputComponent->rotXSmooth;
            playerInputComponent->rotationZ = glm::clamp(playerInputComponent->rotationZ, -45.0f, 45.0f);
            glm::mat4 T = glm::translate(glm::vec3(transformComponent->transform[3])) * glm::mat4(transformComponent->orientation);
            transformComponent->transform = T * glm::mat4(glm::quat(glm::vec3((0, 0, playerInputComponent->rotationZ))));
            playerInputComponent->rotationZ = glm::mix(playerInputComponent->rotationZ, 0.0f, dt * cameraComponent->cameraSmoothFactor);

            // update camera view transform
            glm::vec3 desiredCamPos = glm::vec3(transformComponent->transform[3]) + glm::vec3(transformComponent->transform * glm::vec4(0, cameraComponent->camOffsetY, -4.0f, 0));
            cameraComponent->camPos = glm::mix(cameraComponent->camPos, desiredCamPos, dt * cameraComponent->cameraSmoothFactor);
            cameraComponent->theCam->view = glm::lookAt(cameraComponent->camPos, cameraComponent->camPos + glm::vec3(transformComponent->transform[2]), glm::vec3(transformComponent->transform[1]));

            //particles for thruster of the ship
            const float thrusterPosOffset = 0.365f;
            particleComponent->particleEmitterLeft->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -thrusterPosOffset + transformComponent->transform[2] * particleComponent->emitterOffset), 1);
            particleComponent->particleEmitterLeft->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            particleComponent->particleEmitterRight->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * thrusterPosOffset + transformComponent->transform[2] * particleComponent->emitterOffset), 1);
            particleComponent->particleEmitterRight->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            float t = (playerInputComponent->currentSpeed / playerInputComponent->normalSpeed);

            particleComponent->particleEmitterLeft->data.startSpeed = 1.2 + (3.0f * t);
            particleComponent->particleEmitterLeft->data.endSpeed = 0.0f + (3.0f * t);

            particleComponent->particleEmitterRight->data.startSpeed = 1.2 + (3.0f * t);
            particleComponent->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * t);

            //canons for ship
            const float CanonPosOffset = 0.365f;
            particleComponent->particleCanonLeft->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset), 1);
            particleComponent->particleCanonLeft->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            particleComponent->particleCanonRight->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset), 1);
            particleComponent->particleCanonRight->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);
            float isSpacePressed = playerInputComponent->kbd->held[Input::Key::Space] ? 1.0f : 0.0f;
            if (isSpacePressed >= 1.0f)
            {
                particleComponent->particleCanonLeft->data.startSpeed = -(1000.2 + (3.0f + t));
                particleComponent->particleCanonLeft->data.endSpeed = -(1000.2 + (3.0f + t));
                particleComponent->particleCanonRight->data.startSpeed = -(1000.2 + (3.0f + t));
                particleComponent->particleCanonRight->data.endSpeed = -(1000.2 + (3.0f + t));
            }
            else
            {
                particleComponent->particleCanonLeft->data.startSpeed =  t;
                particleComponent->particleCanonLeft->data.endSpeed = t;
                particleComponent->particleCanonRight->data.startSpeed = t;
                particleComponent->particleCanonRight->data.endSpeed = t;
            }

            //check collisions
            glm::mat4 rotation = glm::mat4(transformComponent->orientation);
            bool hit = false;
            for (int i = 0; i < sizeof(colliderComponent->colliderEndPoints) / sizeof(glm::vec3); i++)
            {
                glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
                glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
                float len = glm::length(colliderComponent->colliderEndPoints[i]);
                Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);

                // debug draw collision rays
                Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

                if (payload.hit)
                {
                    Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                    hit = true;            
                }
                //if hit asteroid, one shot
                if (hit)
                {
                    savedID = entity->id;
                    CreatePlayerShip(true);
                    DestroyEntity(entity->id);
                    hit = false;

                }

            }

        }



    }
}
inline void World::UpdateAsteroid(Entity* entity, float dt) 
{
    if (entity->eType == EntityType::Asteroid) // asteroids
    {
        auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        transformComponent->transform = glm::rotate(transformComponent->transform, dt * glm::radians(transformComponent->rotationSpeed), transformComponent->rotationAxis);
    }
}
inline void World::draw(Entity* entity)
{
    auto renderableComponent = entity->GetComponent<Components::RenderableComponent>();
    if (renderableComponent)
    {
        Debug::DrawDebugText(std::to_string(entity->GetComponent<Components::RenderableComponent>()->ownerId).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], { 0.9f,0.9f,1,1 });

        Render::RenderDevice::Draw(renderableComponent->modelId, entity->GetComponent<Components::TransformComponent>()->transform);
    }
}
