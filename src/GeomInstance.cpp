
#include "GeomInstance.h"


int Instance::C_INS = 0;
int Instance::D_INS = 0;

void Instance::DEBUG_print_CDcount() {
	std::cout << C_INS << " " << D_INS << "\n";
}

Instance::Instance(Blueprint* Template, Scene* scene) : Transform(scene, "Instance") {
	C_INS++;
	this->Template = Template;

	EID_UMap[GetEID()] = shared_from_this();
	Instance_UMap[GetTag()].push_back(GetEID());
}
Instance::Instance(Blueprint* Template, Scene* scene, std::string TagName) : Transform(scene, TagName) {
	C_INS++;
	this->Template = Template;
}

void Instance::AddToUMap(std::shared_ptr<Instance> parent) {
	parent->EID_UMap[GetEID()] = shared_from_this();
	parent->Instance_UMap[GetTag()].push_back(GetEID());
}
void Instance::DelFromUMap(std::shared_ptr<Instance> parent) {
	if (parent->EID_UMap.find(GetEID()) != parent->EID_UMap.end()) parent->EID_UMap.erase(GetEID());

	auto& vectFromTag = parent->Instance_UMap[GetTag()];
	auto iter = std::find(vectFromTag.begin(), vectFromTag.end(), GetEID());
	if (iter != vectFromTag.end()) {
		parent->Instance_UMap[GetTag()].erase(iter);
	}
}

void Instance::Update_Cascade() {
	this->CalculateWorldVectors();
	this->Update();
	if (Children.empty()) return;
	for (auto& ins_uptr : Children) {
		ins_uptr->Update_Cascade();
	}
}

AVector3 Instance::GetLocalPos() {
	return LocalPos;
}
void Instance::SetLocalPos() {
	std::shared_ptr<Instance> P_ptr = Parent.lock();

	if (P_ptr == nullptr) {
		LocalPos = Position;
		return;
	}

	AVector3 relVec3 = (this->Position - P_ptr->Position);

	AVector3 ParentRot = P_ptr->Rotation * (-1);

	relVec3.Rotate_InPlace(ParentRot);

	relVec3.x /= P_ptr->Size.x;
	relVec3.y /= P_ptr->Size.y;
	relVec3.z /= P_ptr->Size.z;

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
	std::shared_ptr<Instance> P_ptr = Parent.lock();
	// Iterate through all ascendents
	while (P_ptr != nullptr) {

		// Scale first according to Parent Size
		this->Position = this->Position * P_ptr->LocalSize;
		// Rotate according to Parent Rotation (Local) before adding the local vectors
		this->Position.Rotate_InPlace(P_ptr->Rotation);
		// Add the local Rotation from the paren
		this->Rotation += P_ptr->LocalRot;
		// Add the local Position from the parent
		this->Position += P_ptr->LocalPos;
		// Multiply Size with local Size
		this->Size = this->Size * P_ptr->LocalSize;
		// Advance upwards in the instance hierrarchy
		P_ptr = P_ptr->Parent.lock();
	}
}

void Instance::SetPosition(AVector3 _Position) {
	Position = _Position;
	SetLocalPos();
	// Update recursively
	Update_Cascade();
}
void Instance::SetRotation(AVector3 _Rotation) {
	LocalRot = _Rotation;
	SetLocalPos();
	// Update recursively
	Update_Cascade();
}
void Instance::SetSize(AVector3 _Size) {
	auto parent = Parent.lock();
	if (parent == nullptr) return;
	AVector3& P_size = parent->Size; // (World-Space)
	LocalSize = AVector3(_Size.x / P_size.x, _Size.y / P_size.y, _Size.z / P_size.z);
	SetLocalPos();
	// Update recursively
	Update_Cascade();
}
void Instance::SetColor(AColor3 _Color) {
	Color = _Color;
	Update();
}
void Instance::SetColor(int R, int G, int B, int A) {
	Color = AColor3(R, G, B, A);
	Update();
}
void Instance::SetTile(Tile* _tile) {
	tile = _tile;
}

void Instance::AddChild(std::shared_ptr<Instance> Ins) {
	// Check if child already is in Children vector
	auto iterator = std::find(Children.begin(), Children.end(), Ins);
	if (iterator == Children.end()) {
		Children.push_back(Ins);
	}
}
void Instance::RemoveChild(std::shared_ptr<Instance> Ins) {
	// Check if child already is in Children vector
	auto iterator = std::find(Children.begin(), Children.end(), Ins);
	if (iterator != Children.end()) {
		Children.erase(iterator);
	}
}
void Instance::SetParent(std::weak_ptr<Instance> _Parent) {
	if (_Parent.lock() == nullptr) throw InstanceException("Parent No Longer Exists", 1);
	// Check if this instance already has a parent
	if (Parent.lock() != nullptr) {
		DelFromUMap(Parent.lock());
		Parent.lock()->RemoveChild(shared_from_this());
	}
	
	Parent = _Parent;

	Parent.lock()->AddChild(shared_from_this());

	SetLocalPos();

	SetPosition(LocalPos);
	SetRotation(LocalRot);
	SetSize(LocalSize);

	AddToUMap(Parent.lock());

	Update_Cascade();
}

int Instance::GetBlueprintID() { return Template->GetID(); }

AVector3 Instance::GetPosition() { return Position; }
AVector3 Instance::GetRotation() { return Rotation; }
AVector3 Instance::GetSize() { return Size; }

void Instance::SetHandleOffset(int offset) { handleOffset = offset; }

// ----------- GET CHILD METHODS

std::shared_ptr<Instance> Instance::FirstChild(std::string _TagName) const {
	auto iterator = Instance_UMap.find(_TagName);
	if (iterator == Instance_UMap.end()) return nullptr;
	// TagName was found in UMap, extract first EntityID
	return FirstChild(
		iterator->second.back() // the first EID from the vector<int> (second attribute of iterator)
	);
}

std::shared_ptr<Instance> Instance::FirstChild(int EntityID) const {
	auto EID_iter = EID_UMap.find( EntityID );
	// Last check if it exists in EID_UMap
	if (EID_iter == EID_UMap.end()) {
		throw InstanceException(
			"Instance TagName Reference to an EID (EntityID) found, but no EID reference to an Instance", 0);
	}
	// Return the Instance found at EID_iter
	return EID_iter->second;
}

const std::vector<std::shared_ptr<Instance>>& Instance::GetChildren() { return Children; }

std::vector<std::shared_ptr<Instance>> Instance::AllChildrenWith(std::string _TagName) {
	std::vector<std::shared_ptr<Instance>> Instance_PTRs;
	for (auto& c : Children) {
		if (c->GetTag() == _TagName) Instance_PTRs.push_back(c);
	}
	return Instance_PTRs;
}

void Instance::GetDescendants(std::vector<std::shared_ptr<Instance>>& container) {
	if (Children.empty()) return;
	for (auto& c : Children) {
		container.push_back(c);
		c->GetDescendants(container);
	}
}

// ------------- GET PARENT METHODS

std::shared_ptr<Instance> Instance::GetParent() {

	std::shared_ptr<Instance> P_ptr = Parent.lock();

	//if (P_ptr == nullptr) { throw InstanceException("Parent does not exist",1); return nullptr; }

	return P_ptr;
}

void Instance::GetAscendants(std::vector<std::shared_ptr<Instance>>& container) {
	if (Parent.lock()==nullptr) return;
	container.push_back(Parent.lock());
	GetAscendants(container);
}

// -------------- INSTANCE DATA METHODS

void InstanceData::SetMatrix(AVector3 Position, AVector3 Rotation, AVector3 Scale) {
	matrix = glm::translate(glm::mat4(1.0f), (glm::vec3)Position);
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
	matrix = glm::scale(matrix, (glm::vec3)Scale);
}
void InstanceData::SetColor(AColor3 _RGBA) {
	RGBA = _RGBA;
}

InstanceData::InstanceData(AVector3 Position, AVector3 Rotation, AVector3 Scale) {
	SetMatrix(Position, Rotation, Scale);
}