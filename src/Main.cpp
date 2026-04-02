#include <iostream>
#include <csignal>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "window";

#include "Engine3D.h"

#define Scene engine->getScene()
#define workspace Scene->GetWorkspace()

// ENVIROMENT VARIABLE, binds to SIGTERM such that Engine3D is terminated
//  without memory leaks and the cleanup process succeds
bool force_exit = false;

void exit_signal(int SIGNAL) {
	if (SIGNAL == SIGTERM) {
		force_exit = true;
	}
}

int main() {

	std::signal(SIGTERM, exit_signal);

	Engine3D* engine = Engine3D::GetEngine3D();
	std::cout << "Engine initialized \n";

	int success = engine->setupWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

	engine->setBackground(0.2f, 0.3f, 0.8f, 1.0f);

	// HERE WE CREATE OUR OBJECTS /////////////////////////////////////////////////

	Blueprint* humanMesh = Scene->LoadSTLGeomFile("resources/BASEmodel.stl", 8.0f);
	if (humanMesh) {
		std::cout << "Created: Human \n";
		for (int i = 0; i < 4; i++) {
			auto human = Scene->CreateInstance(humanMesh, 
				"Human " + std::to_string(i) //TAG NAME
			);
			human->SetPosition(AVector3(0.0f + i * 40.0f, 2.0f, -25.0f));
			human->SetColor(i * 50, i * 50, 255 - i * 50, 255);
			human->SetRotation(AVector3(-90.0f, 0.0f, 0.0f));
		}
	}
	else {
		delete humanMesh;
	}

	Blueprint* cubeB = Scene->CreateCube(10.0f);

	std::shared_ptr<Instance> firstcube = nullptr;
	std::shared_ptr<Instance> prevI = nullptr;
	for (int i = 0; i < 10; i++) {
		auto I = Scene->CreateInstance(cubeB, "Cube " + std::to_string(i));
		if (prevI) {
			I->SetParent(prevI);
		}
		else {
			firstcube = I;
		}
		I->SetColor(0, i * 15, 255, 255);
		I->SetPosition(AVector3(10.0f * i, 5.0f, 10.0f * i));
		prevI = I;
	}

	auto plane = Scene->CreateInstance(cubeB, "Plane");
	plane->SetPosition(AVector3(0.0f, -5.0f, 0.0f));
	plane->SetSize(AVector3(20.0f, 1.0f, 20.0f));
	plane->SetColor(AColor3(50, 255, 25, 255));

	std::vector<AVertex> Verticies = { AVertex(-4.8f, 0.0f, -4.0f), AVertex(3.5f, 0.0f, -3.75f), AVertex(1.6f, 0.0f, 4.15f), AVertex(-2.65f, 0.0f, 2.75f) };
	Blueprint* prismB = Scene->CreatePrism(Verticies, 4, 10.0f);

	auto triPrism = Scene->CreateInstance(prismB, "TriPrism");
	triPrism->SetColor(255, 255, 0, 255);
	triPrism->SetPosition(AVector3(-30.0f, -5.0f, -25.0f));
	triPrism->SetSize(AVector3(5.0f, 5.0f, 5.0f));

	triPrism->SetParent(plane);

	//Print blueprints
	int i = 0;
	for (auto& blueprint : Scene->GetBlueprints()) {
		std::cout << "Blueprint " << i << " has ID " << blueprint->GetID() << "\n";
		i++;
	}
	// GetParent method
	auto parent = triPrism->GetParent();
	if (&*parent == &*plane) {
		std::cout << "These objects are the same \n";
	}
	// AllChildrenWith method
	if (workspace.lock()) {
		auto cube1 = Scene->CreateInstance(cubeB, "Cube 1");
		cube1->SetParent(firstcube);
		const std::string s = "Cube 1";
		const std::vector<std::shared_ptr<Instance>>& vectinst = firstcube->AllChildrenWith(s);
		std::cout << "Instances with name " << s << ": " << vectinst.size() <<"\n";

		// GetAVector3s
		AVector3 pos = cube1->GetPosition();
		AVector3 rot = cube1->GetRotation();
		AVector3 size = cube1->GetSize();
		cube1->SetSize(size * 2.0f);

	}
	////////////////////////////////////////////////////////////////////////////////

	std::cout << "Meshes constructed \n";

	engine->setCamera(0.0f, 40.0f, 120.0f);
	engine->setSunCamera(0.0f, 100.0f, 1.0f);

	engine->setupShaders(); //Uses Camera Class and Mesh Instances

	engine->setupGeometryArrayObjects("static");
	engine->setupInstanceVBO();

	std::cout << "Shaders created\n";

	float _deltaTimeForTIMER = 1.0f / 60.0f;
	double prevTime = glfwGetTime();

	int ROT = 0;

	//FOR FPS COUNTER
	double PREV_TIME = 0.0f;
	int frameCounter = 0;
	const double FPSsampleTime = 1.0f / 20.0f;

	int cntt = 0;

	Scene->DEBUG_PrintInstanceHierarchy(workspace, 0, 10, false);

	// MAIN GAME LOOP
	while (!engine->windowShouldClose() && !force_exit) {

		double CURRENT_TIME = glfwGetTime();
		double timeDifference = CURRENT_TIME - PREV_TIME;
		frameCounter++;
		if (timeDifference >= FPSsampleTime) {
			std::string FPS = std::to_string((1.0f / timeDifference) * frameCounter);
			std::string msPerFrame = std::to_string((timeDifference / frameCounter) * 1000);
			std::string winTitle = "WINDOW | " + FPS + " FPS | " + msPerFrame + " ms/frame";
			engine->setWindowTitle(winTitle);
			PREV_TIME = CURRENT_TIME;
			frameCounter = 0;
		}

		engine->initGameFrame(); // setting up the background color

		// TIMER (global) ////////////
		double currentTime = glfwGetTime();
		if (currentTime - prevTime >= _deltaTimeForTIMER) {
			//engine.DEBUG_showCameraVectors();
			prevTime = currentTime;

			// Sky colors
			float sinROT = sin((double)ROT / 1024.0f);
			float cosROT = cos((double)ROT / 1024.0f);
			engine->getCamera(true).Position = AVector3(10.0f * cosROT, 200.0f * sinROT, 100.0f);
			ROT += 2;

			bool translations = false;

			if (translations) {
				float dt = (float)(cntt / 10 % 100);
				plane->SetPosition(AVector3(dt, -5.0f, 0.0f));
				plane->SetSize(AVector3(25.0f + dt / 5.0f, 0.5f, 25.0f - dt / 5.0f));
				firstcube->SetPosition(AVector3(dt / 2.0f, 5.0f, 0.0f));

				std::string tag = "Human 3";
				auto workspace_sptr = workspace.lock();
				auto child = workspace_sptr->FirstChild(tag); // Finds first Child with Tag "Human 3"

				if (child) child->SetRotation(AVector3(-90.0f + dt, 0.0f, 0.0f));
				else std::cout << "Not found: the human 0! \n";
			}

			cntt++;
		}
		if (cntt > 20000) {
			// Test out dynamic deletions
			plane->Destroy();
		}

		engine->shadowPass();
		engine->renderPass(45.0f, 0.1f, 1000.0f);
	}

	std::cout << " \n\n | -------- CLEANING PROCESS STARTED --------------- | \n\n";

	std::cout << "Game Instance Hierarchy before cleanup! \n";

	Scene->DEBUG_PrintInstanceHierarchy(workspace, 0, 10, true);

	std::cout << "Cleaning up Engine3D... \n";

	Engine3D::EngineTerminate();

	std::cout << "Cleaning process terminated! \n";

	return 0;
}