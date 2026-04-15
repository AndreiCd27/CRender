
#include "Instance.h"

Instance::Instance() : Transform(nullptr, "null") {}
Instance::Instance(const Blueprint* _Template, Scene* scene) : Transform(scene, "Instance"), Template(_Template) {}
Instance::Instance(const Blueprint* _Template, Scene* scene, const std::string& TagName) : Transform(scene, TagName), Template(_Template) {}

void Instance::AddToUMap(UserRef<Instance>& parent) {
	parent->EID_UMap[GetEID()] = self;
	parent->Instance_UMap[GetTag()].push_back(GetEID());
}
void Instance::DelFromUMap(UserRef<Instance>& parent) {
	if (parent->EID_UMap.find(GetEID()) != parent->EID_UMap.end()) parent->EID_UMap.erase(GetEID());

	auto& vectFromTag = parent->Instance_UMap[GetTag()];
	auto iter = std::find(vectFromTag.begin(), vectFromTag.end(), GetEID());
	if (iter != vectFromTag.end()) {
		parent->Instance_UMap[GetTag()].erase(iter);
	}
}

constexpr float oneOverPI = 1.0f / 3.14159f;

void Instance::LookAt(AVector3 from, AVector3 to) {
	AVector3 d = (to - from);

	SetDirection(d);

	SetPosition(from);
}

void Instance::SetDirection(AVector3 Direction) {
	float mag = Direction.Magnitude();
	if (mag < 0.0001f) return;

	glm::vec3 forward = (glm::vec3)Direction.Normalize();
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));
	glm::mat3 rot = glm::mat3(right, up, forward);

	glm::vec3 angles;
	glm::extractEulerAngleXYZ(glm::mat4(rot), angles.x, angles.y, angles.z);
	angles = 180.0f * oneOverPI * angles;

	Rotation = AVector3(angles.x, angles.y, angles.z);
	//LocalRot = AVector3(angles.x, angles.y, angles.z);

	UpToDate = false;
	//Update();
}

void Instance::AddChild(UserRef<Instance>& Ins) {
	// Check if child already is in Children vector
	auto iterator = std::find(Children.begin(), Children.end(), Ins);
	if (iterator == Children.end()) {
		Children.push_back(Ins);
	}
}
void Instance::RemoveChild(UserRef<Instance>& Ins) {
	// Check if child already is in Children vector
	auto iterator = std::find(Children.begin(), Children.end(), Ins);
	if (iterator != Children.end()) {
		Children.erase(iterator);
	}
}
void Instance::SetParent(UserRef<Instance>& _Parent) {
	if (self.getRef() == nullptr) return;
	//if (_Parent.Organizer == nullptr) throw InstanceException("Parent No Longer Exists", 1);
	// Check if this instance already has a parent
	if (Parent.getRef() != nullptr) {
		DelFromUMap(Parent);
		Parent->RemoveChild(self);
	}
	
	Parent = _Parent;

	Parent->AddChild(self);

	SetLocalPos();

	SetPosition(LocalPos);
	SetRotation(LocalRot);
	SetSize(LocalSize);

	AddToUMap(Parent);

	//Update_Cascade();
}

int Instance::GetBlueprintID() { return Template->GetID(); }

AVector3 Instance::GetPosition() { return Position; }
AVector3 Instance::GetRotation() { return Rotation; }
AVector3 Instance::GetSize() { return Size; }

// ----------- GET CHILD METHODS

UserRef<Instance> Instance::FirstChild(const std::string& _TagName) const {
	auto iterator = Instance_UMap.find(_TagName);
	if (iterator == Instance_UMap.end()) return UserRef<Instance>();
	// TagName was found in UMap, extract first EntityID
	return FirstChild(
		iterator->second.back() // the first EID from the vector<int> (second attribute of iterator)
	);
}

UserRef<Instance> Instance::FirstChild(int EntityID) const {
	auto EID_iter = EID_UMap.find( EntityID );
	// Last check if it exists in EID_UMap
	if (EID_iter == EID_UMap.end()) {
		throw InstanceException(
			"Instance TagName Reference to an EID (EntityID) found, but no EID reference to an Instance", 0);
	}
	// Return the Instance found at EID_iter
	return EID_iter->second;
}

std::vector< UserRef<Instance> >& Instance::GetChildren() { return Children; }

// Using std::copy_if
// Example:  (from cppreference)
/*
	std::copy_if(from_vector.begin(), from_vector.end(),
				 std::back_inserter(to_vector),
				 [](int x) { return x % 3 == 0; });
*/
std::vector< UserRef<Instance> > Instance::AllChildrenWith(const std::string& _TagName) {
	std::vector< UserRef<Instance> > Instance_PTRs;
	std::copy_if(Children.begin(), Children.end(), std::back_inserter(Instance_PTRs),
		[_TagName](const UserRef<Instance>& c) { return c->GetTag() == _TagName; }
	);  // ^-- "capture list" - here we put variables we need inside function ---^
	return Instance_PTRs;
}

void Instance::GetDescendants(std::vector< UserRef<Instance> >& container) {
	if (Children.empty()) return;
	for (auto& c : Children) {
		container.push_back(c);
		c->GetDescendants(container);
	}
}

// ------------- GET PARENT METHODS

UserRef<Instance>& Instance::GetParent() {
	return Parent;
}

void Instance::GetAscendants(std::vector< UserRef<Instance> >& container) {
	if (Parent.getRef() == nullptr) return;
	container.push_back(Parent);
	GetAscendants(container);
}

// -------------- INSTANCE DATA METHODS
/*
void InstanceData::SetMatrix(const AVector3& Position, const AVector3& Rotation, const AVector3& Scale, const AVector3& Center) {
	matrix = glm::translate(glm::mat4(1.0f), (glm::vec3)Position);
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
	matrix = glm::scale(matrix, (glm::vec3)Scale);
	matrix = glm::translate(matrix, -(glm::vec3)Center);
}
void InstanceData::SetColor(AColor3 _RGBA) {
	RGBA = _RGBA;
}

InstanceData::InstanceData(const AVector3& Position, const AVector3& Rotation, const AVector3& Scale, const AVector3& Center) {
	SetMatrix(Position, Rotation, Scale, Center);
}
*/