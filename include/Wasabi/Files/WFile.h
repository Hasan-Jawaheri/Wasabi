/** @file WFile.h
 *  @brief Wasabi file storage
 *
 *  Wasabi uses WFile's to store and load all engine assets.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/Core/WCore.h"
#include <iostream>
#include <string>

#define W_MAX_ASSET_NAME_SIZE 128
#define W_MAX_ASSET_TYPE_SIZE 32

class WFileAsset : public WBase {
	friend class WFile;
public:
	WFileAsset(class Wasabi* const app, unsigned int ID = 0);
	virtual ~WFileAsset();

	virtual WError SaveToStream(class WFile* file, std::ostream& outputStream) = 0;
	virtual WError LoadFromStream(class WFile* file, std::istream& inputStream, vector<void*>& args, std::string nameSuffix) = 0;

private:
	class WFile* m_file;
};

class WFile {
	friend class WFileAsset;

public:
	WFile(Wasabi* const app);
	~WFile();

	WError Open(std::string filename);
	void Close();

	WError SaveAsset(class WFileAsset* asset);

	/**
	 * Loads an asset from the file given its name. If the asset with the given
	 * was already loaded, it will be immediately returned (and its refernce count
	 * will increase). If nameSuffix is not "" and the object hasn't been loaded
	 * before, then the object will be loaded into a new name and will not be
	 * saved and reused for the next LoadAsset call.
	 * Default assets, the ones that the engine internally creates, are never
	 * stored in the file, and when loaded, will just fetch the same engine assets
	 * and increase their reference count. Default assets ignore the nameSuffix
	 * parameter.
	 * @param name 
	 * @param loadedAsset
	 * @param args
	 * @param nameSuffix
	 */
	template<typename T>
	WError LoadAsset(std::string name, T** loadedAsset, std::vector<void*> args, std::string nameSuffix = "") {
		WFileAsset* asset = nullptr;
		auto iter = m_assetsMap.find(name);
		bool addReference = iter != m_assetsMap.end() && iter->second->loadedAsset != nullptr;
		WError err = LoadGenericAsset(name, &asset, [this]() { return new T(this->m_app); }, args, nameSuffix);
		if (loadedAsset)
			*loadedAsset = (T*)asset;
		if (asset) {
			if (addReference)
				asset->AddReference();
			asset->SetName(name + nameSuffix);
		}
		return err;
	}

	WError LoadGenericAsset(std::string name, WFileAsset** assetOut, std::function<WFileAsset* ()> createAsset, std::vector<void*> args, std::string nameSuffix);

	uint GetAssetsCount() const;
	/** Returns a pair <name, type> */
	std::pair<std::string, std::string> GetAssetInfo(uint index);
	/** Returns a pair <name, type> */
	std::pair<std::string, std::string> GetAssetInfo(std::string name);

private:
	struct FILE_ASSET {
		char name[W_MAX_ASSET_NAME_SIZE];
		char type[W_MAX_ASSET_TYPE_SIZE];
		std::streamoff start;
		std::streamsize size;
		WFileAsset* loadedAsset;

		FILE_ASSET(std::streamoff _start, std::streamsize _size, std::string _name, std::string _type)
			: start(_start), size(_size), loadedAsset(nullptr) {
			strcpy_s(name, W_MAX_ASSET_NAME_SIZE, _name.c_str());
			strcpy_s(type, W_MAX_ASSET_TYPE_SIZE, _type.c_str());
		}
		static size_t GetSize() { return sizeof(FILE_ASSET) - sizeof(WFileAsset*); }
	};

	struct FILE_HEADER {
		std::streamoff start;
		std::streamoff dataStart;
		std::streamsize dataSize;
		std::streamoff nextHeader;
		std::vector<FILE_ASSET*> assets;
	};

	class Wasabi* const m_app;

	std::fstream m_file;
	std::streamsize m_fileSize;
	std::unordered_map<std::string, FILE_ASSET*> m_assetsMap;
	std::unordered_map<WFileAsset*, FILE_ASSET*> m_loadedAssetsMap;
	std::vector<FILE_HEADER> m_headers;

	void ReleaseAsset(WFileAsset* asset);
	WError LoadHeaders(std::streamsize maxFileSize);
	void CreateNewHeader();
	void WriteAssetToHeader(WFile::FILE_ASSET asset);
};

class WFileManager {
	friend class WFile;

	std::unordered_map<std::string, WFileAsset*> m_defaultAssets;

public:
	WFileManager(class Wasabi* const app);
	~WFileManager();

	void AddDefaultAsset(std::string name, WFileAsset* asset);
};
