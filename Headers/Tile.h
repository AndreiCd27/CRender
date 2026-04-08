#pragma once

#include "Scene.h"

class Scene;

class Tile {
private:
	static int contor;

	static bool DEBUG;

	int TileID;

	Tile* Parent;
	uint16_t TileX, TileZ;
	uint16_t Level = 0;

	Tile* Divisions[2][2] = { {nullptr} };

	std::vector<int> RelatedHandleIDs;

	Tile(Tile* _Parent, uint16_t _TileX, uint16_t _TileZ, uint16_t _Level);
	~Tile();
	void DivideTile(uint16_t i, uint16_t j);

	friend Scene::Scene();
	friend Scene::~Scene();

public:
	static const int shiftComponent;

	std::vector<Tile*> RecurseInTiles();
	void RecurseInTilesOutputHandleIDs(std::vector<int>& HandleIDs);

	inline int GetTileID() const;

	const std::vector<int>& GetRelatedHandleIDs();
	void PushHandleID(int HandleID, ArrayOrganizer<InstanceData>& insArrayOrg);

	friend Tile* Scene::FindTileForPosition(const AVertex& center, AVector3 Position);
	friend void Scene::AssignTilesToInstances();
};