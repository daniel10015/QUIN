#include "qnpch.h"
#include "ParticleSystem.h"

// PARTICLE SYSTEM ONLY macro
#define OUT_OF_BOUNDS_CHECK(x) if (x >= m_emitters.size() || x < 0) return false

namespace Quin
{

	// **************** Particle Emitter **************** //

	// all we really need is velocity and size for now
	ParticleEmitter::ParticleEmitter(size_t size) 
	{
		m_particles.resize(size);
		m_particleSizes.resize(size);
	}

	void ParticleEmitter::OnUpdate(double deltaTime) 
	{
		// if dead and no particles then kill and return
		if (m_life > 0.0f && m_endParticles - m_startParticles == 0)
		{
			m_alive = false;
			return;
		}

		// change position and alpha (float values) in m_particles according to functions
		m_runningTime += deltaTime;

		// create new particle according to frequency requirements
		while (m_runningTime >= m_emittingFrequency)
		{
			// add a new particle
			if (m_endParticles < m_particles.size())
				m_endParticles++;

			// remove a particle 
			while (m_life - (m_emittingFrequency*m_startParticles) > m_lifeLimit && (m_startParticles < m_endParticles || m_endParticles==m_particles.size()))
			{
				m_startParticles++;
			}

			m_runningTime -= m_emittingFrequency;
		}

		float t;
		float sec_diff = NS_TO_S(deltaTime);
		for (size_t i = m_startParticles; i < m_endParticles; i++)
		{
			t = NS_TO_S(m_life - i * m_emittingFrequency + m_runningTime);
			m_particles[i][0] += m_maf_velocity[0]->Calculate({ t })*sec_diff;
			m_particles[i][1] += m_maf_velocity[1]->Calculate({ t })*sec_diff;
			m_particleSizes[i] = std::max( m_maf_size->Calculate({ t }), 0.0f); // clamp
		}
		m_life += deltaTime;
	}

	void ParticleEmitter::SetLife(const float life) 
	{
		m_lifeLimit = MS_TO_NS(life);
	}

	void ParticleEmitter::SetInitialSize(const float size) 
	{
		m_initialSize = size;
	}
	void ParticleEmitter::SetSizeOverLife(const std::string& function) 
	{
		m_maf_size = std::make_unique<QuinMath::MathFunction<1>>();
		m_maf_size->init(definition, function); // temp
	}

	// modify velocity
	void ParticleEmitter::SetInitialVelocity(const std::array<float, 2>& velocity) 
	{
		m_initialVelocity = velocity;
	}
	void ParticleEmitter::SetVelocityOverLife(const std::array<std::string, 2>& functions) 
	{
		m_maf_velocity = { std::make_unique<QuinMath::MathFunction<1>>(), std::make_unique<QuinMath::MathFunction<1>>() };
		m_maf_velocity[0]->init(definition, functions[0]); // temp
		m_maf_velocity[1]->init(definition, functions[1]); // temp
	}

	// modify color
	void ParticleEmitter::SetInitialColor(const std::array<float, 4>& color) 
	{
		m_initialColor = color;
	}

	void ParticleEmitter::SetColorOverLife(const std::array<std::string, 4>& functions) 
	{
		m_maf_color = { std::make_unique<QuinMath::MathFunction<1>>(), std::make_unique<QuinMath::MathFunction<1>>() , std::make_unique<QuinMath::MathFunction<1>>() , std::make_unique<QuinMath::MathFunction<1>>() };
		m_maf_color[0]->init(definition, functions[0]); // temp
		m_maf_color[1]->init(definition, functions[1]); // temp
		m_maf_color[2]->init(definition, functions[2]); // temp
		m_maf_color[3]->init(definition, functions[3]); // temp
	}

	// modify alpha
	void ParticleEmitter::SetInitialAlpha(const float alpha) 
	{
		m_initialAlpha = alpha;
	}
	void ParticleEmitter::SetAlphaOverLife(const std::string& function) 
	{
		m_maf_alpha = std::make_unique<QuinMath::MathFunction<1>>();
		m_maf_alpha->init(definition, function); // temp
	}

	// modify initial position
	void ParticleEmitter::SetInitialPosition(const std::array<float, 2>& position) 
	{
		m_initialPosition = position;
	}

	// input frequency at particles/second
	bool ParticleEmitter::SetEmittingFrequency(const float frequency) 
	{
		if (frequency <= 0.0f)
			return false;
		m_emittingFrequency = 1000000000.0/frequency; // convert to #ns to match frequency
		return true;
	}

	bool ParticleEmitter::SetRandomnessFactor(const size_t emPosition, const float factor)
	{
		// TODO
		return true;
	}

	void ParticleEmitter::reset()
	{
		m_life = 0;
		m_runningTime = 0;
		m_startParticles = 0;
		m_endParticles = 1;
		m_alive = true;
		for (auto& particle : m_particles)
			particle = m_initialPosition;
	}


	// **************** Particle System **************** //
	ParticleSystem::ParticleSystem()
	{

	}

	ParticleSystem::~ParticleSystem()
	{
		for (auto particles : m_particleData)
		{
			delete particles;
		}
	}

	void ParticleSystem::Trigger()
	{
		m_isAlive = true;
		for (auto& emitter : m_emitters)
		{
			emitter->reset();
		}
	}

	std::vector<ParticleData*>* ParticleSystem::GetParticles()
	{
		size_t size;
		size_t i = 0;
		for (auto& emitter : m_emitters)
		{
			size = emitter->GetSize();
			m_particleData[i]->size = size;
			if (size)
			{
				m_particleData[i]->position_data = emitter->GetPositionStart();
				m_particleData[i]->size_data = emitter->GetSizeStart();
			}
			i++;
		}
		return &m_particleData;
	}

	void ParticleSystem::OnUpdate(double deltaTime) 
	{
		if (!m_isAlive)
			return;

		// maybe multithread here... data races don't actually matter here
		// but since this is CPUParticleSystem multithreading probably won't do much...
		bool ran = false;
		for (auto& emitter : m_emitters)
		{
			if (emitter->IsAlive())
			{
				emitter->OnUpdate(deltaTime);
				ran = true;
			}
		}
		m_isAlive = ran; 
	}



	bool ParticleSystem::SetLife(const size_t emPosition, const float life)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetLife(life);
		return true;
	}

	// ownership gets transfered to Particle System implicitly
	bool ParticleSystem::AddEmitter(ParticleEmitter* emitter)
	{
		if (emitter)
		{
			m_emitters.push_back(std::unique_ptr<ParticleEmitter>(emitter));
			m_particleData.push_back(new ParticleData());
			return true;
		}
		return false;
	}
	
	bool ParticleSystem::AddEmitter(size_t size)
	{
		if (size == 0)
			return false;
		m_emitters.push_back(std::make_unique<ParticleEmitter>(size));
		m_particleData.push_back(new ParticleData());
		return true;
	}

	// modify size
	bool ParticleSystem::SetInitialSize(const size_t emPosition, const float size)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetInitialSize(size);
		return true;
	}

	bool ParticleSystem::SetSizeOverLife(const size_t emPosition, const std::string& function)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetSizeOverLife(function);
		return true;
	}

	// modify velocity
	bool ParticleSystem::SetInitialVelocity(const size_t emPosition, const std::array<float, 2>& velocity)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetInitialVelocity(velocity);
		return true;
	}
	bool ParticleSystem::SetVelocityOverLife(const size_t emPosition, const std::array<std::string, 2>& functions)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetVelocityOverLife(functions);
		return true;
	}

	// modify color
	bool ParticleSystem::SetInitialColor(const size_t emPosition, const std::array<float, 4>& color)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetInitialColor(color);
		return true;
	}
	bool ParticleSystem::SetColorOverLife(const size_t emPosition, const std::array<std::string, 4>& functions)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetColorOverLife(functions);
		return true;
	}

	// modify alpha
	bool ParticleSystem::SetInitialAlpha(const size_t emPosition, const float alpha)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetInitialAlpha(alpha);
		return true;
	}

	bool ParticleSystem::SetAlphaOverLifeconst(size_t emPosition, const std::string& function)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetAlphaOverLife(function);
		return true;
	}

	// modify initial position
	bool ParticleSystem::SetInitialPosition(const size_t emPosition, const std::array<float, 2>& position)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetInitialPosition(position);
		return true;
	}

	// particles / second
	bool ParticleSystem::SetEmittingFrequency(const size_t emPosition, const float frequency)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.at(emPosition)->SetEmittingFrequency(frequency);
		return true;
	}

	bool ParticleSystem::SetRandomnessDistribution(const size_t emPosition, const float distribution)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		// TODO
		return true;
	}
	bool ParticleSystem::SetRandomnessFactor(const size_t emPosition, const float factor)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		// TODO
		return true;
	}

	// inefficient, please don't this very often
	// if necessary (for good reason) then maybe use a hash
	bool ParticleSystem::RemoveEmitter(const size_t emPosition)
	{
		OUT_OF_BOUNDS_CHECK(emPosition);
		m_emitters.erase(m_emitters.begin() + emPosition);
		return true;
	}

	bool ParticleSystem::SwapEmitters(const size_t e1, const size_t e2)
	{
		if (e1 >= m_emitters.size() || e2 >= m_emitters.size()) return false;
		std::swap(m_emitters[e1], m_emitters[e2]);
		return true;
	} 
}