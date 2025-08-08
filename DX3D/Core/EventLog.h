#pragma once
#include <vector>
#include <string>
#include <mutex>

namespace dx3d
{
	struct LogEntry
	{
		std::string text;
		std::string time;
		enum class Level
		{
			Error = 0,
			Info,
			Warning
		} severity;
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

		void purgeLogs();
		explicit EventLog(LogStatus logLevel = LogStatus::Error);
		std::vector<LogEntry> fetchRecentLogs(size_t maxCount = 1000) const;
		void log(LogStatus level, const char* message) const;

	private:
		std::string generateTimestamp() const;

	private:
		static constexpr size_t MAX_ENTRIES = 1000;
		mutable std::mutex m_mutex;
		mutable std::vector<LogEntry> m_entries;
		LogStatus m_level = LogStatus::Error;
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