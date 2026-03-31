#pragma once


#include "precompile.h"

// Members are public beacause we don't want to get access errors,
// When OpenGL copies and transfers the data to VRAM
// These public variables should be modified via methods

class A_UV {
public:
	uint32_t UV = 0;
	A_UV() = default;
	A_UV(float U, float V);
};

class AColor3 {
public:
	// Actually stores in ABGR format (for Little-Endian)
	// Use Constructor functions
	uint32_t RGBA = 0;
	AColor3() = default;
	AColor3(int R, int G, int B, int A);
};

// Could have used glm
class AVector3 {
public:
	float x = 0.0f, y = 0.0f, z = 0.0f;
	AVector3() = default;
	AVector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};
	~AVector3() = default;
	AVector3 operator+(const AVector3& dr);
	AVector3 operator-(const AVector3& dr);
	AVector3 operator*(const AVector3& dr);
	AVector3 operator*(const float& scalar);
	AVector3& operator+=(const AVector3& dr);
	AVector3& operator-=(const AVector3& dr);
	AVector3 operator^(const AVector3& dr); // Used for cross product
	AVector3 Normalize();
	void Normalize_InPlace();
	AVector3 Rotate(AVector3& ROT);
	void Rotate_InPlace(AVector3& ROT);

	operator glm::vec3() const; // Converts easily to glm::vec3

	void DEBUG_Print() const;
};

// AVertex SHOULD be 32 BYTES!
// With current features, there is no need for more complex vertex data structures
class AVertex {
public:
	//--------------- 0 bytes
	AVector3 POS;
	AColor3 RGBA;
	//--------------- 16 bytes
	AVector3 NORMAL;
	A_UV UV;                         // TODO: Textures
	//--------------- 32 bytes


	AVertex() = default;
	AVertex(float x, float y, float z);
	AVertex(float x, float y, float z, int R, int G, int B, int A);
	AVertex(AVector3 _POS, AVector3 _NORMAL, int R, int G, int B, int A, float U, float V);
};
