
#include "GeometryOrganizer.h"


const int Tile::shiftComponent = Blueprint::GetShiftComponent();
int Tile::contor = 0;

bool Tile::DEBUG = false;

unsigned int getCBits(double cd) {

	unsigned int c = std::abs(cd);
	unsigned int cBits = 0x80000000;
	cd < 0 ? cBits = cBits - c : cBits = c & cBits;

	return cBits;
}

int TOTAL_C_COUNT = 0;
int TOTAL_D_COUNT = 0;

Tile::Tile(Tile* _Parent, uint16_t _TileX, uint16_t _TileZ, uint16_t _Level) {
	if (DEBUG) std::cout << "C tile " << _Level << "\n";
	TileID = contor; contor++;
	TOTAL_C_COUNT++;
	Parent = _Parent; TileX = _TileX; TileZ = _TileZ; Level = _Level;
	for (uint16_t i = 0; i < 2; i++) {
		for (uint16_t j = 0; j < 2; j++) {
			Divisions[i][j] = nullptr;
			if (_Level < MAX_TILE_LEVEL) Tile::DivideTile(i, j);
		}
	}
}


Tile::~Tile() {
	if (DEBUG) std::cout << "D tile " << Level << "\n";
	TOTAL_D_COUNT++;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			delete Divisions[i][j];
		}
	}
}

int Tile::GetTileID() const {
	return TileID;
}

void Tile::DivideTile(uint16_t i, uint16_t j) {
	if (i >= 2 || j >= 2) return;
	if (Divisions[i][j] != nullptr) {
		delete Divisions[i][j];
	}
	Tile* tile = new Tile(this, i, j, Level + 1);
	Divisions[i][j] = tile;
}

std::vector<Tile*> Tile::RecurseInTiles() {
	static std::vector<Tile*> Tiles;
	Tiles.push_back(this);
	if (this->Divisions[0][0] != nullptr) this->Divisions[0][0]->RecurseInTiles();
	if (this->Divisions[0][1] != nullptr) this->Divisions[0][1]->RecurseInTiles();
	if (this->Divisions[1][0] != nullptr) this->Divisions[1][0]->RecurseInTiles();
	if (this->Divisions[1][1] != nullptr) this->Divisions[1][1]->RecurseInTiles();
	return Tiles;
}

const std::vector<int>& Tile::GetRelatedHandleIDs() {
	return RelatedHandleIDs;
}
void Tile::PushHandleID(int HandleID, ArrayOrganizer<InstanceData>& insArrayOrg) {
	bool isInserted = insArrayOrg.ContainsHandle(HandleID);
	if (isInserted) {
		std::cout<<"HandleID is inserted, this Tile has it := "<<HandleID<<"\n";
		RelatedHandleIDs.push_back(HandleID);
	}
}

void Tile::RecurseInTilesOutputHandleIDs(std::vector<int>& HandleIDs) {
	for (const int& hID : this->GetRelatedHandleIDs()) {
		HandleIDs.push_back(hID);
	}
	if (this->Divisions[0][0] != nullptr) this->Divisions[0][0]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[0][1] != nullptr) this->Divisions[0][1]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[1][0] != nullptr) this->Divisions[1][0]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[1][1] != nullptr) this->Divisions[1][1]->RecurseInTilesOutputHandleIDs(HandleIDs);
}

InstanceData* Instance::GetInstanceData() {
	if (tile != nullptr && Template != nullptr) {
		int HandleID = Template->GetID() | (tile->GetTileID() << Tile::shiftComponent);
		if (ParentScene != nullptr) {
			ArrayOrganizer<InstanceData>& insArrayOrg = ParentScene->GetInstanceOrganizer();
			const Handle& h = insArrayOrg.GetHandleData(HandleID);
			std::vector<InstanceData>& insArray = insArrayOrg.GetMultiArray();
			return &insArray[h.offset + handleOffset];
		}
	}
	return nullptr;
}

void Instance::Update() {
	InstanceData* insData = GetInstanceData();
	if (insData != nullptr) {
		insData->SetMatrix(Position, Rotation, Size);
		insData->SetColor(Color);
	}
}
void Instance::Destroy() {

	SetSize(AVector3(0.00001f, 0.00001f, 0.00001f));
	SetPosition(AVector3(0.0f, -1000.0f, 0.0f));

	if (!Children.empty()) {
		for (auto& c : Children) {
			c->Destroy();
		}
	}

	// shared_from_this() works like a self reference but shared_ptr
	// Delete from parent lsist
	std::shared_ptr<Instance> P_ptr = Parent.lock();
	if (P_ptr != nullptr) P_ptr->RemoveChild(shared_from_this());

	Children.clear(); // Delete all shared pointers

	// Now we have to remove the entry from the InstanceVBO
	/*
	if (tile != nullptr && Template != nullptr) {
		int HandleID = Template->GetID() | (tile->GetTileID() << Tile::shiftComponent);
		if (ParentScene != nullptr) {
			ArrayOrganizer<InstanceData>& insArrayOrg = ParentScene->GetInstanceOrganizer();
			const Handle& h = insArrayOrg.GetHandleData(HandleID);
			
			insArrayOrg.RemoveData(h.offset + handleOffset, HandleID);
		}
	}
	*/
}

std::weak_ptr<const Instance> Scene::GetWorkspace() {
	return std::weak_ptr<const Instance>(workspace);
}

void Scene::DEBUG_PrintInstanceHierarchy(std::weak_ptr<const Instance> start, int depth, int maxdepth, bool details) {
	if (depth <= maxdepth) {
		std::shared_ptr<const Instance> sptr = start.lock();
		if (sptr == nullptr) return;

		for (int i = 0; i < depth-1; i++) std::cout << "|  ";
		if (depth > 0) std::cout << "|__";
		details ? (std::cout << sptr->GetTag() << " [ EID=" << sptr->GetEID() << " Addr=" << &*sptr << " ] \n") :
			(std::cout << sptr->GetTag() << " \n");
		if (details) {
			for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
			if (depth > 0) std::cout << "|++TRANSFORM_GLOBAL: ";
			sptr->Position.DEBUG_Print();
			sptr->Rotation.DEBUG_Print();
			sptr->Size.DEBUG_Print();
			std::cout << "\n";
			for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
			if (depth > 0) std::cout << "|++TRANSFORM_LOCAL: ";
			sptr->LocalPos.DEBUG_Print();
			sptr->LocalRot.DEBUG_Print();
			sptr->LocalSize.DEBUG_Print();
			std::cout << "\n";
		}
		for (auto& c : sptr->Children) {
			DEBUG_PrintInstanceHierarchy(std::weak_ptr(c), depth + 1, maxdepth, details);
		}
	}
}

Scene::Scene() {
	//std::cout << "C -> Scene \n";
	WorldRoot = new Tile(nullptr, 0, 0, START_TILE_LEVEL);

	workspace = std::make_shared<Instance>(nullptr, this, "WORKSPACE");
}

Scene::~Scene() {
	if (WorldRoot != nullptr) delete WorldRoot;
	for (int i = 0; i < (int)Blueprints.size(); i++) {
		if (Blueprints[i] != nullptr) {
			delete Blueprints[i];
		}
	}
}

Tile* Scene::FindTileForPosition(const AVertex& center, AVector3 Position) {
	double cxd = (double)center.POS.x + Position.x;
	double czd = (double)center.POS.z + Position.z;
	unsigned int cBitsX = getCBits(cxd);
	unsigned int cBitsZ = getCBits(czd);

	// Start at bit START_TILE_LEVEL
	// Continue shifting until you reach MAX_TILE_LEVEL
	int lvl = START_TILE_LEVEL;
	unsigned int bin = 1 << (32 - START_TILE_LEVEL);
	Tile* tile = this->WorldRoot;
	while (lvl < MAX_TILE_LEVEL) {

		short int bitX = (cBitsX & bin) >> (32 - lvl);
		short int bitZ = (cBitsZ & bin) >> (32 - lvl);

		tile = tile->Divisions[bitX][bitZ];

		lvl++;
		bin = bin >> 1;
	}
	if (tile == nullptr) {
		std::cout << "Tile not found \n";
		return nullptr;
	}
	return tile;
}

std::shared_ptr<Instance> Scene::CreateInstance(Blueprint* temp, const std::string& name) {
	std::shared_ptr<Instance> newInst = std::make_shared<Instance>(temp, this, name);
	AVertex& center = temp->Center;
	newInst->SetColor(center.RGBA);

	Tile* targetTile = this->FindTileForPosition(temp->Center,AVector3(0,0,0));

	newInst->SetTile(targetTile);

	ArrayOrganizer<InstanceData>& insArrayOrg = GetInstanceOrganizer();

	//Create a Handle for TileID [CONCAT] with BlueprintID (templateID attribute)
	int HandleID = temp->GetID() | (targetTile->GetTileID() << Tile::shiftComponent);

	if (!insArrayOrg.ContainsHandle(HandleID)) {
		GenerateHandle(HandleID, INSTANCE_ORGANIZER_TARGET, 1);
		targetTile->PushHandleID(HandleID, insArrayOrg);
	}

	InstanceOrganizer.Push( HandleID, 
		InstanceData(AVector3(0.0f, 0.0f, 0.0f), AVector3(0.0f, 0.0f, 0.0f), AVector3(1.0f, 1.0f, 1.0f)) );

	newInst->SetHandleOffset( insArrayOrg.GetHandleData(HandleID).size - 1 );

	newInst->SetParent(workspace);

	return newInst;
}

Handle Scene::GetBlueprintHandle(Blueprint* BLUEPRINT, int TARGET) {
	if (TARGET == VBO_ORGANIZER_TARGET) return VBO_Organizer.GetHandleData(BLUEPRINT->GetID());
	if (TARGET == EBO_ORGANIZER_TARGET) return EBO_Organizer.GetHandleData(BLUEPRINT->GetID());
	throw SceneException("Can't extract Blueprint Handle from INSTANCE_ORGANIZER or INVALID TARGET", 1);
}

ArrayOrganizer<InstanceData>& Scene::GetInstanceOrganizer() {
	return InstanceOrganizer;
}
ArrayOrganizer<AVertex>& Scene::GetVBO_Organizer() {
	return VBO_Organizer;
}
ArrayOrganizer<GLuint>& Scene::GetEBO_Organizer() {
	return EBO_Organizer;
}

const InstanceData* Scene::GetInstanceOrganizerPTR(int HandleID) {
	return InstanceOrganizer.GetPointerFromHandle(HandleID);
}
const AVertex* Scene::GetVBO_OrganizerPTR(int HandleID) {
	return VBO_Organizer.GetPointerFromHandle(HandleID);
}
const GLuint* Scene::GetEBO_OrganizerPTR(int HandleID) {
	return EBO_Organizer.GetPointerFromHandle(HandleID);
}

void Scene::GenerateHandle(int HandleID, int TARGET, int capacity) {
	if (TARGET == INSTANCE_ORGANIZER_TARGET) {
		InstanceOrganizer.NewHandle(HandleID, capacity);
		return;
	}
	if (TARGET == VBO_ORGANIZER_TARGET) {
		VBO_Organizer.NewHandle(HandleID, capacity);
		return;
	}
	if (TARGET == EBO_ORGANIZER_TARGET) {
		EBO_Organizer.NewHandle(HandleID, capacity);
		return;
	}
	throw SceneException("Target Organizer Not Found Error (Handle was not generated)", 0);
}

Blueprint* Scene::CreateBlueprint(std::vector<AVertex>& vertices, std::vector<GLuint>& indicies) {

	std::cout << "Creating a new BLUEPRINT with " << vertices.size() << " Vertices and " << indicies.size() << " indicies \n";

	// Create the Blueprint object
	Blueprint* blueprint = new Blueprint();
	int generatedHandleID = blueprint->GetID(); // Number from 0 to 4095

	Blueprint::CalculateSurfaceNormals(vertices, indicies);

	VBO_Organizer.NewHandle(generatedHandleID, (int)vertices.size());
	EBO_Organizer.NewHandle(generatedHandleID, (int)indicies.size());

	VBO_Organizer.PushMultipleData(generatedHandleID, vertices);
	EBO_Organizer.PushMultipleData(generatedHandleID, indicies);

	// Calculate Center

	AVector3 vcenter = { 0.0f, 0.0f, 0.0f };
	const float center_scalar = 1.0f / (float)vertices.size();
	
	for (const AVertex& v : vertices) {
		vcenter += v.POS;
	}

	vcenter = vcenter * center_scalar;

	blueprint->Center.POS = vcenter;

	blueprint->IndiciesHandleID = generatedHandleID;
	blueprint->VerticesHandleID = generatedHandleID;

	Blueprints.push_back(blueprint);
	
	return blueprint;
}

std::vector<Blueprint*>& Scene::GetBlueprints() {
	return Blueprints;
}


Blueprint* Scene::LoadSTLGeomFile(const char* fileName, float scale) {

	try {
		std::vector<float> coords, normals;
		std::vector<unsigned int> tris, solids;

		stl_reader::ReadStlFile(fileName, coords, normals, tris, solids);

		std::vector<AVertex> vert;
		std::vector<GLuint> indicies;

		// Avoid duplicate verticies by using a map from the
		// Original vertex index in the STL file to 
		// A new index to be put in indicies 
		// (inserted even if vertex index is already in map)
		std::unordered_map<int, GLuint> uniqueVert;

		const size_t totalIndices = tris.size();

		//std::cout <<"Mesh coord count: " << coords.size() << " trig count: " << tris.size()<<"\n";

		for (int i = 0; i < (int)totalIndices; i++) {
			int STLfileIndex = tris[i];

			if (uniqueVert.find(STLfileIndex) == uniqueVert.end()) {
				// Found a unique vertex that is not a duplicate
				// Add to our map
				uniqueVert.try_emplace(STLfileIndex, (GLuint)vert.size());

				int coordINDEX = 3 * STLfileIndex;
				const float* c = &coords[coordINDEX];

				vert.push_back(AVertex(c[0] * scale, c[1] * scale, c[2] * scale, 200, 200, 200, 255));
			}

			indicies.push_back(uniqueVert[STLfileIndex]);
		}

		//std::cout << "Mesh created \n";

		Blueprint::CalculateSurfaceNormals(vert, indicies);

		return CreateBlueprint(vert, indicies);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return nullptr;
}

Blueprint* Scene::CreatePrism(const std::vector<AVertex>& vertices, int VertexNumber, float height) {

	std::vector<GLuint> indicies;

	std::vector<AVertex> V = vertices;
	V.reserve(VertexNumber * 2);

	if ((int)vertices.size() < 3) { std::cout << "Mesh does not contain any triangles \n"; return nullptr; };

	// 0 -> 1 -> 2
	// |  / |  / |
	// | /  | /  |
	// 3 -> 4 -> 5

	for (int i = 0; i < VertexNumber; i++) {
		AVertex vclone = vertices[i];
		vclone.POS.y += height;
		V.push_back(vclone); // create bottom vertex
	}
	for (int i = 0; i < VertexNumber - 1; i++) {
		//LATERAL FACE 1
		indicies.push_back(i);
		indicies.push_back(VertexNumber + i);
		indicies.push_back(i + 1);
		//LATERAL FACE 2
		indicies.push_back(VertexNumber + i);
		indicies.push_back(VertexNumber + i + 1);
		indicies.push_back(i + 1);
	}

	//LATERAL FACE 1
	indicies.push_back(VertexNumber - 1);
	indicies.push_back(2 * VertexNumber - 1);
	indicies.push_back(0);
	//LATERAL FACE 2
	indicies.push_back(2 * VertexNumber - 1);
	indicies.push_back(VertexNumber);
	indicies.push_back(0);

	for (int i = 1; i < VertexNumber - 1; i++) {
		//BOTTOM FACE
		indicies.push_back(0);
		indicies.push_back(i);
		indicies.push_back(i + 1);

		//TOP FACE
		indicies.push_back(VertexNumber);
		indicies.push_back(VertexNumber + i + 1);
		indicies.push_back(VertexNumber + i);
	}

	return CreateBlueprint(V, indicies);
}

Blueprint* Scene::CreateRectPrism(float length, float width, float height) {
	std::vector<AVertex> v;
	v.resize(4);
	v[0] = AVertex(-length / 2.0f, -height / 2.0f, -width / 2.0f, 200, 200, 200, 255);
	v[1] = AVertex(length / 2.0f, -height / 2.0f, -width / 2.0f, 200, 200, 200, 255);
	v[2] = AVertex(length / 2.0f, -height / 2.0f, width / 2.0f, 200, 200, 200, 255);
	v[3] = AVertex(-length / 2.0f, -height / 2.0f, width / 2.0f, 200, 200, 200, 255);
	return CreatePrism(v, 4, height);
}

Blueprint* Scene::CreateCube(float length) {
	return CreateRectPrism(length, length, length);
}
