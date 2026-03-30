#include <iostream>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "window";

#include "Engine3D.h"

int main() {

	Engine3D& engine = *Engine3D::GetEngine3D();
	std::cout << "Engine initialized \n";

	int success = engine.setupGLFW(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }

	engine.setBackground(0.2f, 0.3f, 0.8f, 1.0f);

	// HERE WE CREATE OUR OBJECTS /////////////////////////////////////////////////
	
	Blueprint* humanMesh = engine.LoadSTLGeomFile("resources/BASEmodel.stl", 8.0f);
	if (humanMesh) {
		std::cout << "Created: Human \n";
		for (int i = 0; i < 4; i++) {
			Instance* human = engine.getScene()->CreateInstance(humanMesh, AVector3());
			human->SetPosition(AVector3(0.0f + i * 40.0f, 2.0f, -5.0f));
			human->SetColor(i * 50, i * 50, 255 - i * 50, 255);
			human->SetRotation(AVector3(-90.0f, 0.0f, 0.0f));
		}
	}
	else {
		delete humanMesh;
	}
	
	Blueprint* cubeB = engine.CreateCube(10.0f);

	for (int i = 0; i < 10; i++) {
		Instance* I = engine.getScene()->CreateInstance(cubeB, AVector3());
		I->SetColor(0, i*15, 255, 255);
		I->SetPosition(AVector3(10.0f * i, 5.0f, 10.0f * i));
	}

	Instance* plane = engine.getScene()->CreateInstance(cubeB, AVector3());
	plane->SetPosition(AVector3(0.0f, -5.0f, 0.0f));
	plane->SetSize(AVector3(20.0f, 1.0f, 20.0f));
	plane->SetColor(AColor3(50, 255, 25, 255));

	std::vector<AVertex> Verticies = { AVertex(-4.8f, 0.0f, -4.0f), AVertex(3.5f, 0.0f, -3.75f), AVertex(1.6f, 0.0f, 4.15f), AVertex(-2.65f, 0.0f, 2.75f) };
	Blueprint* prismB = engine.CreatePrism(Verticies, 4, 10.0f);

	Instance* triPrism = engine.getScene()->CreateInstance(prismB, AVector3());
	triPrism->SetColor(255, 255, 0, 255);
	triPrism->SetPosition(AVector3(-30.0f, -5.0f, -25.0f));
	triPrism->SetSize(AVector3(5.0f, 5.0f, 5.0f));
	
	////////////////////////////////////////////////////////////////////////////////

	std::cout << "Meshes constructed \n";

	engine.setCamera(0.0f, 80.0f, 120.0f); //Set camera to this position
	engine.setSunCamera(0.0f, 100.0f, 1.0f);

	engine.setupShaders(); //Uses Camera Class and Mesh Instances

	engine.setupGeometryArrayObjects(engine.getDrawStyle("dynamic"));
	engine.setupInstanceVBO();

	std::cout << "Shaders created\n";

	float _deltaTimeForTIMER = 1.0f / 10.0f;
	double prevTime = glfwGetTime();

	int ROT = 0;

	//FOR FPS COUNTER
	double PREV_TIME = 0.0f;
	double CURRENT_TIME = 0.0f;
	double timeDifference;
	int frameCounter = 0;
	const double FPSsampleTime = 1.0f / 20.0f;

	// MAIN GAME LOOP
	while (!engine.windowShouldClose()) {

		CURRENT_TIME = glfwGetTime();
		timeDifference = CURRENT_TIME - PREV_TIME;
		frameCounter++;
		if (timeDifference >= FPSsampleTime) {
			std::string FPS = std::to_string((1.0f / timeDifference) * frameCounter);
			std::string msPerFrame = std::to_string((timeDifference / frameCounter) * 1000);
			std::string winTitle = "WINDOW | " + FPS + " FPS | " + msPerFrame + " ms/frame";
			glfwSetWindowTitle(engine.getWindow(), winTitle.c_str());
			PREV_TIME = CURRENT_TIME;
			frameCounter = 0;
		}


		engine.initGameFrame(); // setting up the background color

		// TIMER (global) ////////////
		double currentTime = glfwGetTime();
		if (currentTime - prevTime >= _deltaTimeForTIMER) {
			//engine.DEBUG_showCameraVectors();
			prevTime = currentTime;

			// Sky colors
			float sinROT = sin((double)ROT / 1024.0f);
			float cosROT = cos((double)ROT / 1024.0f);
			engine.getCamera(true).Position = AVector3( 100.0f * cosROT, 100.0f * sinROT, 50.0f );
			float lightReflactance = std::max(0.0f, sinf((float)ROT / 512.0f));
			float sunsetCoef = std::abs(cos((double)ROT / 512.0f)*1.5f);
			engine.setBackground((sunsetCoef) * 0.25f * (lightReflactance + 0.25f), 
				(lightReflactance + sunsetCoef / 2.0f) * 0.5f * (lightReflactance), 
				lightReflactance, 1.0f);
			ROT+=4;
		}

		engine.shadowPass(false);
		engine.renderPass(false,45.0f, 0.1f, 1000.0f);
	}

	Engine3D::EngineTerminate();

	return 0;
}