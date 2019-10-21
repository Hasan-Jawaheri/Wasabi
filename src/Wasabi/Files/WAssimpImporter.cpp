/** @file WFile.cpp
 *  @brief Wasabi file storage
 *
 *  Wasabi uses WFile's to store and load all engine assets.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Wasabi/Files/WAssimpImporter.hpp"
#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Geometries/WGeometry.hpp"
#include "Wasabi/Images/WImage.hpp"
#include "Wasabi/Lights/WLight.hpp"
#include "Wasabi/Cameras/WCamera.hpp"


WMatrix AssimpToWMatrix(aiMatrix4x4 mtx) {
	return WMatrix(
		mtx.a1, mtx.a2, mtx.a3, mtx.a4,
		mtx.b1, mtx.b2, mtx.b3, mtx.b4,
		mtx.c1, mtx.c2, mtx.c3, mtx.c4,
		mtx.d1, mtx.d2, mtx.d3, mtx.d4
	);
}


WAssimpImporter::WAssimpImporter(Wasabi* const app) : m_app(app) {
}

WAssimpImporter::~WAssimpImporter() {
}

WError WAssimpImporter::LoadSingleObject(std::string filename, WObject*& object) {
	Assimp::Importer importer;
	WError status = WError(W_SUCCEEDED);

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType
	);

	// If the import failed, report it
	if (!scene) {
		return WError(W_FILENOTFOUND);
	}

	uint32_t maxTextures = 8; // maximum number of textures per material
	uint32_t totalVertexCount = 0;
	uint32_t totalIndexCount = 0;
	for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		totalVertexCount += mesh->mNumVertices;
		totalIndexCount += mesh->mNumFaces * 3;
	}

	WDefaultVertex* allVertices = new WDefaultVertex[totalVertexCount];
	uint32_t* allIndices = new uint32_t[totalIndexCount];

	// zero out the vertices
	memset(allVertices, 0, sizeof(WDefaultVertex) * totalVertexCount);

	uint32_t curVertexOffset = 0;
	uint32_t curIndexOffset = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes && status; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		WDefaultVertex* vertices = &allVertices[curVertexOffset];
		uint32_t* indices = &allIndices[curIndexOffset];

		uint32_t face;
		for (face = 0; face < mesh->mNumFaces; face++) {
			if (mesh->mFaces[face].mNumIndices != 3)
				break;
			indices[face * 3 + 0] = mesh->mFaces[face].mIndices[0];
			indices[face * 3 + 1] = mesh->mFaces[face].mIndices[1];
			indices[face * 3 + 2] = mesh->mFaces[face].mIndices[2];

			if (mesh->HasPositions()) {
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
					vertices[i].pos = WVector3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			}

			if (mesh->HasTangentsAndBitangents()) {
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
					vertices[i].tang = WVector3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			}

			if (mesh->HasNormals()) {
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
					vertices[i].norm = WVector3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			if (mesh->HasTextureCoords(0)) {
				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
					vertices[i].texC = WVector2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}

			for (uint32_t i = 0; i < mesh->mNumVertices; i++)
				vertices[i].textureIndex = mesh->mMaterialIndex < maxTextures ? mesh->mMaterialIndex : 0;
		}

		if (face != mesh->mNumFaces)
			status = WError(W_INVALIDFILEFORMAT);

		curVertexOffset += mesh->mNumVertices;
		curIndexOffset += mesh->mNumFaces * 3;
	}

	scene->mMaterials[0]->GetTextureCount(aiTextureType_DIFFUSE);

	WGeometry* geometry = nullptr;
	if (status) {
		geometry = new WGeometry(m_app);
		status = geometry->CreateFromData(static_cast<void*>(allVertices), totalVertexCount, static_cast<void*>(allIndices), totalIndexCount);
	}

	W_SAFE_DELETE_ARRAY(allVertices);
	W_SAFE_DELETE_ARRAY(allIndices);

	if (status) {
		object = m_app->ObjectManager->CreateObject();
		status = object->SetGeometry(geometry);
	}

	std::vector<WImage*> textures;
	if (status) {
		for (uint32_t mat = 0; mat < std::min(scene->mNumMaterials, maxTextures); mat++) {
			aiMaterial* material = scene->mMaterials[mat];
			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
				aiString textureFilename;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilename, nullptr, nullptr, nullptr, nullptr, nullptr);
				if (textureFilename.length > 0 && textureFilename.data[0] != '*') {
					WImage* texture = new WImage(m_app);
					WError textureStatus = texture->Load(textureFilename.C_Str());
					if (textureStatus) {
						textureStatus = object->GetMaterials().SetTexture("diffuseTexture", texture, mat);
					}
					if (!textureStatus) {
						W_SAFE_REMOVEREF(texture);
					} else
						textures.push_back(texture);
				}
			}
		}
	}

	if (!status) {
		W_SAFE_REMOVEREF(geometry);
		W_SAFE_REMOVEREF(object);
		for (auto texture : textures)
			W_SAFE_REMOVEREF(texture);
	}
	return status;
}

WError WAssimpImporter::LoadScene(
    std::string filename,
    std::vector<WObject*>& objects,
    std::vector<WImage*>& textures,
    std::vector<WGeometry*>& geometries,
    std::vector<WLight*> lights,
	std::vector<WCamera*> cameras
) {
	UNREFERENCED_PARAMETER(objects);
	UNREFERENCED_PARAMETER(textures);
	UNREFERENCED_PARAMETER(geometries);
	UNREFERENCED_PARAMETER(lights);

	Assimp::Importer importer;
	WError status = WError(W_SUCCEEDED);

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType
	);

	if (status) {
		for (uint32_t i = 0; i < scene->mNumCameras; i++) {
			aiCamera* camera = scene->mCameras[i];
			WCamera* wcam = new WCamera(m_app);
			std::string name = camera->mName.C_Str();
			while (m_app->CameraManager->GetEntity(name))
				name += "~";
			wcam->SetName(name);
			wcam->SetFOV(camera->mHorizontalFOV);
			wcam->SetRange(camera->mClipPlaneNear, camera->mClipPlaneFar);
			aiMatrix4x4 mtx;
			camera->GetCameraMatrix(mtx);
			wcam->SetToTransformation(WMatrixInverse(AssimpToWMatrix(mtx)));
		}
	}

	return status;
}
