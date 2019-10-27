#include "vertagon/map/platforms.hpp"
#include "vertagon/game.hpp"

#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>


PlatformSpiral::PlatformSpiral(Wasabi* app) : m_app(app) {
    m_firstUpdate = -1.0f;

    m_texture = nullptr;
    m_params.numPlatforms = 12; // number of platforms to create
    m_params.platformLength = 140.0f; // the length of the 2D (top-down view) arc representing each platform
    m_params.distanceBetweenPlatforms = 10.0f; // distance (arc) to leave between platforms
    m_params.platformHeight = 2.0f; // height from the beginning of the platform to the end
    m_params.heightBetweenPlatforms = 15.0f; // height difference between the end of a platform and beginning of the next one
    m_params.towerRadius = 160.0f; // radius of the tower
    m_params.platformWidth = 40.0f;
    m_params.platformResWidth = 5;
    m_params.platformResLength = 20;
    m_params.xzRandomness = 3.0f;
    m_params.yRandomness = 1.0f;
    m_params.lengthRandomness = 60.0f;
}

WError PlatformSpiral::Load() {
    Cleanup();

    WError status = WError(W_SUCCEEDED);

    uint8_t pixels[2*2*4] = {
        76, 187, 27, 255,
        76, 187, 27, 255,
        155, 118, 83, 255,
        155, 118, 83, 255,
    };
    m_texture = m_app->ImageManager->CreateImage(static_cast<void*>(pixels), 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
    if (!m_texture)
        return WError(W_ERRORUNK);

    float curAngle = 0.0f;
    float curHeight = 0.0f;
    float anglePerGap = W_RADTODEG(m_params.distanceBetweenPlatforms / m_params.towerRadius); // angle occupied the gap between platforms
    for (uint32_t platform = 0; platform < m_params.numPlatforms && status; platform++) {
        float platformLength = m_params.platformLength - m_params.lengthRandomness / 2.0f + (m_params.lengthRandomness * (std::rand() % 10000) / 10000.0f);
        float platformAngle = W_RADTODEG(platformLength / m_params.towerRadius); // angle occupied by each platform
        status = BuildPlatform(curAngle, curAngle + platformAngle, curHeight, curHeight + m_params.platformHeight);
        curAngle += platformAngle + anglePerGap;
        curHeight += m_params.platformHeight + m_params.heightBetweenPlatforms;
    }

    return status;
}

WError PlatformSpiral::BuildPlatform(float angleFrom, float angleTo, float heightFrom, float heightTo) {
    WError status = WError(W_SUCCEEDED);
    Platform p;
    float midAngle = (angleTo + angleFrom) / 2.0f;
    p.center =
        (WVector3(cosf(W_DEGTORAD(midAngle)), 0.0f, sinf(W_DEGTORAD(midAngle))) * m_params.towerRadius) +
        WVector3(0.0f, (heightTo + heightFrom) / 2.0f, 0.0f);

    p.object = m_app->ObjectManager->CreateObject();
    p.geometry = new WGeometry(m_app);
    p.rigidBody = new WBulletRigidBody(m_app);

    if (!p.object) status = WError(W_ERRORUNK);

    if (status) {
        status = BuildPlatformGeometry(p.geometry, p.center, angleFrom, angleTo, heightFrom, heightTo);
        if (status) {
            status = p.object->SetGeometry(p.geometry);
            if (status) {
                p.object->SetName("Platform-" + std::to_string(heightFrom) + "-" + std::to_string(heightTo));

                p.object->GetMaterials().SetTexture("diffuseTexture", m_texture);

                status = p.rigidBody->Create(W_RIGID_BODY_CREATE_INFO::ForComplexGeometry(p.geometry, true, nullptr, p.center));
                if (status) {
                    p.rigidBody->BindObject(p.object, p.object);
                }
            }
        }
    }

    if (!status) {
        W_SAFE_REMOVEREF(p.geometry);
        W_SAFE_REMOVEREF(p.object);
        W_SAFE_REMOVEREF(p.rigidBody);
    } else {
        m_platforms.push_back(p);
        ComputePlatformCurrentCenter(m_platforms.size() - 1, 0.0f);
    }

    return status;
}

void PlatformSpiral::Cleanup() {
    W_SAFE_REMOVEREF(m_texture);
    for (auto platform : m_platforms) {
        W_SAFE_REMOVEREF(platform.geometry);
        W_SAFE_REMOVEREF(platform.rigidBody);
        W_SAFE_REMOVEREF(platform.object);
    }
}

void PlatformSpiral::Update(float fDeltaTime) {
    /**
     * Update the platforms
     */
    float time = m_app->Timer.GetElapsedTime() / 6.0f;
    if (m_firstUpdate < 0.0f)
        m_firstUpdate = time;
    time -= m_firstUpdate;
    for (uint32_t i = 0; i < m_platforms.size(); i++) {
        ComputePlatformCurrentCenter(i, time);
        m_platforms[i].rigidBody->SetPosition(m_platforms[i].curCenter);
    }
}

void PlatformSpiral::ComputePlatformCurrentCenter(uint32_t i, float time) {
    float phase = (float)i * 5.0f;
    float scale = 1.0f+ (float)i / (float)m_platforms.size();
    float magnitude = m_params.heightBetweenPlatforms * 0.4f;
    m_platforms[i].curCenter = m_platforms[i].center + WVector3(0.0f, std::sin(phase + time * scale) * magnitude, 0.0f);
}

WVector3 PlatformSpiral::GetSpawnPoint() const {
    return m_platforms[0].center + WVector3(0.0f, 5.0f, 0.0f);
}

void PlatformSpiral::RandomSpawn(WOrientation* object) const {
    WVector3 spawn = m_platforms[rand() % m_platforms.size()].curCenter + WVector3(0.0f, 5.0f, 0.0f);
    object->SetPosition(spawn);
}

WError PlatformSpiral::BuildPlatformGeometry(WGeometry* geometry, WVector3 center, float angleFrom, float angleTo, float heightFrom, float heightTo) {
    uint32_t xsegs = m_params.platformResWidth;
    uint32_t zsegs = m_params.platformResLength;
    float width = m_params.platformWidth;

    /**
     * Create a grid of triangles that is curved (using the radius of the tower)
     * with two parts: a top part and a bottom part. The top part is the platform
     * to walk on and the bottom part is to make it floating-island-like.
     * Multiply by 2 (one for top part one for bottom).
     */
    uint32_t numVertices = ((xsegs + 1) * (zsegs + 1) * 2 * 3) *2;
    uint32_t numIndices = numVertices;

    //allocate the plain vertices
    vector<WDefaultVertex> vertices(numVertices);
    vector<uint32_t> indices(vertices.size());

    std::vector<WVector3> randomValues((xsegs+2) * (zsegs+2));
    for (uint32_t i = 0; i < randomValues.size(); i++)
        randomValues[i] =
            WVector3(-m_params.xzRandomness/2.0f, -m_params.yRandomness/2.0f, -m_params.xzRandomness/2.0f) +
            WVector3(m_params.xzRandomness, m_params.yRandomness, m_params.xzRandomness) *
            WVector3(rand() % 10000, rand() % 10000, rand() % 10000) / 10000.0f;

    auto computeNormal = [&vertices](uint32_t v1, uint32_t v2, uint32_t v3) {
        WVector3 p1 = vertices[v1].pos;
        WVector3 p2 = vertices[v2].pos;
        WVector3 p3 = vertices[v3].pos;
        WVector3 U = p2 - p1;
        WVector3 V = p3 - p1;
        WVector3 norm = WVec3Normalize(WVector3(U.y * V.z - U.z * V.y, U.z * V.x - U.x * V.z, U.x * V.y - U.y * V.x));
        vertices[v1].norm = vertices[v2].norm = vertices[v3].norm = norm;
    };

    auto randomness = [&randomValues](uint32_t l, uint32_t w, uint32_t W) {
        return randomValues[w * W + l];
    };

    /**
     * Create the top part: a curved grid
     */
    uint32_t curVert = 0;
    for (uint32_t celll = 0; celll < zsegs + 1; celll++) {
        float _v1 = ((float)celll / (float)(zsegs+1));
        float _v2 = ((float)(celll+1) / (float)(zsegs+1));
        for (uint32_t cellw = 0; cellw < xsegs + 1; cellw++) {
            float _u1 = (float)(cellw) / (float)(xsegs+1);
            float _u2 = (float)(cellw+1) / (float)(xsegs+1);

            auto vtx = [this, &center, &angleFrom, &angleTo, &heightFrom, &heightTo, &width](float u, float v, WVector3 randomness) {
                auto xc = [this, &angleFrom, &angleTo, &width](float u, float v) {
                    return cosf(W_DEGTORAD(v * (angleTo - angleFrom) + angleFrom)) * (m_params.towerRadius - width / 2.0f + width * u);
                };
                auto yc = [&heightFrom, &heightTo, &width](float u, float v) {
                    float h = heightFrom + (heightTo - heightFrom) * v;
                    if (std::abs(u) < W_EPSILON || std::abs(u - 1.0f) < W_EPSILON || std::abs(v) < W_EPSILON || std::abs(v - 1.0f) < W_EPSILON)
                        h -= width / 10.0f;
                    return h;
                };
                auto zc = [this, &angleFrom, &angleTo, &width](float u, float v) {
                    return sinf(W_DEGTORAD(v * (angleTo - angleFrom) + angleFrom)) * (m_params.towerRadius - width / 2.0f + width * u);
                };

                WDefaultVertex vtx = WDefaultVertex(xc(u, v), yc(u, v), zc(u, v), 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, u, v);
                vtx.pos += randomness - center;
                return vtx;
            };

            vertices[curVert + 0] = vtx(_u1, _v1, randomness(celll+0, cellw+0, xsegs+1));
            vertices[curVert + 1] = vtx(_u1, _v2, randomness(celll+1, cellw+0, xsegs+1));
            vertices[curVert + 2] = vtx(_u2, _v2, randomness(celll+1, cellw+1, xsegs+1));

            vertices[curVert + 3] = vtx(_u1, _v1, randomness(celll+0, cellw+0, xsegs+1));
            vertices[curVert + 4] = vtx(_u2, _v2, randomness(celll+1, cellw+1, xsegs+1));
            vertices[curVert + 5] = vtx(_u2, _v1, randomness(celll+0, cellw+1, xsegs+1));

            // Those 2 vertices are outliers (they make flat triangles that look bad on edges), hide their triangles
            if (cellw == xsegs && celll == 0)
                vertices[curVert + 5].pos = WVector3(vertices[curVert + 1].pos.x, vertices[curVert + 5].pos.y, vertices[curVert + 1].pos.z);
            else if (cellw == 0 && celll == zsegs)
                vertices[curVert + 1].pos = WVector3(vertices[curVert + 5].pos.x, vertices[curVert + 1].pos.y, vertices[curVert + 5].pos.z);

            for (uint32_t i = 0; i < 6; i++)
                indices[curVert + i] = curVert + i;
            computeNormal(curVert + 0, curVert + 1, curVert + 2);
            computeNormal(curVert + 3, curVert + 4, curVert + 5);
            curVert += 6;
        }
    }

    /**
     * Create the bottom part: mirror of the top part but with a different height
     * (and has flipped indices)
     */
    float heightRandomness = 5.0f + 100.0f * m_params.yRandomness * (float)(std::rand() % 10000) / 10000.0f;
    for (; curVert < numVertices; curVert += 3) {
        for (uint32_t i = 0; i < 3; i++) {
            vertices[curVert + i] = vertices[curVert + i - numVertices / 2];
            indices[curVert + i] = curVert + 2 - i;

            // adjust the y component
            float u = vertices[curVert + i].texC.x;
            float v = vertices[curVert + i].texC.y;
            WVector2 dist = WVector2(0.5f - std::abs(u - 0.5f), 0.5f - std::abs(v - 0.5f)) * 2.0f;
            float distFromEdge = std::min(dist.x * ((float)m_params.platformResWidth * 3.0f / (float)m_params.platformResLength), dist.y); // between 0 (at edges) and 1 (in the center)
            vertices[curVert + i].pos.y -= 0.75f * sqrt(distFromEdge * width * heightRandomness);
            if (distFromEdge > W_EPSILON)
                vertices[curVert + i].pos.y -= width / 10.0f;
        }
        computeNormal(curVert + 2, curVert + 1, curVert + 0);
    }

    /**
     * Fix the UVs
     */
    for (uint32_t i = 0; i < numVertices; i++) {
        vertices[i].texC = WVector2((i < numVertices / 2) ? 0.1f : 0.9f, (i < numVertices / 2) ? 0.1f : 0.9f);
    }

    return geometry->CreateFromDefaultVerticesData(vertices, indices);
}