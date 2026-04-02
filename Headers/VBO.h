#pragma once

#include <glad/glad.h>

#include "GeometryBasics.h"

class VBO {
private:
	bool SetupComplete = false;
	GLuint ID;
	size_t Capacity = 0;
public:

	VBO() { 
		//std::cout << "C -> VBO \n"; 
	};

	void Setup(std::vector<AVertex>& worldVertices, GLsizeiptr size, const int drawStyle);
	void Setup(const AVertex* PTR, GLsizeiptr size, const int drawStyle);
	void Bind();
	void Unbind();
	void Delete();

	inline bool GetCompleteStatus() { return SetupComplete; }
};