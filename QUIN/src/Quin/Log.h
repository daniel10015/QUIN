#pragma once

#include "qnpch.h"
#include "core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Quin
{
	class QUIN_API Log
	{
	public:
		static void Init();
	
		inline static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// core log macros
#define QN_CORE_FATAL(...)    ::Quin::Log::GetCoreLogger()->fatal(__VA_ARGS__)
#define QN_CORE_ERROR(...)    ::Quin::Log::GetCoreLogger()->error(__VA_ARGS__)
#define QN_CORE_WARN(...)     ::Quin::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define QN_CORE_INFO(...)     ::Quin::Log::GetCoreLogger()->info(__VA_ARGS__)
#define QN_CORE_TRACE(...)    ::Quin::Log::GetCoreLogger()->trace(__VA_ARGS__)

// client log macros
#define QN_FATAL(...)         ::Quin::Log::GetClientLogger()->fatal(__VA_ARGS__)
#define QN_ERROR(...)         ::Quin::Log::GetClientLogger()->error(__VA_ARGS__)
#define QN_WARN(...)          ::Quin::Log::GetClientLogger()->warn(__VA_ARGS__)
#define QN_INFO(...)          ::Quin::Log::GetClientLogger()->info(__VA_ARGS__)
#define QN_TRACE(...)         ::Quin::Log::GetClientLogger()->trace(__VA_ARGS__)