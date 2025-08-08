#include "../Core/Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

dx3d::Logger::Logger(LogLevel logLevel) : m_logLevel(logLevel)
{
	std::clog << "PardCode | C++ 3D Game Tutorial Series" << "\n";
	std::clog << "--------------------------------------" << "\n";
}

void dx3d::Logger::log(LogLevel level, const char* message) const
{
	auto logLevelToString = [](LogLevel level) {
		switch (level)
		{
		case LogLevel::Info: return "Info";
		case LogLevel::Warning: return "Warning";
		case LogLevel::Error: return "Error";
		default: return "Unknown";
		}
		};

	if (level > m_logLevel) return;

	std::string logMessage = "[DX3D " + std::string(logLevelToString(level)) + "]: " + message;
	std::clog << logMessage << "\n";

	{
		std::lock_guard<std::mutex> lock(m_logMutex);

		LogEntry entry;
		entry.level = static_cast<LogEntry::Level>(level);
		entry.message = message;
		entry.timestamp = getCurrentTimestamp();

		m_logEntries.push_back(entry);

		if (m_logEntries.size() > MAX_LOG_ENTRIES)
		{
			m_logEntries.erase(m_logEntries.begin());
		}
	}
}

std::vector<dx3d::LogEntry> dx3d::Logger::getRecentLogs(size_t maxCount) const
{
	std::lock_guard<std::mutex> lock(m_logMutex);

	if (m_logEntries.size() <= maxCount)
	{
		return m_logEntries;
	}

	return std::vector<LogEntry>(
		m_logEntries.end() - maxCount,
		m_logEntries.end()
	);
}

void dx3d::Logger::clearLogs()
{
	std::lock_guard<std::mutex> lock(m_logMutex);
	m_logEntries.clear();
}

std::string dx3d::Logger::getCurrentTimestamp() const
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	// Create a tm struct to safely hold the time components
	std::tm tm_buf;
	// Use the thread-safe localtime_s instead of localtime
	localtime_s(&tm_buf, &time_t);

	std::stringstream ss;
	// Pass the address of your local tm struct to std::put_time
	ss << std::put_time(&tm_buf, "%H:%M:%S");
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}