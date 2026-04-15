#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>


struct Handle {
	int offset = 0;
	int size = 0;
	int capacity = 0;
	int id = -1;
};

class ArrayOrganizerException : public std::runtime_error {
public:
	ArrayOrganizerException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

template <typename T>
class ArrayOrganizer {

	std::unordered_map<int, int> ID_TO_INDEX;

	std::vector<int> INDEX_TO_ID;

	std::vector<Handle> Handles;

	std::vector<T> Array;

	void DoubleInSize(int HandleIndex) {
		Handle& h = Handles[HandleIndex];
		int capacityOld = h.capacity;

		int arraySizeOld = (int)Array.size();
		Array.resize(arraySizeOld + capacityOld);

		// std::copy_backward
		// Copy in reverse to avoid overwritten data
		// Here arraySize is the size of the vector before resizing
		// EX: double h1
		// | t0 | t1 | t1 | t2 | t3 | t3 |
		//      ^         ^
		// | t0 | t1 | t1 | t2 | t3 | t3 |    |    |			+ 2 spots
		//                ^
		// | t0 | t1 | t1 | t2 | t3 | t3 |    | t3 |			copies first template T with Handle = 3
		// | t0 | t1 | t1 | t2 | t3 |    | t3 | t3 |			copies second template T with Handle = 3
		// | t0 | t1 | t1 | t2 | t3 | t2 | t3 | t3 |			copies first template T with Handle = 2
		// | t0 | t1 | t1 | t2 | t3 | t2 | t3 | t3 |			data from index 3-4 can be OVERWRITTEN now
		// Prevent unwanted behaviour by overwriteing data from index 3-4 with EMPTY template
		auto from = Array.begin() + h.offset + capacityOld;
		auto to = Array.begin() + arraySizeOld;
		std::copy_backward(from, to, Array.end());

		for (int i = HandleIndex + 1; i < (int)Handles.size(); i++) {
			Handles[i].offset += capacityOld;
		}
		h.capacity <<= 1;

		std::fill(Array.begin() + h.offset + capacityOld, Array.begin() + h.offset + h.capacity, T());

	}
public:

	void Reserve(int HandleIndex, int reserveSize) {
		Handle& h = Handles[HandleIndex];
		int capacityOld = h.capacity;

		int arraySizeOld = (int)Array.size();
		Array.resize(arraySizeOld + reserveSize);

		auto from = Array.begin() + h.offset + capacityOld;
		auto to = Array.begin() + arraySizeOld;
		std::copy_backward(from, to, Array.end());

		for (int i = HandleIndex + 1; i < (int)Handles.size(); i++) {
			Handles[i].offset += reserveSize;
		}
		h.capacity += reserveSize;

		std::fill(Array.begin() + h.offset + capacityOld, Array.begin() + h.offset + h.capacity, T());

	}

	void NewHandle(int HandleID, int capacityPow2) {
		// Capacity SHOULD be a power of 2 for correct memory alignment,
		// But it is not enforced

		int h_off = (int) Array.size();

		Handle h = { h_off , 0, capacityPow2, HandleID };
		Handles.push_back(h);

		Array.resize(h_off + capacityPow2);

		ID_TO_INDEX[HandleID] = (int)Handles.size() - 1;
		INDEX_TO_ID.push_back(HandleID);

	}

	//void MoveBefore(int beforeHandleID, int HandleID) {
		//TODO
	//}
	//void MoveAfter(int afterHandleID, int HandleID) {
		//TODO
	//}
	/* DEPRECATED
	void MoveToEnd(int HandleID) {

		int HandleIndex = ID_TO_INDEX[HandleID]; // (OLD INDEX)
		if (HandleIndex == (int)Handles.size() - 1) return;
		const Handle& h = Handles[HandleIndex];

		std::vector<T> temp;
		temp.resize(h.capacity);
		auto from = Array.begin() + h.offset;
		auto to = from + h.capacity;
		std::copy(from, to, temp.begin());

		std::copy(to, Array.end(), from);

		std::copy(temp.begin(), temp.end(), Array.end() - h.capacity);

		for (int i = HandleIndex + 1; i < (int)Handles.size(); i++) {
			Handles[i].offset -= h.capacity;
			int iID = INDEX_TO_ID[i];
			ID_TO_INDEX[iID] = i - 1;
		}

		Handle hcopy = h; // We copy h to prevent erase deleting our handle
		hcopy.offset = (int)Array.size() - hcopy.capacity;

		Handles.erase(Handles.begin() + HandleIndex);
		INDEX_TO_ID.erase(INDEX_TO_ID.begin() + HandleIndex);
		Handles.push_back(hcopy);
		INDEX_TO_ID.push_back(HandleID);

		ID_TO_INDEX[HandleID] = (int)Handles.size() - 1;
	}
	*/
	//void MoveToStart(int HandleID) {
		//TODO
	//}

	void Push(int HandleID, T data) {

		int HandleIndex = ID_TO_INDEX[HandleID];

		Handle& h = Handles[HandleIndex];

		if (h.size >= h.capacity) {
			DoubleInSize(HandleIndex);
		}

		Array[h.offset + h.size] = data;
		h.size++;
	}
	
	void PushMultipleData(int HandleID, std::vector<T>& multidata) {
		int HandleIndex = ID_TO_INDEX[HandleID];

		Handle& h = Handles[HandleIndex];
		int increase = (int)multidata.size();

		if (h.size + increase >= h.capacity) {
			Reserve(HandleIndex, increase);
		}

		std::copy(multidata.begin(), multidata.end(), Array.begin() + h.offset + h.size);
		h.size += increase;
	}
	
	void print() {
		int i = 0;
		for (const Handle& h : Handles) {
			std::cout << i << ": ID = " << INDEX_TO_ID[i] << ", OFFSET = " << h.offset << ", SIZE = " << h.size << ", CAPACITY = " << h.capacity << "\n";
			i++;
		}
	}
	
	// Increments Handle Size and returns size before the incrementation
	int incHandleSize(int HandleIndex, int _size) {
		int prevSize = Handles[HandleIndex].size;
		Handles[HandleIndex].size += _size;
		return prevSize;
	}

	const Handle& GetHandleData(int HandleID) {
		auto iterator = ID_TO_INDEX.find(HandleID);
		if (iterator == ID_TO_INDEX.end()) {
			throw ArrayOrganizerException("Invalid ID " + std::to_string(HandleID), 0);
		}
		return Handles[iterator->second]; // Avoid another look-up in the unordered map
	}

	/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
	const T* GetPointerFromHandle(int HandleID) {
		auto iterator = ID_TO_INDEX.find(HandleID);
		if (iterator == ID_TO_INDEX.end()) {
			throw ArrayOrganizerException("Invalid ID " + std::to_string(HandleID), 0);
		}
		int i = Handles[ID_TO_INDEX[HandleID]].offset;
		return &Array[i];
	}
	*/

	std::vector<T>& GetMultiArray() {
		return Array;
	}

	bool ContainsHandle(int HandleID) {
		return !(ID_TO_INDEX.find(HandleID) == ID_TO_INDEX.end());
	}
	int GetHandleIndex(int HandleID) {
		if (ID_TO_INDEX.find(HandleID) == ID_TO_INDEX.end()) return -1;
		return ID_TO_INDEX[HandleID];
	}
	Handle& GetHandleFromIndex(int HandleIndex) {
		return Handles[HandleIndex];
	}
	const std::vector<Handle>& GetHandles() {
		return Handles;
	}
};

class Scene;
template <typename T>
class Ref {
	ArrayOrganizer<T>* Organizer = nullptr;
	std::vector<T>* pool = nullptr;
	int HIndex = -1;
	int HOffset = -1;
public:
	Ref() = default;
	Ref(ArrayOrganizer<T>* _Organizer, int _HIndex, int _HOffset) : Organizer(_Organizer), HIndex(_HIndex), HOffset(_HOffset) {};
	T* operator->() const {
		if (HIndex == -1) return _Access();
		return &Organizer->GetMultiArray()[Organizer->GetHandleFromIndex(HIndex).offset + HOffset];
	}
	T* _Access() const {
		if (pool == nullptr) throw ArrayOrganizerException("Template not found; Invalid Reference", 9);
		return &pool->at(HOffset);
	}
	bool operator==(const Ref& other) const {
		return HIndex == other.HIndex && HOffset == other.HOffset;
	}
	bool operator==(std::nullptr_t) const {
		return HIndex == -1;
	}
	friend class Scene;
	friend class Instance;
};

template <typename T>
class UserRef {
	std::vector<Ref<T>>* vptr = nullptr;
	int index = -1;
public:
	UserRef() = default;
	UserRef(std::vector<Ref<T>>* _vptr, int _index) : vptr(_vptr), index(_index) {};

	int getIndex() const { return index; }
	void setIndex(int _index) const { index = _index; }
	Ref<Instance>* getRef() const { 
		if (vptr == nullptr) return nullptr;
		return &(vptr->at(index));
	}
	Ref<Instance>& getRef_Direct() const {
		return vptr->at(index);
	}

	T* operator->() const {
		if (vptr == nullptr) return nullptr;
		return vptr->at(index).operator->();
	}

	bool operator==(const UserRef& other) const {
		return index == other.getIndex();
	}
	bool operator==(std::nullptr_t) const {
		return vptr == nullptr;
	}
};