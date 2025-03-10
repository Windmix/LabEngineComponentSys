#pragma once
#include <vector>
#include "entity.h"
#include "chunkAllocator.h"
#include "components.h"  // Assuming components are declared here



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

    

    World();
    ~World();

    static World* instance();
    static void destroy();
    // Register a new entity
    Entity* createEntity(EntityType etype);

    // Attach a component to an entity
    void AttachComponentToEntity(uint32_t entityId, ComponentBase* component, ComponentType compType, EntityType eType);

    // Get an entity by its ID
    Entity* GetEntity(uint32_t entityId);

    // Update all entities
    void Update(float dt);

    // Start all entities
    void start();

    // Destroy an entity and deallocate all components
    void DestroyEntity(uint32_t entityId);
    void Cleanup();
    void DestroyWorld();

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

inline Entity* World::createEntity(EntityType etype)
{

    // Allocate an entity from the chunk allocator
    Entity* entity = entityChunk.Allocate();
    entity->id = nextEntityId++;
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
    for (Entity* entity : entities)
    {

        if (entity->eType == EntityType::Asteroid) // asteroids
        {
            auto transformComponent = entity->GetComponent<Components::TransformComponent>();
            transformComponent->transform = glm::rotate(transformComponent->transform, dt * glm::radians(transformComponent->rotationSpeed), transformComponent->rotationAxis);
        }

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
                transformComponent->transform =  T * glm::mat4(glm::quat(glm::vec3((0, 0, playerInputComponent->rotationZ))));
                playerInputComponent->rotationZ = glm::mix(playerInputComponent->rotationZ, 0.0f, dt * cameraComponent->cameraSmoothFactor);

                // update camera view transform
                glm::vec3 desiredCamPos = glm::vec3(transformComponent->transform[3]) + glm::vec3(transformComponent->transform * glm::vec4(0, cameraComponent->camOffsetY, -4.0f, 0));
                cameraComponent->camPos = glm::mix(cameraComponent->camPos, desiredCamPos, dt * cameraComponent->cameraSmoothFactor);
                cameraComponent->theCam->view = glm::lookAt(cameraComponent->camPos, cameraComponent->camPos + glm::vec3(transformComponent->transform[2]), glm::vec3(transformComponent->transform[1]));

                //particles for the ship
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
                }

            }
            

          
        }
       
       
    }
}

inline void World::start()
{

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
                particleEmitterChunk.Deallocate(particleEmitterComp);
            }

            // Deallocate the entity from the Chunk
            entityChunk.Deallocate(entityToDelete);

            // Remove the entity from the world
            entities.erase(it);
        }
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
