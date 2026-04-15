#pragma once

#include "GeometryLoader.h"
#include "MultiArray.h"

class InstanceException : public std::runtime_error {
public:
	InstanceException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ INSTANCE_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {};
};

class InstanceData {
public:
	glm::mat4 matrix = glm::mat4(1.0f);
	AColor3 RGBA; // 4 bytes
	A_UV UV; // 4 bytes
	InstanceData() = default;
	//InstanceData(const AVector3& Position, const AVector3& Rotation, const AVector3& Scale, const AVector3& Center);
};

class Instance : public Transform {

	UserRef<Instance> self;

	const Blueprint* Template; // Blueprint object
	//Add more stuff, like materials and textures

	Tile* tile = nullptr; // Tile it belongs in, calculated through Position

	int handleID = -1;

	// UpToDate boolean variable to determine if
	// Transform Vectors were already calculated previously
	bool UpToDate = false;

	// This Children vector implements an instance hierarchy
	// A child must belong to ONLY ONE parent  [ Child >o---- Parent ]
	// Additional methods are introduced to make transformations apply to Children,
	// Relative to their Parent
	std::vector< UserRef<Instance> > Children;
	// Additionaly, a Parent shared pointer is introduced
	UserRef<Instance> Parent;
	// Local Position (Relative To Parent)
	AVector3 LocalPos = AVector3(0.0f, 0.0f, 0.0f);
	AVector3 LocalRot = AVector3(0.0f, 0.0f, 0.0f);
	AVector3 LocalSize = AVector3(1.0f, 1.0f, 1.0f);

	AVector3 Center = AVector3(0.0f, 0.0f, 0.0f);

	// 2 unordered_maps are needed to track Tag Names of children
	// EID (Entity IDs) are UNIQUE, they identify exactly ONE Instance
	std::unordered_map< int, UserRef<Instance> > EID_UMap;
	//                [ EID ]   [ Instance_PTR ]
	// A TagName is not unique, they can point to multiple EIDs
	std::unordered_map< std::string, std::vector<int> > Instance_UMap;
	//                  [ TagName ]   [ Instance_PTR ]

	void AddToUMap(UserRef<Instance>& parent);
	void DelFromUMap(UserRef<Instance>& parent);

	inline void Update_Cascade();

	inline void SetLocalPos();

	inline void CalculateWorldVectors();

public:

	inline void Update();

	// --------- CONSTRUCTORS (requires a Blueprint first)

	Instance();
	Instance(const Blueprint* _Template, Scene* scene, const std::string& TagName);
	Instance(const Blueprint* _Template, Scene* scene);

	// --- DESTRUCTOR + DESTROY METHOD

	~Instance() = default;
	void Destroy();

	// --------- TRANSFORM METHODS

	inline void SetPosition(const AVector3& _Position);
	inline void SetRotation(const AVector3& _Rotation);
	inline void SetSize(const AVector3& _Size);
	inline void SetColor(int R, int G, int B, int A);
	inline void SetColor(AColor3 _Color);
	inline void SetTile(Tile* _tile);
	
	void LookAt(AVector3 from, AVector3 to);
	void SetDirection(AVector3 Direction);

	inline void SetDataMatrix(Ref<InstanceData>& data);
	inline void SetDataColor(Ref<InstanceData>& data);
	inline void SetDataMatrix(InstanceData& data);
	inline void SetDataColor(InstanceData& data);

	// ---------- SET PARENT

	void SetParent(UserRef<Instance>& _Parent);

	// ---------- ADD + REMOVE CHILD

	void AddChild(UserRef<Instance>& Ins);
	void RemoveChild(UserRef<Instance>& Ins);

	// ---------- GETTERS

	int GetBlueprintID();

	inline AVector3 GetPosition();
	inline AVector3 GetRotation();
	inline AVector3 GetSize();

	// ----------- GET CHILD METHODS

	UserRef<Instance> FirstChild(const std::string& TagName) const;
	UserRef<Instance> FirstChild(int EntityID) const;

	std::vector< UserRef<Instance> >& GetChildren();

	std::vector< UserRef<Instance> > AllChildrenWith(const std::string& TagName);

	void GetDescendants(std::vector< UserRef<Instance> >& container);

	// ------------- GET PARENT METHODS

	UserRef<Instance>& GetParent();

	void GetAscendants(std::vector< UserRef<Instance> >& container);

	// ------------- FRIENDS (FROM SCENE CLASS)

	friend Scene;

};


void Instance::Update_Cascade() {
	if (!UpToDate) {
		this->CalculateWorldVectors();
		this->Update();
	}
	if (Children.empty()) return;
	for (auto& ins_uptr : Children) {
		ins_uptr->Update_Cascade();
	}
}

void Instance::SetLocalPos() {

	if (Parent.getRef() == nullptr) {
		LocalPos = Position;
		return;
	}

	AVector3 relVec3 = (this->Position - Parent->Position);

	AVector3 ParentRot = Parent->Rotation * (-1);

	relVec3.Rotate_InPlace(ParentRot);

	relVec3.x /= Parent->Size.x;
	relVec3.y /= Parent->Size.y;
	relVec3.z /= Parent->Size.z;

	LocalPos = relVec3;
}

void Instance::CalculateWorldVectors() {
	// This function calculates the World-Space Vectors needed for rendering from ascendants
	// 
	// Through functions like SetPosition(), SetRotation(), SetSize()
	// Users can modify an Instance with all descendants moved relative to it,
	// Moving from child -> parent
	// 
	// At first, World-Space Vectors are set to the Local-Space Vectors
	// Then, all Local-Space Vectors from their parents are added (and rotated accordingly)

	// Reset Position & Rotation & Scale
	this->Position = this->LocalPos;
	this->Rotation = this->LocalRot;
	this->Size = this->LocalSize;
	// Add Parent LocalVectors
	// Iterate through all ascendents
	while (Parent.getRef() != nullptr) {
		if (Parent->UpToDate) return;
		// Scale first according to Parent Size
		this->Position = this->Position * Parent->LocalSize;
		// Rotate according to Parent Rotation (Local) before adding the local vectors
		this->Position.Rotate_InPlace(Parent->Rotation);
		// Add the local Rotation from the paren
		this->Rotation += Parent->LocalRot;
		// Add the local Position from the parent
		this->Position += Parent->LocalPos;
		// Multiply Size with local Size
		this->Size = this->Size * Parent->LocalSize;
		// Advance upwards in the instance hierrarchy
		Parent->UpToDate = true;
		Parent = Parent->Parent;
	}
	UpToDate = true;
}

void Instance::SetPosition(const AVector3& _Position) {
	Position = _Position;
	//SetLocalPos();
	UpToDate = false;
	//Update();
	// Update children recursively
	//Update_Cascade();
}
void Instance::SetRotation(const AVector3& _Rotation) {
	Rotation = _Rotation;
	//LocalRot = _Rotation;
	//SetLocalPos();
	UpToDate = false;
	//Update();
	// Update children recursively
	//Update_Cascade();
}
void Instance::SetSize(const AVector3& _Size) {
	/*
	if (Parent.getInternalPtr() == nullptr) return;
	const AVector3& P_size = Parent->Size; // (World-Space)
	LocalSize = AVector3(_Size.x / P_size.x, _Size.y / P_size.y, _Size.z / P_size.z);
	SetLocalPos();
	*/
	Size = _Size;
	UpToDate = false;
	//Update();
	// Update children recursively
	//Update_Cascade();
}
void Instance::SetColor(AColor3 _Color) {
	Color = _Color;
	UpToDate = false;
	//Update();
}
void Instance::SetColor(int R, int G, int B, int A) {
	Color = AColor3(R, G, B, A);
	UpToDate = false;
	//Update();
}
void Instance::SetTile(Tile* _tile) {
	tile = _tile;
}

void Instance::SetDataMatrix(Ref<InstanceData>& data) {

	auto& matrix = data->matrix;

	matrix = glm::translate(glm::mat4(1.0f), (glm::vec3)Position);
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
	matrix = glm::scale(matrix, (glm::vec3)Size);
	matrix = glm::translate(matrix, -(glm::vec3)Center);
}
void Instance::SetDataColor(Ref<InstanceData>& data) {

	data->RGBA = Color;
}

void Instance::SetDataMatrix(InstanceData& data) {

	auto& matrix = data.matrix;

	matrix = glm::translate(glm::mat4(1.0f), (glm::vec3)Position);
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
	matrix = glm::scale(matrix, (glm::vec3)Size);
	matrix = glm::translate(matrix, -(glm::vec3)Center);
}
void Instance::SetDataColor(InstanceData& data) {

	data.RGBA = Color;
}

/*    TODO
class UniqueGeom : public Transform {
	// Geometry remains unchanged, stored directly as geometry
};

class StaticGeom : public Entity {
	// Un-modify-able
};
*/
