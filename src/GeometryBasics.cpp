
#include "GeometryBasics.h"

AColor3::AColor3(int R, int G, int B, int A) {
	RGBA = ((A & 255) << 24) | ((B & 255) << 16) | ((G & 255) << 8) | (R & 255);
	// | Red 0-255 | Green 0-255 | Blue 0-255 | Aplha 0-255 |
	//    4 bytes  +   4 bytes   +   4 bytes  +   4 bytes
}
const float uvConst = 65536.0f;
A_UV::A_UV(float U, float V) {
	uint16_t _U = (uint16_t)(U * uvConst);
	uint16_t _V = (uint16_t)(V * uvConst);
	UV = ((uint32_t)_U << 16) | _V;
}


AVector3 AVector3::operator+(const AVector3& dr) {
	return { dr.x + this->x, dr.y + this->y, dr.z + this->z };
}
AVector3& AVector3::operator+=(const AVector3& dr) {
	this->x += dr.x; this->y += dr.y; this->z += dr.z;
	return *this;
}
AVector3 AVector3::operator-(const AVector3& dr) {
	return { this->x - dr.x, this->y - dr.y, this->z - dr.z };
}
AVector3& AVector3::operator-=(const AVector3& dr) {
	this->x -= dr.x; this->y -= dr.y; this->z -= dr.z;
	return *this;
}
AVector3 AVector3::operator*(const AVector3& dr) {
	return { dr.x * this->x, dr.y * this->y, dr.z * this->z };
}
AVector3 AVector3::operator*(const float& scalar) {
	return { scalar * this->x, scalar * this->y, scalar * this->z };
}
AVector3 AVector3::operator^(const AVector3& dr) { //CROSS PRODUCT
	return {
		this->y * dr.z - this->z * dr.y,
		this->z * dr.x - this->x * dr.z,
		this->x * dr.y - this->y * dr.x
	};
}
AVector3 AVector3::Normalize() {
	float dist = std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	return { this->x / dist, this->y / dist, this->z / dist };
}
void AVector3::Normalize_InPlace() {
	float dist = std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	this->x /= dist; this->y /= dist; this->z /= dist;
}
AVector3 AVector3::Rotate(AVector3& ROT) {
	glm::vec3 t = (glm::vec3)(*this);

	glm::quat quat = glm::quat(glm::radians((glm::vec3)ROT));

	t = quat * t;

	return AVector3(t.x, t.y, t.z);
}
void AVector3::Rotate_InPlace(AVector3& ROT) {
	glm::vec3 t = (glm::vec3)(*this);
	/*
	glm::mat4 rotMat = glm::mat4(1.0f);
	rotMat = glm::rotate(rotMat, glm::radians(ROT.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotMat = glm::rotate(rotMat, glm::radians(ROT.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotMat = glm::rotate(rotMat, glm::radians(ROT.z), glm::vec3(0.0f, 0.0f, 1.0f));
	*/
	//t = glm::vec3(rotMat * glm::vec4(t, 1.0f));
	// Quaternion for Rotation (easier computation)
	glm::quat quat = glm::quat(glm::radians((glm::vec3)ROT));

	t = quat * t;

	this->x = t.x;
	this->y = t.y;
	this->z = t.z;
}

AVector3::operator glm::vec3() const {
	return glm::vec3(x, y, z);
}

void AVector3::DEBUG_Print() const {
	std::cout << "(" << this->x << ", " << this->y << ", " << this->z << "), ";
}


AVertex::AVertex(float x, float y, float z) : POS(x,y,z) {
	NORMAL = AVector3(0.0f, 1.0f, 0.0f);
}
AVertex::AVertex(float x, float y, float z, int R, int G, int B, int A) : POS(x,y,z) {
	NORMAL = AVector3(0.0f, 1.0f, 0.0f);
	RGBA = AColor3(R, G, B, A);
}
AVertex::AVertex(AVector3 _POS, AVector3 _NORMAL, int R, int G, int B, int A, float U, float V) : 
	POS(_POS), RGBA(R, G, B, A), NORMAL(_NORMAL), UV(U, V) {}