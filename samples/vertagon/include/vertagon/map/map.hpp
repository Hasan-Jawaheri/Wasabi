#pragma once

#include "vertagon/common.hpp"
#include "vertagon/map/platforms.hpp"
#include "vertagon/map/sky.hpp"

class Map {
    Wasabi* m_app;

    Sky m_sky;
    PlatformSpiral m_platforms;

public:
    Map(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    WVector3 GetSpawnPoint() const;
    float GetMinPoint() const;
    void RandomSpawn(WOrientation* object) const;
};
