#pragma once

#include "vertagon/common.hpp"

struct Platform {
    WGeometry* geometry;
    WRigidBody* rigidBody;
    WObject* object;
    WVector3 center;
    WVector3 curCenter;
};

class PlatformSpiral {
    Wasabi* m_app;

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
    } m_params;

    float m_firstUpdate;
    WImage* m_texture;
    std::vector<Platform> m_platforms;

    WError BuildPlatform(float angleFrom, float angleTo, float heightFrom, float heightTo);
    WError BuildPlatformGeometry(WGeometry* geometry, WVector3 center, float angleFrom, float angleTo, float heightFrom, float heightTo);
    void ComputePlatformCurrentCenter(uint32_t i, float time);

public:
    PlatformSpiral(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    WVector3 GetSpawnPoint() const;
    void RandomSpawn(WOrientation* object) const;
};
