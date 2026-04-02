
#include "Scene.h"
#include <numeric>

std::weak_ptr<const Instance> Scene::GetWorkspace() {
	return std::weak_ptr<const Instance>(workspace);
}

void Scene::DEBUG_PrintInstanceHierarchy(std::weak_ptr<const Instance> start, int depth, int maxdepth, bool details) {
	if (depth <= maxdepth) {
		std::shared_ptr<const Instance> sptr = start.lock();
		if (sptr == nullptr) return;

		for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
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

/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
Handle Scene::GetBlueprintHandle(Blueprint* BLUEPRINT, int TARGET) {
	if (TARGET == VBO_ORGANIZER_TARGET) return VBO_Organizer.GetHandleData(BLUEPRINT->GetID());
	if (TARGET == EBO_ORGANIZER_TARGET) return EBO_Organizer.GetHandleData(BLUEPRINT->GetID());
	throw SceneException("Can't extract Blueprint Handle from INSTANCE_ORGANIZER or INVALID TARGET", 1);
}
*/

ArrayOrganizer<InstanceData>& Scene::GetInstanceOrganizer() {
	return InstanceOrganizer;
}
ArrayOrganizer<AVertex>& Scene::GetVBO_Organizer() {
	return VBO_Organizer;
}
ArrayOrganizer<GLuint>& Scene::GetEBO_Organizer() {
	return EBO_Organizer;
}
/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
const InstanceData* Scene::GetInstanceOrganizerPTR(int HandleID) {
	return InstanceOrganizer.GetPointerFromHandle(HandleID);
}
const AVertex* Scene::GetVBO_OrganizerPTR(int HandleID) {
	return VBO_Organizer.GetPointerFromHandle(HandleID);
}
const GLuint* Scene::GetEBO_OrganizerPTR(int HandleID) {
	return EBO_Organizer.GetPointerFromHandle(HandleID);
}
*/
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

	//std::cout << "Creating a new BLUEPRINT with " << vertices.size() << " Vertices and " << indicies.size() << " indicies \n";

	// Create the Blueprint object
	Blueprint* blueprint = new Blueprint();
	int generatedHandleID = blueprint->GetID(); // Number from 0 to 4095

	Blueprint::CalculateSurfaceNormals(vertices, indicies);

	VBO_Organizer.NewHandle(generatedHandleID, (int)vertices.size());
	EBO_Organizer.NewHandle(generatedHandleID, (int)indicies.size());

	VBO_Organizer.PushMultipleData(generatedHandleID, vertices);
	EBO_Organizer.PushMultipleData(generatedHandleID, indicies);

	// Calculate Center using std::accumulate
	// From cppreference:
	// template< class InputIt, class T, class BinaryOp > T accumulate(InputIt first, InputIt last, T init, BinaryOp op);
	AVector3 vcenter = std::accumulate(vertices.begin(), vertices.end(), 
		AVector3(0.0f, 0.0f, 0.0f), [](AVector3 sumvec3, const AVertex& vert) {
			return sumvec3 + vert.POS;
		}
	);

	const float center_scalar = 1.0f / (float)vertices.size();

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
