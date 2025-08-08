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
		} level;
		std::string message;
		std::string timestamp;
	};

	class Logger final
	{
	public:
		enum class LogLevel
		{
			Error = 0,
			Warning,
			Info
		};

		explicit Logger(LogLevel logLevel = LogLevel::Error);
		void log(LogLevel level, const char* message) const;

		std::vector<LogEntry> getRecentLogs(size_t maxCount = 1000) const;
		void clearLogs();

	private:
		std::string getCurrentTimestamp() const;

	private:
		LogLevel m_logLevel = LogLevel::Error;
		mutable std::vector<LogEntry> m_logEntries;
		mutable std::mutex m_logMutex;
		static constexpr size_t MAX_LOG_ENTRIES = 1000;
	};

#define DX3DLogInfo(message)\
	getLoggerInstance().log((Logger::LogLevel::Info), message);

#define DX3DLogWarning(message)\
	getLoggerInstance().log((Logger::LogLevel::Warning), message);

#define DX3DLogError(message)\
	getLoggerInstance().log((Logger::LogLevel::Error), message);

#define DX3DLogErrorAndThrow(message)\
	{\
	DX3DLogError(message);\
	throw std::runtime_error(message);\
	}
}