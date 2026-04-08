#pragma once

#include "shaderClass.h"
#include "Tile.h"
#include "VBO.h"
#include "EBO.h"
#include "VAO.h"
#include "Camera.h"
#include "Texture.h"
#include "Window.h"

#define voidcast(x) reinterpret_cast<void*>(x)

// SINGLETON ENGINE3D
class Engine3D {
private:

	static Engine3D* engine;

	// Initialize a scene and a camera
	Camera UserCamera = Camera(AVector3(0.0f, 0.0f, 0.0f),0.0f, 0.0f);
	Camera SunCamera = Camera(AVector3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
	Scene MainScene;

	Texture depthTextureObject;

	// Shader
	Shader shaderProgram;
	Shader instanceProgram;
	Shader shadowProgram;
	VAO VAO_1;
	VBO VBO_1;
	EBO EBO_1;

	Window window;

	GLuint instanceVBO;
	bool InstanceVBOSetupComplete = false;

	// Appearance
	struct {
		float R = 0.2f;
		float G = 0.3f;
		float B = 0.5f; 
		float A = 1.0f;
	} backgroundColor;

	// Private methods

	void registerCameraInput(float FOVdeg, float zNear, float zFar);

	Engine3D() = default;

	~Engine3D() = default;

	// We don't need a copy and equal constructor, we have a Singleton
	Engine3D(const Engine3D&) = delete;
	Engine3D& operator=(const Engine3D&) = delete;

	void shadowPassInstanceShader();
	void renderPassInstanceShader();

	//FOR FPS COUNTER
	struct {
		int frameCounter = 0;
		float msPerFrame = 0;
		double PREV_TIME = 0.0f;
		const double FPSsampleTime = 1.0f / 20.0f;
	}FPS;

public:

	bool DEBUG = false;

	static Engine3D* GetEngine3D();
	static void EngineTerminate();

	// SETERS

	void setWindowTitle(const std::string& winTitle);

	int setupWindow(const int WINDOW_WIDTH, const int WINDOW_HEIGHT, const char* WINDOW_TITLE);

	void setCamera(float posX, float posY, float posZ);
	void setSunCamera(float posX, float posY, float posZ);

	void setCamera(float posX, float posY, float posZ, float yaw, float pitch);

	int getDrawStyle(const char* style);

	void setupShaders();

	void setupGeometryArrayObjects(const char* style);

	void setupInstanceVBO();
	
	void SetupFull(const char* style);

	inline void setBackground(float R, float G, float B, float A) { backgroundColor = { R,G,B,A }; };

	// GETTERS
	Camera& getCamera(bool Sun);

	Scene* getScene();

	//FUNCTION MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
	//Tile* getVisibleCameraFrustum();

	inline int windowShouldClose() { 
		return window.windowShouldClose(); 
	}
	
	//OTHERS

	void initGameFrame();

	void shadowPass();

	void renderPass(float FOVdeg, float zNear, float zFar);

	void DrawAllInstances();

	void RenderInstances(int timeOfDay);
	
	void DEBUG_showCameraVectors();
	void DEBUG_ArrayOrganizers();
};