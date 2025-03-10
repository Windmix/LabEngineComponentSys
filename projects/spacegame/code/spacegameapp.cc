//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include "render/physics.h"
#include <chrono>
#include "spaceship.h"
#include "./render/entityManagement/world.h""
#include <crtdbg.h>
#include <render/entityManagement/chunkAllocator.h>

using namespace Display;
using namespace Render;

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::SpaceGameApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::~SpaceGameApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
SpaceGameApp::Open()
{
    _CrtDumpMemoryLeaks();
	App::Open();
	this->window = new Display::Window;
    this->window->SetSize(2500, 2000);

    if (this->window->Open())
	{
		// set clear color to gray
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        RenderDevice::Init();

		// set ui rendering function
		this->window->SetUiRender([this]()
		{
			this->RenderUI();
		});
        
        return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Run()
{

    int w;
    int h;
    this->window->GetSize(w, h);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = projection;

    // load all resources
    ModelId models[6] = {
        LoadModel("assets/space/Asteroid_1.glb"),
        LoadModel("assets/space/Asteroid_2.glb"),
        LoadModel("assets/space/Asteroid_3.glb"),
        LoadModel("assets/space/Asteroid_4.glb"),
        LoadModel("assets/space/Asteroid_5.glb"),
        LoadModel("assets/space/Asteroid_6.glb")
    };
    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };
    //fixing for the new component system
    World*  world = World::instance();

    std::vector<std::tuple<ModelId, Physics::ColliderId, glm::mat4>> asteroids;
    Entity* asteroidEntity;



    // Setup asteroids near
    for (int i = 0; i < 100; i++)
    {
        asteroidEntity = world->createEntity(EntityType::Asteroid);
        if (asteroidEntity->eType == EntityType::Asteroid)

            {

                size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
                // Allocating stuff to the chunkAllocator
                Components::TransformComponent* newTransform = world->transformChunk.Allocate();
                asteroidEntity->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::Asteroid);

                
                Components::RenderableComponent* renderable = world->renderableChunk.Allocate(models[resourceIndex]);
                asteroidEntity->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::Asteroid);
               

                Components::ColliderComponent* collider = world->colliderChunk.Allocate(colliderMeshes[resourceIndex]);
                asteroidEntity->AddComponent(collider, ComponentType::COLLIDER, EntityType::Asteroid);
             


                world->start(); // setup satart values
                
                //transform Setup

                newTransform->entityType = EntityType::Asteroid;
                // Randomize position
                glm::vec3 randomPosition(Core::RandomFloatNTP() * 100.0f, Core::RandomFloatNTP() * 100.0f, Core::RandomFloatNTP() * 100.0f);
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




 

       



 
        
        //--------------------------------------------------------------------------------------------------------------------------



     /*   std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        std::get<0>(asteroid) = models[resourceIndex];
        float span = 20.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
        std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
        std::get<2>(asteroid) = transform;
        asteroids.push_back(asteroid);*/
    }
    
    //for (int i = 0; i < 50; i++)
    //{
    //    asteroidEntity = world.createEntity(EntityType::Asteroid);


    //    if (asteroidEntity->eType == EntityType::Asteroid)
    //    {


    //        //fixing for the new component system
    //       // Allocating the TransformComponent using the PoolAllocator
    //        Components::TransformComponent* newTransform = world.transformChunk.Allocate();
    //        asteroidEntity->AddComponent(newTransform, ComponentType::TRANSFORM);

    //        // Allocating the RenderableComponent using the PoolAllocator
    //        Components::RenderableComponent* renderable = world.renderableChunk.Allocate(models[Core::FastRandom() % 6]);
    //        asteroidEntity->AddComponent(renderable, ComponentType::RENDERABLE);

    //        // Allocating the ColliderComponent using the PoolAllocator
    //        Components::ColliderComponent* collider = world.colliderChunk.Allocate(colliderMeshes[Core::FastRandom() % 6]);
    //        asteroidEntity->AddComponent(collider, ComponentType::COLLIDER);

    //        world.start();
    //    }
   
    //}

    //}
     //Setup asteroids far
   /* for (int i = 0; i < 50; i++)
    {
        std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        std::get<0>(asteroid) = models[resourceIndex];
        float span = 80.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
        std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
        std::get<2>(asteroid) = transform;
        asteroids.push_back(asteroid);
    }*/

    // Setup skybox
    std::vector<const char*> skybox
    {
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png"
    };
    TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
    RenderDevice::SetSkybox(skyboxId);
    
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    const int numLights = 40;
    Render::PointLightId lights[numLights];
    // Setup lights
    for (int i = 0; i < numLights; i++)
    {
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f
        );
        glm::vec3 color = glm::vec3(
            Core::RandomFloat(),
            Core::RandomFloat(),
            Core::RandomFloat()
        );
        lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }
 
    //SpaceShip ship;
    //ship.model = LoadModel("assets/space/spaceship.glb");

    auto shipModel = LoadModel("assets/space/spaceship.glb");

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

    Entity* spaceship;
    for (int i = 0; i < 1; i++)
    {
        spaceship = world->createEntity(EntityType::SpaceShip);
        if (spaceship->eType == EntityType::SpaceShip)
        {

            Components::TransformComponent* newTransform = world->transformChunk.Allocate();
            spaceship->AddComponent(newTransform, ComponentType::TRANSFORM, EntityType::SpaceShip);
           /* newTransform->transform[3] = glm::vec4(0.0f + i*2, 0.0f + i*2, 0.0f + i*2, 0.0f);*/

            Components::RenderableComponent* renderable = world->renderableChunk.Allocate(shipModel);
            spaceship->AddComponent(renderable, ComponentType::RENDERABLE, EntityType::SpaceShip);

            Components::ColliderComponent* collider = world->colliderChunk.Allocate();
            spaceship->AddComponent(collider, ComponentType::COLLIDER, EntityType::SpaceShip);
            for (int i = 0; i < sizeof(colliderEndPoints) / sizeof(glm::vec3); i++)
            {
                collider->colliderEndPoints[i] = colliderEndPoints[i];
            }
            Components::CameraComponent* camera = world->cameraChunk.Allocate();
            spaceship->AddComponent(camera, ComponentType::CAMERA, EntityType::SpaceShip);

            Components::RigidBodyComponent* rigidBody = world->rigidBodyChunk.Allocate();
            spaceship->AddComponent(rigidBody, ComponentType::RIGIDBODY, EntityType::SpaceShip);

            Components::PlayerInputComponent* controllinput = world->controlInputChunk.Allocate();
            spaceship->AddComponent(controllinput, ComponentType::INPUT, EntityType::SpaceShip);

             Components::ParticleEmitterComponent* particleEmitter = world->particleEmitterChunk.Allocate();
            spaceship->AddComponent(particleEmitter, ComponentType::PARTICLE_EMITTER, EntityType::SpaceShip);

            particleEmitter->particleEmitterLeft = world->ChunkOfPartcles.Allocate(particleEmitter->numParticles);
            particleEmitter->particleEmitterRight = world->ChunkOfPartcles.Allocate(particleEmitter->numParticles);

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

            ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterLeft);
            ParticleSystem::Instance()->AddEmitter(particleEmitter->particleEmitterRight);

            world->start();

        }
       
    }



   

    std::clock_t c_start = std::clock();
    double dt = 0.01667f;

    // game loop
    while (this->window->IsOpen())
	{
        auto timeStart = std::chrono::steady_clock::now();
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
        
        this->window->Update();
       

        if (kbd->pressed[Input::Key::Code::End])
        {
            ShaderResource::ReloadShaders();
        }
      
        //ship.Update(dt);
        
       //ship.CheckCollisions();

        // Draw some debug text
        Debug::DrawDebugText("FOOBAR", glm::vec3(0), {1,0,1,1});
      
        world->Update(dt);

        for (auto& entity : world->entities)
        {
            auto renderableComponent = entity->GetComponent<Components::RenderableComponent>();

            if (renderableComponent)
            {
                Debug::DrawDebugText(std::to_string(entity->GetComponent<Components::RenderableComponent>()->ownerId).c_str(), entity->GetComponent<Components::TransformComponent>()->transform[3], {0.9f,0.9f,1,1});
  
                RenderDevice::Draw(renderableComponent->modelId, entity->GetComponent<Components::TransformComponent>()->transform);
            }
 
        }
        // Store all drawcalls in the render device
       /* for (auto const& asteroid : asteroids)
        {
            RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
        }*/

      //  RenderDevice::Draw(ship.model, ship.transform);

        // Execute the entire rendering pipeline
        RenderDevice::Render(this->window, dt);

		// transfer new frame to window
		this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

       
       

        if (kbd->pressed[Input::Key::Code::Escape])
        {
         
            this->Exit();
        }
            

	}
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Exit()
{
    
    this->window->Close();
    World::destroy();
    
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::RenderUI()
{
	if (this->window->IsOpen())
	{
        ImGui::Begin("Debug");
        Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
        int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
        if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
            Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
        
        Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
        int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
        if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
            Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);


        
        ImGui::End();

        Debug::DispatchDebugTextDrawing();
	}
}

} // namespace Game