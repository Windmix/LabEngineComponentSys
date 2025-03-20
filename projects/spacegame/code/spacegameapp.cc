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
#include "render/entityManagement/AstarAlgorithm.h"

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

    //síngleton world
    World* world = World::instance();
    //AstartAlgorithm* aStar = AstartAlgorithm::Instance();
   

    // Setup asteroids far
    for (int i = 0; i < 100; i++)
    {
        world->CreateAsteroid(200.0f);
    }
    // Setup asteroids near
    for (int i = 0; i < 100; i++)
    {
        world->CreateAsteroid(100.0f);
    }

    //gridbased nodes
    int size3D = 10;
    world->pureEntityData->NodestackSizescubicRoot = size3D;

    float distanceBetweenPoints = 30.0f;
    for (int i = 0; i < size3D; i++)
    {
        for (int j = 0; j < size3D; j++)
        {
            for (int k = 0; k < size3D; k++)
            {
                world->CreatePathNode(k, j, i, distanceBetweenPoints);
            }
        }
      

    }
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
            Core::RandomFloatNTP() * 20.0f);
        glm::vec3 color = glm::vec3(
            Core::RandomFloat(),
            Core::RandomFloat(),
            Core::RandomFloat());
        lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }

    for (int i = 0; i < 1; i++)
    {
        world->CreateEnemyShip(false);
    }
    world->CreatePlayerShip(false);

   



   

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
      

        // Draw some debug text
        Debug::DrawDebugText("FOOBAR", glm::vec3(0), {1,0,1,1});
      
        world->Update(dt);


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

        // light
        Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
        int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
        if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
            Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
        
        Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
        int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
        if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
            Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);

        //draw AI path
        Core::CVar* r_draw_path = Core::CVarGet("r_draw_path");
        int drawPath = Core::CVarReadInt(r_draw_path);
        if (ImGui::Checkbox("draw AI path", (bool*)&drawPath))
        {
            Core::CVarWriteInt(r_draw_path, drawPath);

        }

        // nodes
        Core::CVar* r_draw_Node_Axis = Core::CVarGet("r_draw_Node_Axis");
        int drawNodeAxis = Core::CVarReadInt(r_draw_Node_Axis);
        if (ImGui::Checkbox("Draw Node Axis", (bool*)&drawNodeAxis))
        {
            Core::CVarWriteInt(r_draw_Node_Axis, drawNodeAxis);

        }
          
        Core::CVar* r_draw_Node_Axis_id = Core::CVarGet("r_draw_Node_Axis_id");
        int nodeAxisID = Core::CVarReadInt(r_draw_Node_Axis_id);
        if (ImGui::InputInt("NodeId", (int*)&nodeAxisID))
            Core::CVarWriteInt(r_draw_Node_Axis_id, nodeAxisID);

        // camera
        Core::CVar* r_camera = Core::CVarGet("r_Camera");
        int drawCamera = Core::CVarReadInt(r_camera);
        if (ImGui::Checkbox("Camera_player/camera_enemy", (bool*)&drawCamera))
        {
            Core::CVarWriteInt(r_camera, drawCamera);

        }

        Core::CVar* r_camera_id = Core::CVarGet("r_Camera_id");
        int drawCameraId = Core::CVarReadInt(r_camera_id);
        if (ImGui::InputInt("CameraId", (int*)&drawCameraId))
            Core::CVarWriteInt(r_camera_id, drawCameraId);

        
        ImGui::End();

        Debug::DispatchDebugTextDrawing();
	}
}

} // namespace Game