
#include "Scene.h"
#include <numeric>

void Scene::DEBUG_PrintInstanceHierarchy(UserRef<Instance> start, int depth, int maxdepth, bool details) {
	if (depth <= maxdepth) {
		auto sptr = start;
		if (sptr == nullptr) return;

		for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
		if (depth > 0) std::cout << "|__";
		details ? (std::cout << sptr->GetTag() << " [ EID=" << sptr->GetEID() << " Addr=" << sptr.getRef()->HIndex << "{"<<sptr.getRef()->HOffset<<"} " << " ] \n") :
			(std::cout << sptr->GetTag() << " \n");
		if (details) {
			for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
			if (depth > 0) std::cout << "|++TRANSFORM_GLOBAL: ";
			sptr->Position.DEBUG_Print();
			sptr->Rotation.DEBUG_Print();
			sptr->Size.DEBUG_Print();
			std::cout << "Center: ";
			if (sptr->Template) sptr->Template->Center.POS.DEBUG_Print();
			std::cout << "\n";
			for (int i = 0; i < depth - 1; i++) std::cout << "|  ";
			if (depth > 0) std::cout << "|++TRANSFORM_LOCAL: ";
			sptr->LocalPos.DEBUG_Print();
			sptr->LocalRot.DEBUG_Print();
			sptr->LocalSize.DEBUG_Print();
			std::cout << "\n";
		}
		for (auto& c : sptr->Children) {
			DEBUG_PrintInstanceHierarchy(c, depth + 1, maxdepth, details);
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

ArrayOrganizer<Instance>& Scene::GetInstanceOrganizer() {
	return InstanceOrganizer;
}
ArrayOrganizer<InstanceData>& Scene::GetMatrixOrganizer() {
	return MatrixOrganizer;
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
	if (TARGET == MATRIX_ORGANIZER_TARGET) {
		MatrixOrganizer.NewHandle(HandleID, capacity);
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

	if (!vertices.empty()) {
		const float center_scalar = 1.0f / (float)vertices.size();

		vcenter = vcenter * center_scalar;
	}

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

Blueprint* Scene::CreateUnitVector() {
	float length = 0.8f, w = 0.0625f;

	std::vector<AVertex> v;
	v.resize(13);
	// BOTTOM FACE
	v[0] = AVertex(w, w, 0.0f, 200, 200, 200, 255);
	v[1] = AVertex(-w, w, 0.0f, 200, 200, 200, 255);
	v[2] = AVertex(-w, -w, 0.0f, 200, 200, 200, 255);
	v[3] = AVertex(w, -w, 0.0f, 200, 200, 200, 255);
	// TOP FACE
	v[4] = AVertex(w, w, length, 200, 200, 200, 255);
	v[5] = AVertex(-w, w, length, 200, 200, 200, 255);
	v[6] = AVertex(-w, -w, length, 200, 200, 200, 255);
	v[7] = AVertex(w, -w, length, 200, 200, 200, 255);
	// TOP FACE 2
	v[8] = AVertex(w * 2.0f, w * 2.0f, length, 200, 200, 200, 255);
	v[9] = AVertex(-w * 2.0f, w * 2.0f, length, 200, 200, 200, 255);
	v[10] = AVertex(-w * 2.0f, -w * 2.0f, length, 200, 200, 200, 255);
	v[11] = AVertex(w * 2.0f, -w * 2.0f, length, 200, 200, 200, 255);
	// ARROW
	v[12] = AVertex(0.0f, 0.0f, 1.0f);
	// CONNECTING VERTICES
	std::vector<GLuint> ind = { 
		0, 3, 2, 2, 1, 0, // BOTTOM FACE

		8, 9, 12, // PYRAMID FACE
		9, 10, 12, // PYRAMID FACE
		10, 11, 12, // PYRAMID FACE
		11, 8, 12, // PYRAMID FACE

		// TOP FACE WITH PYRAMID BASE
		4, 5, 9, 9, 8, 4,
		5, 6, 10, 10, 9, 5,
		6, 7, 11, 11, 10, 6,
		7, 4, 8, 8, 11, 7,

		0, 1, 5, 5, 4, 0, // LATERAL 
		1, 2, 6, 6, 5, 1, // LATERAL
		2, 3, 7, 7, 6, 2, // LATERAL
		3, 0, 4, 4, 7, 3, // LATERAL
	};
	Blueprint* b = CreateBlueprint(v, ind);
	b->Center = AVertex(0.0f, 0.0f, 0.0f);
	return b;
}

void Scene::UpdateBatchVectors(std::vector<UserRef<Instance>>& References,
	std::vector<AVector3>& Vectors, int VECTOR_TYPE_TARGET) {

	auto& matArray = MatrixOrganizer.GetMultiArray();
	auto& insArray = InstanceOrganizer.GetMultiArray();
	auto& handles = MatrixOrganizer.GetHandles();

	for (int i = 0; i < (int)References.size(); ++i) {
		if (References[i] == nullptr) continue;

		int hIdx = References[i].getRef()->HIndex;
		int offset = References[i].getRef()->HOffset;
		int absoluteIdx = handles[hIdx].offset + offset;

		InstanceData& data = matArray[absoluteIdx];
		Instance& ins = insArray[absoluteIdx];

		(VECTOR_TYPE_TARGET == POSITION_TARGET) ? ins.Position = Vectors[i] : (
			(VECTOR_TYPE_TARGET == ROTATION_TARGET) ? ins.Rotation = Vectors[i] : ins.Size = Vectors[i]
		);

		ins.SetDataMatrix(data);
	}
}

void Scene::UpdateBatchColors(std::vector<UserRef<Instance>>& References, std::vector<AColor3>& Colors) {

	auto& matArray = MatrixOrganizer.GetMultiArray();
	auto& insArray = InstanceOrganizer.GetMultiArray();
	auto& handles = MatrixOrganizer.GetHandles();

	for (int i = 0; i < (int)References.size(); ++i) {
		if (References[i] == nullptr) continue;

		int hIdx = References[i].getRef()->HIndex;
		int offset = References[i].getRef()->HOffset;
		int absoluteIdx = handles[hIdx].offset + offset;

		InstanceData& data = matArray[absoluteIdx];
		Instance& ins = insArray[absoluteIdx];

		ins.Color = Colors[i];

		ins.SetDataColor(data);
	}
}