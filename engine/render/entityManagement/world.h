#pragma once

#include "AstarAlgorithm.h"
#include "chunkAllocator.h"
#include "components.h"  // Assuming components are declared here
#include "render/renderdevice.h"
#include <render/model.h>
#include "pureEntityData.h"
#include <gtx/quaternion.hpp>
#include <queue>
#include <map>
#include <cstdint>



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
    ChunkAllocator<Components::AIinputController, 64> AiControllerChunk;
    ChunkAllocator<Components::ParticleEmitterComponent, 64> particleEmitterChunk;
    ChunkAllocator<Render::ParticleEmitter, 64> ChunkOfPartcles;
    ChunkAllocator<Components::AINavNodeComponent, 64> navNodeChunk;


    PureEntityData* pureEntityData;
   

    //saving for destroyed ships
    std::queue<uint32_t> savedEnemyIDs;
    std::queue<uint32_t> savedIDs;

    int randomIndex;
    float respawnTimer;
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
    void DestroyEntity(uint32_t entityId, EntityType eType);
    void DestroyShip(uint32_t shipId, EntityType);
    void Cleanup();
    void DestroyWorld();

    Entity* CreatePlayerShip(bool isRespawning);
    Entity* CreateEnemyShip(bool isRespawning);

    Entity* CreateAsteroid(float spread);
    Entity* CreatePathNode(float xOffset, float yOffset, float zOffset, float deltaXYZ);

    Entity* randomGetNode();
    Entity* getclosestNodeFromAIship(Entity* ship);

private:

    void UpdateNode(Entity* entity, float dt);

    void UpdateShip(Entity* entity, float dt);
    void UpdateAiShip(Entity* entity, float dt);

    void UpdateAsteroid(Entity* entity, float dt);
    void drawNode(Entity* entity);
    void draw(Entity* entity);
    void updateCamera(Entity* entity, float dt);

    void HandleAsteroidHit(Entity* entity, Components::ParticleEmitterComponent* particleEmit);
};


static World* _instance; // singleton


inline World::World()
{

   pureEntityData = PureEntityData::instance();
   respawnTimer = 3.0f;

}

inline World::~World()
{
    pureEntityData->destroy();
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
    entity->eType = etype;

    if (!isRespawning)
    {
        // Count how many existing entities of the same type already exist
        int count = 0;
        for (auto ent : pureEntityData->entities)
        {
            if (ent->eType == entity->eType)
            {
                count++;
            }
        }

        entity->id = count; // This ensures sequential IDs per type
    }
    else if(entity->eType == EntityType::SpaceShip && isRespawning)
    {
        entity->id = savedIDs.front();
        savedIDs.pop();
        pureEntityData->ships.push_back(entity);
    }
    else if (entity->eType == EntityType::EnemyShip && isRespawning)
    {
        entity->id = savedEnemyIDs.front();
        savedEnemyIDs.pop();
        pureEntityData->ships.push_back(entity);
    }
    pureEntityData->entities.push_back(entity);

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
    auto it = std::find_if(pureEntityData->entities.begin(), pureEntityData->entities.end(), [entityId](const Entity* entity) { return entity->id == entityId; });
    if (it != pureEntityData->entities.end())
    {
        return *it;
    }
    // Return nullptr if not found
    return nullptr;
}

inline void World::Update(float dt)
{
    for (auto asteroid : pureEntityData->Asteroids)
    {
        UpdateAsteroid(asteroid, dt);
    }
  
    for (auto ship : pureEntityData->ships)
    {
        UpdateAiShip(ship, dt);
        UpdateShip(ship, dt);
        updateCamera(ship, dt);
        
       
    }
  
    for (auto node : pureEntityData->nodes)
    {
        UpdateNode(node, dt);
    }
   

    //draw Everything
    for (auto entity : pureEntityData->entities)
    {
        draw(entity);
    }
 
    
}

inline void World::DestroyShip(uint32_t shipId, EntityType eType)
{

    auto it = std::find_if(pureEntityData->ships.begin(), pureEntityData->ships.end(), [shipId, eType](Entity* entity)
        {
            return entity->id == shipId && entity->eType == eType;
        });

    if (it != pureEntityData->ships.end())
    {
        Entity* entityToDelete = *it;

        // Deallocate all components using the appropriate chunk allocators
        for (auto* component : entityToDelete->components)
        {
            // Handle component deallocation for each type
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
            else if (auto* cameraComp = dynamic_cast<Components::CameraComponent*>(component))
            {
                cameraChunk.Deallocate(cameraComp);

            }
            else if (auto* playerInputComp = dynamic_cast<Components::PlayerInputComponent*>(component))
            {
                controlInputChunk.Deallocate(playerInputComp);

            }
            else if (auto* aiInputControllerComp = dynamic_cast<Components::AIinputController*>(component))
            {
                AiControllerChunk.Deallocate(aiInputControllerComp);

            }
            else if (auto* particleEmitterComp = dynamic_cast<Components::ParticleEmitterComponent*>(component))
            {
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonLeft);
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonRight);

                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterLeft);
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterRight);

                particleEmitterComp->particleCanonLeft->data = Render::ParticleEmitter::EmitterBlock();
                particleEmitterComp->particleCanonRight->data = Render::ParticleEmitter::EmitterBlock();
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonRight);

                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterRight);

                particleEmitterChunk.Deallocate(particleEmitterComp);
            }



        }


        //// Deallocate the entity from the Chunk
        entityChunk.Deallocate(entityToDelete);
        // Remove the ship from the entity vector
        pureEntityData->ships.erase(it);

    }
}

inline void World::DestroyEntity(uint32_t entityId, EntityType eType)
{
    auto it = std::find_if(pureEntityData->entities.begin(), pureEntityData->entities.end(), [entityId, eType](Entity* entity)
    {
            return entity->id == entityId && entity->eType == eType;
    });
    
    
   

    if (it != pureEntityData->entities.end())
    {
        Entity* entityToDelete = *it;

        // Deallocate all components using the appropriate chunk allocators
        for (auto* component : entityToDelete->components)
        {
            // Handle component deallocation for each type
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
            else if (auto* cameraComp = dynamic_cast<Components::CameraComponent*>(component))
            {
                cameraChunk.Deallocate(cameraComp);

            }
            else if (auto* playerInputComp = dynamic_cast<Components::PlayerInputComponent*>(component))
            {
                controlInputChunk.Deallocate(playerInputComp);

            }
            else if (auto* aiInputControllerComp = dynamic_cast<Components::AIinputController*>(component))
            {
                AiControllerChunk.Deallocate(aiInputControllerComp);

            }
            else if (auto* particleEmitterComp = dynamic_cast<Components::ParticleEmitterComponent*>(component))
            {
                particleEmitterComp->particleCanonLeft = nullptr;
                particleEmitterComp->particleCanonRight = nullptr;
                particleEmitterComp->particleEmitterLeft = nullptr;
                particleEmitterComp->particleEmitterRight = nullptr;
                particleEmitterComp->numParticles = 0;
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonLeft);
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonRight);

                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterLeft);
                Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterRight);

               
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonRight);

                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterRight);

                particleEmitterChunk.Deallocate(particleEmitterComp);
            }
            
       
          
        }


        //// Deallocate the entity from the Chunk
        entityChunk.Deallocate(entityToDelete);

        // Remove the entities from the entity vector
       pureEntityData->entities.erase(it);
      
       

       
    }

}

inline void World::Cleanup()
{
    // Deallocate all entities and their components
    // Deallocate all entities and their components
    for (Entity* entity : pureEntityData->entities)
    {
        for (auto* component : entity->components)
        {
            if (component) 
            {
                // Handle component deallocation for each type
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
                else if (auto* aiInputControllerComp = dynamic_cast<Components::AIinputController*>(component))
                {
                    AiControllerChunk.Deallocate(aiInputControllerComp);
                }
                else if (auto* cameraComp = dynamic_cast<Components::CameraComponent*>(component))
                {
                    cameraChunk.Deallocate(cameraComp);
                }
                else if (auto* particleEmitterComp = dynamic_cast<Components::ParticleEmitterComponent*>(component))
                {
                    Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonLeft);
                    Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleCanonRight);

                    Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterLeft);
                    Render::ParticleSystem::Instance()->RemoveEmitter(particleEmitterComp->particleEmitterRight);

                    particleEmitterComp->particleCanonLeft->data = Render::ParticleEmitter::EmitterBlock();
                    particleEmitterComp->particleCanonRight->data = Render::ParticleEmitter::EmitterBlock();
                    ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonLeft);
                    ChunkOfPartcles.Deallocate(particleEmitterComp->particleCanonRight);

                    ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterLeft);
                    ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterRight);

                    particleEmitterChunk.Deallocate(particleEmitterComp);
                }
                else if (auto* aiNavNodeComp = dynamic_cast<Components::AINavNodeComponent*>(component))
                {
                    navNodeChunk.Deallocate(aiNavNodeComp);
                }
            }
        }

        // Deallocate the entity itself
        entityChunk.Deallocate(entity);

    }

    // Clear the entity list after deallocation
    pureEntityData->entities.clear();
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
inline  Entity* World::CreatePlayerShip(bool isRespawning)
{
    Render::ModelId  shipModel = Render::LoadModel("assets/space/spaceship.glb");
    Physics::ColliderMeshId  ColliderMeshShip = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
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

    const glm::vec3 rayCastEndPoints[50] =
    {
        // Forward rays
        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, 0.0, 50.0),

        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, 3.0, 20.0),

        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, -3.0,20.0),

       // up rays
       glm::vec3(0.0, 0.0, 0.3), 
       glm::vec3(0.0, 7.0, 20.0),

       // down rays
       glm::vec3(0.0, 0.0, 0.3), 
       glm::vec3(0.0, -7.0, 20.0), 

       // Forward rays L
       glm::vec3(1.33578, 0.0, 0.3),
       glm::vec3(1.33578, 0.0, 21.0),

       glm::vec3(1.33578, 0.0, 0.3),
       glm::vec3(1.33578, 3.0, 20.0),

       glm::vec3(1.33578, 0.0, 0.3),
       glm::vec3(1.33578, -3.0,20.0),

       // up rays L
       glm::vec3(1.33578, 0.0, 0.3),
       glm::vec3(1.33578, 7.0, 20.0),

       // down rays L
       glm::vec3(1.33578, 0.0, 0.3),
       glm::vec3(1.33578, -7.0, 20.0),

       // Forward rays R
       glm::vec3(-1.33578, 0.0, 0.3),
       glm::vec3(-1.33578, 0.0, 21.0),

       glm::vec3(-1.33578, 0.0, 0.3),
       glm::vec3(-1.33578, 3.0, 20.0),

       glm::vec3(-1.33578, 0.0, 0.3),
       glm::vec3(-1.33578,-3.0,20.0),

       // up rays R
       glm::vec3(-1.33578, 0.0, 0.3),
       glm::vec3(-1.33578, 7.0, 20.0),

       // down rays R
       glm::vec3(-1.33578, 0.0, 0.3),
       glm::vec3(-1.33578, -7.0, 20.0),

       //left rays
       glm::vec3(1.33578, 0.0, 0.088893),
       glm::vec3(1.33578, 0.0, 21.0),

        glm::vec3(1.33578, 0.0, 0.088893),
       glm::vec3(10.0, 0.0, 21.0),

       //right rays
       glm::vec3(-1.33578, 0.0, 0.088893),
       glm::vec3(-1.33578, 0.0, 21.0),

        glm::vec3(-1.33578, 0.0, 0.088893),
       glm::vec3(-10.0, 0.0, 21.0),

    };
    

    Entity* spaceship;

    if (!isRespawning)
    {
        spaceship = createEntity(EntityType::SpaceShip, isRespawning);


    }
    else
    {
        spaceship = createEntity(EntityType::SpaceShip, isRespawning);
    }

    Components::TransformComponent* newTransform = transformChunk.Allocate();
    spaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::SpaceShip);
    newTransform->transform[3] = glm::vec4(0.0f, 0.0f + 10.0f, 0.0f, 0.0f);

    Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
    spaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::SpaceShip);

    Components::ColliderComponent* collider = colliderChunk.Allocate();
    spaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::SpaceShip);
    for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
    {
        collider->colliderEndPoints.push_back(colliderEndPoints[i]);
    }
    for (int i = 0; i < sizeof(collider->rayCastPoints) / sizeof(glm::vec3); i++)
    {
        collider->rayCastPoints.push_back(rayCastEndPoints[i]);
    }

    collider->colliderID = Physics::CreateCollider(ColliderMeshShip, newTransform->transform);
    collider->UsingEntityType = EntityType::SpaceShip;

    Components::CameraComponent* camera = cameraChunk.Allocate();
    spaceship->AddComponent(camera, ComponentType::CAMERA, EntityType::SpaceShip);
    camera->theCam = Render::CameraManager::GetCamera(CAMERA_MAIN);

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
        .endColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f),
        .numParticles = particleEmitter->numParticles,
        .theta = glm::radians(1.0f),
        .startSpeed = 1.2f,
        .endSpeed = 0.0f,
        .startScale = 0.01f,
        .endScale = 0.0f,
        .decayTime = 2.58f,
        .randomTimeOffsetDist = 2.58f,
        .looping = 1,
        .emitterType = 1,
        .discRadius = 0.1f
    };


    particleEmitter->particleEmitterRight->data = particleEmitter->particleEmitterLeft->data;

    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterLeft);
    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterRight);


    particleEmitter->particleCanonLeft->data = {
       .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->canonEmitterOffset), 1.0f),
       .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
       .startColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) * 3.0f,
       .endColor = glm::vec4(0,0,0,1.0f),
       .numParticles = particleEmitter->numParticles,
       .theta = glm::radians(0.0f),
       .startSpeed = 10.0f,
       .endSpeed = 10.0f,
       .startScale = 0.1f,
       .endScale = 0.015f,
       .decayTime = 0.5f,
       .randomTimeOffsetDist = 0.00001f,
       .looping = 0,
       .emitterType = 1,
       .discRadius = 0.1f
    };

    particleEmitter->particleCanonRight->data = particleEmitter->particleCanonLeft->data;

    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonLeft);
    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonRight);

    return spaceship;
}
inline  Entity* World::CreateEnemyShip(bool isRespawning)
{
    auto shipModel = Render::LoadModel("assets/space/spaceship.glb");
    Physics::ColliderMeshId  ColliderMeshShip = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");

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
    const glm::vec3 rayCastEndPoints[50] =
    {
        // Forward rays
        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, 0.0, 21.0),

        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, 6.0, 20.0),

        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, -6.0,20.0),

        // up rays
        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, 10.0, 20.0),

        // down rays
        glm::vec3(0.0, 0.0, 0.3),
        glm::vec3(0.0, -10.0, 20.0),

        // Forward rays L
        glm::vec3(1.33578, 0.0, 0.3),
        glm::vec3(1.33578, 0.0, 21.0),

        glm::vec3(1.33578, 0.0, 0.3),
        glm::vec3(1.33578, 6.0, 20.0),

        glm::vec3(1.33578, 0.0, 0.3),
        glm::vec3(1.33578, -6.0,20.0),

        // up rays L
        glm::vec3(1.33578, 0.0, 0.3),
        glm::vec3(1.33578, 10.0, 20.0),

        // down rays L
        glm::vec3(1.33578, 0.0, 0.3),
        glm::vec3(1.33578, -10.0, 20.0),

        // Forward rays R
        glm::vec3(-1.33578, 0.0, 0.3),
        glm::vec3(-1.33578, 0.0, 21.0),

        glm::vec3(-1.33578, 0.0, 0.3),
        glm::vec3(-1.33578, 6.0, 20.0),

        glm::vec3(-1.33578, 0.0, 0.3),
        glm::vec3(-1.33578,-6.0,20.0),

        // up rays R
        glm::vec3(-1.33578, 0.0, 0.3),
        glm::vec3(-1.33578, 10.0, 20.0),

        // down rays R
        glm::vec3(-1.33578, 0.0, 0.3),
        glm::vec3(-1.33578, -10.0, 20.0),

        //left rays
        glm::vec3(1.33578, 0.0, 0.088893),
        glm::vec3(1.33578, 0.0, 21.0),

         glm::vec3(1.33578, 0.0, 0.088893),
        glm::vec3(10.0, 0.0, 21.0),

        //right rays
        glm::vec3(-1.33578, 0.0, 0.088893),
        glm::vec3(-1.33578, 0.0, 21.0),

         glm::vec3(-1.33578, 0.0, 0.088893),
        glm::vec3(-10.0, 0.0, 21.0),

    };



    Entity* AIspaceship;
    if (!isRespawning)
    {
        AIspaceship = createEntity(EntityType::EnemyShip, isRespawning);
    }
    else
    {
        AIspaceship = createEntity(EntityType::EnemyShip, isRespawning);
    }

    Components::TransformComponent* newTransform = transformChunk.Allocate();
    AIspaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::EnemyShip);
    /* newTransform->transform[3] = glm::vec4(0.0f + i*2, 0.0f + i*2, 0.0f + i*2, 0.0f);*/

    Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
    AIspaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::EnemyShip);

    Components::ColliderComponent* collider = colliderChunk.Allocate();
    AIspaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::EnemyShip);
    for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
    {
        collider->colliderEndPoints.push_back(colliderEndPoints[i]);
    }
    for (int i = 0; i < sizeof(collider->rayCastPoints) / sizeof(glm::vec3); i++)
    {
        collider->rayCastPoints.push_back(rayCastEndPoints[i]);
    }
    collider->colliderID = Physics::CreateCollider(ColliderMeshShip, newTransform->transform);
    collider->UsingEntityType = EntityType::EnemyShip;

    Components::AIinputController* controllinput = AiControllerChunk.Allocate();
    AIspaceship->AddComponent(controllinput, ComponentType::AI_CONTROLLER, EntityType::EnemyShip);

    Components::CameraComponent* camera = cameraChunk.Allocate();
    AIspaceship->AddComponent(camera, ComponentType::CAMERA, EntityType::EnemyShip);

    Components::ParticleEmitterComponent* particleEmitter = particleEmitterChunk.Allocate();
    AIspaceship->AddComponent(particleEmitter, ComponentType::PARTICLE_EMITTER, EntityType::EnemyShip);


    particleEmitter->particleEmitterLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
    particleEmitter->particleEmitterRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);

    particleEmitter->particleCanonLeft = ChunkOfPartcles.Allocate(particleEmitter->numParticles);
    particleEmitter->particleCanonRight = ChunkOfPartcles.Allocate(particleEmitter->numParticles);



    particleEmitter->particleEmitterLeft->data = {
    .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->emitterOffset), 1.0f),
    .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
    .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
    .endColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f),
    .numParticles = particleEmitter->numParticles,
    .theta = glm::radians(1.0f),
    .startSpeed = 1.2f,
    .endSpeed = 0.0f,
    .startScale = 0.01f,
    .endScale = 0.0f,
    .decayTime = 2.58f,
    .randomTimeOffsetDist = 2.58f,
    .looping = 1,
    .emitterType = 1,
    .discRadius = 0.1f
    };


    particleEmitter->particleEmitterRight->data = particleEmitter->particleEmitterLeft->data;

    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterLeft);
    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterRight);

    particleEmitter->particleCanonLeft->data = {
       .origin = glm::vec4(glm::vec3(newTransform->transform[3]) + (glm::vec3(newTransform->transform[2]) * particleEmitter->canonEmitterOffset), 1.0f),
       .dir = glm::vec4(glm::vec3(newTransform->transform[2]), 0),
       .startColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) * 3.0f,
       .endColor = glm::vec4(0,0,0,1.0f),
       .numParticles = particleEmitter->numParticles,
       .theta = glm::radians(0.0f),
       .startSpeed = 10.0f,
       .endSpeed = 10.0f,
       .startScale = 0.1f,
       .endScale = 0.015f,
       .decayTime = 0.5f,
       .randomTimeOffsetDist = 0.00001f,
       .looping = 0,
       .emitterType = 1,
       .discRadius = 0.1f
    };

    particleEmitter->particleCanonRight->data = particleEmitter->particleCanonLeft->data;

    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonLeft);
    Render::ParticleSystem::Instance()->AddEmitter(particleEmitter->particleCanonRight);
    return AIspaceship;

}
inline Entity* World::CreateAsteroid(float spread)
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
        float rotationSpeed = Core::RandomFloat() * 1.0f + 9.0f;  // Random speed between 1 and 9
        newTransform->rotationSpeed = rotationSpeed;
        collider->colliderID = Physics::CreateCollider(colliderMeshes[resourceIndex], newTransform->transform);
        collider->UsingEntityType = EntityType::Asteroid;

    }
    return asteroidEntity;
}
inline Entity* World::CreatePathNode(float xOffset, float yOffset, float zOffset, float deltaXYZ)
{
    const glm::vec3 EndPoints[6] =
    {
        glm::vec3(-1.0f, 0.0f, 0.0f),  // left
        glm::vec3(1.0f, 0.0f, 0.0f),  // right

        glm::vec3(0.0f, -1.0f, 0.0f),  // down
        glm::vec3(0.0f, 1.0f, 0.0f),  // up

        glm::vec3(0.0f, 0.0f, -1.0f),  // backward
        glm::vec3(0.0f, 0.0f, 1.0f),  // forward
       
        
    };

    Entity* node = createEntity(EntityType::Node, false);
    Components::ColliderComponent* colComp = colliderChunk.Allocate();
    node->AddComponent(colComp, ComponentType::COLLIDER, EntityType::Node);
    for (int i = 0; i < sizeof(EndPoints) / sizeof(glm::vec3); i++)
    {
        colComp->EndPointsNodes[i] = EndPoints[i];
    }
    Components::TransformComponent* newTransform = transformChunk.Allocate();
    node->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::Node);
    newTransform->transform[3] = glm::vec4(-100.0f + xOffset * deltaXYZ, -100.0f + yOffset * deltaXYZ, -100.0f + zOffset * deltaXYZ, 0);

   
    Components::AINavNodeComponent* NodeComp = navNodeChunk.Allocate();
    node->AddComponent(NodeComp, ComponentType::NAVNODE, EntityType::Node);
    for (int i = 0; i < sizeof(EndPoints) / sizeof(glm::vec3); i++)
    {
        NodeComp->EndPoints[i];
    }

    for (int i = 0; i < sizeof(colComp->EndPointsNodes) / sizeof(glm::vec3); i++)
    {
        bool hit = false;
        glm::vec3 pos = glm::vec3(newTransform->transform[3]);
        glm::vec3 dir = newTransform->transform * glm::vec4(glm::normalize(colComp->EndPointsNodes[i]), 0.0f);
        float len = glm::length(colComp->EndPointsNodes[i]);
        Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(newTransform->transform[3]), dir, len);

        // debug draw collision rays
        //Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

        if (payload.hit)
        {
            Debug::DrawDebugText("Node_hit_asteroids", payload.hitPoint, glm::vec4(1, 1, 1, 1));
            hit = true;
        }
        //if hit asteroid, one shot
        if (hit)
        {
            NodeComp->isCollidedAsteroids = true;
        }

    }


    //give it to the map too
    pureEntityData->gridNodes[node->id] = node;

    return node;

   
}
inline Entity* World::randomGetNode()
{
    randomIndex = std::rand() % pureEntityData->nodes.size();
    return pureEntityData->gridNodes[randomIndex];
}
inline Entity* World::getclosestNodeFromAIship(Entity* ship)
{
    Entity* closestNode = nullptr;
    float minDistanceSquared = std::numeric_limits<float>::max();  // Use squared distance to avoid sqrt
    auto shipPosition = glm::vec3(ship->GetComponent<Components::TransformComponent>()->transform[3]);

    for (auto& node : pureEntityData->nodes)
    {
        // Ensure the node has a TransformComponent and is valid
        auto nodeTransform = node->GetComponent<Components::TransformComponent>();


        // Get the position of the node
        auto nodePosition = glm::vec3(nodeTransform->transform[3]);

        // Calculate the squared distance
        glm::vec3 vectorShipPos2NodePos = nodePosition - shipPosition;
        float distanceSquared = glm::dot(vectorShipPos2NodePos, vectorShipPos2NodePos);

        // Update closest node if a new minimum distance is found
        if (distanceSquared < minDistanceSquared)
        {
            minDistanceSquared = distanceSquared;
            closestNode = node;
        }
    }

    return closestNode;

    
}
inline void World::UpdateNode(Entity* entity, float dt)
{
    
    drawNode(entity);
}
inline void World::UpdateShip(Entity* entity, float dt)
{
    if (entity->eType == EntityType::SpaceShip && entity) //playerShip
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
            if (cameraComponent->theCam != nullptr)
            {
                cameraComponent->camPos = glm::mix(cameraComponent->camPos, desiredCamPos, dt * cameraComponent->cameraSmoothFactor);
                cameraComponent->theCam->view = glm::lookAt(cameraComponent->camPos, cameraComponent->camPos + glm::vec3(transformComponent->transform[2]), glm::vec3(transformComponent->transform[1]));
            }        
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
                particleComponent->particleCanonLeft->data.looping = 1;
                particleComponent->particleCanonRight->data.looping = 1;

                particleComponent->particleCanonLeft->data.randomTimeOffsetDist = 0.0001;
                particleComponent->particleCanonRight->data.randomTimeOffsetDist = 0.0001;

                particleComponent->particleCanonLeft->data.startSpeed = -500.0f;
                particleComponent->particleCanonLeft->data.endSpeed = -500.0f;
                particleComponent->particleCanonRight->data.startSpeed = -500.0f;
                particleComponent->particleCanonRight->data.endSpeed = -500.0f;
            }
            else
            {
                particleComponent->particleCanonLeft->data.looping = 0;
                particleComponent->particleCanonRight->data.looping = 0;
            }

            //check collisions
            glm::mat4 rotation = glm::mat4(transformComponent->orientation);
            bool hit = false;

            for (int i = 0; i < colliderComponent->colliderEndPoints.size(); i++)
            {
                glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
                glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
                float len = glm::length(colliderComponent->colliderEndPoints[i]);
                Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);

                // debug draw collision rays
                int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
                if (menuIsUsingRayCasts == 1.0f)
                {
                    Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
                }


                if (payload.hit)
                {
                    Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));

                    for (auto entityIn : pureEntityData->Asteroids)
                    {
                        auto entComp = entityIn->GetComponent<Components::ColliderComponent>();
                        if (payload.collider == entComp->colliderID && entityIn->eType == EntityType::Asteroid)
                        {
                            particleComponent->particleCanonLeft->data.looping = 0;
                            particleComponent->particleCanonRight->data.looping = 0;

                            savedIDs.push(entity->id);


                            DestroyShip(entity->id, entity->eType);
                            DestroyEntity(entity->id, entity->eType);
                            CreatePlayerShip(true);

                            return;
                        }
                    }

                }

            }
      

            //glm::mat4 transform = transformComponent->transform;
            //glm::vec3 avoidanceOffset(0.0f);
            //entity->isAvoidingAsteroids = false;

            ////Define raycast result holders
            //Physics::RaycastPayload pf, pf1, pf2;
            //Physics::RaycastPayload pu, pd;
            //Physics::RaycastPayload pfl, pfl1, pfl2;
            //Physics::RaycastPayload pul, pdl;
            //Physics::RaycastPayload pfr, pfr1, pfr2;
            //Physics::RaycastPayload pur, pdr;
            //Physics::RaycastPayload pl, pl1;
            //Physics::RaycastPayload pr, pr1;

            //// === Forward rays (center) ===
            //glm::vec3 fStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[0], 1.0f));
            //glm::vec3 fEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[1], 1.0f));
            //float fLength = glm::length(fEnd - fStart);
            //pf = Physics::Raycast(fStart, fEnd, fLength);

            //glm::vec3 f1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[2], 1.0f));
            //glm::vec3 f1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[3], 1.0f));
            //float f1Length = glm::length(f1End - f1Start);

            //pf1 = Physics::Raycast(f1Start, f1End, f1Length);

            //glm::vec3 f2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[4], 1.0f));
            //glm::vec3 f2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[5], 1.0f));
            //float f2Length = glm::length(f2End - f2Start);
            //pf2 = Physics::Raycast(f2Start, f2End, f2Length);

            //// === Up ray (center) ===
            //glm::vec3 uStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[6], 1.0f));
            //glm::vec3 uEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[7], 1.0f));
            //float uLength = glm::length(uEnd - uStart);
            //pu = Physics::Raycast(uStart, uEnd, uLength);

            //// === Down ray (center) ===
            //glm::vec3 dStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[8], 1.0f));
            //glm::vec3 dEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[9], 1.0f));
            //float dLength = glm::length(dEnd - dStart);

            //pd = Physics::Raycast(dStart, dEnd, dLength);

            //// === Forward rays (Left) ===
            //glm::vec3 flStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[10], 1.0f));
            //glm::vec3 flEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[11], 1.0f));
            //float flLength = glm::length(flEnd - flStart);

            //pfl = Physics::Raycast(flStart, flEnd, flLength);

            //glm::vec3 fl1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[12], 1.0f));
            //glm::vec3 fl1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[13], 1.0f));
            //float fl1Length = glm::length(fl1End - fl1Start);

            //pfl1 = Physics::Raycast(fl1Start, fl1End, fl1Length);

            //glm::vec3 fl2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[14], 1.0f));
            //glm::vec3 fl2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[15], 1.0f));
            //float fl2Length = glm::length(fl2End - fl2Start);

            //pfl2 = Physics::Raycast(fl2Start, fl2End, fl2Length);

            //// === Up ray (Left) ===
            //glm::vec3 ulStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[16], 1.0f));
            //glm::vec3 ulEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[17], 1.0f));
            //float ulLength = glm::length(ulEnd - ulStart);

            //pul = Physics::Raycast(ulStart, ulEnd, ulLength);

            //// === Down ray (Left) ===
            //glm::vec3 dlStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[18], 1.0f));
            //glm::vec3 dlEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[19], 1.0f));
            //float dlLength = glm::length(dlEnd - dlStart);

            //pdl = Physics::Raycast(dlStart, dlEnd, dlLength);

            //// === Forward rays (Right) ===
            //glm::vec3 frStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[20], 1.0f));
            //glm::vec3 frEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[21], 1.0f));
            //float frLength = glm::length(frEnd - frStart);

            //pfr = Physics::Raycast(frStart, frEnd, frLength);

            //glm::vec3 fr1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[22], 1.0f));
            //glm::vec3 fr1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[23], 1.0f));
            //float fr1Length = glm::length(fr1End - fr1Start);

            //pfr1 = Physics::Raycast(fr1Start, fr1End, fr1Length);

            //glm::vec3 fr2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[24], 1.0f));
            //glm::vec3 fr2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[25], 1.0f));
            //float fr2Length = glm::length(fr2End - fr2Start);

            //pfr2 = Physics::Raycast(fr2Start, fr2End, fr2Length);

            //// === Up ray (Right) ===
            //glm::vec3 urStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[26], 1.0f));
            //glm::vec3 urEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[27], 1.0f));
            //float urLength = glm::length(urEnd - urStart);

            //pur = Physics::Raycast(urStart, urEnd, urLength);

            //// === Down ray (Right) ===
            //glm::vec3 drStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[28], 1.0f));
            //glm::vec3 drEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[29], 1.0f));
            //float drLength = glm::length(drEnd - drStart);

            //pdr = Physics::Raycast(drStart, drEnd, drLength);

            //// === Left rays ===
            //glm::vec3 lStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[30], 1.0f));
            //glm::vec3 lEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[31], 1.0f));
            //float lLength = glm::length(lEnd - lStart);

            //pl = Physics::Raycast(lStart, lEnd, lLength);

            //glm::vec3 l1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[32], 1.0f));
            //glm::vec3 l1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[33], 1.0f));
            //float l1Length = glm::length(l1End - l1Start);

            //pl1 = Physics::Raycast(l1Start, l1End, l1Length);

            //// === Right rays ===
            //glm::vec3 rStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[34], 1.0f));
            //glm::vec3 rEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[35], 1.0f));
            //float rLength = glm::length(rEnd - rStart);

            //pr = Physics::Raycast(rStart, rEnd, rLength);

            //glm::vec3 r1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[36], 1.0f));
            //glm::vec3 r1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[37], 1.0f));
            //float r1Length = glm::length(r1End - r1Start);

            //pr1 = Physics::Raycast(r1Start, r1End, r1Length);

            //int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
            //if (menuIsUsingRayCasts == 1.0f)
            //{
            //    Debug::DrawLine(fStart, fEnd, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(f1Start, f1End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(f2Start, f2End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(uStart, uEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(dStart, dEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(flStart, flEnd, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(fl1Start, fl1End, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(fl2Start, fl2End, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(ulStart, ulEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(dlStart, dlEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(frStart, frEnd, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(fr1Start, fr1End, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(fr2Start, fr2End, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(urStart, urEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(drStart, drEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(lStart, lEnd, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(l1Start, l1End, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(rStart, rEnd, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //    Debug::DrawLine(r1Start, r1End, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
            //}



            //// Logic to determine avoidance actions
            //if ((pf.hit || pf1.hit || pf2.hit || pu.hit || pd.hit || pl.hit || pl1.hit || pr.hit || pr1.hit))
            //{
            //    entity->isAvoidingAsteroids = true;

            //   
            //    entity->avoidanceTime = 0.0f;
            //}

            //else
            //{
            //    // Cooldown is over, stop avoiding and resume pathfinding
            //    entity->isAvoidingAsteroids = false;
            //}
          

            //// Logic to determine avoidance actions
            //if (
            //    pf.hit || pf1.hit || pf2.hit ||
            //    pfl.hit || pfl1.hit || pfl2.hit ||
            //    pfr.hit || pfr1.hit || pfr2.hit ||
            //    pu.hit || pd.hit || pul.hit || pdl.hit || pur.hit || pdr.hit ||
            //    pl.hit || pl1.hit || pr.hit || pr1.hit)
            //{
            //    entity->isAvoidingAsteroids = true;
            //    entity->avoidanceTime = 0.0f;
            //}
            //else
            //{
            //    // Cooldown is over, stop avoiding and resume pathfinding
            //    entity->isAvoidingAsteroids = false;
            //}

            //if (
            //    entity->isAvoidingAsteroids &&
            //    (
            //        pf.hit || pf1.hit || pf2.hit ||
            //        pfl.hit || pfl1.hit || pfl2.hit ||
            //        pfr.hit || pfr1.hit || pfr2.hit ||
            //        pu.hit || pd.hit || pul.hit || pdl.hit || pur.hit || pdr.hit ||
            //        pl.hit || pl1.hit || pr.hit || pr1.hit
            //        )
            //    )
            //{
            //    float rotationSpeed2 = 20.0 * dt; // Adjust this value to control the speed of rotation

            //    // Handle avoidance logic based on hit rays
            //    if (pf.hit || pf1.hit || pf2.hit || pfl.hit || pfl1.hit || pfl2.hit || pfr.hit || pfr1.hit || pfr2.hit)
            //    {
            //        // Up if forward rays hit
            //        playerInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
            //    }

            //    if (pu.hit || pul.hit || pur.hit)
            //    {
            //        // Down if upward rays hit
            //        playerInputComponent->rotYSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
            //    }

            //    if (pd.hit || pdl.hit || pdr.hit)
            //    {
            //        // Up if downward rays hit
            //        playerInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
            //    }

            //    if (pl.hit || pl1.hit)
            //    {
            //        // right if left rays hit
            //        playerInputComponent->rotXSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
            //    }
            //    else if (pr.hit || pr1.hit)
            //    {
            //        // left if right rays hit
            //        playerInputComponent->rotXSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
            //    }
            //}

        }

    }
}
inline void World::UpdateAiShip(Entity* entity, float dt)
{
    if (entity->eType == EntityType::EnemyShip && entity)
    {
        auto aiInputComponent = entity->GetComponent<Components::AIinputController>();
        auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        auto cameraComponent = entity->GetComponent<Components::CameraComponent>();
        auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
        auto particleComponent = entity->GetComponent<Components::ParticleEmitterComponent>();

        AstarAlgorithm* astar = AstarAlgorithm::Instance();

        Components::TransformComponent* closeNodeTranscomp;
        glm::vec3 targetDirection;
        glm::quat targetRotation;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        Debug::DrawDebugText(std::to_string(entity->id).c_str(), transformComponent->transform[3], { 0.9f,0.9f,1,1 });

        if (aiInputComponent->isForward)
        {
            aiInputComponent->currentSpeed = glm::min(aiInputComponent->currentSpeed + dt, 1.0f);

        }

        // Find the closest node
        if (!entity->closestNodeCalled)
        {
            entity->closestNodeFromShip = getclosestNodeFromAIship(entity);
            entity->closestNodeCalled = true;
            
        }

        closeNodeTranscomp = entity->closestNodeFromShip->GetComponent<Components::TransformComponent>();
     
        glm::vec3 currentPos = glm::vec3(transformComponent->transform[3]);

     

        // If the ship hasn't reached the start node, go to start node
        if (!entity->hasReachedTheStartNode && !entity->isAvoidingAsteroids)
        {
            glm::vec3 targetPos = glm::vec3(closeNodeTranscomp->transform[3]);
            glm::vec3 fromCurrent2Target = targetPos - currentPos;
            float distance = glm::dot(fromCurrent2Target, fromCurrent2Target);

            int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
            if (menuIsUsingRayCasts == 1.0f)
            {
                Debug::DrawLine(transformComponent->transform[3], closeNodeTranscomp->transform[3], 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            }

            if (distance <= 40.0f && entity->path.empty() && !entity->isAvoidingAsteroids) // automatic waypoint system
            {
                entity->hasReachedTheStartNode = true;

                // Request the actual path
                auto randomDestination = randomGetNode();
                entity->path = astar->findPath(entity->closestNodeFromShip, randomDestination);
            }

            else
            {
                // Normalize the direction to the target
                targetDirection = glm::normalize(fromCurrent2Target);
                glm::vec3 currentForward = glm::vec3(0, 0, 1);

                // Calculate the required angle and axis to rotate towards the target
                float angle = glm::acos(glm::dot(currentForward, targetDirection));
                glm::vec3 axis = glm::normalize(glm::cross(currentForward, targetDirection));


                // Create the target rotation quaternion based on the angle and axis
                targetRotation = glm::angleAxis(angle, axis);

                // Interpolate between current and target rotation to smoothly turn
                float rotationSpeed = 1.0f * dt;
                glm::quat newOrientation = glm::slerp(glm::quat(transformComponent->orientation), targetRotation, rotationSpeed);

                // Update the ship's forward direction based on the new orientation
                glm::vec3 newForward = newOrientation * currentForward;
                aiInputComponent->rotationInputX = newForward.x;
                aiInputComponent->rotationInputY = newForward.y;
                aiInputComponent->rotationInputZ = newForward.z;
                aiInputComponent->isForward = true; // Ensure the ship is moving forward

                // Update the ship's orientation
                transformComponent->orientation = newOrientation;
            }
        }

        // If the ship is following the path
        else if (!entity->path.empty() && entity->pathIndex < entity->path.size() && entity->hasReachedTheStartNode && !entity->isAvoidingAsteroids)
        {
            Entity* nextNode = entity->path[entity->pathIndex];
            auto nextTransform = nextNode->GetComponent<Components::TransformComponent>();
            glm::vec3 targetPos = glm::vec3(nextTransform->transform[3]);
            glm::vec3 fromCurrent2Target = targetPos - currentPos;
            float distance = glm::dot(fromCurrent2Target, fromCurrent2Target);
            int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
            if (menuIsUsingRayCasts == 1.0f)
            {
                Debug::DrawLine(transformComponent->transform[3], nextTransform->transform[3], 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            }
            // If close enough to the target node
            if (distance <= 40.0f)
            {

                //adjust updates
                entity->nodeArrivalTimer += dt;
                if (entity->nodeArrivalTimer >= 0.05f)
                {
                    entity->pathIndex++;
                    entity->nodeArrivalTimer = 0.0f;
                }
            }


            else
            {
                // Normalize the direction to the target
                targetDirection = glm::normalize(fromCurrent2Target);
                glm::vec3 currentForward = glm::vec3(0, 0, 1);

                // Calculate the required angle and axis to rotate towards the target
                float angle = glm::acos(glm::dot(currentForward, targetDirection));
                glm::vec3 axis = glm::normalize(glm::cross(currentForward, targetDirection));

                // If the target is almost directly behind, adjust the axis to 'up'
                if (glm::length(axis) < 0.001f)
                {
                    axis = glm::vec3(0.0f, 1.0f, 0.0f); // Up vector
                }

                // Handle movement and speed adjustments
                if (aiInputComponent->isForward)
                {
                    aiInputComponent->currentSpeed = glm::min(aiInputComponent->currentSpeed + dt, 1.0f);
                    
                }


                // Create the target rotation quaternion based on the angle and axis
                targetRotation = glm::angleAxis(angle, axis);

                // Interpolate between current and target rotation to smoothly turn
                float rotationSpeed = 2.0 * dt;
                glm::quat newOrientation = glm::slerp(glm::quat(transformComponent->orientation), targetRotation, rotationSpeed);

                // Update the ship's forward direction based on the new orientation
                glm::vec3 newForward = newOrientation * currentForward;
                aiInputComponent->rotationInputX = newForward.x;
                aiInputComponent->rotationInputY = newForward.y;
                aiInputComponent->rotationInputZ = newForward.z;
                aiInputComponent->isForward = true; // Ensure the ship is moving forward

                // Update the ship's orientation
                transformComponent->orientation = newOrientation;
            }

        }
        // If the ship has completed the path
        if (entity->pathIndex >= entity->path.size() && entity->hasReachedTheStartNode && !entity->isAvoidingAsteroids)
        {
            entity->closestNodeCalled = false;
            entity->hasReachedTheStartNode = false;
            entity->path.clear();
            entity->closestNodeFromShip = nullptr;
            entity->pathIndex = 0;
        }

        // Draw the path
        for (auto node : entity->path)
        {
            auto aiComp = node->GetComponent<Components::AINavNodeComponent>();
            int menuIsUsingDrawPath(Core::CVarReadInt(aiComp->r_draw_path));
            if (menuIsUsingDrawPath)
            {
                auto nextNode = node->parentNode;
                auto currentNode = node;

                if (nextNode != nullptr)
                {
                    auto transformComponentdestNode = nextNode->GetComponent<Components::TransformComponent>();
                    auto transformComponentprevNode = currentNode->GetComponent<Components::TransformComponent>();
                    Debug::DrawLine(transformComponentprevNode->transform[3], transformComponentdestNode->transform[3], 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
                }
            }
            else
            {
                break;
            }

        }

        // Update ship movement and orientation
        if (aiInputComponent && transformComponent && cameraComponent)
        {


            glm::vec3 desiredVelocity = glm::vec3(0, 0, aiInputComponent->currentSpeed);
            desiredVelocity = transformComponent->transform * glm::vec4(desiredVelocity, 0.0f);
            transformComponent->linearVelocity = glm::mix(transformComponent->linearVelocity, desiredVelocity, aiInputComponent->accelerationFactor);

            float rotX = aiInputComponent->rotXSmooth;
            float rotY = aiInputComponent->rotYSmooth;
            float rotZ = aiInputComponent->rotZSmooth;

            transformComponent->transform[3] += glm::vec4(transformComponent->linearVelocity * dt * 10.0f, 0.0f);

            const float rotationSpeed = 0.5f * dt;
            aiInputComponent->rotXSmooth = glm::mix(aiInputComponent->rotXSmooth, rotX * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);
            aiInputComponent->rotYSmooth = glm::mix(aiInputComponent->rotYSmooth, rotY * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);
            aiInputComponent->rotZSmooth = glm::mix(aiInputComponent->rotZSmooth, rotZ * rotationSpeed, dt * cameraComponent->cameraSmoothFactor);

            glm::quat localOrientation = glm::quat(glm::vec3(-aiInputComponent->rotYSmooth, aiInputComponent->rotXSmooth, aiInputComponent->rotZSmooth));
            transformComponent->orientation = transformComponent->orientation * localOrientation;
            aiInputComponent->rotationZ -= aiInputComponent->rotXSmooth;
            aiInputComponent->rotationZ = glm::clamp(aiInputComponent->rotationZ, -45.0f, 45.0f);
            glm::mat4 T = glm::translate(glm::vec3(transformComponent->transform[3])) * glm::mat4(transformComponent->orientation);
            transformComponent->transform = T * glm::mat4(glm::quat(glm::vec3((0, 0, aiInputComponent->rotationZ))));
            aiInputComponent->rotationZ = glm::mix(aiInputComponent->rotationZ, 0.0f, dt * cameraComponent->cameraSmoothFactor);

            // Update camera view transform
            glm::vec3 desiredCamPos = glm::vec3(transformComponent->transform[3]) + glm::vec3(transformComponent->transform * glm::vec4(0, cameraComponent->camOffsetY, -4.0f, 0));
            if (cameraComponent->theCam != nullptr)
            {
                cameraComponent->camPos = glm::mix(cameraComponent->camPos, desiredCamPos, dt * cameraComponent->cameraSmoothFactor);
                cameraComponent->theCam->view = glm::lookAt(cameraComponent->camPos, cameraComponent->camPos + glm::vec3(transformComponent->transform[2]), glm::vec3(transformComponent->transform[1]));

            }

            // Particles for thruster of the ship
            const float thrusterPosOffset = 0.365f;
            particleComponent->particleEmitterLeft->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -thrusterPosOffset + transformComponent->transform[2] * particleComponent->emitterOffset), 1);
            particleComponent->particleEmitterLeft->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            particleComponent->particleEmitterRight->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * thrusterPosOffset + transformComponent->transform[2] * particleComponent->emitterOffset), 1);
            particleComponent->particleEmitterRight->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            float t = (aiInputComponent->currentSpeed / aiInputComponent->normalSpeed);

            particleComponent->particleEmitterLeft->data.startSpeed = 1.2 + (3.0f * t);
            particleComponent->particleEmitterLeft->data.endSpeed = 0.0f + (3.0f * t);

            particleComponent->particleEmitterRight->data.startSpeed = 1.2 + (3.0f * t);
            particleComponent->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * t);

            // Canons for ship
            const float CanonPosOffset = 0.365f;
            particleComponent->particleCanonLeft->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset), 1);
            particleComponent->particleCanonLeft->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            particleComponent->particleCanonRight->data.origin = glm::vec4(glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset), 1);
            particleComponent->particleCanonRight->data.dir = glm::vec4(glm::vec3(-transformComponent->transform[2]), 0);

            if (aiInputComponent->isShooting)
            {
                particleComponent->particleCanonLeft->data.looping = 1;
                particleComponent->particleCanonRight->data.looping = 1;

                particleComponent->particleCanonLeft->data.randomTimeOffsetDist = 0.0001;
                particleComponent->particleCanonRight->data.randomTimeOffsetDist = 0.0001;

                particleComponent->particleCanonLeft->data.startSpeed = -500.0f;
                particleComponent->particleCanonLeft->data.endSpeed = -500.0f;
                particleComponent->particleCanonRight->data.startSpeed = -500.0f;
                particleComponent->particleCanonRight->data.endSpeed = -500.0f;
            }
            else
            {
                particleComponent->particleCanonLeft->data.looping = 0;
                particleComponent->particleCanonRight->data.looping = 0;
            }



           
          


         
            
        }
        for (int i = 0; i < colliderComponent->colliderEndPoints.size(); i++)
        {
            glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
            glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
            float len = glm::length(colliderComponent->colliderEndPoints[i]);
            Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);

            // debug draw collision rays
            int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
            if (menuIsUsingRayCasts == 1.0f)
            {
                Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            }


            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));

                for (auto entityIn : pureEntityData->Asteroids)
                {
                    auto entComp = entityIn->GetComponent<Components::ColliderComponent>();
                    if (payload.collider == entComp->colliderID && entityIn->eType == EntityType::Asteroid)
                    {
                       // Stop particle emitters
                       particleComponent->particleCanonLeft->data.looping = 0;
                       particleComponent->particleCanonRight->data.looping = 0;

                        // Save and flag for respawn
                        savedEnemyIDs.push(entity->id);
                        entity->isRespawning = true;

                        DestroyShip(entity->id, entity->eType);
                        DestroyEntity(entity->id, entity->eType);
                        CreateEnemyShip(true);

                        return;
                    }
                }

            }

        }
       

        //glm::mat4 transform = transformComponent->transform;
        //entity->isAvoidingAsteroids = false;

        ////Define raycast result holders
        //Physics::RaycastPayload pf, pf1, pf2;
        //Physics::RaycastPayload pu, pd;
        //Physics::RaycastPayload pfl, pfl1, pfl2;
        //Physics::RaycastPayload pul, pdl;
        //Physics::RaycastPayload pfr, pfr1, pfr2;
        //Physics::RaycastPayload pur, pdr;
        //Physics::RaycastPayload pl, pl1;
        //Physics::RaycastPayload pr, pr1;


        //// === Forward rays (center) ===
        //glm::vec3 fStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[0], 1.0f));
        //glm::vec3 fEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[1], 1.0f));
        //float fLength = glm::length(fEnd - fStart);
        //pf = Physics::Raycast(fStart, fEnd, fLength);

        //glm::vec3 f1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[2], 1.0f));
        //glm::vec3 f1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[3], 1.0f));
        //float f1Length = glm::length(f1End - f1Start);

        //pf1 = Physics::Raycast(f1Start, f1End, f1Length);

        //glm::vec3 f2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[4], 1.0f));
        //glm::vec3 f2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[5], 1.0f));
        //float f2Length = glm::length(f2End - f2Start);
        //pf2 = Physics::Raycast(f2Start, f2End, f2Length);

        //// === Up ray (center) ===
        //glm::vec3 uStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[6], 1.0f));
        //glm::vec3 uEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[7], 1.0f));
        //float uLength = glm::length(uEnd - uStart);
        //pu = Physics::Raycast(uStart, uEnd, uLength);

        //// === Down ray (center) ===
        //glm::vec3 dStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[8], 1.0f));
        //glm::vec3 dEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[9], 1.0f));
        //float dLength = glm::length(dEnd - dStart);

        //pd = Physics::Raycast(dStart, dEnd, dLength);

        //// === Forward rays (Left) ===
        //glm::vec3 flStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[10], 1.0f));
        //glm::vec3 flEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[11], 1.0f));
        //float flLength = glm::length(flEnd - flStart);

        //pfl = Physics::Raycast(flStart, flEnd, flLength);

        //glm::vec3 fl1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[12], 1.0f));
        //glm::vec3 fl1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[13], 1.0f));
        //float fl1Length = glm::length(fl1End - fl1Start);

        //pfl1 = Physics::Raycast(fl1Start, fl1End, fl1Length);

        //glm::vec3 fl2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[14], 1.0f));
        //glm::vec3 fl2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[15], 1.0f));
        //float fl2Length = glm::length(fl2End - fl2Start);

        //pfl2 = Physics::Raycast(fl2Start, fl2End, fl2Length);

        //// === Up ray (Left) ===
        //glm::vec3 ulStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[16], 1.0f));
        //glm::vec3 ulEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[17], 1.0f));
        //float ulLength = glm::length(ulEnd - ulStart);

        //pul = Physics::Raycast(ulStart, ulEnd, ulLength);

        //// === Down ray (Left) ===
        //glm::vec3 dlStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[18], 1.0f));
        //glm::vec3 dlEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[19], 1.0f));
        //float dlLength = glm::length(dlEnd - dlStart);

        //pdl = Physics::Raycast(dlStart, dlEnd, dlLength);

        //// === Forward rays (Right) ===
        //glm::vec3 frStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[20], 1.0f));
        //glm::vec3 frEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[21], 1.0f));
        //float frLength = glm::length(frEnd - frStart);

        //pfr = Physics::Raycast(frStart, frEnd, frLength);

        //glm::vec3 fr1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[22], 1.0f));
        //glm::vec3 fr1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[23], 1.0f));
        //float fr1Length = glm::length(fr1End - fr1Start);

        //pfr1 = Physics::Raycast(fr1Start, fr1End, fr1Length);

        //glm::vec3 fr2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[24], 1.0f));
        //glm::vec3 fr2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[25], 1.0f));
        //float fr2Length = glm::length(fr2End - fr2Start);

        //pfr2 = Physics::Raycast(fr2Start, fr2End, fr2Length);

        //// === Up ray (Right) ===
        //glm::vec3 urStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[26], 1.0f));
        //glm::vec3 urEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[27], 1.0f));
        //float urLength = glm::length(urEnd - urStart);

        //pur = Physics::Raycast(urStart, urEnd, urLength);

        //// === Down ray (Right) ===
        //glm::vec3 drStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[28], 1.0f));
        //glm::vec3 drEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[29], 1.0f));
        //float drLength = glm::length(drEnd - drStart);

        //pdr = Physics::Raycast(drStart, drEnd, drLength);

        //// === Left rays ===
        //glm::vec3 lStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[30], 1.0f));
        //glm::vec3 lEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[31], 1.0f));
        //float lLength = glm::length(lEnd - lStart);

        //pl = Physics::Raycast(lStart, lEnd, lLength);

        //glm::vec3 l1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[32], 1.0f));
        //glm::vec3 l1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[33], 1.0f));
        //float l1Length = glm::length(l1End - l1Start);

        //pl1 = Physics::Raycast(l1Start, l1End, l1Length);

        //// === Right rays ===
        //glm::vec3 rStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[34], 1.0f));
        //glm::vec3 rEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[35], 1.0f));
        //float rLength = glm::length(rEnd - rStart);

        //pr = Physics::Raycast(rStart, rEnd, rLength);

        //glm::vec3 r1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[36], 1.0f));
        //glm::vec3 r1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[37], 1.0f));
        //float r1Length = glm::length(r1End - r1Start);

        //pr1 = Physics::Raycast(r1Start, r1End, r1Length);

        //int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
        //if (menuIsUsingRayCasts == 1.0f)
        //{
        //    Debug::DrawLine(fStart, fEnd, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(f1Start, f1End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(f2Start, f2End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(uStart, uEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(dStart, dEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(flStart, flEnd, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(fl1Start, fl1End, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(fl2Start, fl2End, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(ulStart, ulEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(dlStart, dlEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(frStart, frEnd, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(fr1Start, fr1End, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(fr2Start, fr2End, 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(urStart, urEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(drStart, drEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(lStart, lEnd, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(l1Start, l1End, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(rStart, rEnd, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //    Debug::DrawLine(r1Start, r1End, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
        //}


        //// Logic to determine avoidance actions
        //if ((pf.hit || pf1.hit || pf2.hit || pu.hit || pd.hit || pl.hit || pl1.hit || pr.hit || pr1.hit))
        //{
        //    entity->isAvoidingAsteroids = true;



        //    entity->avoidanceTime = 0.0f;
        //}

        //else
        //{
        //    // Cooldown is over, stop avoiding and resume pathfinding
        //    entity->isAvoidingAsteroids = false;
        //}

        //if (
        //    entity->isAvoidingAsteroids &&
        //    (
        //        pf.hit || pf1.hit || pf2.hit ||
        //        pfl.hit || pfl1.hit || pfl2.hit ||
        //        pfr.hit || pfr1.hit || pfr2.hit ||
        //        pu.hit || pd.hit || pul.hit || pdl.hit || pur.hit || pdr.hit ||
        //        pl.hit || pl1.hit || pr.hit || pr1.hit
        //        )
        //    )
        //{
        //    // Handle movement and speed adjustments
        //    if (aiInputComponent->isForward)
        //    {
        //        // Decrease speed when avoiding obstacles
        //        aiInputComponent->currentSpeed = glm::max(aiInputComponent->currentSpeed - dt * 2.0f, 0.0f);


        //    }
        //    float rotationSpeed2 = 20.0 * dt; //rotationSpeed



        //    // Handle avoidance logic based on hit rays
        //    if (pf.hit || pf1.hit || pf2.hit || pfl.hit || pfl1.hit || pfl2.hit || pfr.hit || pfr1.hit || pfr2.hit)
        //    {
        //        // Up if forward rays hit
        //        aiInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);

        //    }

        //    if (pu.hit || pul.hit || pur.hit)
        //    {
        //        // Down if upward rays hit
        //        aiInputComponent->rotYSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);

        //    }

        //    if (pd.hit || pdl.hit || pdr.hit)
        //    {
        //        // Up if downward rays hit
        //        aiInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);

        //    }

        //    if (pl.hit || pl1.hit)
        //    {
        //        // right if left rays hit
        //        aiInputComponent->rotXSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);

        //    }
        //    else if (pr.hit || pr1.hit)
        //    {
        //        // left if right rays hit
        //        aiInputComponent->rotXSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);

        //    }
        //}
    }
}
inline void World::UpdateAsteroid(Entity* entity, float dt) 
{
    if (entity->eType == EntityType::Asteroid) // asteroids
    {
         auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
        auto renderComponent = entity->GetComponent<Components::RenderableComponent>();

        // Apply rotation to the asteroid's transform matrix
        transformComponent->transform = glm::rotate(transformComponent->transform, dt * glm::radians(transformComponent->rotationSpeed), transformComponent->rotationAxis);

        // Ensure that the rotation is preserved correctly and the matrix remains valid
        // This part normalizes the basis vectors (rows of the transform matrix)
        transformComponent->transform = glm::mat4(glm::normalize(transformComponent->transform[0]), 
                                                   glm::normalize(transformComponent->transform[1]), 
                                                   glm::normalize(transformComponent->transform[2]), 
                                                   transformComponent->transform[3]);

        // Update the collider's transform with the updated mesh transform
        Physics::SetTransform(colliderComponent->colliderID, transformComponent->transform);


    }
}
inline void World::drawNode(Entity* entity)
{
    auto navNodeComponent = entity->GetComponent< Components::AINavNodeComponent>();
    if (entity->eType == EntityType::Node) // nodes
    {
        int drawId = Core::CVarReadInt(navNodeComponent->r_draw_Node_Axis_id);
        int drawBool = Core::CVarReadInt(navNodeComponent->r_draw_Node_Axis);
        auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        if (drawId >= 0 && drawBool == 1)
        {
            for (size_t i = 0; i < pureEntityData->nodes.size(); i++)
            {
                if (entity->id == drawId)
                {
                    glm::vec3 pos = glm::vec3(transformComponent->transform[3]);

                    // -X(Red)
                    glm::vec3 dirXminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[0]), 0.0f);
                    float lenXminus = glm::length(navNodeComponent->EndPoints[0]);
                    Debug::DrawLine(pos, pos + dirXminus * lenXminus, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);

                    // +X (Light Red)
                    glm::vec3 dirXplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[1]), 0.0f);
                    float lenXplus = glm::length(navNodeComponent->EndPoints[1]);
                    Debug::DrawLine(pos, pos + dirXplus * lenXplus, 1.0f, glm::vec4(1.5f, 0.4f, 0.4f, 1), glm::vec4(1.5f, 0.4f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

                    // -Y (Green)
                    glm::vec3 dirYminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[2]), 0.0f);
                    float lenYminus = glm::length(navNodeComponent->EndPoints[2]);
                    Debug::DrawLine(pos, pos + dirYminus * lenYminus, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

                    // +Y (Light Green)
                    glm::vec3 dirYplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[3]), 0.0f);
                    float lenYplus = glm::length(navNodeComponent->EndPoints[3]);
                    Debug::DrawLine(pos, pos + dirYplus * lenYplus, 1.0f, glm::vec4(0.4f, 1.5f, 0.4f, 1), glm::vec4(0.4f, 1.5f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

                    // -Z (Blue)
                    glm::vec3 dirZminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[4]), 0.0f);
                    float lenZminus = glm::length(navNodeComponent->EndPoints[4]);
                    Debug::DrawLine(pos, pos + dirZminus * lenZminus, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);

                    // +Z (Light Blue)
                    glm::vec3 dirZplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[5]), 0.0f);
                    float lenZplus = glm::length(navNodeComponent->EndPoints[5]);
                    Debug::DrawLine(pos, pos + dirZplus * lenZplus, 1.0f, glm::vec4(0.4f, 0.4f, 1.5f, 1), glm::vec4(0.4f, 0.4f, 1.5f, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawDebugText(std::to_string(entity->id).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], { 0.9f,0.9f,1,1 });
                }
            }
        }
       
        if (drawId < 0 && drawBool == 1)
        {
            glm::vec3 pos = glm::vec3(transformComponent->transform[3]);

            // -X(Red)
            glm::vec3 dirXminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[0]), 0.0f);
            float lenXminus = glm::length(navNodeComponent->EndPoints[0]);
            Debug::DrawLine(pos, pos + dirXminus * lenXminus, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);

            // +X (Light Red)
            glm::vec3 dirXplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[1]), 0.0f);
            float lenXplus = glm::length(navNodeComponent->EndPoints[1]);
            Debug::DrawLine(pos, pos + dirXplus * lenXplus, 1.0f, glm::vec4(1.5f, 0.4f, 0.4f, 1), glm::vec4(1.5f, 0.4f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

            // -Y (Green)
            glm::vec3 dirYminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[2]), 0.0f);
            float lenYminus = glm::length(navNodeComponent->EndPoints[2]);
            Debug::DrawLine(pos, pos + dirYminus * lenYminus, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            // +Y (Light Green)
            glm::vec3 dirYplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[3]), 0.0f);
            float lenYplus = glm::length(navNodeComponent->EndPoints[3]);
            Debug::DrawLine(pos, pos + dirYplus * lenYplus, 1.0f, glm::vec4(0.4f, 1.5f, 0.4f, 1), glm::vec4(0.4f, 1.5f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

            // -Z (Blue)
            glm::vec3 dirZminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[4]), 0.0f);
            float lenZminus = glm::length(navNodeComponent->EndPoints[4]);
            Debug::DrawLine(pos, pos + dirZminus * lenZminus, 1.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);

            // +Z (Light Blue)
            glm::vec3 dirZplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[5]), 0.0f);
            float lenZplus = glm::length(navNodeComponent->EndPoints[5]);
            Debug::DrawLine(pos, pos + dirZplus * lenZplus, 1.0f, glm::vec4(0.4f, 0.4f, 1.5f, 1), glm::vec4(0.4f, 0.4f, 1.5f, 1), Debug::RenderMode::AlwaysOnTop);
           // Debug::DrawDebugText(std::to_string(entity->id).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], { 0.9f,0.9f,1,1 });
        }

        // Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);

    }
}
inline void World::draw(Entity* entity) // literally draw everything that renders
{
    auto renderableComponent = entity->GetComponent<Components::RenderableComponent>();
    /* auto Aicomponent = entity->GetComponent<Components::AINavNodeComponent>();*/
    auto trans = entity->GetComponent<Components::TransformComponent>();
    if (renderableComponent)
    {
        Render::RenderDevice::Draw(renderableComponent->modelId, entity->GetComponent<Components::TransformComponent>()->transform);
        //Debug::DrawDebugText(std::to_string(renderableComponent->ownerId).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], { 0.9f,0.9f,1,1 });
    }
}
inline void World::updateCamera(Entity* entity, float dt)
{
    

    if (entity->eType == EntityType::SpaceShip || entity->eType == EntityType::EnemyShip)
    {
        // First pass: Check if the selected entity already has a camera
        auto camComp = entity->GetComponent<Components::CameraComponent>();

        if (camComp)
        {
            int selectedID(Core::CVarReadInt(camComp->r_camera_id));
            int isPlayerOrEnemy(Core::CVarReadInt(camComp->r_camera));

            // Check if the entity is selected and has a camera
            if ((isPlayerOrEnemy == 1 && entity->eType == EntityType::SpaceShip) ||
                (isPlayerOrEnemy == 0 && entity->eType == EntityType::EnemyShip))
            {
                if (entity->id == selectedID)
                {
                    // Assign the main camera to the selected entity
                    if (camComp->theCam == nullptr) {
                        camComp->theCam = Render::CameraManager::GetCamera(CAMERA_MAIN);
                    }
                }
                else
                {
                    // If the entity is not selected, clear the camera
                    camComp->theCam = nullptr;
                }
            }
            else
            {
                // Null out the camera for all others
                camComp->theCam = nullptr;
            }
        }
    }
}

inline void World::HandleAsteroidHit(Entity* entity, Components::ParticleEmitterComponent* particleEmit)
{
   

   
}
