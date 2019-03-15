/** @file WFile.cpp
 *  @brief Wasabi file storage
 *
 *  Wasabi uses WFile's to store and load all engine assets.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#include "WFile.h"

const short FILE_MAGIC = 0x3DE0;

WFileAsset::WFileAsset() {
	m_assetId = -1;
	m_file = nullptr;
}

WFileAsset::~WFileAsset() {
	if (m_file)
		m_file->ReleaseAsset(m_assetId);
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
	m_fileSize = m_file.tellg();
	m_file.seekg(0);
	m_maxId = 0;

	// Read the headers
	WError err = WError(W_SUCCEEDED);
	if (m_fileSize == 0)
		CreateNewHeader();
	else
		err = LoadHeaders();

	return err;
}

void WFile::Close() {
	m_file.close();
	for (auto iter = m_assetsMap.begin(); iter != m_assetsMap.end(); iter++) {
		if (iter->second->loadedAsset) {
			iter->second->loadedAsset->m_assetId = -1;
			iter->second->loadedAsset->m_file = nullptr;
		}
	}
	m_assetsMap.clear();
}

WError WFile::SaveAsset(WFileAsset* asset, uint* assetId) {
	if (!m_file.is_open())
		return WError(W_FILENOTFOUND);

	if (assetId != nullptr) {
		auto iter = m_assetsMap.find(*assetId);
		if (iter != m_assetsMap.end())
			return WError(W_INVALIDPARAM); // already exists
	}

	uint newAssetId = assetId == nullptr ? m_maxId + 1 : *assetId;

	m_file.seekp(m_fileSize); // write it at the end
	std::streampos start = m_file.tellp();
	WError err = asset->SaveToStream(this, m_file);
	std::streampos writtenSize = m_file.tellp() - start;

	if (!err)
		return err;

	WriteAssetToHeader(WFile::FILE_ASSET(start, writtenSize, newAssetId));

	*assetId = newAssetId;

	return err;
}

WError WFile::LoadGenericAsset(uint assetId, WFileAsset** assetOut, std::function<WFileAsset* ()> createAsset) {
	if (!m_file.is_open())
		return WError(W_FILENOTFOUND);

	auto iter = m_assetsMap.find(assetId);
	if (iter == m_assetsMap.end())
		return WError(W_INVALIDPARAM); // no such asset exists

	WFile::FILE_ASSET* assetData = iter->second;

	WError err(W_SUCCEEDED);
	if (assetData->loadedAsset) {
		*assetOut = assetData->loadedAsset;
	} else {
		*assetOut = createAsset();
		m_file.seekg(assetData->start);
		err = (*assetOut)->LoadFromStream(this, m_file);
		if (err) {
			(*assetOut)->m_file = this;
			(*assetOut)->m_assetId = assetId;
			assetData->loadedAsset = (*assetOut);
		} else
			W_SAFE_DELETE((*assetOut)); // not using removeref because we assume that only 1 reference exists, this keeps it general and not require asset to be a WBase
	}
	return err;
}

void WFile::ReleaseAsset(uint assetId) {
	auto iter = m_assetsMap.find(assetId);
	if (iter != m_assetsMap.end())
		iter->second->loadedAsset = nullptr;
}

WError WFile::LoadHeaders() {
	std::streamoff curOffset = 0;

	while (curOffset != -1) {
		if (m_fileSize < curOffset + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize) + sizeof(FILE_HEADER::nextHeader))
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

		std::streamoff curDataOffset = header.dataStart;
		FILE_ASSET curAsset(0, 0, 0);
		while (curDataOffset < header.dataStart + header.dataSize) {
			m_file.read((char*)&curAsset, FILE_ASSET::GetSize());
			curDataOffset += FILE_ASSET::GetSize();
			if (curAsset.size == 0)
				break;
			header.assets.push_back(new FILE_ASSET(curAsset));
			m_assetsMap.insert(std::pair<uint, FILE_ASSET*>(curAsset.id, header.assets[header.assets.size() - 1]));
			m_maxId = max(m_maxId, curAsset.id);
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
	for (uint i = 0; i < header.dataSize; i++)
		m_file.put(0);
	m_fileSize = max(m_fileSize, header.dataStart + header.dataSize);

	if (m_headers.size() > 0) {
		m_file.seekp(m_headers[m_headers.size()-1].start + sizeof(FILE_MAGIC) + sizeof(FILE_HEADER::dataSize));
		m_file.write((char*)&header.start, sizeof(FILE_HEADER::nextHeader));
	}
	m_headers.push_back(header);
}

void WFile::WriteAssetToHeader(WFile::FILE_ASSET asset) {
	FILE_HEADER* curHeader = &m_headers[m_headers.size() - 1];
	m_file.seekp(curHeader->dataStart + curHeader->assets.size() * FILE_ASSET::GetSize());
	m_file.write((char*)&asset, FILE_ASSET::GetSize());
	curHeader->assets.push_back(new FILE_ASSET(asset));
	m_assetsMap.insert(std::pair<uint, FILE_ASSET*>(asset.id, curHeader->assets[curHeader->assets.size()-1]));

	m_fileSize = max(m_fileSize, asset.start + asset.size);
	m_maxId = max(m_maxId, asset.id);

	if (curHeader->assets.size() * FILE_ASSET::GetSize() >= curHeader->dataSize) {
		CreateNewHeader();
	}
}
