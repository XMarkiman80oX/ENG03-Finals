#pragma once
#include <vector>
#include <string>
#include <mutex>

namespace dx3d
{
	struct LogEntry
	{
		enum class Level
		{
			Error = 0,
			Warning,
			Info
		} severity;
		std::string text;
		std::string time;
	};

	class EventLog final
	{
	public:
		enum class LogStatus
		{
			Error = 0,
			Warning,
			Info
		};

	private:
		LogStatus m_level = LogStatus::Error;
		mutable std::vector<LogEntry> m_entries;
		mutable std::mutex m_mutex;
		static constexpr size_t MAX_ENTRIES = 1000;

	public:
		explicit EventLog(LogStatus logLevel = LogStatus::Error);

		void log(LogStatus level, const char* message) const;
		void purgeLogs();
		std::vector<LogEntry> fetchRecentLogs(size_t maxCount = 1000) const;

	private:
		std::string generateTimestamp() const;
	};

#define DX3DLogInfo(message)\
	getLoggerInstance().log((EventLog::LogStatus::Info), message);

#define DX3DLogWarning(message)\
	getLoggerInstance().log((EventLog::LogStatus::Warning), message);

#define DX3DLogError(message)\
	getLoggerInstance().log((EventLog::LogStatus::Error), message);

#define DX3DLogErrorAndThrow(message)\
	{\
	DX3DLogError(message);\
	throw std::runtime_error(message);\
	}
}