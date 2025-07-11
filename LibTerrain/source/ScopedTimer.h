#pragma once

// Performance Checker
#include <chrono>
#include <format>  // C++20 or use fmtlib if not available

#include "../../LibGL/source/utils.h"

class ScopedTimer
{
public:
	using Clock = std::chrono::steady_clock;

	explicit ScopedTimer(std::string_view tag)
		: m_stTag(tag), m_start(Clock::now())
	{
	}

	~ScopedTimer()
	{
		const auto end = Clock::now();
		const auto duration = end - m_start;
		LogDuration(m_stTag, duration);
	}

	// Delete copy/move to prevent misuse
	ScopedTimer(const ScopedTimer&) = delete;
	ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
	void LogDuration(std::string_view tag, const Clock::duration& duration) const
	{
		// C++20 std::format (or use fmtlib)
		sys_log(std::format("{} took {:.3f}ms", tag, std::chrono::duration<double, std::milli>(duration).count()).c_str());
	}

	std::string m_stTag;
	Clock::time_point m_start;
};
