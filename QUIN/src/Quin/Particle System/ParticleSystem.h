#pragma once
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "Quin/core.h"
#include <ExpressionEvaluation.h>
#include <string>
#include <vector>
#include <memory>
#include <array>

#define CACHE_CALCULATION

namespace Quin
{
	namespace QuinMath
	{
		// thou shall constexpr the default types
		/*
		Art by Joan Stark
				 !
				.-.
			  __|=|__
			 (_/`-`\_)
			 //\___/\\
			 <>/   \<>
			  \|_._|/
			   <_I_>
				|||
		jgs    /_|_\
		*/
		enum FunctionTypes
		{
			Constant = 0,
			Linear,
			Quadratic, 
			Exponential,
			Sin,
			Cos,
			Custom,
		};

		// S is the number of inputs into the function
		template <size_t S>
		struct MathFunction
		{
		private:
			MathEvaluator<S>* m_function = nullptr;
			bool initialized = false;
		public:
			std::string m_name = "function";

			// can only initialize once
			void init(std::unordered_map<std::string, size_t>& definition, std::string function)
			{
				if (initialized)
					return;

				initialized = true;
				m_function = new MathEvaluator<S>(function, definition);
				m_function->Setup();
			}

			// compile time cache, because I feel caching will mostly be yes 
			float Calculate(std::array<float, S> input)
			{
				
				#ifdef CACHE_CALCULATION
					return m_function->Evaluate(input, true);
				#else
					return m_function->Evaluate(input);
				#endif /*CACHE_CALCULATION*/
			}

			~MathFunction()
			{
				if (m_function)
					delete m_function;
			}
		};
	} /* QuinMath */

	struct ParticleData
	{
		const std::array<float, 2>* position_data;
		const float* size_data; // square, so both length and width
		// soon add std::array<float, 4> color_data and size_t tex_id
		size_t size;
	};

	class QUIN_API ParticleEmitter
	{
	public:
		ParticleEmitter() = delete;
		ParticleEmitter(size_t size);

		void OnUpdate(double deltaTime);
		// in milliseconds
		void SetLife(const float life);
		inline bool IsAlive() const { return m_alive; }
		inline const std::array<float, 2>*  GetPositionStart() const { return m_particles.data() + m_startParticles; }
		inline const float* GetSizeStart() const { return m_particleSizes.data() + m_startParticles; }
		inline size_t GetSize() const { return m_endParticles - m_startParticles; }

		void SetInitialSize(const float size);
		void SetSizeOverLife(const std::string& function);

		// modify velocity
		void SetInitialVelocity(const std::array<float, 2>& velocity);
		void SetVelocityOverLife(const std::array<std::string, 2>& functions);

		// modify color
		void SetInitialColor(const std::array<float, 4>& color);
		void SetColorOverLife(const std::array<std::string, 4>& functions);

		// modify alpha
		void SetInitialAlpha(const float alpha);
		void SetAlphaOverLife(const std::string& function);

		// modify initial position
		void SetInitialPosition(const std::array<float, 2>& position);

		bool SetEmittingFrequency(const float frequency);

		bool SetRandomnessFactor(const size_t emPosition, const float factor);

		// start
		void reset();
		static void SetupDefinition()
		{
			definition["t"] = (size_t)0;
		}

		// maf => mathFunction
	private:
		// dynamic vars
		size_t m_endParticles = 0;
		size_t m_startParticles = 0;
		float m_currentSize;
		// number of particles
		std::vector<std::array<float, 2>> m_particles; // resize in constructor
		std::vector<float> m_particleSizes; // resize in constructor
		std::array<float, 2> m_initialPosition;
		std::array<float, 2> m_initialVelocity;
		std::array<float, 4> m_initialColor;

		// parametric functions of t (time) in units
		std::unique_ptr<QuinMath::MathFunction<1>> m_maf_size;
		std::array< std::unique_ptr<QuinMath::MathFunction<1>>, 2 > m_maf_velocity;
		std::array< std::unique_ptr<QuinMath::MathFunction<1>>, 4> m_maf_color;
		std::unique_ptr<QuinMath::MathFunction<1>> m_maf_alpha;
		// randomness variable

		float m_lifeLimit; // maximum lifespan of a particle
		float m_life = 0; // global current lifetime
		double m_runningTime = 0; // time in-between particle emmision

		float m_initialSize;
		float m_initialAlpha;
		float m_emittingFrequency;
		bool m_alive;

		static inline std::unordered_map<std::string, size_t> definition;
	};

	class QUIN_API ParticleSystem
	{
	public:
		ParticleSystem();
		~ParticleSystem();

		// trigger before start a sequence of OnUpdate() to reset the emitters
		void Trigger();
		std::vector<ParticleData*>* GetParticles();

		void OnUpdate(double deltaTime);
		bool SetLife(const size_t emPosition, const float life);

		inline bool IsAlive() const { return m_isAlive; }

		bool AddEmitter(ParticleEmitter* emitter);
		bool AddEmitter(size_t size);
		// modify size
		bool SetInitialSize(const size_t emPosition, const float size);
		bool SetSizeOverLife(const size_t emPosition, const std::string& function);

		// modify velocity
		bool SetInitialVelocity(const size_t emPosition, const std::array<float, 2>& velocity);
		bool SetVelocityOverLife(const size_t emPosition, const std::array<std::string, 2>& functions);

		// modify color
		bool SetInitialColor(const size_t emPosition, const std::array<float, 4>& color);
		bool SetColorOverLife(const size_t emPosition, const std::array<std::string, 4>& functions);

		// modify alpha
		bool SetInitialAlpha(const size_t emPosition, const float alpha);
		bool SetAlphaOverLifeconst (size_t emPosition, const std::string& function);

		// modify initial position
		bool SetInitialPosition(const size_t emPosition, const std::array<float, 2>& position);

		// particles / second
		bool SetEmittingFrequency(const size_t emPosition, const float frequency);

		bool SetRandomnessDistribution(const size_t emPosition, const float distribution);
		bool SetRandomnessFactor(const size_t emPosition, const float factor);

		bool RemoveEmitter(const size_t emPosition);
		bool SwapEmitters(const size_t e1, const size_t e2); // returns true upon success, false upon failure

		// Move constructor
		ParticleSystem(ParticleSystem&& other) noexcept
			: m_emitters(std::move(other.m_emitters)) {}

		// Move assignment operator
		ParticleSystem& operator=(ParticleSystem&& other) noexcept {
			if (this != &other) {
				m_emitters = std::move(other.m_emitters);
			}
			return *this;
		}
	private:
		std::vector< std::unique_ptr<ParticleEmitter> > m_emitters;
		std::vector< ParticleData* > m_particleData;
		bool m_isAlive = false;
	};
}

#endif /* PARTICLE_SYSTEM_H */