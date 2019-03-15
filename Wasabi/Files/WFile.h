/** @file WFile.h
 *  @brief Wasabi file storage
 *
 *  Wasabi uses WFile's to store and load all engine assets.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"
#include <iostream>
#include <string>

class WFileAsset {
	friend class WFile;

public:
	WFileAsset();
	virtual ~WFileAsset();

	virtual WError SaveToStream(class WFile* file, std::ostream& outputStream) = 0;
	virtual WError LoadFromStream(class WFile* file, std::istream& inputStream) = 0;

private:
	class WFile* m_file;

protected:
	void _MarkFileEnd(class WFile* file, std::streampos pos);
};

class WFile {
	friend class WFileAsset;

public:
	WFile(Wasabi* const app);
	~WFile();

	WError Open(std::string filename);
	void Close();

	WError SaveAsset(class WFileAsset* asset, uint* assetId);

	template<typename T>
	WError LoadAsset(uint assetId, T** loadedAsset) {
		WFileAsset* asset;
		auto iter = m_assetsMap.find(assetId);
		bool addReference = iter != m_assetsMap.end() && iter->second->loadedAsset != nullptr;
		WError err = LoadGenericAsset(assetId, &asset, [this]() { return new T(this->m_app); });
		*loadedAsset = (T*)asset;
		if (addReference)
			(*loadedAsset)->AddReference();
		return err;
	}
	WError LoadGenericAsset(uint assetId, WFileAsset** assetOut, std::function<WFileAsset* ()> createAsset);

private:
	struct FILE_ASSET {
		std::streamoff start;
		std::streamsize size;
		uint id;
		WFileAsset* loadedAsset;

		FILE_ASSET(std::streamoff _start, std::streamsize _size, uint _id)
			: start(_start), size(_size), id(_id), loadedAsset(nullptr) {}
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

	uint m_maxId;
	std::fstream m_file;
	std::streamsize m_fileSize;
	std::unordered_map<uint, FILE_ASSET*> m_assetsMap;
	std::unordered_map<WFileAsset*, FILE_ASSET*> m_loadedAssetsMap;
	std::vector<FILE_HEADER> m_headers;

	void MarkFileEnd(std::streampos pos);
	void ReleaseAsset(WFileAsset* asset);
	WError LoadHeaders(std::streamsize maxFileSize);
	void CreateNewHeader();
	void WriteAssetToHeader(WFile::FILE_ASSET asset);
};
