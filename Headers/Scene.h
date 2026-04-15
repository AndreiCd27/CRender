
#include "Instance.h"
#include "MultiArray.h"

// .stl geometry file reader by sreiter https://github.com/sreiter/stl_reader
#include <stl_reader.h>

#define TILE_MAXSCALE_FACTOR 14
#define MAX_TILE_LEVEL 14
#define START_TILE_LEVEL 10

#define INSTANCE_ORGANIZER_TARGET 0
#define VBO_ORGANIZER_TARGET 1
#define EBO_ORGANIZER_TARGET 2
#define MATRIX_ORGANIZER_TARGET 3

#define POSITION_TARGET 5
#define ROTATION_TARGET 6
#define SIZE_TARGET 7

class Tile;

class InstancePool {
public:
	std::vector<Instance> Elements;

	std::vector<unsigned int> cBitsX;
	std::vector<unsigned int> cBitsZ;

	std::vector<std::pair<int,int>> HandleID_UMap;

	//std::vector<Ref<Instance>> ToUpdate;

	friend class Instance;
};

class SceneException : public std::runtime_error {
public:
	SceneException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

class Scene {
private:

	// Handles are generated for every Blueprint and for every Tile
	ArrayOrganizer<InstanceData> MatrixOrganizer;
	//
	ArrayOrganizer<Instance> InstanceOrganizer;

	// Handles are generated for every Tile
	ArrayOrganizer<AVertex> VBO_Organizer;
	ArrayOrganizer<GLuint> EBO_Organizer;

	// Clasify Blueprints
	std::vector<Blueprint*> Blueprints;

	std::vector<Ref<Instance>> Refrences;

	std::unordered_map<std::string, int> Alias_TO_ID;
	std::unordered_map<int, Blueprint*> ID_TO_Blueprint;

	// Sample Buffer to use when selecting instances (store their index)
	// Updates every frame
	// Right now not used
	std::vector<int> VisibleInstances;

	InstancePool pool;

public:

	void AssignTilesToInstances();
	void ExecuteInstancePool();

	Tile* WorldRoot = nullptr;

	//Ref<Instance> workspace = Ref<Instance>(nullptr, -1, -1);

	Scene();
	~Scene();

	// Copy and equal constructor are FORBIDEN
	// This is a design choice, beacause one clone of a Scene
	// With a big memory space (on the heap) may crash the program
	// Or trigger many heap reallocations
	// A new Scene may be added from zero, but right now Engine3D only has MainScene
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Tile* FindTileForPosition(const AVertex& center, AVector3 Position);

	Blueprint* CreateBlueprint(std::vector<AVertex>& vertices, std::vector<GLuint>& indicies);
	const UserRef<Instance> CreateInstance(const Blueprint* temp, const std::string& name);

	ArrayOrganizer<Instance>& GetInstanceOrganizer();
	ArrayOrganizer<InstanceData>& GetMatrixOrganizer();
	ArrayOrganizer<AVertex>& GetVBO_Organizer();
	ArrayOrganizer<GLuint>& GetEBO_Organizer();

	std::vector<Blueprint*>& GetBlueprints();
	/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
	const InstanceData* GetInstanceOrganizerPTR(int HandleID);
	const AVertex* GetVBO_OrganizerPTR(int HandleID);
	const GLuint* GetEBO_OrganizerPTR(int HandleID);
	*/

	void GenerateHandle(int HandleID, int TARGET, int capacity);

	void DEBUG_PrintInstanceHierarchy(UserRef<Instance> start, int depth, int maxdepth, bool details);

	Blueprint* LoadSTLGeomFile(const char* filePath, float scale);

	Blueprint* CreatePrism(const std::vector<AVertex>& vertices, int VertexNumber, float height);
	Blueprint* CreateRectPrism(float length, float width, float height);
	Blueprint* CreateCube(float length);

	Blueprint* CreateUnitVector();

	friend void Instance::Update();

	void UpdateBatchVectors(std::vector<UserRef<Instance>>& References, 
		std::vector<AVector3>& Vectors, int VECTOR_TYPE_TARGET);

	void UpdateBatchColors(std::vector<UserRef<Instance>>& References, std::vector<AColor3>& Colors);
};