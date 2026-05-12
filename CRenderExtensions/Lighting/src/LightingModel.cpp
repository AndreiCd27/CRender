
#include "LightingModel.h"

const std::string path = "CRenderExtensions/Lighting";

SHLM::SHLM(Engine3D* engine, EngineConfig* config, AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax)
	: VoxelGrid(VoxelResolution, WorldMin, WorldMax), LightingService(engine,config)
{

	SphericalHarmonics = new SH<SH_Order>();
	SH_OBJ_Blockers = new BlockerXYZR<MAX_BLOCKER_COUNT>;
	SH_SKY_Blockers = new Blockers<MAX_BLOCKER_COUNT>;

	SH_Program.Setup((path+"/Shaders/SH.vert").c_str(), (path + "/Shaders/SH.frag").c_str());

	//shadowMaskProgram.Setup((path + "/Shaders/instance.vert").c_str(), (path + "/Shaders/shadowMask.frag").c_str());

}

void SHLM::BindToEngine(float FOVdeg, float zNear, float zFar) {

	std::function<void(float, float, float)> SH_f = [this](float f1, float f2, float f3) {
		this->SH_renderPass(f1, f2, f3);
	};

	config->RenderOverride = true;

	config->AddAction(RENDER_INSTANCES_STAGE, SH_f, FOVdeg, zNear, zFar);
}

void SHLM::SetBlockerSKY(float phi, float theta, float radius, float dist) {
	if (SH_SKY_Blockers != nullptr) {

		SH_SKY_Blockers->phi[BlockerCountSKY] = phi;
		SH_SKY_Blockers->theta[BlockerCountSKY] = theta;
		SH_SKY_Blockers->radius[BlockerCountSKY] = radius;
		SH_SKY_Blockers->dist[BlockerCountSKY] = dist;

		BlockerCountSKY++;
	}
}
void SHLM::SetBlockerOBJ(float x, float y, float z, float radius, int insID) {
	if (SH_OBJ_Blockers != nullptr) {

		SH_OBJ_Blockers->x[BlockerCountOBJ] = x;
		SH_OBJ_Blockers->y[BlockerCountOBJ] = y;
		SH_OBJ_Blockers->z[BlockerCountOBJ] = z;
		SH_OBJ_Blockers->r[BlockerCountOBJ] = radius;
		SH_OBJ_Blockers->insID[BlockerCountOBJ] = insID;

		BlockerCountOBJ++;
	}
}

// Called only by Load_Cubemap functions
void SHLM::GenTextures_Cubemap() {
	glGenTextures(4, texture3D_IDs);

	for (int i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_3D, texture3D_IDs[i]);

		// Texture Parameters
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
}

void SHLM::Load_Cubemap_MultiThreaded() {

	GenTextures_Cubemap();

	VoxelGrid.VoxelizeBlockers(*SH_OBJ_Blockers, BlockerCountOBJ);

	VoxelGrid.Bake3DGrid(*SH_OBJ_Blockers, BlockerCountOBJ, *SphericalHarmonics);

	for (int i = 0; i < 4; i++) {
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
			VoxelGrid.GetGridX(), // Width (x)
			VoxelGrid.GetGridY(), // Height (y)
			VoxelGrid.GetGridZ(), // Depth (z)
			0, GL_RGBA, GL_FLOAT,
			VoxelGrid.volumeData[i].data()
		);
	}
	glBindTexture(GL_TEXTURE_3D, 0);
}

void SHLM::Load_Cubemap_GPU_ComputeShader() {

	const int gridX = VoxelGrid.GetGridX();
	const int gridY = VoxelGrid.GetGridY();
	const int gridZ = VoxelGrid.GetGridZ();

	const AVector3 worldMin = VoxelGrid.GetWorldMin();
	const AVector3 worldMax = VoxelGrid.GetWorldMax();

	if (!VoxelizeMeshes.GetCompleteStatus()) {

		GenTextures_Cubemap();

		VoxelizeMeshes.Setup((path + "/ComputeShaders/VoxelizeMesh.comp").c_str());
		BakeVoxels.Setup((path + "/ComputeShaders/SH_VoxelVis.comp").c_str());

		idxTrigBuffer = VoxelizeMeshes.CreateSSBO();
		idxVoxelInsIDs = VoxelizeMeshes.CreateSSBO();
		idxBlockerBuffer = VoxelizeMeshes.CreateSSBO();


		for (int i = 0; i < 4; i++) {
			glBindTexture(GL_TEXTURE_3D, texture3D_IDs[i]);
			glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, gridX, gridY, gridZ);
		}
	}

	std::vector<GPU_Trig> trigData;

	//// Get Triangles From All Meshes

	const auto& BlueprintData = engine->getScene()->GetBlueprints();

	// each matrix is an instance model matrix
	const ArrayOrganizer<InstanceData>& MatrixOrg = engine->getScene()->GetMatrixOrganizer();
	const ArrayOrganizer<Instance>& InstanceOrg = engine->getScene()->GetInstanceOrganizer();

	const std::vector<InstanceData>& MatrixArray = MatrixOrg.GetMultiArray();
	const std::vector<Instance>& InstanceArray = InstanceOrg.GetMultiArray();

	const auto& VertIndOrg = engine->getScene()->GetEBO_Organizer();
	const auto& VertOrg = engine->getScene()->GetVBO_Organizer();

	const std::vector<GLuint>& VertIndArray = VertIndOrg.GetMultiArray();
	const std::vector<AVertex>& VertArray = VertOrg.GetMultiArray();

	AVector3 TempV[3];

	for (const Handle& matHandle : MatrixOrg.GetHandles()) {

		// Ref: int handleID = ins.Template->GetID() | (ins.tile->GetTileID() << Tile::shiftComponent);
		const Blueprint* B = BlueprintData[matHandle.id & Blueprint::GetSafeBlueprintCount()];
		//const int VertHandleID = B->GetVerticesHandleID();
		const int IndHandleID = B->GetVerticesHandleID();

		const Handle& IndHandle = VertIndOrg.GetHandleData(IndHandleID);
		//const Handle& VertHandle = VertOrg.GetHandleData(VertHandleID);

		for (int i = matHandle.offset; i < matHandle.offset + matHandle.size; i++) {
			const auto& InstanceMatrix = MatrixArray[i];
			int InsID = InstanceArray[i].GetEID();

			for (int j = IndHandle.offset; j < IndHandle.offset + IndHandle.size; j++) {
				int Vidx = VertIndArray[j];
				const AVertex& V = VertArray[Vidx];
				glm::vec4 glmV = InstanceMatrix.matrix * glm::vec4(V.POS.x, V.POS.y, V.POS.z, 1.0f);
				TempV[j % 3] = AVector3(glmV.x, glmV.y, glmV.z);
				if (j % 3 == 2) {
					trigData.push_back(GPU_Trig(TempV[0], TempV[1], TempV[2], InsID));
				}
			}
		}
	}


	////

	VoxelizeMeshes.SetDataSSBO<GPU_Trig>(trigData, (int)trigData.size(), idxTrigBuffer);

	////

	std::vector<GPU_Blocker> blockerData;
	blockerData.resize(BlockerCountOBJ);

	for (int i = 0; i < BlockerCountOBJ; i++) {
		blockerData[i] = {
			glm::vec4(SH_OBJ_Blockers->x[i],SH_OBJ_Blockers->y[i],SH_OBJ_Blockers->z[i],SH_OBJ_Blockers->r[i]),
			glm::vec4(SH_OBJ_Blockers->insID[i])
		};
	}

	BakeVoxels.SetDataSSBO<GPU_Blocker>(blockerData, (int)blockerData.size(), idxBlockerBuffer);

	// For Voxels, we allocate empty space
	VoxelizeMeshes.AllocateEmptySSBO<int>(gridX * gridY * gridZ, idxVoxelInsIDs);

	VoxelizeMeshes.Activate();
	VoxelizeMeshes.SetInt("triangleCount", (int)trigData.size());
	BakeVoxels.SetUniformVector3("worldMin", worldMin);
	BakeVoxels.SetUniformVector3("worldMax", worldMax);
	BakeVoxels.SetUniformVector3_int("gridRes", glm::ivec3(gridX, gridY, gridZ));
	VoxelizeMeshes.BindSSBO<0>(idxTrigBuffer);
	VoxelizeMeshes.BindSSBO<1>(idxVoxelInsIDs);

	int clearVal = -1; // Reset Buffer Data
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &clearVal);

	int numGroups = (trigData.size() + 127) / 128;
	glDispatchCompute(numGroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	BakeVoxels.Activate();
	BakeVoxels.SetInt("BlockerCount", BlockerCountOBJ);
	BakeVoxels.SetUniformVector3("worldMin", worldMin);
	BakeVoxels.SetUniformVector3("worldMax", worldMax);
	BakeVoxels.SetUniformVector3_int("gridRes", glm::ivec3(gridX, gridY, gridZ));
	// Bindings 0-3 are reserved for our textures
	BakeVoxels.BindSSBO<4>(idxBlockerBuffer);
	BakeVoxels.BindSSBO<5>(idxVoxelInsIDs); // Reuse SSBO

	// Bind texture3Ds
	for (int i = 0; i < 4; i++) {
		glBindImageTexture(i, texture3D_IDs[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	glDispatchCompute(gridX / 4, gridY / 4, gridZ / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	glDeleteBuffers(1, &idxBlockerBuffer);
	glDeleteBuffers(1, &idxTrigBuffer);
	glDeleteBuffers(1, &idxVoxelInsIDs);
}

void SHLM::Upload_Cubemap() {

	glGenTextures(4, texture3D_IDs);

	for (int i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_3D, texture3D_IDs[i]);

		// Texture Parameters
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// Loading the data
		// gridX = width, gridZ = length, gridY = height / depth (our altitude layers)

		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
			VoxelGrid.GetGridX(), // Width (x)
			VoxelGrid.GetGridY(), // Height (y)
			VoxelGrid.GetGridZ(), // Depth (z)
			0, GL_RGBA, GL_FLOAT,
			VoxelGrid.volumeData[i].data()
		);
	}
	glBindTexture(GL_TEXTURE_3D, 0);
}

SHLM::~SHLM() {
	delete SphericalHarmonics;
	delete[] SH_OBJ_Blockers;
	delete[] SH_SKY_Blockers;
}


void SHLM::SH_renderPass(float FOVdeg, float zNear, float zFar) {

	SH_Program.Activate();
	engine->VAO_1.Bind();

	GLint blockersLoc = SH_Program.GetUniformLocation("Blockers");
	glUniform1i(blockersLoc, 0);

	GLint countLoc = SH_Program.GetUniformLocation("BlockerCount");
	glUniform1i(countLoc, BlockerCountOBJ);

	engine->registerCameraInput(FOVdeg, zNear, zFar);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[0]);
	glUniform1i(SH_Program.GetUniformLocation("V0"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[1]);
	glUniform1i(SH_Program.GetUniformLocation("V1"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[2]);
	glUniform1i(SH_Program.GetUniformLocation("V2"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[3]);
	glUniform1i(SH_Program.GetUniformLocation("V3"), 3);

	SH_Program.SetUniformVector3("WorldMin", VoxelGrid.GetWorldMin());
	SH_Program.SetUniformVector3("WorldMax", VoxelGrid.GetWorldMax());

	float LightSH[16]; for (int i = 0; i < 16; i++) LightSH[i] = 0.0f;
	AVector3 L = engine->SunCamera.Position.Normalize();
	// Evaluated direction in SH with the ZH convolution normalization constants
	//SphericalHarmonics->ComputeLogSHCoefficientsXYZR(L.x, L.y, L.z, SphericalHarmonics->ZHconv, LightSH);
	SphericalHarmonics->GetLightBasisYUp(L.x, L.y, L.z, LightSH);
	SphericalHarmonics->ApplyFunkHeckeCosine(LightSH, true);
	SH_Program.SetUniformVec4Array("LightSH", LightSH, 16);

	// Send the Light Normalized Direction for Lambertian shading
	SH_Program.SetUniformVector3("LightDir", L);

	//float ZHtoSH[4]; SphericalHarmonics->GetCombinedZHtoSH(ZHtoSH);
	//SH_Program.SetUniformVec4Array("ZHtoSH", ZHtoSH, 4);

	static bool debug_once = true;
	if (debug_once) {
		std::cout << "\n\nDEBUG SH SOFT SHADOWS \n\n";
		int ind = 0;
		for (int l = 0; l <= SH_Order; l++) {
			for (int m = -l; m <= l; m++) {
				std::cout << "Sun SH coefficient at L = " << l << ", M = " << m << " : " << LightSH[ind] << "\n";
				ind++;
			}
		}
		debug_once = false;
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, engine->window.getWidth(), engine->window.getHeight());

	// 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	engine->DrawAllInstances();
}

/*
void SHLM::shadowMaskPass(float FOVdeg, float zNear, float zFar, Engine3D* e)
{
	shadowMaskProgram.Activate(); // Masking ACTUAL shadow over geometry
	e->VAO_1.Bind();

	e->registerCameraInput(FOVdeg, zNear, zFar);
	e->SunCamera.LightMatrix(500.0f, shadowMaskProgram, true, e->UserCamera.Position);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, e->depthTextureObject.depthTexture);
	glUniform1i(e->instanceProgram.GetUniformLocation("shadowMap"), 0);

	// --- BLENDING ---
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // Draw over geometry from SH_Pass
	glDepthMask(GL_FALSE);  //

	glCullFace(GL_BACK);

	e->DrawAllInstances();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}
*/