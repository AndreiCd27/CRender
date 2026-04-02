

#include "Engine3D.h"
#include <cstring>

Engine3D* Engine3D::engine = nullptr;

Engine3D* Engine3D::GetEngine3D() {
	if (!engine) engine = new Engine3D();
	return engine;
}

void Engine3D::setWindowTitle(const std::string& winTitle) {
	glfwSetWindowTitle(window.getWindow(), winTitle.c_str());
}

void Engine3D::setCamera(float posX, float posY, float posZ) {
	AVector3 Position = { posX, posY, posZ };
	UserCamera = Camera(Position, -90.0f, 0.0f);
}

void Engine3D::setCamera(float posX, float posY, float posZ, float yaw, float pitch) {
	//EX: Camera({ 0.0f, 80.0f, 120.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });;
	AVector3 Position = { posX, posY, posZ };
	UserCamera = Camera(Position, yaw, pitch);
}

void Engine3D::setSunCamera(float posX, float posY, float posZ) {
	//EX: Camera({ 0.0f, 80.0f, 120.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });;
	AVector3 Position = { posX, posY, posZ };
	SunCamera = Camera(Position, 0.0f, 0.0f);
}


void Engine3D::DEBUG_showCameraVectors() {
	std::cout << UserCamera.Position.x << " " << UserCamera.Position.y << " " << UserCamera.Position.z<<"\n";
	std::cout << UserCamera.Yaw <<" "<< UserCamera.Pitch << "\n";
}
void Engine3D::DEBUG_ArrayOrganizers() {

	std::cout << "VBO Total Bytes: " << getScene()->GetVBO_Organizer().GetMultiArray().size() * sizeof(AVertex) << "\n";
	std::cout << "EBO Total Bytes: " << getScene()->GetEBO_Organizer().GetMultiArray().size() * sizeof(GLuint) << "\n";

	std::cout << "Instances Size: " << getScene()->GetInstanceOrganizer().GetMultiArray().size() << "\n";
	std::cout << "VBO Size: " << getScene()->GetVBO_Organizer().GetMultiArray().size() << "\n";
	std::cout << "EBO Size: " << getScene()->GetEBO_Organizer().GetMultiArray().size() << "\n";
}

int Engine3D::setupWindow(const int WINDOW_WIDTH, const int WINDOW_HEIGHT, const char * WINDOW_TITLE) {
	
	bool success = window.CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	if (!success) {
		std::cout << "Error when creating window \n";
		EngineTerminate();
		return -1;
	}
	//Load GLAD (needed to configure OpenGL)
	gladLoadGL();
	//Specify Viewport to OpenGL ( from (0,0) to (W_WIDTH,W_HEIGHT) )
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	return 1;
}

int Engine3D::getDrawStyle(const char* style) {
	if (strcmp(style, "static") == 0) return GL_STATIC_DRAW;
	if (strcmp(style, "stream") == 0) return GL_STREAM_DRAW;
	if (strcmp(style, "dynamic") == 0) return GL_DYNAMIC_DRAW;
	return GL_STATIC_DRAW;
}

void Engine3D::setupShaders() {

	shaderProgram.Setup("Shaders/default.vert", "Shaders/default.frag");

	instanceProgram.Setup("Shaders/instance.vert", "Shaders/instance.frag");

	shadowProgram.Setup("Shaders/shadow.vert", "Shaders/shadow.frag");

	glEnable(GL_DEPTH_TEST);

}

void Engine3D::setupGeometryArrayObjects(const char* style) {
	
	const int drawStyle = getDrawStyle(style);

	VAO_1.Setup();

	VAO_1.Bind();

	if (DEBUG)std::cout << "Got vertex and indicies buffers \n";

	std::vector<AVertex>& vert = MainScene.GetVBO_Organizer().GetMultiArray();
	std::vector<GLuint>& indicies = MainScene.GetEBO_Organizer().GetMultiArray();
	VBO_1.Setup(vert.data(), vert.size() * sizeof(AVertex), drawStyle);
	EBO_1.Setup(indicies.data(), indicies.size() * sizeof(GLuint), drawStyle);

	if (DEBUG)std::cout << "Total VBO elements: " << vert.size() << "\n";

	if (DEBUG)std::cout << "VBO & EBO setup complete \n";

	GLsizei stride = sizeof(AVertex); //32 bytes
	
	// APosition ( 3 floats)
	VAO_1.LinkVBO(VBO_1, 0, 3, GL_FLOAT, stride, GL_FALSE, voidcast(0));
	// RGBA	( uint32 = 4 * byte )
	VAO_1.LinkVBO(VBO_1, 1, 4, GL_UNSIGNED_BYTE, stride, GL_TRUE, voidcast(12));
	// ANormal ( 3 floats )
	VAO_1.LinkVBO(VBO_1, 2, 3, GL_FLOAT, stride, GL_FALSE, voidcast(16));
	// UV ( uint32 = short + short )
	VAO_1.LinkVBO(VBO_1, 3, 2, GL_UNSIGNED_SHORT, stride, GL_TRUE, voidcast(28));

	if (DEBUG)std::cout << "VBO linking complete \n";

	depthTextureObject.setupFBO();
	depthTextureObject.setupDepthTexture(4096);

	if (DEBUG)std::cout << "Setup FBO complete \n";

	VAO_1.Unbind();
	VBO_1.Unbind();
	EBO_1.Unbind();

	if (DEBUG)std::cout << "Unbinding..\n";
}

void Engine3D::setupInstanceVBO() {

	// We create a VBO for matrices of instances
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// We specify GL_DYNAMIC_DRAW beacause our values will change frequently as the camera moves
	// We need a new relative translation matrix every frame for every instance,
	// Even though our camera view and proj matrix stay the same for every instance

	int matarraysize = (int)MainScene.GetInstanceOrganizer().GetMultiArray().size();

	glBufferData(GL_ARRAY_BUFFER, matarraysize * sizeof(InstanceData), NULL, GL_DYNAMIC_DRAW);

	// Configure VAO for mat4 matricies (4*vec4 size)
	VAO_1.Bind();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	for (int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(4 + i);
		glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), voidcast(sizeof(glm::vec4) * i));
		glVertexAttribDivisor(4 + i, 1);
	}
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(InstanceData), voidcast(64));
	glVertexAttribDivisor(8, 1);
	glEnableVertexAttribArray(9);
	glVertexAttribPointer(9, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(InstanceData), voidcast(68));
	glVertexAttribDivisor(9, 1);
	VAO_1.Unbind();

	InstanceVBOSetupComplete = true;

}

void Engine3D::SetupFull(const char* drawStyle) {
	engine->setCamera(0.0f, 40.0f, 120.0f); // Default Player Camera Position
	engine->setSunCamera(200.0f, 200.0f, 200.0f); // Default Sun Camera Position

	// Configure shaders
	engine->setupShaders();

	// Configure OpenGL essentials:
	// 1) VAO (Vertex Array Object) - here we store "layouts"; we bind with VBO, EBO, instanceVBO
	// 2) VBO (Vertex Buffer Object) - here we store Blueprint vertices and send them to OpenGL
	// 3) EBO (Entity Buffer Object) - here we store vertex indicies, such that OpenGL knows
	// how to connect each vertex, having the result of drawing a sequence of triangles in 3D space
	// 4) instanceVBO - here we store translation matricies (4x4)
	engine->setupGeometryArrayObjects(drawStyle);
	engine->setupInstanceVBO();
}
/* TO FIX
void Engine3D::DrawInstances(Blueprint* BLUEPRINT, const Tile* TILE) {

	if (TILE == nullptr) return;

	try {

		int HandleID = BLUEPRINT->GetID() | (TILE->GetTileID() << Tile::shiftComponent);
		Handle InstancesHandle = MainScene.GetInstanceOrganizer().GetHandleData(HandleID);

		Handle BlueprintHandle = MainScene.GetBlueprintHandle(BLUEPRINT, EBO_ORGANIZER_TARGET);

		void* offsetPtr = (void*)(uintptr_t)(BlueprintHandle.offset * sizeof(GLuint));

		glDrawElementsInstanced(GL_TRIANGLES, BlueprintHandle.size, GL_UNSIGNED_INT, offsetPtr, InstancesHandle.size);
	}
	catch (ArrayOrganizerException& e) {
		std::cout << e.what() << "\n";
	}
}
*/

Camera& Engine3D::getCamera(bool Sun) { if (Sun) { return SunCamera; } return UserCamera; }
Scene* Engine3D::getScene() { return &MainScene; }

void Engine3D::initGameFrame() {
	//Set background color to be drawn
	glClearColor(backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);
}


void Engine3D::registerCameraInput(float FOVdeg, float zNear, float zFar) {
	UserCamera.Inputs(window.getWindow());
	//UserCamera.Matrix(FOVdeg, zNear, zFar, windowAspectRatio, shaderProgram);
	UserCamera.Matrix(FOVdeg, zNear, zFar, window.getAspectRatio(), instanceProgram);
}

void Engine3D::DrawAllInstances() {
	std::vector<int> handleIDsFromRoot;
	MainScene.WorldRoot->RecurseInTilesOutputHandleIDs(handleIDsFromRoot);
	for (int i = 0; i < (int)handleIDsFromRoot.size(); i++) {
		int HandleID = handleIDsFromRoot[i];

		Handle InstH = MainScene.GetInstanceOrganizer().GetHandleData(HandleID);

		Handle BlueprintHandle = MainScene.GetEBO_Organizer().GetHandleData(HandleID & 4095);

		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		uint32_t stride = sizeof(InstanceData);
		
		uintptr_t base = (uintptr_t)(InstH.offset * stride);
		for (int j = 0; j < 4; j++) {
			glEnableVertexAttribArray(4 + j);
			glVertexAttribPointer(4 + j, 4, GL_FLOAT, GL_FALSE, stride, voidcast(base + j * 16));
			glVertexAttribDivisor(4 + j, 1);
		}

		uint32_t offRGBA = 64;
		uint32_t offUV = 68;

		glEnableVertexAttribArray(8);
		void* offset = voidcast((uintptr_t)(InstH.offset * stride + offRGBA));
		glVertexAttribPointer(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offset);
		glVertexAttribDivisor(8, 1);
		glEnableVertexAttribArray(9);
		offset = voidcast((uintptr_t)(InstH.offset * stride + offUV));
		glVertexAttribPointer(9, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, offset);
		glVertexAttribDivisor(9, 1);

		void* offsetPtr = voidcast(BlueprintHandle.offset * sizeof(GLuint));

		Handle VBO_Handle = MainScene.GetVBO_Organizer().GetHandleData(HandleID & 4095);

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, BlueprintHandle.size, GL_UNSIGNED_INT, 
			offsetPtr, InstH.size, VBO_Handle.offset);

		//std::cout << "\n //////Rendering HandleID: " << HandleID << " with " << InstH.size << " instances. \n\n";

	}
}

void Engine3D::shadowPassInstanceShader() {
	shadowProgram.Activate();
	VAO_1.Bind();

	int matarraysize = (int)MainScene.GetInstanceOrganizer().GetMultiArray().size();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, matarraysize * sizeof(InstanceData), &MainScene.GetInstanceOrganizer().GetMultiArray()[0]);

	// The sun looks from the UserCamera's position, so the shadowsMap doesn't stay forever at (0,0,0)
	SunCamera.LightMatrix(500.0f, shadowProgram, false, UserCamera.Position);

	DrawAllInstances();
}


void Engine3D::renderPassInstanceShader() {
	// START TO DRAW INSTANCES

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTextureObject.depthTexture);

	int matarraysize = (int)MainScene.GetInstanceOrganizer().GetMultiArray().size();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, matarraysize * sizeof(InstanceData), &MainScene.GetInstanceOrganizer().GetMultiArray()[0]);

	DrawAllInstances();
}

void Engine3D::shadowPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, depthTextureObject.FBO_ID);
	glViewport(0, 0, 4096, 4096);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	shadowPassInstanceShader();
	
}

void Engine3D::renderPass(float FOVdeg, float zNear, float zFar) {
	
	instanceProgram.Activate();
	VAO_1.Bind();

	this->registerCameraInput(FOVdeg, zNear, zFar);
	instanceProgram.SetUniformVector3("lightDirection", SunCamera.Position);
	// The sun looks from the UserCamera's position, so the shadowsMap doesn't stay forever at (0,0,0)
	SunCamera.LightMatrix(500.0f, instanceProgram, true, UserCamera.Position);

	glUniform1i(instanceProgram.GetUniformLocation("shadowMap"), 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glViewport(0, 0, window.getWidth(), window.getHeight());
	//Clear the BACK BUFFER and assign our color to it
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glDepthMask(GL_TRUE);
	
	renderPassInstanceShader();

	//Swap BACK BUFFER with FRONT BUFFER
	glfwSwapBuffers(window.getWindow());
	// Get events (for controls, event handling, closing, etc.)
	glfwPollEvents();
}

void Engine3D::RenderInstances(int timeOfDay) {
	double ROT = (timeOfDay - 12) / 12.0f * glm::pi<double>();
	float sinROT = sin(ROT);
	float cosROT = cos(ROT);
	SunCamera.Position = AVector3(100.0f * cosROT, 100.0f * sinROT, 50.0f);
	initGameFrame();
	shadowPass();
	renderPass(45.0f, 0.1f, 1000.0f);
}
/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
Tile* Engine3D::getVisibleCameraFrustum() {
	return MainScene.FindTileForPosition(
		AVertex(), UserCamera.Position
	);
}
*/

void Engine3D::EngineTerminate() {

	//Delete our VAOs, VBOs, EBOs
	if (engine->VAO_1.GetCompleteStatus()) engine->VAO_1.Delete();
	if (engine->VBO_1.GetCompleteStatus()) engine->VBO_1.Delete();
	if (engine->EBO_1.GetCompleteStatus()) engine->EBO_1.Delete();

	//Delete instanceVBO
	if (engine->InstanceVBOSetupComplete) glDeleteBuffers(1, &engine->instanceVBO);

	//Delete shader
	if (engine->shaderProgram.GetCompleteStatus()) engine->shaderProgram.Delete();
	if (engine->instanceProgram.GetCompleteStatus()) engine->instanceProgram.Delete();

	//Destroy WINDOW OBJECT
	engine->window.Terminate();

	delete engine;

}