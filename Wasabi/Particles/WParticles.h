/** @file WParticles.h
 *  @brief Particle systems in Wasabi
 *
 *  Particle systems in Wasabi are point-based geometries that render into
 *  quads (in a geometry shader) 
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/** A vertex of a particle */
struct WParticlesVertex {
	/** local-position of the single particle */
	WVector3 pos;
	/** particle size */
	WVector3 size;
	/** particle alpha */
	float alpha;
};

/** Type of default particle effects supported by WParticlesManager */
enum W_DEFAULT_PARTICLE_EFFECT_TYPE {
	/** Default particles effect with additive blending */
	W_DEFAULT_PARTICLES_ADDITIVE = 0,
	/** Default particles effect with alpha blending */
	W_DEFAULT_PARTICLES_ALPHA = 1,
	/** Default particles effect with subtractive blending */
	W_DEFAULT_PARTICLES_SUBTRACTIVE = 2,
};

/**
 * This class can be derived to implement custom behavior for particles.
 * Custom behavior includes rules for emission, motion, growth, etc...
 * The typename T must have identical bytes to a WParticlesVertex at its
 * beginning.
 */
class WParticlesBehavior {
	/** Maximum number of particles that the m_buffer can hold */
	unsigned int m_maxParticles;
	/** Size of each particle in m_buffer */
	unsigned int m_particleSize;
	/** Number of particles currently active */
	unsigned int m_numParticles;
	/** Buffer to store particles data */
	void* m_buffer;

protected:
	void Emit(void* particle);

public:
	/**
	 * @param max_particles  Maximum number of particles that the target system
	 *                       can render
	 * @param particle_size  Size of each particle structure
	 */
	WParticlesBehavior(unsigned int max_particles, unsigned int particle_size);

	~WParticlesBehavior();

	/**
	 * Called by the WParticles implementation to update the particles and fill
	 * in the particles vertex buffer. The size of the vertex buffer is
	 * m_maxParticles * sizeof(WParticlesVertex)
	 * @param curTime  The time elapsed since the beginning of the program
	 * @param vb       Pointer to the vertex buffer to be filled
	 * @return         Number of particles copied
	 */
	unsigned int UpdateAndCopyToVB(float cur_time, void* vb, unsigned int max_particles);

	/**
	 * Must be implemented by a derived class to define the per-frame behavior
	 * of the particle system. Typically this should control emission by
	 * calling Emit() when a particle needs to be emitted.
	 * @param cur_time The time elapsed since the beginning of the program
	 */
	virtual void UpdateSystem(float cur_time) = 0;

	/**
	 * Must be implemented by a derived class to define the per-frame behavior
	 * of a single particle.
	 * @param cur_time  The time elapsed since the beginning of the program
	 * @param particle  The particle to update
	 * @return  True if the particle should die, false otherwise
	 */
	virtual inline bool UpdateParticle(float cur_time, void* particle) = 0;
};

/**
 * Default particles behavior implementation, good for basic/starter particle
 * systems.
 */
class WDefaultParticleBehavior : public WParticlesBehavior {
	struct Particle {
		/** Required WParticleVertex at the beginning of the data structure */
		WParticlesVertex vtx;
		/** Spawn time of this particle */
		float spawnTime;
		/** Initial position of this particle */
		WVector3 initialPos;
		/** Velocity of this particle */
		WVector3 velocity;
	};

	/** Time of last emission */
	float m_lastEmit;

public:

	/** Type of the particle */
	enum Type {
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
	/** Lifetime (in seconds) of each particle (default is 3) */
	float m_particleLife;
	/** A direction/velocity vector for spawned particles default is (0, 2, 0) */
	WVector3 m_particleSpawnVelocity;
	/** Number of particles to emit per second (default is 20) */
	float m_emissionFrequency;
	/** Size of a particle at emission (default is 1) */
	float m_emissionSize;
	/** Size of a particle at death (default is 3) */
	float m_deathSize;
	/** Type of the particle (default is BILLBOARD) */
	Type m_type;

	WDefaultParticleBehavior(unsigned int max_particles);
	virtual void UpdateSystem(float cur_time);
	virtual inline bool UpdateParticle(float cur_time, void* particle);
};

/**
 * @ingroup engineclass
 * This represents a particles system and is responsible for animating and
 * rendering it.
 */
class WParticles : public WBase, public WOrientation {
	/**
	 * Returns "Particles" string.
	 * @return Returns "Particles" string
	 */
	virtual std::string GetTypeName() const;

public:
	WParticles(class Wasabi* const app, unsigned int ID = 0);
	~WParticles();

	/**
	 * Retrieves the behavior object for this particle system.
	 * @return  The particle system's behavior
	 */
	WParticlesBehavior* GetBehavior() const;

	/**
	 * Initializes the particle system. This must be called for a WParticles
	 * object to be Valid().
	 * @param effect         Effect to be used for this particle system, default
	 *                       effects can be retrieved from
	 *                       WParticlesManager::GetDefaultEffect()
	 * @param max_particles  Maximum number of particles that the system can
	 *                       render simultaneously. Pass 0 to free resources
	 *                       and uninitialize the system
	 * @param behavior       Behavior object for this system, pass nullptr to
	 *                       use WDefaultParticleBehavior
	 * @return Error code, see WError.h
	 */
	WError Create(class WEffect* effect, unsigned int max_particles = 5000, WParticlesBehavior* behavior = nullptr);

	/**
	 * Renders the particle system to the given render target.
	 * @params rt  Render target to render to
	 */
	void Render(class WRenderTarget* rt);

	/**
	 * Sets the texture of each particle
	 * @param texture  Texture image to use
	 * @return Error code, see WError.h
	 */
	WError SetTexture(class WImage* texture);

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
	virtual bool Valid() const;

	virtual void OnStateChange(STATE_CHANGE_TYPE type);

private:
	/** true if the world matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** true if the particle system is hidden, false otherwise */
	bool m_hidden;
	/** true if frustum culling is enabled, false otherwise */
	bool m_bFrustumCull;
	/** Maximum number of particles that can be rendered by this system */
	unsigned int m_maxParticles;
	/** Local world matrix */
	WMatrix m_WorldM;

	/** Behavior of the particle system */
	WParticlesBehavior* m_behavior;
	/** Geometry of points used to render the particles */
	class WGeometry* m_geometry;
	/** Material used to render the geometry */
	class WMaterial* m_material;

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
	 * Renders all particle systems registered by this manager (by calling
	 * their WParticles::Render() function).
	 * @param rt Render target to render to
	 */
	void Render(class WRenderTarget* rt);

	/**
	 * Retrieves the WEffect used by all particle systems.
	 * @return Particle systems effect
	 */
	class WEffect* GetDefaultEffect(W_DEFAULT_PARTICLE_EFFECT_TYPE type) const;

private:
	/** Default effect used by different particle systems */
	unordered_map<W_DEFAULT_PARTICLE_EFFECT_TYPE, class WEffect*> m_particleEffects;
	/** Default vertex shader used by the default effect */
	class WShader* m_vertexShader;
	/** Default geometry shader used by the default effect */
	class WShader* m_geometryShader;
	/** Default fragment shader used by the default effect */
	class WShader* m_fragmentShader;
};
