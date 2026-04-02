#pragma once


#include "precompile.h"

#include "GeometryBasics.h"

class Blueprint;
class Tile;
class Instance;
class Scene;

class BlueprintException : public std::runtime_error {
public:
	BlueprintException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {};
};

class Blueprint {
	static const int safeBlueprintCount; // Throws an exception if we go beyond 4095 blueprints
	// If any modifications are to be made, consider looking into the shiftComponent variable
	// In the Tile class and modify it such that no conflicts arise
	// At the moment, the shift component is calculated as follows: (int) log2l(Blueprint::safeBlueprintCount) + 1;

	static int contor; // Used to set templateID, simple incremented variable, starting at 0

	// Stores the vertices and indicies of the blueprint, any instance of this blueprint
	// Has this same geometry, but with different Position, Rotation and Scale
	int VerticesHandleID;  // FROM templateID TO vertices_buffer in Scene
	int IndiciesHandleID;  // FROM templateID TO indicies_buffer in Scene
	// Toghether, they form the geometry that define this object

	int templateID; // TemplateID is used in combination with TileID to form a HandleID
	// HandleID will be the key to access data shared between tiles and blueprints 
	// (like transformation matricies)
	// Only 4095 Blueprints may be created, the rest 20 bits are reserved for tileIDs
	// A simple OR (|) operation can generate the key we need for Blueprint A in Tile B 
	// ( HandleID = A | ( B << 12 ) )

	const char* Alias;

	Blueprint();

public:

	int GetID();

	AVertex Center;
	static int GetShiftComponent();

	void SetColor(int R, int G, int B, int A);

	static void CalculateSurfaceNormals(std::vector<AVertex>& worldVert, std::vector<GLuint>& indicies);

	friend class Scene;
};

// ABSTRACT CLASS ENTITY
class Entity {
	static int contor;
	int EID;
	std::string TagName = "";
	
protected:
	Scene* ParentScene = nullptr;
public:
	Entity(Scene* scene, const std::string& _TagName);
	virtual ~Entity() = default;

	const std::string& GetTag() const;
	int GetEID() const;

	virtual void Update() = 0;
};

// ABSTRACT CLASS TRANSFORM
class Transform : public Entity {
	void Defaults();
protected:
	AVector3 Rotation = AVector3(0.0f, 0.0f, 0.0f); // Rotation in degrees for X,Y,Z axis
	AVector3 Size = AVector3(1.0f, 1.0f, 1.0f); // Scale Vector for X,Y,Z axis
	AVector3 Position = AVector3(0.0f, 0.0f, 0.0f); // Translate Vector for X,Y,Z axis
	AColor3 Color;
public:
	explicit Transform(Scene* scene);
	Transform(Scene* scene, const std::string& TagName);
	virtual ~Transform() = default;
};
