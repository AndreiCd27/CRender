#pragma once


#include "precompile.h"
#include "shaderClass.h"
#include "ComputeShader.h"
#include "SH.h"
#include "SH_VoxelGrid.h"

#include "Engine3D.h"

// LightingService

class LightingService {
protected:
	Engine3D* engine = nullptr;
	EngineConfig* config = nullptr;
public:
	LightingService(Engine3D* engine, EngineConfig* config) : engine(engine), config(config) {}
};


#define MAX_BLOCKER_COUNT 65536
#define SH_Order 3

// Spherical Harmonic Lighting Model (SHLM)

class SHLM : public LightingService {
	// For shadow rendering using Spherical Harmonics (see SHEXP.h)

	SH<SH_Order>* SphericalHarmonics = nullptr;
	BlockerXYZR<MAX_BLOCKER_COUNT>* SH_OBJ_Blockers = nullptr;
	Blockers<MAX_BLOCKER_COUNT>* SH_SKY_Blockers = nullptr;
	int BlockerCountOBJ = 0;
	int BlockerCountSKY = 0;

	SHVoxelGrid<MAX_BLOCKER_COUNT, SH_Order> VoxelGrid;

	Shader SH_Program;

	Shader shadowMaskProgram;

	GLuint texture3D_IDs[4];

	ComputeShader VoxelizeMeshes, BakeVoxels;
	GLuint idxTrigBuffer, idxVoxelInsIDs, idxBlockerBuffer;

	void GenTextures_Cubemap();

public:

	struct GPU_Blocker {
		glm::vec4 pos_rad;
		glm::ivec4 instanceID;
	};

	struct GPU_Trig {
		glm::vec4 v0, v1, v2;
		GPU_Trig(const AVector3& V0, const AVector3& V1, const AVector3& V2, const int InsID)
			: v0(V0.x, V0.y, V0.z, (float)InsID), v1(V1.x, V1.y, V1.z, 0.0f), v2(V2.x, V2.y, V2.z, 0.0f)
		{
		}
	};

	// SPHERICAL HARMONICS RENDERING //////////////////////////////////////////////////

	SHLM(Engine3D* engine, EngineConfig* config, AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax);

	void BindToEngine(float FOVdeg, float zNear, float zFar);

	void SetBlockerSKY(float phi, float theta, float radius, float dist);
	void SetBlockerOBJ(float x, float y, float z, float radius, int insID);

	//

	void Load_Cubemap_MultiThreaded();
	// FASTER
	void Load_Cubemap_GPU_ComputeShader();

	void Upload_Cubemap();

	////////////////////////////////////////////////////////////////////////////////////


	void shadowMaskPass(float FOVdeg, float zNear, float zFar);

	void SH_renderPass(float FOVdeg, float zNear, float zFar);

	~SHLM();

};