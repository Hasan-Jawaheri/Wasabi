#pragma once

#include "vertagon/common.hpp"

class Map {
    Wasabi* m_app;

    WGeometry* m_skyGeometry;
    WEffect* m_skyEffect;
    WObject* m_sky;

    struct {
        uint32_t numPlatforms; // number of platforms to create
        float platformLength; // the length of the 2D (top-down view) arc representing each platform
        float distanceBetweenPlatforms; // distance (arc) to leave between platforms
        float platformHeight; // height from the beginning of the platform to the end
        float heightBetweenPlatforms; // height difference between the end of a platform and beginning of the next one
        float towerRadius; // radius of the tower
        float platformWidth; // width of a platform
        uint32_t platformResWidth; // platform segmentation on the width
        uint32_t platformResLength; // platform segmentation on the width
        float xzRandomness; // randomness of platform vertices on xz axis
        float yRandomness; // randomness of platform vertices on y axis
        float lengthRandomness; // randomness of platform length
    } m_towerParams;

    struct TOWER_PLATFORM {
        WGeometry* geometry;
        WRigidBody* rigidBody;
        WObject* object;
		WVector3 center;
        WVector3 curCenter;
    };
    float m_firstTowerUpdate;
    WImage* m_towerTexture;
    std::vector<TOWER_PLATFORM> m_tower;
    WError BuildTower();
    WError BuildTowerPlatform(float angleFrom, float angleTo, float heightFrom, float heightTo);
    WError BuildPlatformGeometry(WGeometry* geometry, WVector3 center, float angleFrom, float angleTo, float heightFrom, float heightTo);
    void ComputePlatformCurrentCenter(uint32_t i, float time);

public:
    Map(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    WVector3 GetSpawnPoint() const;
    float GetMinPoint() const;
    void RandomSpawn(WOrientation* object) const;
};
