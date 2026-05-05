#pragma once

#include "SH.h"
#include "GeometryBasics.h"

template <int num, int MaxL>
class SHVoxelGrid {

	const int gridX = 256;
	const int gridY = 32;
	const int gridZ = 256;

	const float worldMinX, worldMinY, worldMinZ;
	const float worldMaxX, worldMaxY, worldMaxZ;

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

	SHVoxelGrid(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) :
		worldMinX(minX), worldMinY(minY), worldMinZ(minZ), 
		worldMaxX(maxX), worldMaxY(maxY), worldMaxZ(maxZ) 
	{
		for (int i = 0; i < 4; i++) volumeData[i].resize(gridX * gridY * gridZ * 4);
		voxelizedBlockers.resize(gridX * gridY * gridZ);
		std::fill(voxelizedBlockers.begin(), voxelizedBlockers.end(), -1);
	}

	int GetGridX() { return gridX; }
	int GetGridY() { return gridY; }
	int GetGridZ() { return gridZ; }

	AVector3 GetWorldMin() {
		return AVector3(worldMinX, worldMinY, worldMinZ);
	}
	AVector3 GetWorldMax() {
		return AVector3(worldMaxX, worldMaxY, worldMaxZ);
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
			((worldMaxX - worldMinX) / gridX) * 0.5f + 
			((worldMaxZ - worldMinZ) / gridZ) * 0.5f;
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

					int startX = worldToGrid(bx - br, worldMinX, worldMaxX, gridX);
					int endX = worldToGrid(bx + br, worldMinX, worldMaxX, gridX);
					int startY = worldToGrid(by - br, worldMinY, worldMaxY, gridY);
					int endY = worldToGrid(by + br, worldMinY, worldMaxY, gridY);
					int startZ = worldToGrid(bz - br, worldMinZ, worldMaxZ, gridZ);
					int endZ = worldToGrid(bz + br, worldMinZ, worldMaxZ, gridZ);

					for (int y = startY; y <= endY; y++) {
						float py = worldMinY + (float(y) / (float)(gridY - 1)) * (worldMaxY - worldMinY);
						for (int z = startZ; z <= endZ; z++) {
							float pz = worldMinZ + (float(z) / (float)(gridZ - 1)) * (worldMaxZ - worldMinZ);
							for (int x = startX; x <= endX; x++) {
								float px = worldMinX + (float(x) / (float)(gridX - 1)) * (worldMaxX - worldMinX);

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
				float py = worldMinY + (float(y) / (float)(gridY - 1)) * (worldMaxY - worldMinY);

				for (int z = 0; z < gridZ; z++) {
					float pz = worldMinZ + (float(z) / (float)(gridZ - 1)) * (worldMaxZ - worldMinZ);

					for (int x = 0; x < gridX; x++) {
						float px = worldMinX + (float(x) / (float)(gridX - 1)) * (worldMaxX - worldMinX);

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
					float px = worldMinX + (float(x) / (gridX - 1)) * (worldMaxX - worldMinX);

					for (int z = 0; z < gridZ; z++) {
						float pz = worldMinZ + (float(z) / (gridZ - 1)) * (worldMaxZ - worldMinZ);

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

							int gridYStart = std::clamp((int)((yStart - worldMinY) / (worldMaxY - worldMinY) * (gridY - 1)), 0, gridY - 1);
							int gridYEnd = std::clamp((int)((yEnd - worldMinY) / (worldMaxY - worldMinY) * (gridY - 1)), 0, gridY - 1);

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