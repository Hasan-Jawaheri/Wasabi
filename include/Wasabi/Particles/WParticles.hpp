/** @file WParticles.hpp
 *  @brief Particle systems in Wasabi
 *
 *  Particle systems in Wasabi are billboard-like geometries that are updated
 *  per frame to face the screen
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Materials/WMaterialsStore.hpp"

/** A vertex of a particle */
struct WParticlesVertex {
	/** local-space position of the single particle vertex */
	WVector3 pos;
};

/** An instance of a particle */
struct WParticlesInstance {
	/** Encoded WVP transformation matrix: (m11, m12, m13, m41) */
	WVector4 mat1;
	/** Encoded WVP transformation matrix: (m21, m22, m23, m42) */
	WVector4 mat2;
	/** Encoded WVP transformation matrix: (m31, m32, m33, m43) */
	WVector4 mat3;
	/**
	 * Encoded particle color (multiplied by texture color) and UVs:
	 * - The integer component of each float is rgba [0-255]
	 * - The floating point component of each float is (uv top-left, uv bottom-right) [0.01-0.99]
	 */
	WVector4 colorAndUVs;
	/** Size multiplier applied before any transformation (in local space) in the VS */
	float sizeLocalSpace;
	/** Size added to the x and y components of the particles in view space */
	float sizeViewSpace;
	/** Padding to be aligned to pixel size */
	float pad1;
	/** Padding to be aligned to pixel size */
	float pad2;

	/** Utility to encode the particle instance parameters into its fields */
	inline void SetParameters(const WMatrix& WVP, WColor color, WVector2 uvTopLeft, WVector2 uvBottomRight, float localSize, float viewSize);
};

/** Type of default particle effects supported by WParticlesManager */
enum W_DEFAULT_PARTICLE_EFFECT_TYPE: uint8_t {
	/** Default particles effect with a custom effect */
	W_DEFAULT_PARTICLES_CUSTOM = 0,
	/** Default particles effect with additive blending */
	W_DEFAULT_PARTICLES_ADDITIVE = 1,
	/** Default particles effect with alpha blending */
	W_DEFAULT_PARTICLES_ALPHA = 2,
	/** Default particles effect with subtractive blending */
	W_DEFAULT_PARTICLES_SUBTRACTIVE = 3,
};

/**
 * This class can be derived to implement custom behavior for particles.
 * Custom behavior includes rules for emission, motion, growth, etc...
 * The typename T must have identical bytes to a WParticlesVertex at its
 * beginning.
 */
class WParticlesBehavior {
	/** Maximum number of particles that the m_buffer can hold */
	uint32_t m_maxParticles;
	/** Size of each particle in m_buffer */
	uint32_t m_particleSize;
	/** Number of particles currently active */
	uint32_t m_numParticles;
	/** Buffer to store particles data (per-particle, not vertex) */
	void* m_particlesData;

protected:
	/**
	 * Emits a particle if not at the limit. Emittion means placing the
	 * particle data in m_buffer and incrementing m_numParticles.
	 * @param particle  Particle to emit
	 */
	void Emit(void* particle);

public:
	/**
	 * @param maxParticles  Maximum number of particles that the target system
	 *                      can render
	 * @param particleSize  Size of each particle structure
	 */
	WParticlesBehavior(uint32_t maxParticles, uint32_t particleSize);

	virtual ~WParticlesBehavior();

	/**
	 * Called by the WParticles implementation to update the particles and fill
	 * in the particles buffer with instance data. The size of the buffer is
	 * m_maxParticles * sizeof(WParticlesInstance)
	 * @param curTime      The time elapsed since the beginning of the program
	 * @param buffer       Pointer to the buffer to be filled
	 * @param maxParticles Maximum number of particles in the vb
	 * @param worldMatrix  The world matrix of the particles system
	 * @param camera       The camera used to render the frame
	 * @return             Number of particles copied
	 */
	uint32_t UpdateAndCopyToBuffer(float curTime, void* buffer, uint32_t maxParticles, const WMatrix& worldMatrix, class WCamera* camera);

	/**
	 * @return The number of particles currently active (as as what
	 * WParticlesBehavior::UpdateAndCopyToBuffer() would return)
	 */
	uint32_t GetNumParticles() const;

	/**
	 * Must be implemented by a derived class to define the per-frame behavior
	 * of the particle system. Typically this should control emission by
	 * calling Emit() when a particle needs to be emitted.
	 * @param curTime     The time elapsed since the beginning of the program
	 * @param worldMatrix The world matrix of the camera rendering the particles
	 * @param camera      The camera used to render the frame
	 */
	virtual void UpdateSystem(float curTime, const WMatrix& worldMatrix, class WCamera* camera) = 0;

	/**
	 * Must be implemented by a derived class to define the per-frame behavior
	 * of a single particle.
	 * @param curTime        The time elapsed since the beginning of the program
	 * @param particleData   A pointer to the beginning of the vertices memory to
	 *                       fill. The memory will have sizeof(WParticlesInstance)
	 *                       bytes to be filled with the 4 vertices of the particle
	 *                       billboard. The output vertex must be in *VIEW SPACE*
	 * @param outputInstance A pointer to the buffer to be filled with the instance
	 *                       data for the particle particleData. The transformation
	 *                       matrix is used as-is to transform from local space to
	 *                       projection space
	 * @param worldMatrix    The world matrix of the particles system
	 * @param camera         The camera used to render the frame
	 * @return  True if the particle should die, false otherwise
	 */
	virtual bool UpdateParticle(float curTime, void* particleData, WParticlesInstance* outputInstance, const WMatrix& worldMatrix, class WCamera* camera) = 0;

	/**
	 * Must be implemented to return the minimum point in the bounding box of the
	 * particles in local space of the instances.
	 */
	virtual WVector3& GetMinPoint() = 0;

	/**
	 * Must be implemented to return the maximum point in the bounding box of the
	 * particles in local space of the instances.
	 */
	virtual WVector3& GetMaxPoint() = 0;
};

/**
 * Default particles behavior implementation, good for basic/starter particle
 * systems.
 */
class WDefaultParticleBehavior : public WParticlesBehavior {
	struct Particle {
		/** Spawn time of this particle */
		float spawnTime;
		/** UV coordinates of the top-left vertex */
		WVector2 UVTopLeft;
		/** UV coordinates of the bottom-right vertex */
		WVector2 UVBottomRight;
		/** Initial position of this particle */
		WVector3 initialPos;
		/** Velocity of this particle */
		WVector3 velocity;
	};

	/** Time of last emission */
	float m_lastEmit;
	WVector3 m_minPoint, m_maxPoint;

public:

	/** Type of the particle */
	enum Type: uint8_t {
		/** Particle is rendered such that it always faces the camera */
		BILLBOARD = 0,
		/** Particle is rendered to be facing up */
		NOVA = 1,
	};

	/** Position at which new particles spawn (default is (0,0,0)) */
	WVector3 m_emissionPosition;
	/** Dimensions of a cube at m_emissionPosition where particles randomly spawn
	    (default is (1, 1, 1)*/
	WVector3 m_emissionRandomness;
	/** Lifetime (in seconds) of each particle (default is 1.5) */
	float m_particleLife;
	/** A direction/velocity vector for spawned particles default is (0, 2, 0) */
	WVector3 m_particleSpawnVelocity;
	/** If set to true, initial particle velocity will not be m_particleSpawnVelocity, but instead
	    it will be calculated as the vector from m_emissionPosition to the spawn position (affected
		by m_emissionRandomness) with the speed being the length of m_particleSpawnVelocity */
	bool m_moveOutwards;
	/** Number of particles to emit per second (default is 20) */
	float m_emissionFrequency;
	/** Size of a particle at emission (default is 1) */
	float m_emissionSize;
	/** Size of a particle at death (default is 3) */
	float m_deathSize;
	/** Type of the particle (default is BILLBOARD) */
	WDefaultParticleBehavior::Type m_type;
	/** Number of columns in the texture if it is tiled (otherwise 1, which is the default) */
	uint32_t m_numTilesColumns;
	/** Total number of tiles in the texture (default is 1) */
	uint32_t m_numTiles;
	/** An array of (color, time) where color is the color of the particle (multiplied by the texture)
	    at a given time (starting from 0) and lasting for 'time'. The sum of the time element
		in the vector elements should be equal to 1, where 1 is the time of the particle's death.
		(default is {<(1,1,1,0), 0.2>, <(1,1,1,1), 0.8>, <(1,1,1,0), 0.0>} */
	std::vector<std::pair<WColor, float>> m_colorGradient;

	WDefaultParticleBehavior(uint32_t maxParticles);
	virtual void UpdateSystem(float curTime, const WMatrix& worldMatrix, class WCamera* camera) override;
	virtual inline bool UpdateParticle(float curTime, void* particleData, WParticlesInstance* outputInstance, const WMatrix& worldMatrix, class WCamera* camera) override;
	virtual inline WVector3& GetMinPoint() override;
	virtual inline WVector3& GetMaxPoint() override;
};

/**
 * @ingroup engineclass
 * This represents a particles system and is responsible for animating and
 * rendering it.
 */
class WParticles : public WBase, public WOrientation, public WMaterialsStore {
	friend class WParticlesManager;

protected:
	virtual ~WParticles();
	WParticles(class Wasabi* const app, W_DEFAULT_PARTICLE_EFFECT_TYPE type, uint32_t ID = 0);

public:
	/**
	 * Returns "Particles" string.
	 * @return Returns "Particles" string
	 */
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	/**
	 * Initializes the particle system. This must be called for a WParticles
	 * object to be Valid().
	 * @param maxParticles  Maximum number of particles that the system can
	 *                      render simultaneously. Pass 0 to free resources
	 *                      and uninitialize the system
	 * @param behavior      Behavior object for this system, pass nullptr to
	 *                      use WDefaultParticleBehavior
	 * @return Error code, see WError.h
	 */
	WError Create(uint32_t maxParticles = 5000, WParticlesBehavior* behavior = nullptr);

	/**
	 * Retrieves the behavior object for this particle system.
	 * @return  The particle system's behavior
	 */
	WParticlesBehavior* GetBehavior() const;

	/**
	 * Checks whether a call to Render() will cause any rendering (draw call) to
	 * happen.
	 */
	bool WillRender(class WRenderTarget* rt);

	/**
	 * Renders the particle system to the given render target. If a material is
	 * provided, the following variables will be set:
	 * * "worldMatrix": World matrix of the particles
	 * * "viewMatrix": View matrix of the camera of rt
	 * * "projectionMatrix": Projection matrix of the camera of rt
	 * @params rt      Render target to render to
	 * @param material Material to set its variables and bind (if provided)
	 */
	void Render(class WRenderTarget* rt, class WMaterial* material = nullptr);

	/**
	 * Sets the priority of the particles. Particles with higher priority will render
	 * on top of those with lower priority (i.e. renders after after).
	 * @param priority New priority to set
	 */
	void SetPriority(uint32_t priority);

	/**
	 * Retrieves the current priority of the sprite. See SetPriority().
	 * @return The current priority of the sprite
	 */
	uint32_t GetPriority() const;

	/**
	 * @return the effect type of this particles system
	 */
	W_DEFAULT_PARTICLE_EFFECT_TYPE GetEffectType() const;

	/**
	 * Shows the particle system so that it can be rendered.
	 */
	void Show();

	/**
	 * Hides the particle system.
	 */
	void Hide();

	/**
	 * Checks if the particle system is hidden.
	 * @return true if the particle system is hidden, false otherwise
	 */
	bool Hidden() const;

	/**
	 * Enables frustum culling. Frustum culling causes the particle system to
	 * only be rendered if part of its geometry is within the viewing frustum
	 * of the camera.
	 */
	void EnableFrustumCulling();

	/**
	 * Disables frustum culling, see EnableFrustumCulling() for more info.
	 */
	void DisableFrustumCulling();

	/**
	 * Checks if the particle system appears anywhere in the view of the camera
	 * @param  cam Camera to check against
	 * @return     true if the particle system is in the viewing frustum of
	 *             cam, false otherwise
	 */
	bool InCameraView(class WCamera* cam);

	/**
	 * Retrieves the world matrix for the particle system.
	 * @return World matrix for the particle system
	 */
	WMatrix GetWorldMatrix();

	/**
	 * Updates the world matrix of the particle system.
	 * @return true if the matrix has changed, false otherwise
	 */
	bool UpdateLocals();

	/**
	 * @return Whether or not the particles system will render properly
	 */
	virtual bool Valid() const override;

	virtual void OnStateChange(STATE_CHANGE_TYPE type) override;

private:
	/** Type of tyhis particle system */
	W_DEFAULT_PARTICLE_EFFECT_TYPE m_effectType;
	/** true if the world matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** true if the particle system is hidden, false otherwise */
	bool m_hidden;
	/** true if frustum culling is enabled, false otherwise */
	bool m_bFrustumCull;
	/** Maximum number of particles that can be rendered by this system */
	uint32_t m_maxParticles;
	/** Rendering priority */
	uint32_t m_priority;
	/** Local world matrix */
	WMatrix m_WorldM;

	/** Behavior of the particle system */
	WParticlesBehavior* m_behavior;
	/** Texture containing instancing data */
	class WImage* m_instancesTexture;

	/**
	 * Destroys all resources held by this particles system
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WParticles.
 */
class WParticlesManager : public WManager<WParticles> {
	friend class WParticles;

	/**
	 * Returns "Particles" string.
	 * @return Returns "Particles" string
	 */
	virtual std::string GetTypeName() const;

public:
	WParticlesManager(class Wasabi* const app);
	~WParticlesManager();

	/**
	 * Loads the manager.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Updates all the particle systems
	 */
	void Update(WRenderTarget* rt);

	/**
	 * Creates a WEffect that can be used to render particles
	 * @return Newly allocated particle systems effect
	 */
	class WEffect* CreateParticlesEffect(W_DEFAULT_PARTICLE_EFFECT_TYPE type) const;

	/**
	 * Allocates and initializes a new particles system.
	 * @param type         Type of the new particles system
	 * @param maxParticles Maximum number of particles that the system can
	 *                     render simultaneously. Pass 0 to free resources
	 *                     and uninitialize the system
	 * @param behavior     Behavior object for this system, pass nullptr to
	 *                     use WDefaultParticleBehavior
	 * @param ID           ID of the new particles system
	 * @return Newly allocated particle system
	 */
	WParticles* CreateParticles(W_DEFAULT_PARTICLE_EFFECT_TYPE type, uint32_t maxParticles = 5000, WParticlesBehavior* behavior = nullptr, uint32_t ID = 0) const;

private:
	/** Default vertex shader used by the default effect */
	class WShader* m_vertexShader;
	/** Default fragment shader used by the default effect */
	class WShader* m_fragmentShader;
	/** Geometry of a plain used to render a single particle */
	class WGeometry* m_plainGeometry;
};
