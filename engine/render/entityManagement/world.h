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
    ChunkAllocator<Components::State, 64> stateChunk;
    ChunkAllocator<Components::AI, 64> AIChunk;



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

    void wanderingState(Entity* entity, float dt);
    void attackState(Entity* entity, float dt);
    void fleeingState(Entity* entity, float dt);

    //AI functions
    void moveToStartNode(Entity* entity, Components::TransformComponent* startNode, float dt);
    void drawPath(Entity* entity);
    void resetPath(Entity* entity);
    void followPath(Entity* entity, float dt);
    void updateShipMovementAndParticles(Entity* entity, float dt);
    bool IsShipNearby(Entity* entity, float detectionRadius, Entity*& outShip);
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
        int count = 0;
        for (auto ent : pureEntityData->entities)
        {
            if (ent->eType == entity->eType)
                count++;
        }
            
        entity->id = count;
    }
   

    else if (entity->eType == EntityType::SpaceShip && isRespawning)
    {
        if (!savedIDs.empty())
        {
            entity->id = savedIDs.front();
            savedIDs.pop();
            std::cout << "[Respawn] ✔ SpaceShip respawned successfully with ID: "
                << entity->id << "\n";
        }
        else
        {
            std::cout << "[Respawn] ❌ SpaceShip respawn failed: savedIDs queue is EMPTY!\n";
        }

        pureEntityData->ships.push_back(entity);
    }
    else if (entity->eType == EntityType::EnemyShip && isRespawning)
    {
        if (!savedEnemyIDs.empty())
        {
            entity->id = savedEnemyIDs.front();
            savedEnemyIDs.pop();
            std::cout << "[Respawn] ✔ EnemyShip respawned successfully with ID: "
                << entity->id << "\n";
        }
        else
        {
            std::cout << "[Respawn] ❌ EnemyShip respawn failed: savedEnemyIDs queue is EMPTY!\n";
        }

        pureEntityData->ships.push_back(entity);
    }
    // always add to entities so update loop works
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

    for (int i = 0; i < pureEntityData->ships.size(); i++)
    {
        Entity* ship = pureEntityData->ships[i];

        // --- SAFETY CHECKS ---
        if (!ship)
        {
            std::cout << "[ShipUpdate] ❌ Ship is nullptr. Removing.\n";
            pureEntityData->ships.erase(pureEntityData->ships.begin() + i);
            continue;
        }
        auto ShipState = ship->GetComponent<Components::State>();
        if (!ShipState)
        {
            std::cout << "[ShipUpdate] ❌ Ship has NO State component. Removing.\n";
            pureEntityData->ships.erase(pureEntityData->ships.begin() + i);
            continue;
        }
        // --- DESTROYED SHIPS SHOULD NOT UPDATE ---
        if (ShipState->isDestroyed)
        {
            std::cout << "[ShipUpdate] ⚠ Ship destroyed. Removing from list.\n";
            pureEntityData->ships.erase(pureEntityData->ships.begin() + i);
            continue;
        }
      
          // --- SAFE TO UPDATE ---
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

        auto ShipState = entityToDelete->GetComponent<Components::State>();
        ShipState->isDestroyed = true;

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
            else if (auto* aiComp = dynamic_cast<Components::AI*>(component))
            {
                AIChunk.Deallocate(aiComp);
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
                else if (auto* aiComp = dynamic_cast<Components::AI*>(component))
                {
                    AIChunk.Deallocate(aiComp);
                }
                else if (auto* stateComp = dynamic_cast<Components::State*>(component))
                {
                    stateChunk.Deallocate(stateComp);
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
    //add random position from the nodes placed out
    int randomIndex = rand() % pureEntityData->nodes.size();
    auto nodeTransformComponent = pureEntityData->nodes[randomIndex]->GetComponent<Components::TransformComponent>();
    Components::TransformComponent* newTransform = transformChunk.Allocate();
    newTransform->transform[3] = nodeTransformComponent->transform[3];

    spaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::SpaceShip);

    Components::State* state = stateChunk.Allocate();
    spaceship->AddComponent(state, ComponentType::STATE, EntityType::SpaceShip);

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
        collider->rayCastPoints[i] = rayCastEndPoints[i];
    }


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

   

    //add random position from the nodes placed out
    int randomIndex = rand() % pureEntityData->nodes.size();
    auto nodeTransformComponent = pureEntityData->nodes[randomIndex]->GetComponent<Components::TransformComponent>();
    Components::TransformComponent* newTransform = transformChunk.Allocate();
    newTransform->transform[3] = nodeTransformComponent->transform[3];

    AIspaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::EnemyShip);


    Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
    AIspaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::EnemyShip);

    Components::State* state = stateChunk.Allocate();
    AIspaceship->AddComponent(state, ComponentType::STATE, EntityType::EnemyShip);

    Components::ColliderComponent* collider = colliderChunk.Allocate();
    AIspaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::EnemyShip);
    for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
    {
        collider->colliderEndPoints.push_back(colliderEndPoints[i]);
    }
    for (int i = 0; i < sizeof(collider->rayCastPoints) / sizeof(glm::vec3); i++)
    {
        collider->rayCastPoints[i] = rayCastEndPoints[i];
    }
    

    Components::AIinputController* controllinput = AiControllerChunk.Allocate();
    controllinput->currentState = AIState::Roaming;
    AIspaceship->AddComponent(controllinput, ComponentType::AI_CONTROLLER, EntityType::EnemyShip);

    Components::AI* AiComp = AIChunk.Allocate();
    AIspaceship->AddComponent(AiComp, ComponentType::AI, EntityType::EnemyShip);

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
    if(AIspaceship)
    {
        std::cout << "[AI Ship] ✔ Successfully created AI spaceship with ID: "
            << AIspaceship->id << "\n";
    }
    else
    {
        std::cerr << "[AI Ship] ❌ Failed to create AI spaceship!\n";
    }
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

    Components::AI* AINodes = AIChunk.Allocate(); // use for the enemy ship to track path
    node->AddComponent(AINodes, ComponentType::AI, EntityType::Node);

    Components::AINavNodeComponent* NodeComp = navNodeChunk.Allocate();
    node->AddComponent(NodeComp, ComponentType::NAVNODE, EntityType::Node);
    for (int i = 0; i < sizeof(EndPoints) / sizeof(glm::vec3); i++)
    {
        NodeComp->EndPoints[i] = EndPoints[i];
    }

    for (int i = 0; i < sizeof(colComp->EndPointsNodes) / sizeof(glm::vec3); i++)
    {
        glm::vec3 pos = glm::vec3(newTransform->transform[3]);
        glm::vec3 dir = newTransform->transform * glm::vec4(glm::normalize(colComp->EndPointsNodes[i]), 0.0f);
        float len = glm::length(colComp->EndPointsNodes[i]);
        Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(newTransform->transform[3]), dir, len);

        // debug draw collision rays
        //Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

        if (payload.hit)
        {
            Debug::DrawDebugText("Node_hit_asteroids", payload.hitPoint, glm::vec4(1, 1, 1, 1));
            for (auto entityIn : pureEntityData->Asteroids)
            {
                auto entComp = entityIn->GetComponent<Components::ColliderComponent>();
                if (payload.collider == entComp->colliderID && entityIn->eType == EntityType::Asteroid)
                {
                    NodeComp->isCollidedAsteroids = true;

                }
            }
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
        if (distanceSquared <= minDistanceSquared)
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
        auto stateComponent = entity->GetComponent<Components::State>();

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

            float isSpacePressed = playerInputComponent->kbd->held[Input::Key::Space] ? 1.0f : 0.0f;

     
            // Cannon base positions (used for reset)
            const float CanonPosOffset = 0.365f;
            glm::vec3 leftOrigin = glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset);
            glm::vec3 rightOrigin = glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * CanonPosOffset + transformComponent->transform[2] * particleComponent->canonEmitterOffset);

            // Initialize emitter positions if first frame
            if (particleComponent->travelLeft == 0.0f) particleComponent->leftCanonPos = leftOrigin;
            if (particleComponent->travelRight == 0.0f) particleComponent->rightCanonPos = rightOrigin;

            particleComponent->particleCanonLeft->data.randomTimeOffsetDist = 0.01;
            particleComponent->particleCanonRight->data.randomTimeOffsetDist = 0.01; 
            particleComponent->particleCanonLeft->data.startSpeed = -50.0f;
            particleComponent->particleCanonLeft->data.endSpeed = -50.0f;
            particleComponent->particleCanonRight->data.startSpeed = -50.0f;
            particleComponent->particleCanonRight->data.endSpeed = -50.0f;

            const float maxDistance = 5000.0f;

            static bool lastSpaceState = false;
            bool currentSpaceState = (isSpacePressed >= 1.0f);

            // Trigger firing only on key press
            if (currentSpaceState && !lastSpaceState)
            {
                particleComponent->particleCanonLeft->data.looping = 1;
                particleComponent->particleCanonRight->data.looping = 1;
                particleComponent->hasFired = true;
            }

            // Save state for next frame
            lastSpaceState = currentSpaceState;

            // Only set direction when starting a new shot
            if (particleComponent->hasFired)
            {
                Entity* closestEntity = nullptr;
                Components::TransformComponent* closestTComp = nullptr;

                // --- Update direction once per shot (based on ship rotation) ---
                if (particleComponent->travelLeft == 0.0f)
                    particleComponent->particleCanonLeft->data.dir = glm::vec4(-glm::vec3(transformComponent->transform[2]), 0);

                if (particleComponent->travelRight == 0.0f)
                    particleComponent->particleCanonRight->data.dir = glm::vec4(-glm::vec3(transformComponent->transform[2]), 0);

                glm::vec3 leftDir = glm::normalize(glm::vec3(particleComponent->particleCanonLeft->data.dir));
                glm::vec3 rightDir = glm::normalize(glm::vec3(particleComponent->particleCanonRight->data.dir));

                float speedLeft = particleComponent->particleCanonLeft->data.startSpeed;
                float speedRight = particleComponent->particleCanonRight->data.startSpeed;

                // --- Simulate bullet travel ---
                particleComponent->leftCanonPos += leftDir * speedLeft * dt;
                particleComponent->rightCanonPos += rightDir * speedRight * dt;

                particleComponent->travelLeft += glm::abs(speedLeft) * dt * 500.0f;
                particleComponent->travelRight += glm::abs(speedRight) * dt * 500.0f;

                // --- Find closest target ship ---
                float minDistSq = std::numeric_limits<float>::max();
                for (auto entityIn : pureEntityData->ships)
                {
                    if (entityIn->eType == EntityType::SpaceShip) // skip self/player
                        continue;

                    auto tComp = entityIn->GetComponent<Components::TransformComponent>();
                    if (!tComp) continue;

                    glm::vec3 targetPos = glm::vec3(tComp->transform[3]);
                    glm::vec3 originAvg = 0.5f * (
                        glm::vec3(particleComponent->particleCanonLeft->data.origin) +
                        glm::vec3(particleComponent->particleCanonRight->data.origin)
                        );

                    float distSq = glm::length2(targetPos - originAvg);
                    if (distSq < minDistSq)
                    {
                        minDistSq = distSq;
                        closestEntity = entityIn;
                        closestTComp = tComp;
                    }
                }

                bool colliderIsClose = (closestEntity && glm::sqrt(minDistSq) < 1.5f);

                glm::vec3 leftStart = particleComponent->leftCanonPos;
                glm::vec3 rightStart = particleComponent->rightCanonPos;

                // --- Debug rays to visualize travel ---
                Debug::DrawLine(leftStart, leftStart + leftDir * 200.0f, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                Debug::DrawLine(rightStart, rightStart + rightDir * 200.0f, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));

                // --- Now target the enemy collider endpoints ---
                if (closestEntity)
                {
                    auto enemyCollider = closestEntity->GetComponent<Components::ColliderComponent>();
                    if (enemyCollider)
                    {
                        for (auto& endpoint : enemyCollider->colliderEndPoints)
                        {
                            glm::vec3 worldEndpoint = glm::vec3(closestTComp->transform * glm::vec4(endpoint, 1.0f));

                            // Pick cannon origin (alternate for left/right)
                            glm::vec3 cannonOrigin = glm::vec3(particleComponent->particleCanonLeft->data.origin);
                            glm::vec3 dirToEndpoint = glm::normalize(worldEndpoint - cannonOrigin);
                            float len = glm::length(worldEndpoint - cannonOrigin);

                            Debug::DrawLine(cannonOrigin, cannonOrigin + dirToEndpoint * len, 1.0f,
                                glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
                        }
                    }
                   
                }
                auto closestEnemyState = closestEntity->GetComponent<Components::State>();
                 if (colliderIsClose && closestEntity->eType == EntityType::EnemyShip && !closestEnemyState->isRespawning)
                    {

                        std::cout << "AI CANNON HIT ENEMY ENDPOINT! Ship ID: " << closestEntity->id << std::endl;

                        particleComponent->hasFired = false;
                        particleComponent->leftCanonPos = leftOrigin;
                        particleComponent->rightCanonPos = rightOrigin;
                        particleComponent->travelLeft = 0.0f;
                        particleComponent->travelRight = 0.0f;
                        particleComponent->hasFired = false;
                        particleComponent->particleCanonLeft->data.looping = 0;
                        particleComponent->particleCanonRight->data.looping = 0;

                        if (closestEnemyState->isRespawning) return; // stop if already respawning
                        // Respawn + destroy logic
                        savedEnemyIDs.push(closestEntity->id);
                        closestEnemyState->isRespawning = true;
                        CreateEnemyShip(true);
                        DestroyShip(closestEntity->id, closestEntity->eType);
                        DestroyEntity(closestEntity->id, closestEntity->eType);
         
                       
                    }

                // --- Reset if exceeded max travel distance ---
                if (particleComponent->travelLeft >= maxDistance || particleComponent->travelRight >= maxDistance)
                {
                    particleComponent->leftCanonPos = leftOrigin;
                    particleComponent->rightCanonPos = rightOrigin;
                    particleComponent->travelLeft = 0.0f;
                    particleComponent->travelRight = 0.0f;
                    particleComponent->hasFired = false;
                    particleComponent->particleCanonLeft->data.looping = 0;
                    particleComponent->particleCanonRight->data.looping = 0;
                }
            }

            // Update emitter origins
            particleComponent->particleCanonLeft->data.origin = glm::vec4(particleComponent->leftCanonPos, 1.0f);
            particleComponent->particleCanonRight->data.origin = glm::vec4(particleComponent->rightCanonPos, 1.0f);

            glm::vec3 leftDir = glm::normalize(glm::vec3(particleComponent->particleCanonLeft->data.dir));
            glm::vec3 rightDir = glm::normalize(glm::vec3(particleComponent->particleCanonRight->data.dir));

            

            //check collisions asteroids
            glm::mat4 rotation = glm::mat4(transformComponent->orientation);
            bool hit = false;

            bool colliderIsClose = false;
            Entity* closeEntity;
            Components::TransformComponent* closeTComp;

            auto shipPos = glm::vec3(transformComponent->transform[3]);

            for (auto entityIn : pureEntityData->Asteroids)
            {
                closeTComp = entityIn->GetComponent<Components::TransformComponent>();
                auto closestPos = glm::vec3(closeTComp->transform[3]);

                auto closestposLenght = glm::length(closestPos - shipPos);

                if (closestposLenght < 8.0f && closeTComp != nullptr)
                {

                    colliderIsClose = true;
                    closeEntity = entityIn;
                    break;
                }
            }

            auto entityState = entity->GetComponent<Components::State>();

            for (int i = 0; i < colliderComponent->colliderEndPoints.size(); i++)
            {
                glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
                glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
                float len = glm::length(colliderComponent->colliderEndPoints[i]);
                // debug draw collision rays
                int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));

                if (colliderIsClose)
                {
                    if (menuIsUsingRayCasts == 1.0f)
                    {
                        Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
                    }
                    Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);
                    if (payload.hit)
                    {
                        Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));

                        auto entComp = closeEntity->GetComponent<Components::ColliderComponent>();
                        if (payload.collider == entComp->colliderID && closeEntity->eType == EntityType::Asteroid)
                        {
                            // Stop particle emitters
                            particleComponent->particleCanonLeft->data.looping = 0;
                            particleComponent->particleCanonRight->data.looping = 0;

                            // Save and flag for respawn
                            savedIDs.push(entity->id);

                           
                            entityState->isRespawning = true;

                            CreatePlayerShip(true);
                            DestroyShip(entity->id, entity->eType);
                            DestroyEntity(entity->id, entity->eType);


                            return;
                        }

                    }
                }
            }

            glm::mat4 transform = transformComponent->transform;
         
            entityState->isAvoidingAsteroids = false;

            //Define raycast result holders
            Physics::RaycastPayload pf, pf1, pf2;
            Physics::RaycastPayload pu, pd;
            Physics::RaycastPayload pfl, pfl1, pfl2;
            Physics::RaycastPayload pul, pdl;
            Physics::RaycastPayload pfr, pfr1, pfr2;
            Physics::RaycastPayload pur, pdr;
            Physics::RaycastPayload pl, pl1;
            Physics::RaycastPayload pr, pr1;


            // === Forward rays (center) ===
            glm::vec3 fStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[0], 1.0f));
            glm::vec3 fEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[1], 1.0f));
            float fLength = glm::length(fEnd - fStart);


            glm::vec3 f1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[2], 1.0f));
            glm::vec3 f1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[3], 1.0f));
            float f1Length = glm::length(f1End - f1Start);


            glm::vec3 f2Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[4], 1.0f));
            glm::vec3 f2End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[5], 1.0f));
            float f2Length = glm::length(f2End - f2Start);


            // === Up ray (center) ===
            glm::vec3 uStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[6], 1.0f));
            glm::vec3 uEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[7], 1.0f));
            float uLength = glm::length(uEnd - uStart);


            // === Down ray (center) ===
            glm::vec3 dStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[8], 1.0f));
            glm::vec3 dEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[9], 1.0f));
            float dLength = glm::length(dEnd - dStart);

            // === Left rays ===
            glm::vec3 lStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[30], 1.0f));
            glm::vec3 lEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[31], 1.0f));
            float lLength = glm::length(lEnd - lStart);


            glm::vec3 l1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[32], 1.0f));
            glm::vec3 l1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[33], 1.0f));
            float l1Length = glm::length(l1End - l1Start);



            // === Right rays ===
            glm::vec3 rStart = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[34], 1.0f));
            glm::vec3 rEnd = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[35], 1.0f));
            float rLength = glm::length(rEnd - rStart);



            glm::vec3 r1Start = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[36], 1.0f));
            glm::vec3 r1End = glm::vec3(transform * glm::vec4(colliderComponent->rayCastPoints[37], 1.0f));
            float r1Length = glm::length(r1End - r1Start);


            bool colliderIsCloseSensor = false;
            Entity* theCloseEntity;
            Components::TransformComponent* theCloseTcomp;

            auto shipPosition = glm::vec3(transformComponent->transform[3]);
            for (auto entityIn : pureEntityData->Asteroids)
            {
                theCloseTcomp = entityIn->GetComponent<Components::TransformComponent>();
                auto closestPos = glm::vec3(closeTComp->transform[3]);
                auto closestposLenght = glm::length(closestPos - shipPosition);
                if (closestposLenght <= 15.0f && closeTComp != nullptr)
                {

                    colliderIsCloseSensor = true;
                    theCloseEntity = entityIn;
                    break;
                }
            }
            int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));

            float delayTime = 0.01f; // Delay in seconds
            float elapsedTime = 0.0f; // Time accumulated so far

            if (colliderIsCloseSensor)
            {
                elapsedTime += dt;
                if (elapsedTime >= delayTime)
                {
                    pf = Physics::Raycast(fStart, fEnd, fLength);
                    pf1 = Physics::Raycast(f1Start, f1End, f1Length);
                    pf2 = Physics::Raycast(f2Start, f2End, f2Length);
                    pu = Physics::Raycast(uStart, uEnd, uLength);
                    pd = Physics::Raycast(dStart, dEnd, dLength);
                    pl = Physics::Raycast(lStart, lEnd, lLength);
                    pl1 = Physics::Raycast(l1Start, l1End, l1Length);
                    pr = Physics::Raycast(rStart, rEnd, rLength);
                    pr1 = Physics::Raycast(r1Start, r1End, r1Length);
                    elapsedTime = 0.0f;
                }

                if (menuIsUsingRayCasts == 1.0f)
                {
                    Debug::DrawLine(fStart, fEnd, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(f1Start, f1End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(f2Start, f2End, 1.0f, glm::vec4(1), glm::vec4(1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(uStart, uEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(dStart, dEnd, 1.0f, glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 1, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(lStart, lEnd, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(l1Start, l1End, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(rStart, rEnd, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
                    Debug::DrawLine(r1Start, r1End, 1.0f, glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);
                }

            }

            // Logic to determine avoidance actions
            if (
                pf.hit || pf1.hit || pf2.hit ||
                pfl.hit || pfl1.hit || pfl2.hit ||
                pfr.hit || pfr1.hit || pfr2.hit ||
                pu.hit || pd.hit || pul.hit || pdl.hit || pur.hit || pdr.hit ||
                pl.hit || pl1.hit || pr.hit || pr1.hit)
            {

                entityState->isAvoidingAsteroids = true;
                entityState->avoidanceTime = 0.0f;
            }
            else
            {
                // Cooldown is over, stop avoiding and resume pathfinding
                entityState->isAvoidingAsteroids = false;
            }

            if (
                entityState->isAvoidingAsteroids &&
                (
                    pf.hit || pf1.hit || pf2.hit ||
                    pfl.hit || pfl1.hit || pfl2.hit ||
                    pfr.hit || pfr1.hit || pfr2.hit ||
                    pu.hit || pd.hit || pul.hit || pdl.hit || pur.hit || pdr.hit ||
                    pl.hit || pl1.hit || pr.hit || pr1.hit
                    )
                )
            {
                float rotationSpeed2 = 20.0 * dt; // Adjust this value to control the speed of rotation

                // Handle avoidance logic based on hit rays
                if (pf.hit || pf1.hit || pf2.hit || pfl.hit || pfl1.hit || pfl2.hit || pfr.hit || pfr1.hit || pfr2.hit)
                {
                    // Up if forward rays hit
                    playerInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
                }

                if (pu.hit || pul.hit || pur.hit)
                {
                    // Down if upward rays hit
                    playerInputComponent->rotYSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
                }

                if (pd.hit || pdl.hit || pdr.hit)
                {
                    // Up if downward rays hit
                    playerInputComponent->rotYSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
                }

                if (pl.hit || pl1.hit)
                {
                    // right if left rays hit
                    playerInputComponent->rotXSmooth = glm::mix(0.0f, -rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
                }
                else if (pr.hit || pr1.hit)
                {
                    // left if right rays hit
                    playerInputComponent->rotXSmooth = glm::mix(0.0f, rotationSpeed2, dt * cameraComponent->cameraSmoothFactor);
                }
            }

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

        //// Check if any element in the 4x4 matrix is NaN
        //bool hasNaN = false;
        //for (int i = 0; i < 4 && !hasNaN; i++)
        //{
        //    for (int j = 0; j < 4 && !hasNaN; j++)
        //    {
        //        if (glm::isnan(transformComponent->transform[i][j]))
        //            hasNaN = true;
        //    }
        //}

        //// If NaN found, reset to identity
        //if (hasNaN)
        //{
        //    transformComponent->transform = glm::mat4(1.0f);
        //}
  

        if (!aiInputComponent || !transformComponent || !colliderComponent)
            return;

        switch (aiInputComponent->currentState)
        {
        case AIState::Roaming:
        {
            wanderingState(entity, dt);
            break;
        }
        case AIState::ChasingEnemy:
        {
            attackState(entity, dt);
            break;
        }
        case  AIState::Fleeing:
            fleeingState(entity, dt);
            break;
        }
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
                    Debug::DrawLine(pos, pos + dirXminus * lenXminus, 10.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1), Debug::RenderMode::AlwaysOnTop);

                    // +X (Light Red)
                    glm::vec3 dirXplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[1]), 0.0f);
                    float lenXplus = glm::length(navNodeComponent->EndPoints[1]);
                    Debug::DrawLine(pos, pos + dirXplus * lenXplus, 10.0f, glm::vec4(1.5f, 0.4f, 0.4f, 1), glm::vec4(1.5f, 0.4f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

                    // -Y (Green)
                    glm::vec3 dirYminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[2]), 0.0f);
                    float lenYminus = glm::length(navNodeComponent->EndPoints[2]);
                    Debug::DrawLine(pos, pos + dirYminus * lenYminus, 10.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

                    // +Y (Light Green)
                    glm::vec3 dirYplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[3]), 0.0f);
                    float lenYplus = glm::length(navNodeComponent->EndPoints[3]);
                    Debug::DrawLine(pos, pos + dirYplus * lenYplus, 10.0f, glm::vec4(0.4f, 1.5f, 0.4f, 1), glm::vec4(0.4f, 1.5f, 0.4f, 1), Debug::RenderMode::AlwaysOnTop);

                    // -Z (Blue)
                    glm::vec3 dirZminus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[4]), 0.0f);
                    float lenZminus = glm::length(navNodeComponent->EndPoints[4]);
                    Debug::DrawLine(pos, pos + dirZminus * lenZminus, 10.0f, glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1), Debug::RenderMode::AlwaysOnTop);

                    // +Z (Light Blue)
                    glm::vec3 dirZplus = transformComponent->transform * glm::vec4(glm::normalize(navNodeComponent->EndPoints[5]), 0.0f);
                    float lenZplus = glm::length(navNodeComponent->EndPoints[5]);
                    Debug::DrawLine(pos, pos + dirZplus * lenZplus, 10.0f, glm::vec4(0.4f, 0.4f, 1.5f, 1), glm::vec4(0.4f, 0.4f, 1.5f, 1), Debug::RenderMode::AlwaysOnTop);
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
    /* auto AIInputComponent = entity->GetComponent<Components::AINavNodeComponent>();*/
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
inline void World::wanderingState(Entity* entity, float dt)
{

    auto transformComponent = entity->GetComponent<Components::TransformComponent>();
    auto cameraComponent = entity->GetComponent<Components::CameraComponent>();
    auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
    auto particleComponent = entity->GetComponent<Components::ParticleEmitterComponent>();
    auto AIInputComponent = entity->GetComponent<Components::AIinputController>();
    auto AIcomponent = entity->GetComponent<Components::AI>();
    auto stateComponent = entity->GetComponent<Components::State>();


    AstarAlgorithm* astar = AstarAlgorithm::Instance();
    Debug::DrawDebugText(std::to_string(entity->id).c_str(), transformComponent->transform[3], { 0.9f,0.9f,1,1 });

    Components::TransformComponent* closeNodeTranscomp = nullptr;
    glm::vec3 targetDirection;
    glm::quat targetRotation;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Find the closest node
    if (!AIcomponent->closestNodeCalled)
    {
       
        AIcomponent->closestNodeCalled = true;
        AIcomponent->closestNodeFromShip = getclosestNodeFromAIship(entity);
        // Fallback if closestNodeFromShip was nullptr
        if (!AIcomponent->closestNodeFromShip)
        {
            int randomIndex = rand() % pureEntityData->nodes.size();
            AIcomponent->closestNodeFromShip = pureEntityData->nodes[randomIndex];
            std::cout << "[AI] Count not find closest node, using Fallback node selected at random index " << randomIndex << " for ship ID " << entity->id << "\n";
           
        }

    }
        closeNodeTranscomp = AIcomponent->closestNodeFromShip->GetComponent<Components::TransformComponent>();
       
    // Fallback if closestNodeFromShip was nullptr
    if (!closeNodeTranscomp)
    {
        if (!pureEntityData->nodes.empty())
        {
            int randomIndex = rand() % pureEntityData->nodes.size();
            closeNodeTranscomp = pureEntityData->nodes[randomIndex]->GetComponent<Components::TransformComponent>();
            if (!closeNodeTranscomp)
            {
                std::cerr << "[wanderingState] ❌ Random node transform is nullptr, destroying ship ID " << entity->id << "\n";

       
                savedEnemyIDs.push(entity->id);
                stateComponent->isRespawning = true;
                CreateEnemyShip(true);
                DestroyShip(entity->id, entity->eType);
                DestroyEntity(entity->id, entity->eType); // implement or use your existing removal logic
                return; // exit early
            }
        }
        else
        {
            std::cerr << "[wanderingState] ❌ No nodes available, destroying ship ID " << entity->id << "\n";

            savedEnemyIDs.push(entity->id);
            stateComponent->isRespawning = true;
            CreateEnemyShip(true);
            DestroyShip(entity->id, entity->eType);
            DestroyEntity(entity->id, entity->eType); // implement or use your existing removal logic
            return;
        }
    }
    if (transformComponent && glm::any(glm::isnan(glm::vec3(transformComponent->transform[3]))))
    {
        std::cerr << "[wanderingState] ❌ Fallback node transform is NaN! Destroying ship ID " << entity->id << "\n";
        savedEnemyIDs.push(entity->id);
        stateComponent->isRespawning = true;
        CreateEnemyShip(true);
        DestroyShip(entity->id, entity->eType);
        DestroyEntity(entity->id, entity->eType);
        return; // exit early

       
    }

       
   
    


    // If the ship hasn't reached the start node, go to start node
    if (!AIcomponent->hasReachedTheStartNode)
    {
        moveToStartNode(entity, closeNodeTranscomp, dt);
    }

    // If the ship is following the path
    else if (!AIcomponent->path.empty() && AIcomponent->pathIndex < AIcomponent->path.size() &&  AIcomponent->hasReachedTheStartNode)
    {
        followPath(entity, dt);

    }
    // If the ship has completed the path
    if ( AIcomponent->pathIndex >=  AIcomponent->path.size() &&  AIcomponent->hasReachedTheStartNode)
    {
        resetPath(entity);

    }
    drawPath(entity);

    Entity* nearbyShip = nullptr;
    float detectionRadius = 30.0f; // adjust as needed
    if (IsShipNearby(entity, detectionRadius, nearbyShip))
    {
        resetPath(entity);

        int chance = rand() % 100; // 0..99
        if (chance < 70) 
        {
            AIInputComponent->currentState = AIState::ChasingEnemy; // attack
            AIInputComponent->target = nearbyShip;
        }
        else 
        {
            AIInputComponent->currentState = AIState::Fleeing;
            AIInputComponent->target = nearbyShip; // could keep target to know which to flee from
        }

        return; // skip wandering logic this frame
    }

    updateShipMovementAndParticles(entity, dt);


    //checking if the colliders close to 8.0 radius or less, ray casting will be activated
    bool colliderIsClose = false;
    Entity* closeEntity;
    Components::TransformComponent* closeTComp;
    auto shipPos = glm::vec3(transformComponent->transform[3]);
    for (auto entityIn : pureEntityData->Asteroids)
    {
        closeTComp = entityIn->GetComponent<Components::TransformComponent>();
        auto closestPos = glm::vec3(closeTComp->transform[3]);
        auto closestposLenght = glm::length(closestPos - shipPos);
        if (closestposLenght <= 8.0f && closeTComp != nullptr)
        {

            colliderIsClose = true;
            closeEntity = entityIn;
            break;
        }
    }

   
    for (int i = 0; i < colliderComponent->colliderEndPoints.size(); i++)
    {
        glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
        glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
        float len = glm::length(colliderComponent->colliderEndPoints[i]);
        int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
        if (colliderIsClose)
        {
            if (menuIsUsingRayCasts == 1.0f)
            {
                Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            }
            Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);
            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                auto entComp = closeEntity->GetComponent<Components::ColliderComponent>();
                if (payload.collider == entComp->colliderID && closeEntity->eType == EntityType::Asteroid)
                {
                    // Stop particle emitters
                    particleComponent->particleCanonLeft->data.looping = 0;
                    particleComponent->particleCanonRight->data.looping = 0;

                    // Save and flag for respawn
                    savedEnemyIDs.push(entity->id);

                    stateComponent->isRespawning = true;

                    CreateEnemyShip(stateComponent->isRespawning);
                    DestroyShip(entity->id, entity->eType);
                    DestroyEntity(entity->id, entity->eType);


                    return;
                }

            }
        }

    }





}
inline void World::attackState(Entity* entity, float dt)
{
    if (!entity) return;

    auto aiInput = entity->GetComponent<Components::AIinputController>();
    auto transformComponent = entity->GetComponent<Components::TransformComponent>();
    auto particle = entity->GetComponent<Components::ParticleEmitterComponent>();
    auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
    auto stateComponent = entity->GetComponent<Components::State>();
    if (!aiInput || !transformComponent || !particle || !colliderComponent) return;


  

    glm::vec3 currentPos = glm::vec3(transformComponent->transform[3]);
    Entity* targetShip = static_cast<Entity*>(aiInput->target);

    // --- Validate existing target ---
    if (targetShip)
    {
        auto targetState = targetShip->GetComponent<Components::State>();
        if (!targetState || targetState->isDestroyed || targetState->isRespawning)
        {
            targetShip = nullptr;
            aiInput->target = nullptr;
            aiInput->currentState = AIState::Roaming;
            return;
        }
    }

    // --- Find a new target if needed ---
    if (!targetShip)
    {
        float minDistSq = std::numeric_limits<float>::max();
        for (auto ship : pureEntityData->ships)
        {
            if (ship == entity) continue; // skip self

            auto stateComp = ship->GetComponent<Components::State>();
            if (!stateComp || stateComp->isDestroyed || stateComp->isRespawning) continue;

            auto tComp = ship->GetComponent<Components::TransformComponent>();
            if (!tComp) continue;

            float distSq = glm::length2(glm::vec3(tComp->transform[3]) - currentPos);
            if (distSq < minDistSq)
            {
                minDistSq = distSq;
                targetShip = ship;
            }
        }

        if (!targetShip)
        {
            aiInput->currentState = AIState::Roaming;
            aiInput->target = nullptr;
            return;
        }

        aiInput->target = targetShip;
    }
    if (transformComponent && glm::any(glm::isnan(glm::vec3(transformComponent->transform[3]))))
    {
        std::cerr << "[AttackState] ❌ Fallback node transform is NaN! Destroying ship ID " << entity->id << "\n";
        savedEnemyIDs.push(entity->id);
        stateComponent->isRespawning = true;
        CreateEnemyShip(true);
        DestroyShip(entity->id, entity->eType);
        DestroyEntity(entity->id, entity->eType);
        return; // exit early
    }

    auto targetTransform = targetShip->GetComponent<Components::TransformComponent>();
    if (!targetTransform)
    {
        aiInput->currentState = AIState::Roaming;
        aiInput->target = nullptr;
        return;
    }

    glm::vec3 toTarget = glm::vec3(targetTransform->transform[3]) - currentPos;
    float dist = glm::length(toTarget);

    // --- Switch back to roaming if target is out of range ---
    float attackRadius = 50.0f; // tweak as needed
    if (dist > attackRadius)
    {
        aiInput->currentState = AIState::Roaming;
        aiInput->target = nullptr;
        return;
    }

    glm::vec3 desiredDir = glm::normalize(toTarget);

    // --- Debug line to target ---
    Debug::DrawLine(currentPos, glm::vec3(targetTransform->transform[3]), 1.0f,
        glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));

    // --- Smooth rotation ---
    glm::vec3 currentForward = transformComponent->orientation * glm::vec3(0, 0, 1);
    glm::quat targetRotation = glm::rotation(currentForward, desiredDir);
    float rotationSpeed = 2.0f * dt;
    transformComponent->orientation = glm::slerp(transformComponent->orientation, targetRotation * transformComponent->orientation, rotationSpeed);

    // --- Move forward ---
    aiInput->isForward = true;
  

    // --- Update movement, thrusters, canons ---
    updateShipMovementAndParticles(entity, dt);

    // --- Firing range ---
    const float shootRange = 30.0f;
    aiInput->isShooting = (dist <= shootRange);

    // --- Cannon base positions (for reset + initialization) ---
    const float CanonPosOffset = 0.365f;
    glm::vec3 leftOrigin = glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * -CanonPosOffset + transformComponent->transform[2] * particle->canonEmitterOffset);
    glm::vec3 rightOrigin = glm::vec3(transformComponent->transform[3] + transformComponent->transform[0] * CanonPosOffset + transformComponent->transform[2] * particle->canonEmitterOffset);

    if (particle->travelLeft == 0.0f) particle->leftCanonPos = leftOrigin;
    if (particle->travelRight == 0.0f) particle->rightCanonPos = rightOrigin;

    particle->particleCanonLeft->data.randomTimeOffsetDist = 0.01;
    particle->particleCanonRight->data.randomTimeOffsetDist = 0.01;
    particle->particleCanonLeft->data.startSpeed = -50.0f;
    particle->particleCanonLeft->data.endSpeed = -50.0f;
    particle->particleCanonRight->data.startSpeed = -50.0f;
    particle->particleCanonRight->data.endSpeed = -50.0f;

    const float maxDistance = 5000.0f;

    // --- Firing logic ---
    if (aiInput->isShooting)
    {
        particle->particleCanonLeft->data.looping = 1;
        particle->particleCanonRight->data.looping = 1;
        particle->hasFired = true;
    }

    // --- Bullet simulation ---
    if (particle->hasFired)
    {
        Entity* closestEntity = nullptr;
        Components::TransformComponent* closestTComp = nullptr;

        // Set direction once per shot
        if (particle->travelLeft == 0.0f)
            particle->particleCanonLeft->data.dir = glm::vec4(-glm::vec3(transformComponent->transform[2]), 0);
        if (particle->travelRight == 0.0f)
            particle->particleCanonRight->data.dir = glm::vec4(-glm::vec3(transformComponent->transform[2]), 0);

        glm::vec3 leftDir = glm::normalize(glm::vec3(particle->particleCanonLeft->data.dir));
        glm::vec3 rightDir = glm::normalize(glm::vec3(particle->particleCanonRight->data.dir));

        float speedLeft = particle->particleCanonLeft->data.startSpeed;
        float speedRight = particle->particleCanonRight->data.startSpeed;

        // Move projectiles
        particle->leftCanonPos += leftDir * speedLeft * dt;
        particle->rightCanonPos += rightDir * speedRight * dt;

        particle->travelLeft += glm::abs(speedLeft) * dt * 500.0f;
        particle->travelRight += glm::abs(speedRight) * dt * 500.0f;

        // --- Find closest target ---
        const float hitRadius = 2.0f;
        float minDistSq = std::numeric_limits<float>::max();

        for (auto entityIn : pureEntityData->ships)
        {
            if (entityIn == entity) 
                continue; // skip self
            auto tComp = entityIn->GetComponent<Components::TransformComponent>();
            if (!tComp)
                continue;

            glm::vec3 targetPos = glm::vec3(tComp->transform[3]);
            // Check distance to both bullets
            float leftDistSq = glm::length2(targetPos - particle->leftCanonPos);
            float rightDistSq = glm::length2(targetPos - particle->rightCanonPos);
            float currentMinDistSq = std::min(leftDistSq, rightDistSq);


            if (currentMinDistSq < minDistSq)
            {
                minDistSq = currentMinDistSq;
                closestEntity = entityIn;
    
            }
        }

        bool colliderIsClose = (closestEntity && glm::sqrt(minDistSq) < 2.0f);

        // Debug bullet paths
        Debug::DrawLine(particle->leftCanonPos, particle->leftCanonPos + leftDir * 200.0f, 1.0f, glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
        Debug::DrawLine(particle->rightCanonPos, particle->rightCanonPos + rightDir * 200.0f, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));

        // --- Hit logic ---
        if (closestEntity)
        {
            auto* closestEntityStateComp = closestEntity->GetComponent<Components::State>();

            if (colliderIsClose && closestEntity != entity && !closestEntityStateComp->isRespawning)
            {
                if (closestEntity->eType == EntityType::EnemyShip || closestEntity->eType == EntityType::SpaceShip && closestEntity)
                {
                    std::cout << "AI CANNON HIT TARGET! Ship ID: " << closestEntity->id << std::endl;



                    // Respawn only enemies
                    if (closestEntity->eType == EntityType::EnemyShip)
                    {
                        if (closestEntityStateComp->isRespawning) return; // stop if already respawning
                        savedEnemyIDs.push(closestEntity->id);
                        closestEntityStateComp->isRespawning = true;
                        CreateEnemyShip(true);



                    }
                    // Respawn only players
                    if (closestEntity->eType == EntityType::SpaceShip)
                    {
                        if (closestEntityStateComp->isRespawning) return; // stop if already respawning
                        savedIDs.push(closestEntity->id);
                        closestEntityStateComp->isRespawning = true;
                         CreatePlayerShip(true);


                    }

                    // Destroy logic
                    DestroyShip(closestEntity->id, closestEntity->eType);
                    DestroyEntity(closestEntity->id, closestEntity->eType);

                    // Reset particle cannon
                    particle->hasFired = false;
                    particle->leftCanonPos = leftOrigin;
                    particle->rightCanonPos = rightOrigin;
                    particle->travelLeft = 0.0f;
                    particle->travelRight = 0.0f;
                    particle->particleCanonLeft->data.looping = 0;
                    particle->particleCanonRight->data.looping = 0;
                }
                //return out of here after killing
                aiInput->currentState = AIState::Roaming;
                return;
            }

            // Reset if bullet exceeds range
            if (particle->travelLeft >= maxDistance || particle->travelRight >= maxDistance)
            {
                particle->leftCanonPos = leftOrigin;
                particle->rightCanonPos = rightOrigin;
                particle->travelLeft = 0.0f;
                particle->travelRight = 0.0f;
                particle->hasFired = false;
                particle->particleCanonLeft->data.looping = 0;
                particle->particleCanonRight->data.looping = 0;
            }
        }

        // Update emitter origins
        particle->particleCanonLeft->data.origin = glm::vec4(particle->leftCanonPos, 1.0f);
        particle->particleCanonRight->data.origin = glm::vec4(particle->rightCanonPos, 1.0f);

        // --- Switch back to wandering if target too far ---
        if (dist > 80.0f)
            aiInput->currentState = AIState::Roaming;
        }

        //checking  collider asteroids
        bool colliderIsClose = false;
        Entity* closeEntity;
        Components::TransformComponent* closeTComp;
        auto shipPos = glm::vec3(transformComponent->transform[3]);
        for (auto entityIn : pureEntityData->Asteroids)
        {
            closeTComp = entityIn->GetComponent<Components::TransformComponent>();
            auto closestPos = glm::vec3(closeTComp->transform[3]);
            auto closestposLenght = glm::length(closestPos - shipPos);
            if (closestposLenght <= 8.0f && closeTComp != nullptr)
            {

                colliderIsClose = true;
                closeEntity = entityIn;
                break;
            }
        }

        for (int i = 0; i < colliderComponent->colliderEndPoints.size(); i++)
        {
            glm::vec3 pos = glm::vec3(transformComponent->transform[3]);
            glm::vec3 dir = transformComponent->transform * glm::vec4(glm::normalize(colliderComponent->colliderEndPoints[i]), 0.0f);
            float len = glm::length(colliderComponent->colliderEndPoints[i]);
            int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
            if (colliderIsClose)
            {
                if (menuIsUsingRayCasts == 1.0f)
                {
                    Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
                }
                Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transformComponent->transform[3]), dir, len);
                if (payload.hit)
                {
                    Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                    auto entComp = closeEntity->GetComponent<Components::ColliderComponent>();
                    if (payload.collider == entComp->colliderID && closeEntity->eType == EntityType::Asteroid)
                    {
                        // Stop particle emitters
                        particle->particleCanonLeft->data.looping = 0;
                        particle->particleCanonRight->data.looping = 0;

                        // Save and flag for respawn
                        savedEnemyIDs.push(entity->id);

                        stateComponent->isRespawning = true;
                        CreateEnemyShip(stateComponent->isRespawning);
                        DestroyShip(entity->id, entity->eType);
                        DestroyEntity(entity->id, entity->eType);
                        return;
                    }

                }
            }

        }
  
}
inline void World::fleeingState(Entity* entity, float dt)
{

    if (!entity) return;

    auto aiInput = entity->GetComponent<Components::AIinputController>();
    auto transform = entity->GetComponent<Components::TransformComponent>();
    auto particle = entity->GetComponent<Components::ParticleEmitterComponent>();
    auto collider = entity->GetComponent<Components::ColliderComponent>();
    auto stateComponent = entity->GetComponent<Components::State>();

    if (!aiInput || !transform || !particle || !collider) return;

    if (transform && glm::any(glm::isnan(glm::vec3(transform->transform[3]))))
    {
        std::cerr << "[fleeingState] ❌ Fallback node transform is NaN! Destroying ship ID " << entity->id << "\n";
        savedEnemyIDs.push(entity->id);
        stateComponent->isRespawning = true;
        CreateEnemyShip(true);
        DestroyShip(entity->id, entity->eType);
        DestroyEntity(entity->id, entity->eType);
        return; // exit early
    }

    glm::vec3 currentPos = glm::vec3(transform->transform[3]);

    // --- Find target ---
    Entity* targetShip = static_cast<Entity*>(aiInput->target);
    if (!targetShip)
    {
        float minDistSq = std::numeric_limits<float>::max();
        targetShip = nullptr;
        for (auto ship : pureEntityData->ships)
        {
            if ((ship->eType == EntityType::EnemyShip || ship->eType == EntityType::SpaceShip) && ship != entity)
            {
                auto tComp = ship->GetComponent<Components::TransformComponent>();
                if (!tComp) continue;
                float distSq = glm::length2(glm::vec3(tComp->transform[3]) - currentPos);
                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    targetShip = ship;
                }
            }
        }
        aiInput->target = targetShip;
    }
    if (!aiInput->target)
    {
        aiInput->currentState = AIState::Roaming;
        return;
    }

   

    auto targetTransform = targetShip->GetComponent<Components::TransformComponent>();
    if (!targetTransform) return;

    // --- Direction away from target (flee) ---
    glm::vec3 toTarget = glm::vec3(targetTransform->transform[3]) - currentPos;
    float dist = glm::length(toTarget);
    if (dist < 0.001f) return;

    // --- Stop fleeing if target is far enough ---
    float fleeRadius = 80.0f; // tweak as needed
    if (dist > fleeRadius)
    {
        aiInput->currentState = AIState::Roaming;
        aiInput->target = nullptr;
        return;
    }

    glm::vec3 desiredDir = -glm::normalize(toTarget); // flee direction (opposite)

    // --- Debug draw ---
    Debug::DrawLine(currentPos, currentPos + desiredDir * 10.0f, 1.0f,
        glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));

    // --- Smooth rotation using slerp ---
    glm::vec3 currentForward = transform->orientation * glm::vec3(0, 0, 1);
    glm::quat targetRotation = glm::rotation(currentForward, desiredDir);
    float rotationSpeed = 2.0f * dt;
    transform->orientation = glm::slerp(transform->orientation, targetRotation * transform->orientation, rotationSpeed);

    // --- Move forward in flee direction ---
    aiInput->isForward = true;

    // --- Update movement and thrusters ---
    updateShipMovementAndParticles(entity, dt);

    // Optional: disable shooting while fleeing
    aiInput->isShooting = false;

    //checking  collider asteroids
    bool colliderIsClose = false;
    Entity* closeEntity;
    Components::TransformComponent* closeTComp;
    auto shipPos = glm::vec3(transform->transform[3]);
    for (auto entityIn : pureEntityData->Asteroids)
    {
        closeTComp = entityIn->GetComponent<Components::TransformComponent>();
        auto closestPos = glm::vec3(closeTComp->transform[3]);
        auto closestposLenght = glm::length(closestPos - shipPos);
        if (closestposLenght <= 8.0f && closeTComp != nullptr)
        {

            colliderIsClose = true;
            closeEntity = entityIn;
            break;
        }
    }

    for (int i = 0; i < collider->colliderEndPoints.size(); i++)
    {
        glm::vec3 pos = glm::vec3(transform->transform[3]);
        glm::vec3 dir = transform->transform * glm::vec4(glm::normalize(collider->colliderEndPoints[i]), 0.0f);
        float len = glm::length(collider->colliderEndPoints[i]);
        int menuIsUsingRayCasts(Core::CVarReadInt(collider->r_Raycasts));
        if (colliderIsClose)
        {
            if (menuIsUsingRayCasts == 1.0f)
            {
                Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
            }
            Physics::RaycastPayload payload = Physics::Raycast(glm::vec3(transform->transform[3]), dir, len);
            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                auto entComp = closeEntity->GetComponent<Components::ColliderComponent>();
                if (payload.collider == entComp->colliderID && closeEntity->eType == EntityType::Asteroid)
                {
                    // Stop particle emitters
                    particle->particleCanonLeft->data.looping = 0;
                    particle->particleCanonRight->data.looping = 0;

                    // Save and flag for respawn
                    savedEnemyIDs.push(entity->id);

                    stateComponent->isRespawning = true;
                    CreateEnemyShip(stateComponent->isRespawning);
                    DestroyShip(entity->id, entity->eType);
                    DestroyEntity(entity->id, entity->eType);
                    return;
                }

            }
        }

    }
}
inline void World::moveToStartNode(Entity* entity, Components::TransformComponent* startNode, float dt)
{
    auto aiInputComponent = entity->GetComponent<Components::AIinputController>();
    auto transformComponent = entity->GetComponent<Components::TransformComponent>();
    auto AIcomponent = entity->GetComponent<Components::AI>();
    auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();

    glm::vec3 currentPos = glm::vec3(transformComponent->transform[3]);

    AstarAlgorithm* astar = AstarAlgorithm::Instance();

    Components::TransformComponent* closeNodeTranscomp = startNode;
    glm::vec3 targetDirection;
    glm::quat targetRotation;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 targetPos = glm::vec3(closeNodeTranscomp->transform[3]);
    glm::vec3 fromCurrent2Target = targetPos - currentPos;
    float distance = glm::dot(fromCurrent2Target, fromCurrent2Target);

    int menuIsUsingRayCasts(Core::CVarReadInt(colliderComponent->r_Raycasts));
    if (menuIsUsingRayCasts == 1.0f)
    {
        Debug::DrawLine(transformComponent->transform[3], closeNodeTranscomp->transform[3], 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);
    }

    if (distance <= 40.0f &&  AIcomponent->path.empty()) // automatic waypoint system
    {
         AIcomponent->hasReachedTheStartNode = true;

        // Request the actual path
        auto randomDestination = randomGetNode();
         AIcomponent->path = astar->findPath( AIcomponent->closestNodeFromShip, randomDestination);
    }

    else
    {
        // Normalize the direction to the target

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
inline void World::drawPath(Entity* entity)
{
    auto* AIcomponent = entity->GetComponent<Components::AI>();
    // Draw the path
    for (auto node : AIcomponent->path)
    {
        auto aiComp = node->GetComponent<Components::AINavNodeComponent>();
        int menuIsUsingDrawPath(Core::CVarReadInt(aiComp->r_draw_path));
        if (menuIsUsingDrawPath)
        {
            auto* nodeAI = node->GetComponent<Components::AI>();

            auto* nextNode = nodeAI->parentNode;
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
}
inline void World::resetPath(Entity* entity)
{
    auto aiComp = entity->GetComponent<Components::AI>();
    aiComp->closestNodeCalled = false;
    aiComp->path.clear();
    aiComp->hasReachedTheStartNode = false;
    aiComp->closestNodeFromShip = nullptr;
    aiComp->pathIndex = 0;
}
inline void World::followPath(Entity* entity, float dt)
{
    auto transformComponent = entity->GetComponent<Components::TransformComponent>();
    auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
    auto AIInputComponent = entity->GetComponent<Components::AIinputController>();
    auto AIcomponent = entity->GetComponent<Components::AI>();


    Debug::DrawDebugText(std::to_string(entity->id).c_str(), transformComponent->transform[3], { 0.9f,0.9f,1,1 });

    glm::vec3 targetDirection;
    glm::quat targetRotation;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    auto currentPos = glm::vec3(transformComponent->transform[3]);
    Entity* nextNode =  AIcomponent->path[ AIcomponent->pathIndex];
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
         AIcomponent->nodeArrivalTimer += dt;
        if ( AIcomponent->nodeArrivalTimer >= 0.05f)
        {
             AIcomponent->pathIndex++;
             AIcomponent->nodeArrivalTimer = 0.0f;
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
        if ( AIInputComponent->isForward)
        {
            updateShipMovementAndParticles(entity, dt);

        }


        // Create the target rotation quaternion based on the angle and axis
        targetRotation = glm::angleAxis(angle, axis);

        // Interpolate between current and target rotation to smoothly turn
        float rotationSpeed = 2.0 * dt;
        glm::quat newOrientation = glm::slerp(glm::quat(transformComponent->orientation), targetRotation, rotationSpeed);

        // Update the ship's forward direction based on the new orientation
        glm::vec3 newForward = newOrientation * currentForward;
        AIInputComponent->rotationInputX = newForward.x;
        AIInputComponent->rotationInputY = newForward.y;
        AIInputComponent->rotationInputZ = newForward.z;
        AIInputComponent->isForward = true; // Ensure the ship is moving forward

        // Update the ship's orientation
        transformComponent->orientation = newOrientation;
    }
}
inline void World::updateShipMovementAndParticles(Entity* entity, float dt)
{
    if (!entity) return;

    auto aiInput = entity->GetComponent<Components::AIinputController>();
    auto transform = entity->GetComponent<Components::TransformComponent>();
    auto particle = entity->GetComponent<Components::ParticleEmitterComponent>();
    auto camera = entity->GetComponent<Components::CameraComponent>();
    auto collider = entity->GetComponent<Components::ColliderComponent>();
    auto state = entity->GetComponent<Components::State>();
    if (!aiInput || !transform || !particle || !camera || !collider || !state) return;

    // --- Decide behavior based on state ---
    float targetSpeed = aiInput->normalSpeed;
    float rotationFactor = 0.5f;   // base smoothing
    bool isAggressive = false;

    switch (aiInput->currentState)
    {
    case AIState::Roaming:
    {
        targetSpeed *= 10.0f;        // full speed
        rotationFactor = 10.0f;      // rotate faster toward target
        isAggressive = false;
        break;
    }
        
    case AIState::ChasingEnemy:
    {
        targetSpeed *= 12.0f;        // full speed
        rotationFactor = 10.0f;      // rotate faster toward target
        isAggressive = true;
        break;
    }
      

    case AIState::Fleeing:
    {
         targetSpeed *= 10.0f;        // full speed
        rotationFactor = 10.0f;      // rotate faster toward target
        isAggressive = false;
        break;
    }
       

    default:
        break;
    }

    // Smooth acceleration toward targetSpeed
    aiInput->currentSpeed = glm::mix(aiInput->currentSpeed, targetSpeed, dt * 2.5f);


    // ================================================
    //  VELOCITY UPDATE (REAL FIX)
    // ================================================
    glm::vec3 localVel(0, 0, aiInput->currentSpeed);
    glm::vec3 worldVel = glm::vec3(transform->transform * glm::vec4(localVel, 0));

    // smooth velocity
    transform->linearVelocity = glm::mix(
        transform->linearVelocity,
        worldVel,
        1.0f - pow(0.0001f, dt)   // framerate independent smoothing
    );

    // Apply movement – FIX: Removed dt * 10.0f
    transform->transform[3] += glm::vec4(transform->linearVelocity * dt, 0.0f);


    // ================================================
    //  ROTATION UPDATE
    // ================================================
    float rotX = aiInput->rotXSmooth * rotationFactor;
    float rotY = aiInput->rotYSmooth * rotationFactor;
    float rotZ = aiInput->rotZSmooth * rotationFactor;

    glm::quat dq = glm::quat(glm::vec3(-rotY, rotX, rotZ));
    transform->orientation = transform->orientation * dq;

    // Clamp roll
    aiInput->rotationZ -= rotX;
    aiInput->rotationZ = glm::clamp(aiInput->rotationZ, -45.0f, 45.0f);

    glm::mat4 T = glm::translate(glm::vec3(transform->transform[3])) *
        glm::mat4_cast(transform->orientation);

    transform->transform = T * glm::mat4(glm::quat(glm::vec3(0, 0, aiInput->rotationZ)));

    aiInput->rotationZ = glm::mix(aiInput->rotationZ, 0.0f, dt * camera->cameraSmoothFactor);


    // ================================================
    // PARTICLE UPDATE (unchanged, clean)
    // ================================================
    const float thrusterOffset = 0.365f;
    const float canonOffset = 0.365f;
    float speedFactor = aiInput->currentSpeed / aiInput->normalSpeed;

    // Thrusters
    particle->particleEmitterLeft->data.origin =
        glm::vec4(glm::vec3(transform->transform[3] + transform->transform[0] * -thrusterOffset +
            transform->transform[2] * particle->emitterOffset), 1);

    particle->particleEmitterRight->data.origin =
        glm::vec4(glm::vec3(transform->transform[3] + transform->transform[0] * thrusterOffset +
            transform->transform[2] * particle->emitterOffset), 1);

    particle->particleEmitterLeft->data.dir = glm::vec4(-glm::vec3(transform->transform[2]), 0);
    particle->particleEmitterRight->data.dir = glm::vec4(-glm::vec3(transform->transform[2]), 0);

    particle->particleEmitterLeft->data.startSpeed = 1.2f + 3.0f * speedFactor;
    particle->particleEmitterLeft->data.endSpeed = 0.0f + 3.0f * speedFactor;
    particle->particleEmitterRight->data.startSpeed = 1.2f + 3.0f * speedFactor;
    particle->particleEmitterRight->data.endSpeed = 0.0f + 3.0f * speedFactor;


    // ================================================
    //  CANNON PARTICLES (unchanged)
    // ================================================
    particle->particleCanonLeft->data.origin =
        glm::vec4(glm::vec3(transform->transform[3] + transform->transform[0] * -canonOffset +
            transform->transform[2] * particle->canonEmitterOffset), 1);

    particle->particleCanonRight->data.origin =
        glm::vec4(glm::vec3(transform->transform[3] + transform->transform[0] * canonOffset +
            transform->transform[2] * particle->canonEmitterOffset), 1);

    particle->particleCanonLeft->data.dir = glm::vec4(-glm::vec3(transform->transform[2]), 0);
    particle->particleCanonRight->data.dir = glm::vec4(-glm::vec3(transform->transform[2]), 0);

    if (aiInput->isShooting && isAggressive)
    {
        particle->particleCanonLeft->data.looping = 1;
        particle->particleCanonRight->data.looping = 1;

        particle->particleCanonLeft->data.startSpeed = -500.0f;
        particle->particleCanonRight->data.startSpeed = -500.0f;
        particle->particleCanonLeft->data.endSpeed = -500.0f;
        particle->particleCanonRight->data.endSpeed = -500.0f;
    }
    else
    {
        particle->particleCanonLeft->data.looping = 0;
        particle->particleCanonRight->data.looping = 0;
    }


    // ================================================
    //  CAMERA UPDATE (unchanged)
    // ================================================
    if (camera->theCam)
    {
        glm::vec3 desiredCamPos =
            glm::vec3(transform->transform[3]) +
            glm::vec3(transform->transform * glm::vec4(0, camera->camOffsetY, -4.0f, 0));

        camera->camPos = glm::mix(camera->camPos, desiredCamPos, dt * camera->cameraSmoothFactor);

        camera->theCam->view = glm::lookAt(camera->camPos,
            camera->camPos + glm::vec3(transform->transform[2]),
            glm::vec3(transform->transform[1]));
    }
}
inline bool World::IsShipNearby(Entity* entity, float detectionRadius, Entity*& outShip)
{
    glm::vec3 shipPos = glm::vec3(entity->GetComponent<Components::TransformComponent>()->transform[3]);

    for (auto other : pureEntityData->ships) // or whatever container holds other ships
    {
        if (other == entity) continue; // skip self

        auto otherTrans = other->GetComponent<Components::TransformComponent>();
        glm::vec3 otherPos = glm::vec3(otherTrans->transform[3]);
        float distance = glm::length(otherPos - shipPos);

        if (distance <= detectionRadius)
        {
            outShip = other;
            return true;
        }
    }

    return false;
}