#pragma once
#ifndef TIMER_H
#define TIMER_h
#include "qnpch.h"
#include <chrono>
#include "core.h"
// nanosecond to second
#define NS_TO_S(x)  (x)/1000000000.0
// nanosecond to millisecond
#define NS_TO_MS(x) (x)/1000000.0
// nanosecond to microsecond
#define NS_TO_US(x) (x)/1000.0
// millisecond to nanosecond
#define MS_TO_NS(x) (x)*1000000.0

namespace Quin
{
typedef std::chrono::high_resolution_clock QuinTime;
	// nanosecond timer
	class QUIN_API timer
	{
	public:
		timer() : m_startTime(QuinTime::now())
		{ }
		void start()
		{
			m_startTime = QuinTime::now();
		}
		double peek() const
		{
			auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(QuinTime::now() - m_startTime);
			return elapsed_time.count();
		}
		// peek and reset timer
		double Mark()
		{
			auto currentTime = QuinTime::now();
			auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - m_startTime);
			m_startTime = currentTime;
			return elapsed_time.count();
		}
	private:
		std::chrono::time_point<QuinTime> m_startTime;
	};
}
#endif // TIMER_H