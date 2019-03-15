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
	uint m_assetId;
	class WFile* m_file;
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
		WError err = LoadGenericAsset(assetId, &asset, [this]() { return new T(this->m_app); });
		*loadedAsset = (T*)asset;
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
	std::vector<FILE_HEADER> m_headers;

	void ReleaseAsset(uint assetId);
	WError LoadHeaders();
	void CreateNewHeader();
	void WriteAssetToHeader(WFile::FILE_ASSET asset);
};
