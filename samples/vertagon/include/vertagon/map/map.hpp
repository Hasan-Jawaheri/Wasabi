#pragma once

#include "vertagon/common.hpp"
#include "vertagon/map/platforms.hpp"
#include "vertagon/map/sky.hpp"
#include "vertagon/map/water.hpp"

class Map {
    Vertagon* m_app;

    Sky m_sky;
    PlatformSpiral m_platforms;
    Water m_water;

public:
    Map(Vertagon* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    WVector3 GetSpawnPoint() const;
    float GetMinPoint() const;
    void RandomSpawn(WOrientation* object) const;
};
