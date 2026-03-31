#pragma once


#include "precompile.h"

#include "GeometryBasics.h"
#include "MultiArray.h"
#include "Camera.h"
#include "VBO.h"
#include "EBO.h"

#define TILE_MAXSCALE_FACTOR 14
#define MAX_TILE_LEVEL 14
#define START_TILE_LEVEL 10

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
	Entity(Scene* scene, std::string _TagName);
	virtual ~Entity() = default;

	const std::string& GetTag() const;
	const int GetEID() const;

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
	Transform(Scene* scene);
	Transform(Scene* scene, std::string TagName);
	virtual ~Transform() = default;
};

class InstanceData {
private:
	glm::mat4 matrix = glm::mat4(1.0f);
	AColor3 RGBA; // 4 bytes
	A_UV UV; // 4 bytes
public:
	InstanceData() = default;
	InstanceData(AVector3 Position, AVector3 Rotation, AVector3 Scale);
	void SetMatrix(AVector3 Position, AVector3 Rotation, AVector3 Scale);
	void SetColor(AColor3 _RGBA);
};

class Instance : public Transform, public std::enable_shared_from_this<Instance> {

	static int C_INS;
	static int D_INS;

	Blueprint* Template = nullptr; // Blueprint object
	//Add more stuff, like materials and textures
	Tile* tile = nullptr; // Tile it belongs in, calculated through Position
	int handleOffset = 0; 
	// The offset calculated from the coresponding HandleID

	// This Children vector implements an instance hierarchy
	// A child must belong to ONLY ONE parent  [ Child >o---- Parent ]
	// Additional methods are introduced to make transformations apply to Children,
	// Relative to their Parent
	std::vector< std::shared_ptr<Instance> > Children;
	// Additionaly, a Parent shared pointer is introduced
	std::weak_ptr<Instance> Parent;
	// Local Position (Relative To Parent)
	AVector3 LocalPos = AVector3(0.0f, 0.0f, 0.0f);
	AVector3 LocalRot = AVector3(0.0f, 0.0f, 0.0f);
	AVector3 LocalSize = AVector3(1.0f, 1.0f, 1.0f);

	void Update() override;
	void Update_Cascade();

	void SetLocalPos();

	void CalculateWorldVectors();

public:

	// --------- CONSTRUCTORS (requires a Blueprint first)

	Instance(Blueprint* _Template, Scene* scene, std::string TagName);
	Instance(Blueprint* _Template, Scene* scene);

	// --- DESTRUCTOR + DESTROY METHOD

	~Instance() = default;
	void Destroy();

	// --------- TRANSFORM METHODS

	void SetPosition(AVector3 _Position);
	void SetRotation(AVector3 _Rotation);
	void SetSize(AVector3 _Size);
	void SetColor(int R, int G, int B, int A);
	void SetColor(AColor3 _Color);
	void SetTile(Tile* _tile);

	AVector3 GetLocalPos();

	// ---------- SET PARENT

	void SetParent(std::weak_ptr<Instance> _Parent);

	// ---------- ADD + REMOVE CHILD

	void AddChild(std::shared_ptr<Instance> Ins);
	void RemoveChild(std::shared_ptr<Instance> Ins);

	// ---------- GETTERS

	int GetBlueprintID();

	InstanceData* GetInstanceData();

	AVector3 GetPosition();
	AVector3 GetRotation();
	AVector3 GetSize();

	// ----------- INDEXING RELATIVE TO A HANDLE 
	// (See MultiArray.h & InstanceArrayOrganizer in Scene class)

	void SetHandleOffset(int offset);

	// ------------ DEBUG

	static void DEBUG_print_CDcount();

	// ------------- FRIENDS (FROM SCENE CLASS)

	friend Scene;
	
};

class UniqueGeom : public Transform {
	// Geometry remains unchanged, stored directly as geometry
};

class StaticGeom : public Entity {

};
