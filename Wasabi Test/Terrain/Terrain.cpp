#include "Terrain.hpp"

TerrainDemo::TerrainDemo(Wasabi* const app) : WTestState(app) {
	terrain = nullptr;
	player = nullptr;
	playerPos = WVector3(0, 0, 0);
}

void TerrainDemo::Load() {
	terrain = m_app->TerrainManager->CreateTerrain();

	player = m_app->ObjectManager->CreateObject();
	WGeometry* g = new WGeometry(m_app);
	g->CreateCube(1.0f);
	player->SetGeometry(g);
	g->RemoveReference();
}

void TerrainDemo::Update(float fDeltaTime) {
	if (m_app->WindowAndInputComponent->KeyDown('W'))
		playerPos.z += 10.0f * fDeltaTime;
	if (m_app->WindowAndInputComponent->KeyDown('S'))
		playerPos.z -= 10.0f * fDeltaTime;
	if (m_app->WindowAndInputComponent->KeyDown('A'))
		playerPos.x -= 10.0f * fDeltaTime;
	if (m_app->WindowAndInputComponent->KeyDown('D'))
		playerPos.x += 10.0f * fDeltaTime;

	if (terrain && player) {
		playerPos.y = terrain->GetHeight(WVector2(playerPos.x, playerPos.z));
		((WasabiTester*)m_app)->SetCameraPosition(playerPos);
		player->SetPosition(playerPos);
		//terrain->SetViewpoint(playerPos);
	}
}

void TerrainDemo::Cleanup() {
	W_SAFE_REMOVEREF(player);
	W_SAFE_REMOVEREF(terrain);
}