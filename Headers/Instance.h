
#include "GeometryLoader.h"


class InstanceException : public std::runtime_error {
public:
	InstanceException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ INSTANCE_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {};
};

class InstanceData {
private:
	glm::mat4 matrix = glm::mat4(1.0f);
	AColor3 RGBA; // 4 bytes
	A_UV UV; // 4 bytes
public:
	InstanceData() = default;
	InstanceData(AVector3 Position, AVector3 Rotation, AVector3 Scale, AVector3 Center);
	void SetMatrix(AVector3 Position, AVector3 Rotation, AVector3 Scale, AVector3 Center);
	void SetColor(AColor3 _RGBA);
};

class Instance : public Transform, public std::enable_shared_from_this<Instance> {

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

	// 2 unordered_maps are needed to track Tag Names of children
	// EID (Entity IDs) are UNIQUE, they identify exactly ONE Instance
	std::unordered_map< int, std::shared_ptr<Instance> > EID_UMap;
	//                [ EID ]   [ Instance_PTR ]
	// A TagName is not unique, they can point to multiple EIDs
	std::unordered_map< std::string, std::vector<int> > Instance_UMap;
	//                  [ TagName ]   [ Instance_PTR ]

	void AddToUMap(std::shared_ptr<Instance> parent);
	void DelFromUMap(std::shared_ptr<Instance> parent);

	void Update() override;
	void Update_Cascade();

	void SetLocalPos();

	void CalculateWorldVectors();

public:

	// --------- CONSTRUCTORS (requires a Blueprint first)

	Instance(Blueprint* _Template, Scene* scene, const std::string& TagName);
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
	
	void LookAt(AVector3 from, AVector3 to);
	void SetDirection(AVector3 Direction);

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

	// ----------- GET CHILD METHODS

	std::shared_ptr<Instance> FirstChild(const std::string& TagName) const;
	std::shared_ptr<Instance> FirstChild(int EntityID) const;

	const std::vector<std::shared_ptr<Instance>>& GetChildren();

	std::vector<std::shared_ptr<Instance>> AllChildrenWith(const std::string& TagName);

	void GetDescendants(std::vector<std::shared_ptr<Instance>>& container);

	// ------------- GET PARENT METHODS

	std::shared_ptr<Instance> GetParent();

	void GetAscendants(std::vector<std::shared_ptr<Instance>>& container);

	// ----------- INDEXING RELATIVE TO A HANDLE 
	// (See MultiArray.h & InstanceArrayOrganizer in Scene class)

	void SetHandleOffset(int offset);

	// ------------- FRIENDS (FROM SCENE CLASS)

	friend Scene;

};

/*    TODO
class UniqueGeom : public Transform {
	// Geometry remains unchanged, stored directly as geometry
};

class StaticGeom : public Entity {
	// Un-modify-able
};
*/
