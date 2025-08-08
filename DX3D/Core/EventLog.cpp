#include "../Core/EventLog.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

dx3d::EventLog::EventLog(LogStatus logLevel) : m_level(logLevel)
{
	std::clog << "PardCode | C++ 3D Game Tutorial Series" << "\n";
	std::clog << "--------------------------------------" << "\n";
}

void dx3d::EventLog::log(LogStatus level, const char* message) const
{
	auto logLevelToString = [](LogStatus level) {
		switch (level)
		{
		case LogStatus::Info: return "Info";
		case LogStatus::Warning: return "Warning";
		case LogStatus::Error: return "Error";
		default: return "Unknown";
		}
		};

	if (level > m_level) return;

	std::string logMessage = "[DX3D " + std::string(logLevelToString(level)) + "]: " + message;
	std::clog << logMessage << "\n";

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		LogEntry entry;
		entry.severity = static_cast<LogEntry::Level>(level);
		entry.text = message;
		entry.time = generateTimestamp();

		m_entries.push_back(entry);

		if (m_entries.size() > MAX_ENTRIES)
		{
			m_entries.erase(m_entries.begin());
		}
	}
}

std::vector<dx3d::LogEntry> dx3d::EventLog::fetchRecentLogs(size_t maxCount) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_entries.size() <= maxCount)
	{
		return m_entries;
	}

	return std::vector<LogEntry>(
		m_entries.end() - maxCount,
		m_entries.end()
	);
}

void dx3d::EventLog::purgeLogs()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_entries.clear();
}

std::string dx3d::EventLog::generateTimestamp() const
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	std::tm tm_buf;
	localtime_s(&tm_buf, &time_t);

	std::stringstream ss;
	ss << std::put_time(&tm_buf, "%H:%M:%S");
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}