/** @file WFile.cpp
 *  @brief Wasabi file storage
 *
 *  Wasabi uses WFile's to store and load all engine assets.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#include "Wasabi/Files/WFile.h"

const short FILE_MAGIC = 0x3DE0;

WFileAsset::WFileAsset(Wasabi* app, uint32_t ID) : WBase(app, ID) {
	m_file = nullptr;
}

WFileAsset::~WFileAsset() {
	if (m_file)
		m_file->ReleaseAsset(this);
}

WFileManager::WFileManager(class Wasabi* const app) {
	UNREFERENCED_PARAMETER(app);
}

WFileManager::~WFileManager() {
}

void WFileManager::AddDefaultAsset(std::string name, WFileAsset* asset) {
	auto it = m_defaultAssets.find(name);
	if (it != m_defaultAssets.end())
		m_defaultAssets.erase(it);
	m_defaultAssets.insert(std::make_pair(name, asset));
}

WFile::WFile(Wasabi* const app) : m_app(app) {
}

WFile::~WFile() {
	Close();
}

WError WFile::Open(std::string filename) {
	Close();

	// Open file, and create one if it doesn't exist
	m_file.open(filename, ios::in | ios::out | ios::binary);
	if (!m_file.is_open()) {
		m_file.open(filename, ios::out | ios::binary);
		if (!m_file.is_open())
			return WError(W_FILENOTFOUND);
		m_file.close();
		m_file.open(filename, ios::in | ios::out | ios::binary);
		if (!m_file.is_open())
			return WError(W_FILENOTFOUND);
	}
	m_file.seekg(0, ios::end);
	std::streamsize maxFileSize = m_file.tellg();
	m_file.seekg(0);

	// Read the headers
	WError err = WError(W_SUCCEEDED);
	if (maxFileSize == 0) {
		m_fileSize = maxFileSize;
		CreateNewHeader();
	} else
		err = LoadHeaders(maxFileSize);

	return err;
}

void WFile::Close() {
	m_file.close();
	for (auto iter = m_loadedAssetsMap.begin(); iter != m_loadedAssetsMap.end(); iter++) {
		if (iter->second->loadedAsset) {
			iter->second->loadedAsset->m_file = nullptr;
		}
	}
	m_assetsMap.clear();
	m_loadedAssetsMap.clear();
	for (auto iter : m_headers)
		for (uint32_t i = 0; i < iter.assets.size(); i++)
			delete iter.assets[i];
	m_headers.clear();
	m_fileSize = 0;
}

WError WFile::SaveAsset(WFileAsset* asset) {
	std::string name = asset->GetName();
	std::string type = asset->GetTypeName();

	if (!m_file.is_open())
		return WError(W_FILENOTFOUND);

	auto defaultIter = m_app->FileManager->m_defaultAssets.find(name);
	if (defaultIter != m_app->FileManager->m_defaultAssets.end())
		return WError(W_SUCCEEDED);

	auto iter = m_loadedAssetsMap.find(asset);
	if (iter != m_loadedAssetsMap.end()) {
		auto nameIter = m_assetsMap.find(iter->second->name);
		if (nameIter == m_assetsMap.end())
			return WError(W_ERRORUNK);
		if (nameIter->second->loadedAsset != asset)
			return WError(W_NAMECONFLICT);
		return WError(W_SUCCEEDED);
	}

	std::streampos originalStart = m_file.tellp();
	m_fileSize = std::max((size_t)m_fileSize, (size_t)originalStart);
	m_file.seekp(m_fileSize); // write it at the end
	std::streampos start = m_file.tellp();
	WError status = asset->SaveToStream(this, m_file);
	std::streampos writtenSize = m_file.tellp() - start;

	if (status) {
		WriteAssetToHeader(FILE_ASSET(start, writtenSize, name, type));

		FILE_ASSET* assetData = m_assetsMap.find(name)->second;
		asset->m_file = this;
		assetData->loadedAsset = asset;
		m_loadedAssetsMap.insert(std::pair<WFileAsset*, FILE_ASSET*>(asset, assetData));
	}

	m_file.seekp(originalStart);

	return status;
}

WError WFile::LoadGenericAsset(std::string name, WFileAsset** assetOut, std::function<WFileAsset* ()> createAsset, std::vector<void*> args, std::string nameSuffix) {
	if (!m_file.is_open())
		return WError(W_FILENOTFOUND);

	auto defaultIter = m_app->FileManager->m_defaultAssets.find(name);
	if (defaultIter != m_app->FileManager->m_defaultAssets.end()) {
		*assetOut = defaultIter->second;
		defaultIter->second->AddReference();
		return WError(W_SUCCEEDED);
	}

	auto iter = m_assetsMap.find(name);
	if (iter == m_assetsMap.end())
		return WError(W_INVALIDPARAM); // no such asset exists

	WFile::FILE_ASSET* assetData = iter->second;

	WError err(W_SUCCEEDED);
	if (assetData->loadedAsset) {
		*assetOut = assetData->loadedAsset;
	} else {
		*assetOut = createAsset();
		m_file.seekg(assetData->start);
		err = (*assetOut)->LoadFromStream(this, m_file, args, nameSuffix);
		if (err) {
			if (nameSuffix == "") {
				(*assetOut)->m_file = this;
				assetData->loadedAsset = (*assetOut);
				m_loadedAssetsMap.insert(std::pair<WFileAsset*, FILE_ASSET*>(*assetOut, assetData));
			}
		} else
			W_SAFE_DELETE((*assetOut)); // not using removeref because we assume that only 1 reference exists, this keeps it general and not require asset to be a WBase
	}
	return err;
}

uint32_t WFile::GetAssetsCount() const {
	uint32_t count = 0;
	for (auto it : m_headers) {
		count += (uint32_t)it.assets.size();
	}
	return count;
}

std::pair<std::string, std::string> WFile::GetAssetInfo(uint32_t index) {
	for (auto it : m_headers) {
		if (index < it.assets.size())
			return std::make_pair(std::string(it.assets[index]->name), std::string(it.assets[index]->type));
		index -= (uint32_t)it.assets.size();
	}
	return std::make_pair(std::string(), std::string());
}

std::pair<std::string, std::string> WFile::GetAssetInfo(std::string name) {
	auto it = m_assetsMap.find(name);
	if (it != m_assetsMap.end())
		return std::make_pair(std::string(it->second->name), std::string(it->second->type));
	return std::make_pair(std::string(), std::string());
}

void WFile::ReleaseAsset(WFileAsset* asset) {
	auto iter = m_loadedAssetsMap.find(asset);
	if (iter != m_loadedAssetsMap.end()) {
		iter->second->loadedAsset = nullptr;
		m_loadedAssetsMap.erase(asset);
	}
}

WError WFile::LoadHeaders(std::streamsize maxFileSize) {
	std::streamoff curOffset = 0;

	while (curOffset != (std::streamoff)-1) {
		if (maxFileSize < (std::streamoff)(curOffset + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize) + sizeof(FILE_HEADER::nextHeader)))
			return WError(W_INVALIDFILEFORMAT);

		short magic;
		FILE_HEADER header;
		m_file.seekg(curOffset);
		header.start = curOffset;
		header.dataStart = curOffset + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize) + sizeof(FILE_HEADER::nextHeader);
		m_file.read((char*)&magic, sizeof(FILE_MAGIC));
		m_file.read((char*)&header.dataSize, sizeof(header.dataSize));
		m_file.read((char*)&header.nextHeader, sizeof(header.nextHeader));

		if (magic != FILE_MAGIC)
			return WError(W_INVALIDFILEFORMAT);

		m_fileSize = std::max((std::streamoff)m_fileSize, header.dataStart + header.dataSize);

		std::streamoff curDataOffset = header.dataStart;
		FILE_ASSET curAsset(0, 0, "", "");
		while (curDataOffset < header.dataStart + header.dataSize) {
			if (maxFileSize < (std::streamsize)(curDataOffset + FILE_ASSET::GetSize()))
				return WError(W_INVALIDFILEFORMAT);

			m_file.read((char*)&curAsset, FILE_ASSET::GetSize());
			curDataOffset += FILE_ASSET::GetSize();
			if (curAsset.size < 0)
				return WError(W_INVALIDFILEFORMAT);
			if (curAsset.size == 0)
				break;
			header.assets.push_back(new FILE_ASSET(curAsset));
			m_assetsMap.insert(std::pair<std::string, FILE_ASSET*>(curAsset.name, header.assets[header.assets.size() - 1]));
			m_fileSize = std::max((std::streamoff)m_fileSize, curAsset.start + curAsset.size);
		}

		m_headers.push_back(header);

		curOffset = header.nextHeader;
	}

	return WError(W_SUCCEEDED);
}

void WFile::CreateNewHeader() {
	FILE_HEADER header;
	m_file.seekp(m_fileSize);
	header.start = m_fileSize;
	header.dataStart = header.start + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize) + sizeof(FILE_HEADER::nextHeader);
	header.dataSize = 200 * FILE_ASSET::GetSize();
	header.nextHeader = -1;
	m_file.write((char*)&FILE_MAGIC, sizeof(FILE_MAGIC));
	m_file.write((char*)&header.dataSize, sizeof(header.dataSize));
	m_file.write((char*)&header.nextHeader, sizeof(header.nextHeader));
	for (uint32_t i = 0; i < header.dataSize; i++)
		m_file.put(0);
	m_fileSize = std::max((std::streamoff)m_fileSize, header.dataStart + header.dataSize);

	if (m_headers.size() > 0) {
		m_file.seekp(m_headers[m_headers.size()-1].start + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize));
		m_file.write((char*)&header.start, sizeof(FILE_HEADER::nextHeader));
	}
	m_headers.push_back(header);
}

void WFile::WriteAssetToHeader(FILE_ASSET asset) {
	FILE_HEADER* curHeader = &m_headers[m_headers.size() - 1];
	m_file.seekp(curHeader->dataStart + curHeader->assets.size() * FILE_ASSET::GetSize());
	m_file.write((char*)&asset, FILE_ASSET::GetSize());
	curHeader->assets.push_back(new FILE_ASSET(asset));
	m_assetsMap.insert(std::make_pair(asset.name, curHeader->assets[curHeader->assets.size()-1]));

	m_fileSize = std::max((std::streamoff)m_fileSize, asset.start + asset.size);

	if ((std::streamsize)(curHeader->assets.size() * FILE_ASSET::GetSize()) >= curHeader->dataSize) {
		CreateNewHeader();
	}
}
