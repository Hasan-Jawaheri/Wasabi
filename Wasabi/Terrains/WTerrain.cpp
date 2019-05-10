#include "WTerrain.h"
#include "../Cameras/WCamera.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WMaterial.h"
#include "../Materials/WEffect.h"

WTerrainManager::WTerrainManager(class Wasabi* const app)
	: WManager<WTerrain>(app) {
}

WTerrainManager::~WTerrainManager() {
}

std::string WTerrainManager::GetTypeName() const {
	return "Terrain";
}

WError WTerrainManager::Load() {
	return WError(W_SUCCEEDED);
}

WTerrain* WTerrainManager::CreateTerrain(unsigned int N, float size, unsigned int numRings, unsigned int ID) {
	WTerrain* terrain = new WTerrain(m_app, ID);
	WError err = terrain->Create(N, size, numRings);
	if (!err)
		W_SAFE_REMOVEREF(terrain);
	return terrain;
}

WTerrain::WTerrain(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_bAltered = true;

	m_WorldM = WMatrix();

	m_MxMGeometry = nullptr;
	m_Mx3Geometry = nullptr;
	m_2Mp1Geometry = nullptr;
	m_instanceTexture = nullptr;
	m_heightTexture = nullptr;

	m_viewpoint = WVector3(0.0f, 0.0f, 0.0f);
	m_LOD = 7;

	app->TerrainManager->AddEntity(this);
}

WTerrain::~WTerrain() {
	_DestroyResources();

	m_app->TerrainManager->RemoveEntity(this);
}

std::string WTerrain::_GetTypeName() {
	return "Terrain";
}

std::string WTerrain::GetTypeName() const {
	return _GetTypeName();
}

void WTerrain::_DestroyResources() {
	W_SAFE_REMOVEREF(m_MxMGeometry);
	W_SAFE_REMOVEREF(m_Mx3Geometry);
	W_SAFE_REMOVEREF(m_2Mp1Geometry);
	W_SAFE_REMOVEREF(m_instanceTexture);
	W_SAFE_REMOVEREF(m_heightTexture);
	for (auto pieceTypeIt : m_pieces)
		for (auto pieceIt : pieceTypeIt.second)
			delete pieceIt;
	for (auto ringIt : m_rings)
		delete ringIt;
	m_rings.clear();
	m_pieces.clear();
}

bool WTerrain::Valid() const {
	return m_MxMGeometry && m_Mx3Geometry && m_2Mp1Geometry && m_instanceTexture;
}

void WTerrain::Show() {
	m_hidden = false;
}

void WTerrain::Hide() {
	m_hidden = true;
}

bool WTerrain::Hidden() const {
	return m_hidden;
}

WError WTerrain::Create(unsigned int N, float size, unsigned int numRings) {
	if (N > 1 && ((N+1) & N) == 0) // check (power of 2) - 1
		return WError(W_INVALIDPARAM);
	if (size <= 0)
		return WError(W_INVALIDPARAM);
	_DestroyResources();

	N = 15;
	int M = (N + 1) / 4;
	size = 2;
	//m_viewpoint = WVector3(10, 0, 0);

	m_MxMGeometry = new WGeometry(m_app);
	WError err = m_MxMGeometry->CreatePlain((M-1) * size, M - 2, M - 2);
	if (err) {
		m_Mx3Geometry = new WGeometry(m_app);
		err = m_Mx3Geometry->CreateRectanglePlain(2 * size, (M-1) * size, 1, M - 2);
		if (err) {
			m_2Mp1Geometry = new WGeometry(m_app);
			err = m_2Mp1Geometry->CreateRectanglePlain(1 * size, (2 * M) * size, 0, 2 * M - 1);
			if (err) {
				m_instanceTexture = new WImage(m_app);
				err = m_instanceTexture->CreateFromPixelsArray(nullptr, 64, 64, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_REWRITE_EVERY_FRAME);
				if (err) {
					m_heightTexture = new WImage(m_app);
					err = m_heightTexture->CreateFromPixelsArray(nullptr, N + 1, N + 1, 1, VK_FORMAT_R32_SFLOAT, numRings + 1, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_REWRITE_EVERY_FRAME);
				}
			}
		}
	}

	if (!err)
		_DestroyResources();
	else {
		m_N = N;
		m_M = M;
		m_size = size;
		m_LOD = numRings + 1;

		for (int i = 0; i < m_LOD; i++) {
			LODRing* ring = new LODRing();
			ring->level = i;

			float unitSize = max(pow(2, i - 1), 1) * m_size;
			float levelSize = unitSize * (m_N - 1);
			float Msize = unitSize * (m_M - 1);
			float halfM = Msize / 2.0f;
			WVector2 bottomLeft = -WVector2(levelSize, levelSize) / 2.0f;
			if (i == 0)
				bottomLeft = -WVector2(levelSize, levelSize) / 4.0f + WVector2(unitSize, unitSize) / 2.0f;
			WVector2 topRight = -bottomLeft;
			WVector2 bottomRight = WVector2(topRight.x, bottomLeft.y);
			WVector2 topLeft = WVector2(bottomLeft.x, topRight.y);

			ring->center = WVector2(0.0f, 0.0f);

			if (i > 0) {
				// 2 strips for the L-shape (bias)
				ring->pieces.push_back(new RingPiece(ring, m_2Mp1Geometry, 0.0f, WVector2(0.0f, 0.0f)));
				ring->pieces.push_back(new RingPiece(ring, m_2Mp1Geometry, 1.0f, WVector2(0.0f, 0.0f)));

				// 4 sets of 3 MxM pieces, 1 set on each corner
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomLeft + WVector2(halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomLeft + WVector2(Msize + halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomLeft + WVector2(halfM, Msize + halfM)));

				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topRight + WVector2(-halfM, -halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topRight + WVector2(-Msize - halfM, -halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topRight + WVector2(-halfM, -Msize - halfM)));

				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomRight + WVector2(-halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomRight + WVector2(-Msize - halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomRight + WVector2(-halfM, Msize + halfM)));

				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topLeft + WVector2(halfM, -halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topLeft + WVector2(Msize + halfM, -halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topLeft + WVector2(halfM, -Msize - halfM)));

				// 4 2xM connectors, one on each side
				ring->pieces.push_back(new RingPiece(ring, m_Mx3Geometry, 0.0f, WVector2(0.0f, bottomLeft.y + halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_Mx3Geometry, 0.0f, WVector2(0.0f, topLeft.y - halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_Mx3Geometry, 1.0f, WVector2(bottomLeft.x + halfM, 0.0f)));
				ring->pieces.push_back(new RingPiece(ring, m_Mx3Geometry, 1.0f, WVector2(bottomRight.x - halfM, 0.0f)));
			} else {
				// 2 strips for the L-shape (bias)
				ring->pieces.push_back(new RingPiece(ring, m_2Mp1Geometry, 0.0f, WVector2(0.0f, 0.0f)));
				ring->pieces.push_back(new RingPiece(ring, m_2Mp1Geometry, 1.0f, WVector2(0.0f, 0.0f)));

				// 4 MxM pieces in the center
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomLeft + WVector2(halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topRight + WVector2(-halfM, -halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, bottomRight + WVector2(-halfM, halfM)));
				ring->pieces.push_back(new RingPiece(ring, m_MxMGeometry, 0.0f, topLeft + WVector2(halfM, -halfM)));
			}

			m_rings.push_back(ring);
		}

		for (auto ringIt : m_rings) {
			for (auto pieceIt : ringIt->pieces) {
				auto piecesIt = m_pieces.find(pieceIt->geometry);
				if (piecesIt == m_pieces.end())
					m_pieces.insert(std::make_pair(pieceIt->geometry, std::vector<RingPiece*>()));
				m_pieces[pieceIt->geometry].push_back(pieceIt);
			}
		}
	}

	return err;
}

void WTerrain::SetViewpoint(WVector3 point) {
	m_viewpoint = point;
}

bool WTerrain::WillRender(WRenderTarget* rt) {
	if (Valid() && !m_hidden) {
		/*WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return false;
		}*/
		return true;
	}
	return false;
}

void WTerrain::Render(class WRenderTarget* const rt, WMaterial* material) {
	if (!material)
		return;

	WMatrix worldM = GetWorldMatrix();
	material->SetVariableMatrix("worldMatrix", worldM);
	material->SetTexture("instancingTexture", m_instanceTexture);
	material->SetTexture("heightTexture", m_heightTexture);
	material->Bind(rt, true, false);

	for (int i = m_LOD - 1; i >= 0; i--) {
		LODRing* ring = m_rings[i];
		float unitSize = max(pow(2, i - 1), 1) * m_size;
		float levelSize = unitSize * (m_N - 1);
		float Msize = unitSize * (m_M - 1);
		if (i == m_LOD - 1) {
			// when there is more than a unit size difference, align to half unit size
			float halfUnitSize = unitSize / 2.0f;
			if (abs(m_viewpoint.x - ring->center.x) >= unitSize)
				ring->center.x = (float)(int)(m_viewpoint.x / halfUnitSize) * halfUnitSize;
			if (abs(m_viewpoint.z - ring->center.y) >= unitSize)
				ring->center.y = (float)(int)(m_viewpoint.z / halfUnitSize) * halfUnitSize;
		} else {
			WVector2 bias = i == 0 ? WVector2(0.0f, 0.0f) : WVector2(
				m_rings[i + 1]->center.x > m_viewpoint.x ? -1 : 1,
				m_rings[i + 1]->center.y > m_viewpoint.z ? -1 : 1
			);
			ring->center = m_rings[i + 1]->center + bias * unitSize;

			// given the bias, we can now set the parent ring's L-shape, which is the first 2 pieces
			if (i == 0) {
				m_rings[i]->pieces[0]->offsetFromCenter =     WVector2(-levelSize / 4.0f, 0.0f);
				m_rings[i]->pieces[1]->offsetFromCenter =     WVector2(0.0f, -levelSize / 4.0f);
				m_rings[i + 1]->pieces[0]->offsetFromCenter = WVector2(levelSize / 4.0f, 0.0f);
				m_rings[i + 1]->pieces[1]->offsetFromCenter = WVector2(0.0f, levelSize / 4.0f);
			} else {
				m_rings[i + 1]->pieces[0]->offsetFromCenter = WVector2(-bias.x * (levelSize / 2.0f), 0.0f);
				m_rings[i + 1]->pieces[1]->offsetFromCenter = WVector2(0.0f, -bias.y * (levelSize / 2.0f));
			}
		}
	}

	uint numGeometryTypes = m_pieces.size();

	WColor* pixels;
	m_instanceTexture->MapPixels((void**)& pixels, W_MAP_WRITE);
	// first m_LOD floats of the texture are for level data
	for (int i = 0; i < m_LOD; i++) {
		WColor levelData = WColor(
			m_rings[i]->center.x, // level center x
			m_rings[i]->center.y, // level center y
			m_N, // N
			m_size // terrain scale
		);
		pixels[i] = levelData;
	}

	pixels += m_LOD;
	for (auto it : m_pieces) {
		for (uint i = 0; i < it.second.size(); i++) {
			RingPiece* piece = it.second[i];
			WColor instanceData = WColor(
				piece->offsetFromCenter.x + piece->ring->center.x,
				piece->offsetFromCenter.y + piece->ring->center.y,
				piece->ring->level + 0.01f,
				piece->orientation
			);
			pixels[i] = instanceData;
		}
		pixels += it.second.size();
	}
	m_instanceTexture->UnmapPixels();

	uint totalNumPieces = m_LOD; // start from m_LOD since the first <m_LOD> pixels are for level data
	for (auto it : m_pieces) {
		WGeometry* geometry = it.first;
		uint numPieces = it.second.size();
		material->SetVariableInt("geometryOffsetInTexture", totalNumPieces); // <-- push constant
		material->Bind(rt, false, true);
		geometry->Draw(rt, -1, numPieces, false);
		totalNumPieces += numPieces;
	}
}

float WTerrain::GetHeight(WVector2 pos2D) {
	return 0.0f;
}

WMatrix WTerrain::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WTerrain::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		m_WorldM = ComputeTransformation();

		return true;
	}

	return false;
}

void WTerrain::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}
