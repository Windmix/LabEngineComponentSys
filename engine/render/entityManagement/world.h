#pragma once

#include "AstarAlgorithm.h"
#include "chunkAllocator.h"
#include "components.h"  // Assuming components are declared here
#include "render/renderdevice.h"
#include <render/model.h>
#include "pureEntityData.h"



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
    uint32_t savedID = 0;
    uint32_t savedEnemyID = 0;

    int randomIndex;

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
    void Cleanup();
    void DestroyWorld();

    void CreatePlayerShip(bool isRespawning);
    void CreateEnemyShip(bool isRespawning);

    void CreateAsteroid(float spread);
    void CreatePathNode(float xOffset, float yOffset, float zOffset, float deltaXYZ);

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

};


static World* _instance; // singleton


inline World::World()
{

   pureEntityData = PureEntityData::instance();

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
        entity->id = savedID;
    }
    else if (entity->eType == EntityType::EnemyShip && isRespawning)
    {
        entity->id = savedEnemyID;
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
    //RNG

    //updates everything
    for (Entity* entity : pureEntityData->entities)
    {
        
        UpdateAsteroid(entity, dt);
        UpdateAiShip(entity, dt);
        UpdateShip(entity, dt);
        UpdateNode(entity, dt);
        draw(entity);
        updateCamera(entity, dt);
       
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
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterLeft);
                ChunkOfPartcles.Deallocate(particleEmitterComp->particleEmitterRight);
                particleEmitterChunk.Deallocate(particleEmitterComp);
            }
            
       
          
        }


        //// Deallocate the entity from the Chunk
        entityChunk.Deallocate(entityToDelete);

        // Remove the entity from the entity vector
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
        newTransform->transform[3] = glm::vec4(0.0f, 0.0f + 10.0f, 0.0f, 0.0f);

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
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
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
           .startSpeed = -0.0f,
           .endSpeed = -0.0f,
           .startScale = 0.02f,
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
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
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
           .startSpeed = -0.0f,
           .endSpeed = -0.0f,
           .startScale = 0.02f,
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
    }

}
inline void World::CreateEnemyShip(bool isRespawning)
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
        Entity* AIspaceship = createEntity(EntityType::EnemyShip, isRespawning);

        Components::TransformComponent* newTransform = transformChunk.Allocate();
        AIspaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::EnemyShip);
       // newTransform->transform[3] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
        AIspaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::EnemyShip);

        Components::ColliderComponent* collider = colliderChunk.Allocate();
        AIspaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::EnemyShip);
        for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
        {
            collider->colliderEndPoints[i] = colliderEndPoints[i];
        }
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
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = particleEmitter->numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.01f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.080f
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
           .startSpeed = -0.0f,
           .endSpeed = -0.0f,
           .startScale = 0.02f,
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
    }
    else
    {
        Entity* AIspaceship = createEntity(EntityType::EnemyShip, isRespawning);

        Components::TransformComponent* newTransform = transformChunk.Allocate();
        AIspaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::EnemyShip);
        /* newTransform->transform[3] = glm::vec4(0.0f + i*2, 0.0f + i*2, 0.0f + i*2, 0.0f);*/

        Components::RenderableComponent* renderable = renderableChunk.Allocate(shipModel);
        AIspaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::EnemyShip);

        Components::ColliderComponent* collider = colliderChunk.Allocate();
        AIspaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::EnemyShip);
        for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
        {
            collider->colliderEndPoints[i] = colliderEndPoints[i];
        }

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
           .startColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) * 3.0f,
           .endColor = glm::vec4(0,0,0,1.0f),
           .numParticles = particleEmitter->numParticles,
           .theta = glm::radians(0.0f),
           .startSpeed = -0.0f,
           .endSpeed = -0.0f,
           .startScale = 0.02f,
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
inline void World::CreatePathNode(float xOffset, float yOffset, float zOffset, float deltaXYZ)
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



    Components::TransformComponent* newTransform = transformChunk.Allocate();
    node->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::Node);
    newTransform->transform[3] = glm::vec4(-100.0f + xOffset * deltaXYZ, -100.0f + yOffset * deltaXYZ, -100.0f + zOffset * deltaXYZ, 0);

   
    Components::AINavNodeComponent* NodeComp = navNodeChunk.Allocate();
    node->AddComponent(NodeComp, ComponentType::NAVNODE, EntityType::Node);
    for (int i = 0; i < sizeof(EndPoints) / sizeof(glm::vec3); i++)
    {
        NodeComp->EndPoints[i] = EndPoints[i];
    }
    // pushback for use of the Debug menu
    pureEntityData->nodes.push_back(node);

    //give it to the map too
    pureEntityData->gridNodes[node->id] = node;
   
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

                    particleComponent->particleCanonLeft->data.looping = 0;
                    particleComponent->particleCanonRight->data.looping = 0;
                    savedID = entity->id;
                    CreatePlayerShip(true);
                    DestroyEntity(entity->id, entity->eType);
                    hit = false;

                }

            }

        }



    }
}
inline void World::UpdateAiShip(Entity* entity, float dt)
{
    if (entity->eType == EntityType::EnemyShip && entity) // PlayerShip
    {
        auto aiInputComponent = entity->GetComponent<Components::AIinputController>();
        auto transformComponent = entity->GetComponent<Components::TransformComponent>();
        auto cameraComponent = entity->GetComponent<Components::CameraComponent>();
        auto colliderComponent = entity->GetComponent<Components::ColliderComponent>();
        auto particleComponent = entity->GetComponent<Components::ParticleEmitterComponent>();
        AstarAlgorithm* astar = AstarAlgorithm::Instance();

        Entity* closestNodeFromShip;
        glm::vec3 targetDirection;
        glm::quat targetRotation;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // Up vector

        Debug::DrawDebugText(std::to_string(aiInputComponent->currentSpeed).c_str(), transformComponent->transform[3], { 0.9f,0.9f,1,1 });

        // Find the closest node if not already called
        if (!entity->closestNodeCalled)
        {
            closestNodeFromShip = getclosestNodeFromAIship(entity);
        }

        auto closeNodeTranscomp = closestNodeFromShip->GetComponent<Components::TransformComponent>();
        glm::vec3 currentPos = glm::vec3(transformComponent->transform[3]);

        //make sure it is not nullptr
        if (closeNodeTranscomp == nullptr)
        {
            return;
        }
        // If the ship hasn't reached the start node
        if (!entity->hasReachedTheStartNode)
        {
            glm::vec3 targetPos = glm::vec3(closeNodeTranscomp->transform[3]);
            glm::vec3 fromCurrent2Target = targetPos - currentPos;
            float distance = glm::dot(fromCurrent2Target, fromCurrent2Target);

            Debug::DrawLine(transformComponent->transform[3], closeNodeTranscomp->transform[3], 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (distance <= 40.0f && entity->path.empty()) // automatic waypoint system
            {
                entity->hasReachedTheStartNode = true;

                // Request the actual path
                auto randomDestination = randomGetNode();
                entity->path = astar->findPath(closestNodeFromShip, randomDestination);
            }
           
            else
            {
                // Smoothly rotate towards the target
                targetDirection = glm::normalize(fromCurrent2Target);
                glm::vec3 currentForward = glm::vec3(0, 0, 1); // Assuming z-axis is forward

                // Calculate the rotation needed
                float angle = glm::acos(glm::dot(currentForward, targetDirection));
                glm::vec3 axis = glm::normalize(glm::cross(currentForward, targetDirection));

                // Handle edge case where target is directly behind
                if (glm::length(axis) < 0.001f)
                {
                    axis = up; // Use the up vector as the axis
                }

                if (aiInputComponent->isForward)
                {
                    if (aiInputComponent->isBoosting)
                        aiInputComponent->currentSpeed = glm::mix(aiInputComponent->currentSpeed, aiInputComponent->boostSpeed, std::min(1.0f, dt * 30.0f));
                    else
                    {
                        if (distance > 100.0f)
                        {
                            // Beyond max range? Cap at full speed
                            aiInputComponent->currentSpeed = glm::min(aiInputComponent->currentSpeed + dt, 1.0f);

                        }
                        else if (distance > 40.0f && distance <= 100.0f)
                        {
                            // Map distance 20 -> 9 to speed 1.0 -> 0.2
                            float t = glm::clamp((distance - 40.0f) / (100.0f - 40.0f), 0.0f, 1.0f);
                            float targetSpeed = glm::mix(0.2f, 1.0f, t); // Closer to 9 = slower
                            aiInputComponent->currentSpeed = glm::mix(aiInputComponent->currentSpeed, targetSpeed, dt * 2.0f); // Smoothly interpolate
                        }
                    }
                }
                else
                {
                    aiInputComponent->currentSpeed = 0.0f;
                }

              
                // Create the target rotation quaternion
                targetRotation = glm::angleAxis(angle, axis);

                // Interpolate between current and target rotation
                float rotationSpeed = 3.0f * dt; // Adjust for smoothness
                glm::quat newOrientation = glm::slerp(glm::quat(transformComponent->orientation), targetRotation, rotationSpeed);

                // Update the ship's forward direction
                glm::vec3 newForward = newOrientation * currentForward;

                // Set the AI input based on the new forward direction
                aiInputComponent->rotationInputX = newForward.x;
                aiInputComponent->rotationInputY = newForward.y;
                aiInputComponent->rotationInputZ = newForward.z;

                aiInputComponent->isForward = true;

                // Update the ship's orientation
                transformComponent->orientation = newOrientation;
            }
        }
        // If the ship is following the path
        else if (!entity->path.empty() && entity->pathIndex < entity->path.size() && entity->hasReachedTheStartNode)
        {
            Entity* nextNode = entity->path[entity->pathIndex];
            auto nextTransform = nextNode->GetComponent<Components::TransformComponent>();
            glm::vec3 targetPos = glm::vec3(nextTransform->transform[3]);

            glm::vec3 fromCurrent2Target = targetPos - currentPos;
            float distance = glm::dot(fromCurrent2Target, fromCurrent2Target); // Distance to the target node

            Debug::DrawLine(transformComponent->transform[3], nextTransform->transform[3], 1.0f, glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (distance <= 40.0f) // If close enough to the target node
            {


                entity->nodeArrivalTimer += dt; // Timer for arrival
                if (entity->nodeArrivalTimer >= 0.05f) // Delay before moving to the next node
                {
                    entity->pathIndex++; // Move to the next node
                    entity->nodeArrivalTimer = 0.0f; // Reset timer
                }
            }
            
            
            else
            {
                // Smoothly rotate towards the target
                targetDirection = glm::normalize(fromCurrent2Target);
                glm::vec3 currentForward = glm::vec3(0, 0, 1); // Assuming z-axis is forward

                // Calculate the rotation needed
                float angle = glm::acos(glm::dot(currentForward, targetDirection));
                glm::vec3 axis = glm::normalize(glm::cross(currentForward, targetDirection));

                // Handle edge case where target is directly behind
                if (glm::length(axis) < 0.001f)
                {
                    axis = up; // Use the up vector as the axis
                }

                // Create the target rotation quaternion
                targetRotation = glm::angleAxis(angle, axis);

                
                if (aiInputComponent->isForward)
                {
                    if (aiInputComponent->isBoosting)
                        aiInputComponent->currentSpeed = glm::mix(aiInputComponent->currentSpeed, aiInputComponent->boostSpeed, std::min(1.0f, dt * 30.0f));
                    else
                    {
                        if (distance > 100.0f)
                        {
                            // Beyond max range? Cap at full speed
                            aiInputComponent->currentSpeed = glm::min(aiInputComponent->currentSpeed + dt, 1.0f);

                        }
                        else if (distance > 40.0f && distance <= 100.0f)
                        {
                            // Map distance 20 -> 9 to speed 1.0 -> 0.2
                            float t = glm::clamp((distance - 40.0f) / (100.0f - 40.0f), 0.0f, 1.0f);
                            float targetSpeed = glm::mix(0.2f, 1.0f, t); // Closer to 9 = slower
                            aiInputComponent->currentSpeed = glm::mix(aiInputComponent->currentSpeed, targetSpeed, dt * 2.0f); // Smoothly interpolate
                        }
                    }
                }
                else
                {
                    aiInputComponent->currentSpeed = 0.0f;
                }

                // Interpolate between current and target rotation
                float rotationSpeed = 3.0f * dt; // Adjust for smoothness
                glm::quat newOrientation = glm::slerp(glm::quat(transformComponent->orientation), targetRotation , rotationSpeed);

                // Update the ship's forward direction
                glm::vec3 newForward = newOrientation * currentForward;

                // Set the AI input based on the new forward direction
                aiInputComponent->rotationInputX = newForward.x;
                aiInputComponent->rotationInputY = newForward.y;
                aiInputComponent->rotationInputZ = newForward.z;

                // Set the speed and forward movement
                
                aiInputComponent->isForward = true;

                // Update the ship's orientation
                transformComponent->orientation = newOrientation;
            }
        }
        // If the ship has completed the path
        else if (entity->pathIndex >= entity->path.size() && entity->hasReachedTheStartNode)
        {
            entity->closestNodeCalled = false;
            entity->hasReachedTheStartNode = false;
            entity->path.clear();
            entity->pathIndex = 0;
        }

        // Draw the path
        auto aiComp = entity->GetComponent<Components::AINavNodeComponent>();
        int menuIsUsingDrawPath(Core::CVarReadInt(aiComp->r_draw_path));
        if (menuIsUsingDrawPath)
        {
            for (auto node : entity->path)
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
        }
      

        // Update ship movement and orientation
        if (aiInputComponent && transformComponent && cameraComponent)
        {
           

            glm::vec3 desiredVelocity = glm::vec3(0, 0, aiInputComponent->currentSpeed);
            desiredVelocity = transformComponent->transform * glm::vec4(desiredVelocity, 0.0f);
            transformComponent->linearVelocity = glm::mix(transformComponent->linearVelocity, desiredVelocity, aiInputComponent->accelerationFactor);

            float rotX = aiInputComponent->rotationInputX;
            float rotY = aiInputComponent->rotationInputY;
            float rotZ = aiInputComponent->rotationInputZ;

            transformComponent->transform[3] += glm::vec4(transformComponent->linearVelocity * dt * 10.0f, 0.0f);

            const float rotationSpeed = 1.8f * dt;
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

                    particleComponent->particleCanonLeft->data.looping = 0;

                    particleComponent->particleCanonRight->data.looping = 0;
                    //entity->path.clear();
                    savedEnemyID = entity->id;
                    CreateEnemyShip(true);
                    DestroyEntity(entity->id, entity->eType);
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
   /* if (Aicomponent)
    {
        Debug::DrawDebugText(std::to_string(Aicomponent->ownerId).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], { 0.9f,0.9f,1,1 });
    }*/

}
inline void World::updateCamera(Entity* entity, float dt)
{
    if (entity->eType == EntityType::EnemyShip || entity->eType == EntityType::SpaceShip) // PlayerShip
    {
        // First pass: Check if the selected entity already has a camera

        Components::CameraComponent* savedCamera;

        //Render::CameraManager::GetCamera(CAMERA_MAIN);
        auto camComp = entity->GetComponent<Components::CameraComponent>();
        if (camComp)
        {
            int selectedID(Core::CVarReadInt(camComp->r_camera_id));

            // If entity has a camera component and it matches the current selected ID
            if (entity->id == selectedID)
            {
                // Assign main camera only to the selected entity
                if (camComp->theCam == nullptr)
                {
                    camComp->theCam = Render::CameraManager::GetCamera(CAMERA_MAIN);
                }
            }
            else
            {
                // Null out the camera for all others
                if (camComp->theCam != nullptr)
                {
                    camComp->theCam = nullptr;
                }
            }
        }
        return;
       
    }
   
}
