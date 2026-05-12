#pragma once

#include "SH.h"
#include "GeometryBasics.h"

template <int num, int MaxL>
class SHVoxelGrid {

	const int gridX;
	const int gridY;
	const int gridZ;

	const AVector3 WorldMin;
	const AVector3 WorldMax;

	const AVector3 WorldDelta;

	// Weighted shadow functions for soft shadows
	/*
	const float w[16] = {
		1.0f,                                     // L0
		0.95f, 0.95f, 0.95f,                      // L1
		0.75f, 0.75f, 0.75f, 0.75f, 0.75f,        // L2
		0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.50f // L3
	};
	*/

public:

	// We generate 4 3D_TEXTURES (RGBA32F)
	std::vector<float> volumeData[4];
	std::vector<int> voxelizedBlockers;

	SHVoxelGrid(AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax) :
		gridX((int)VoxelResolution.x), gridY((int)VoxelResolution.y), gridZ((int)VoxelResolution.z),
		WorldMin(WorldMin), WorldMax(WorldMax), WorldDelta(WorldMax - WorldMin)
	{
		for (int i = 0; i < 4; i++) volumeData[i].resize(gridX * gridY * gridZ * 4);
		voxelizedBlockers.resize(gridX * gridY * gridZ);
		std::fill(voxelizedBlockers.begin(), voxelizedBlockers.end(), -1);
	}

	int GetGridX() { return gridX; }
	int GetGridY() { return gridY; }
	int GetGridZ() { return gridZ; }

	AVector3 GetWorldMin() {
		return WorldMin;
	}
	AVector3 GetWorldMax() {
		return WorldMax;
	}

	void ComputeSHEXP(float* f) {
		float f0 = f[0];
		// f0 is DC component of LogSH
		// Multiply it with Y<0,0> (0.282)
		float g0 = exp(f0 * 0.282095f);

		// DC Component of linear visibility vector:
		f[0] = g0 * 3.544907f; // sqrt(4pi)

		for (int i = 1; i < 16; i++) {
			// Scale Visibility on higher directions (L>0) with average g0
			f[i] = f[i] * g0;
		}
	}

	void AccumulateLogSH_AtPoint(const float Px, const float Py, const float Pz,
		const BlockerXYZR<num>& b, const int BlockerCount, float* LogSH, const SH<MaxL>& s, const int blockerIdx) const {

		int insID = voxelizedBlockers[blockerIdx];

		for (int i = 0; i < BlockerCount; i++) {

			if (insID != -1 && b.insID[i] == insID) {
				continue;
			}

			// Relative Vector
			float dx = b.x[i] - Px;
			float dy = b.y[i] - Py;
			float dz = b.z[i] - Pz;
			float d2 = dx * dx + dy * dy + dz * dz;

			float d = sqrt(d2);

			float D = std::max(d, b.r[i] * 1.05f);

			float sinAlpha = b.r[i] / D;
			float cosAlpha = sqrt(1.0f - sinAlpha * sinAlpha);
			
			float ZHcoeff[4];
			s.ComputeZHVisibilityXYZR_LogEpsilon(cosAlpha, sinAlpha, ZHcoeff);

			// Normalized direction to sphere
			float nx = dx / d;
			float ny = dy / d;
			float nz = dz / d;

			s.ComputeLogSHCoefficientsXYZR(nx, ny, nz, ZHcoeff, LogSH);
		}
	}

	int worldToGrid(float val, float minW, float maxW, int gridRes) {
		float t = (val - minW) / (maxW - minW);
		return std::clamp((int)(t * (gridRes - 1)), 0, gridRes - 1);
	};

	void VoxelizeBlockers(const BlockerXYZR<num>& b, int count) {

		const float avrgGridResolution = 
			(WorldDelta.x / gridX) * 0.5f + 
			(WorldDelta.z / gridZ) * 0.5f;
		const float biasGrid = avrgGridResolution * 0.125f;

		int nrThreads = std::thread::hardware_concurrency();
		if (nrThreads == 0) nrThreads = 2;

		int layersPerThread = (count + nrThreads - 1) / nrThreads;

		std::vector<std::future<void>> F;

		for (int t = 0; t < nrThreads; t++) {

			int startI = t * layersPerThread;
			int endI = std::min(startI + layersPerThread, count);

			if (startI >= endI) break;

			F.push_back(std::async(std::launch::async, [this, &b, count, startI, endI, biasGrid]() {

				for (int i = startI; i < endI; i++) {
					float br = b.r[i];
					float bx = b.x[i];
					float by = b.y[i];
					float bz = b.z[i];

					float R = (br + biasGrid) * (br + biasGrid);

					int startX = worldToGrid(bx - br, WorldMin.x, WorldMax.x, gridX);
					int endX = worldToGrid(bx + br, WorldMin.x, WorldMax.x, gridX);
					int startY = worldToGrid(by - br, WorldMin.y, WorldMax.y, gridY);
					int endY = worldToGrid(by + br, WorldMin.y, WorldMax.y, gridY);
					int startZ = worldToGrid(bz - br, WorldMin.z, WorldMax.z, gridZ);
					int endZ = worldToGrid(bz + br, WorldMin.z, WorldMax.z, gridZ);


					for (int y = startY; y <= endY; y++) {
						float py = WorldMin.y + (float(y) / (float)(gridY - 1)) * WorldDelta.y;
						for (int z = startZ; z <= endZ; z++) {
							float pz = WorldMin.z + (float(z) / (float)(gridZ - 1)) * WorldDelta.z;
							for (int x = startX; x <= endX; x++) {
								float px = WorldMin.x + (float(x) / (float)(gridX - 1)) * WorldDelta.x;

								float dx = px - bx;
								float dy = py - by;
								float dz = pz - bz;
								if ((dx * dx + dy * dy + dz * dz) < R) {
									int vIdx = (z * gridY * gridX + y * gridX + x);
									voxelizedBlockers[vIdx] = b.insID[i];
								}
							}
						}
					}
				}
			}));
		}
		for (auto& f : F) {
			f.get();
		}
	}

	void Bake3DGrid(const BlockerXYZR<num> & b, int count, const SH<MaxL>& s) {
		int nrThreads = std::thread::hardware_concurrency();
		if (nrThreads == 0) nrThreads = 2;

		int layersPerThread = (gridY + nrThreads - 1) / nrThreads;

		std::vector<std::future<void>> F;

		for (int t = 0; t < nrThreads; t++) {

			int startY = t * layersPerThread;
			int endY = std::min(startY + layersPerThread, gridY);

			if (startY >= endY) break;

			F.push_back(std::async(std::launch::async, [this, &b, count, &s, startY, endY]() {

			for (int y = startY; y < endY; y++) {
				// Y-Layer height
				float py = WorldMin.y + (float(y) / (float)(gridY - 1)) * WorldDelta.y;

				for (int z = 0; z < gridZ; z++) {
					float pz = WorldMin.z + (float(z) / (float)(gridZ - 1)) * WorldDelta.z;

					for (int x = 0; x < gridX; x++) {
						float px = WorldMin.x + (float(x) / (float)(gridX - 1)) * WorldDelta.x;

						float voxelSH[16];
						for (int i = 0; i < 16; i++) voxelSH[i] = 0.0f;
						int blockerIdx = (z * gridY * gridX + y * gridX + x);

						this->AccumulateLogSH_AtPoint(px, py, pz, b, count, voxelSH, s, blockerIdx);

						this->ComputeSHEXP(voxelSH);

						//for (int i = 0; i < 16; i++) {
						//	voxelSH[i] *= w[i];
						//}

						int vIdx = (z * gridY * gridX + y * gridX + x) * 4;
						for (int t = 0; t < 4; t++) {
							volumeData[t][vIdx + 0] = voxelSH[t * 4 + 0];
							volumeData[t][vIdx + 1] = voxelSH[t * 4 + 1];
							volumeData[t][vIdx + 2] = voxelSH[t * 4 + 2];
							volumeData[t][vIdx + 3] = voxelSH[t * 4 + 3];
						}
					}
				}
			}
			}));
		}

		for (auto& f : F) {
			f.get();
		}
	}

	std::pair<AVector3, float> CalculateOptimalBaseSphere(const std::vector<AVertex>& vertices) {
		AVector3 center(0.0f, 0.0f, 0.0f);
		for (const auto& v : vertices) {
			center.x += v.x;
			center.y += v.y;
			center.z += v.z;
		}
		center.x /= vertices.size();
		center.y /= vertices.size();
		center.z /= vertices.size();

		float maxDistSq = 0;
		float avgDist = 0;

		for (const auto& v : vertices) {
			float dx = v.x - center.x;
			float dy = v.y - center.y;
			float dz = v.z - center.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

			avgDist += dist;
			if (dist > maxDistSq) maxDistSq = dist;
		}

		float rMax = std::sqrt(maxDistSq);
		float rAvg = avgDist / vertices.size();

		return { center, (rMax + rAvg) * 0.5f };
	}

	struct Triangle { AVector3 v0, v1, v2; };

	// Fast Ray-Trace (for Y axis) over Triangle
	bool RayTriangleIntersectY(float rx, float rz, const Triangle& tri, float& outY) {
		// 2D Projection to verify if (rx, rz) is inside Triangle in XZ plane
		float det = (tri.v1.z - tri.v2.z) * (tri.v0.x - tri.v2.x) + (tri.v2.x - tri.v1.x) * (tri.v0.z - tri.v2.z);
		float l1 = ((tri.v1.z - tri.v2.z) * (rx - tri.v2.x) + (tri.v2.x - tri.v1.x) * (rz - tri.v2.z)) / det;
		float l2 = ((tri.v2.z - tri.v0.z) * (rx - tri.v2.x) + (tri.v0.x - tri.v2.x) * (rz - tri.v2.z)) / det;
		float l3 = 1.0f - l1 - l2;

		if (l1 >= 0 && l2 >= 0 && l3 >= 0) {
			// Ray Intersected, calculate Y
			outY = l1 * tri.v0.y + l2 * tri.v1.y + l3 * tri.v2.y;
			return true;
		}
		return false;
	}

	void VoxelizeMesh(const std::vector<AVertex>& vert, const std::vector<GLuint>& ind, int gridX, int gridY, int gridZ, int meshID) {

		int nrThreads = std::thread::hardware_concurrency();
		int rowsPerThread = (gridX + nrThreads - 1) / nrThreads;
		std::vector<std::future<void>> futures;

		for (int t = 0; t < nrThreads; t++) {
			int startX = t * rowsPerThread;
			int endX = std::min(startX + rowsPerThread, gridX);

			futures.push_back(std::async(std::launch::async, [this, &vert, &ind, startX, endX, gridX, gridY, gridZ, meshID]() {
				for (int x = startX; x < endX; x++) {
					float px = WorldMin.x + (float(x) / (gridX - 1)) * WorldDelta.x;

					for (int z = 0; z < gridZ; z++) {
						float pz = WorldMin.z + (float(z) / (gridZ - 1)) * WorldDelta.z;

						// Find all intersections over Y axis
						std::vector<float> intersections;
						for (int i = 0; i < (int)ind.size(); i++) {
							if (i % 3 == 2) {
								// Found 3 verticies that form a triangle
								Triangle tri = { vert[ind[i - 2]], vert[ind[i - 1]], vert[ind[i]] };
								
								float hitY;
								if (RayTriangleIntersectY(px, pz, tri, hitY)) {
									intersections.push_back(hitY);
								}
							}
						}

						// Sort intersections up -> down
						std::sort(intersections.begin(), intersections.end());

						// Fill voxels using the parity rule
						for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
							float yStart = intersections[i];
							float yEnd = intersections[i + 1];

							int gridYStart = std::clamp((int)((yStart - WorldMin.y) / WorldDelta.y * (gridY - 1)), 0, gridY - 1);
							int gridYEnd = std::clamp((int)((yEnd - WorldMin.y) / WorldDelta.y * (gridY - 1)), 0, gridY - 1);

							for (int y = gridYStart; y <= gridYEnd; y++) {
								int vIdx = (z * gridY * gridX + y * gridX + x);
								voxelizedBlockers[vIdx] = meshID; // mesh ID
							}
						}
					}
				}
				}));
		}
		for (auto& f : futures) f.get();
	}

};