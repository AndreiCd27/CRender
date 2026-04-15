
#include "Tile.h"

/////////////////////////////////////////////////////////////////////
// 
// Some functions from class SCENE had to be defined inside Tile.cpp
// 
/////////////////////////////////////////////////////////////////////

Scene::Scene() {
	//std::cout << "C -> Scene \n";
	WorldRoot = new Tile(nullptr, 0, 0, START_TILE_LEVEL);
	std::vector<AVertex> emptyv;
	std::vector<GLuint> emptyi;
	//workspace = CreateInstance(CreateBlueprint(emptyv, emptyi), "WORKSPACE");
}
Scene::~Scene() {
	if (WorldRoot != nullptr) delete WorldRoot;
	for (int i = 0; i < (int)Blueprints.size(); i++) {
		if (Blueprints[i] != nullptr) {
			delete Blueprints[i];
		}
	}
}

// This makes the correct conversion from INT + 2^31 to UNSIGNED_INT
// To get an index between 0-1 for our tile, we need a function that makes
// The conversion from negative to positive unsigned
unsigned int getCBits(double cd) {

	unsigned int c = std::abs(cd);
	unsigned int cBits = 0x80000000;
	cd < 0 ? cBits = cBits - c : cBits = c | cBits;

	return cBits;
}
// To determine where in our Quadtree a position is located,
// We interpret the bits starting from START_TILE_LEVEL,
// Assuming the function getCBits() gets called before
// getCBits() correctly transforms INT + 2^31 into UNSIGNED_INT
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

const UserRef<Instance> Scene::CreateInstance(const Blueprint* temp, const std::string& name) {
	Instance newInst(temp, this, name);
	const AVertex& center = temp->Center;
	newInst.Color = center.RGBA;
	newInst.Center = center.POS;

	//newInst.SetParent(workspace);

	newInst.tile = WorldRoot;

	Ref<Instance> ref(&InstanceOrganizer, -1, pool.Elements.size());
	ref.pool = &pool.Elements;

	newInst.self = UserRef(&Refrences, (int)Refrences.size());
	Refrences.push_back(ref);

	pool.Elements.push_back(newInst);
	pool.cBitsX.push_back(0);
	pool.cBitsZ.push_back(0);

	UserRef<Instance> uref(pool.Elements.back().self);

	return uref;
}

void Scene::AssignTilesToInstances() {
	for (int i = 0; i < (int)pool.Elements.size(); i++) {
		const auto& ins = pool.Elements[i];
		double cxd = (double)ins.Center.x + ins.Position.x;
		double czd = (double)ins.Center.z + ins.Position.z;
		pool.cBitsX[i] = getCBits(cxd);
		pool.cBitsZ[i] = getCBits(czd);
	}
	// Start at bit START_TILE_LEVEL
		// Continue shifting until you reach MAX_TILE_LEVEL
	int lvl = START_TILE_LEVEL;
	unsigned int bin = 1 << (32 - START_TILE_LEVEL);

	while (lvl < MAX_TILE_LEVEL) {
		for (int i = 0; i < (int)pool.Elements.size(); i++) {
			auto& ins= pool.Elements[i];
			short int bitX = (pool.cBitsX[i] & bin) >> (32 - lvl);
			short int bitZ = (pool.cBitsZ[i] & bin) >> (32 - lvl);

			ins.tile = ins.tile->Divisions[bitX][bitZ];
		}
		lvl++;
		bin = bin >> 1;
	}
	for (int i = 0; i < (int)pool.Elements.size(); i++) {
		const auto& ins = pool.Elements[i];
		int handleID = ins.Template->GetID() | (ins.tile->GetTileID() << Tile::shiftComponent);
		pool.HandleID_UMap.push_back(
			std::pair<int, int>(handleID, i)
		);
	}
	std::sort(pool.HandleID_UMap.begin(), pool.HandleID_UMap.end());
}

void Scene::ExecuteInstancePool() {

	ArrayOrganizer<Instance>& insArrayOrg = GetInstanceOrganizer();
	ArrayOrganizer<InstanceData>& matArrayOrg = GetMatrixOrganizer();

	if (!pool.Elements.empty()) {

		AssignTilesToInstances();

		if (pool.HandleID_UMap.empty()) return;

		int poolSize = (int)pool.HandleID_UMap.size();
		auto prevPair = pool.HandleID_UMap[0];
		auto p = pool.HandleID_UMap[0];
		for (int i = 0; i < (int)pool.HandleID_UMap.size(); ) {
			int j = i;
			while (j < poolSize && pool.HandleID_UMap[j].first == pool.HandleID_UMap[i].first) {
				p = pool.HandleID_UMap[j];
				j++;
			}
			int capacity = j - i;

			int HandleIndex = insArrayOrg.GetHandleIndex(p.first);
			int HandlePrevSize = 0;
			if (HandleIndex == -1) {
				// inline GenerateHandle
				// inline PushHandleID
				GenerateHandle(p.first, INSTANCE_ORGANIZER_TARGET, capacity);
				GenerateHandle(p.first, MATRIX_ORGANIZER_TARGET, capacity);
				HandleIndex = insArrayOrg.GetHandleIndex(p.first);
				HandleIndex = matArrayOrg.GetHandleIndex(p.first);
				// -----------^ insert into this tile that has tileID contained in HandleID
			}
			else {
				insArrayOrg.Reserve(HandleIndex, capacity);
				matArrayOrg.Reserve(HandleIndex, capacity);
			}
			HandlePrevSize = insArrayOrg.incHandleSize(HandleIndex, capacity);
			HandlePrevSize = matArrayOrg.incHandleSize(HandleIndex, capacity);
			pool.Elements[prevPair.second].tile->PushHandleID(p.first, matArrayOrg);
			//std::cout << "HandleID = " << p.first << "\n";
			//std::cout << "Batch from index " << i << " to index " << j << "\n";
			for (int k = i; k < j; k++) {
				int originalIndex = pool.HandleID_UMap[k].second;
				auto& ins = pool.Elements[originalIndex];
				const auto& userRef = ins.self;
				auto& ref = userRef.getRef_Direct();
				ref.Organizer = &insArrayOrg;
				ref.pool = nullptr;
				ref.HIndex = HandleIndex;
				ref.HOffset = HandlePrevSize + k - i;

				userRef->self = userRef;
				userRef->Color = ins.Color;
				userRef->Template = ins.Template;
				userRef->ParentScene = ins.ParentScene;
				userRef->Position = ins.Position;
				userRef->Rotation = ins.Rotation;
				userRef->Size = ins.Size;
				userRef->Parent = ins.Parent;
				
				//std::cout << "Instance | HandleIndex = " << ptr->HIndex << ", HandleOffset = " << ptr->HOffset<<"\n";
				//std::cout << ins_shr_ptr->GetTag() << "\n";
			}

			prevPair = p;
			i = j;
		}

		pool.Elements.clear();
		pool.cBitsX.clear();
		pool.cBitsZ.clear();
		pool.HandleID_UMap.clear();
	}

	std::vector<Instance>& insArray = insArrayOrg.GetMultiArray();
	std::vector<InstanceData>& matArray = matArrayOrg.GetMultiArray();
	for (int i = 0; i < (int)insArray.size(); i++) {
		auto& ins = insArray[i];
		auto& data = matArray[i];
		if (!ins.UpToDate) {
			ins.SetDataMatrix(data);
			ins.SetDataColor(data);
			ins.UpToDate = true;
		}
	}

	/*
	if (pool.ToUpdate.empty()) return;

	//std::vector<Instance>& insArray = insArrayOrg.GetMultiArray();
	//std::vector<InstanceData>& matArray = matArrayOrg.GetMultiArray();
	for (int i = 0; i < (int)pool.ToUpdate.size(); i++) {
		const auto& insRef = pool.ToUpdate[i];
		Ref<InstanceData> Data = Ref<InstanceData>(&matArrayOrg, 
			insRef.HIndex, insRef.HOffset
		);
		insRef->SetDataMatrix(Data);
		insRef->SetDataColor(Data);
		insRef->UpToDate = true;
	}
	pool.ToUpdate.clear();
	*/
	
}

/////////////////////////////////////////////////////////////////////

const int Tile::shiftComponent = Blueprint::GetShiftComponent();
int Tile::contor = 0;

bool Tile::DEBUG = false;

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
	return this->TileID;
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
		//std::cout<<"HandleID is inserted, this Tile has it := "<<HandleID<<"\n";
		RelatedHandleIDs.push_back(HandleID);
	}
}

void Tile::RecurseInTilesOutputHandleIDs(std::vector<int>& HandleIDs) {
	//for (const int& hID : this->GetRelatedHandleIDs()) {
	//	HandleIDs.push_back(hID);
	//}
	std::copy(this->GetRelatedHandleIDs().begin(), this->GetRelatedHandleIDs().end(),
		std::back_inserter(HandleIDs)
	);
	if (this->Divisions[0][0] != nullptr) this->Divisions[0][0]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[0][1] != nullptr) this->Divisions[0][1]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[1][0] != nullptr) this->Divisions[1][0]->RecurseInTilesOutputHandleIDs(HandleIDs);
	if (this->Divisions[1][1] != nullptr) this->Divisions[1][1]->RecurseInTilesOutputHandleIDs(HandleIDs);
}

void Instance::Update() {
	UpToDate = false;
	//ParentScene->pool.ToUpdate.push_back(*self.getInternalPtr());
	
	const auto& ref = self.getRef_Direct();

	if (ref.HIndex == -1) return;

	auto& matArrOrg = ParentScene->GetMatrixOrganizer();
	Ref<InstanceData> dataRef{ &matArrOrg, ref.HIndex, ref.HOffset };

	SetDataMatrix(dataRef);
	SetDataColor(dataRef);
	
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
	if (Parent != nullptr) Parent->RemoveChild(self);

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