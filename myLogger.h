
#pragma once
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include <plog/Appenders/ColorConsoleAppender.h>
#include <windows.h>
#include <string>
#include <cstdio>

inline void initLogger()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender; // Create the 2nd appender.
	char strModuleFileName[MAX_PATH] = { 0 };
	if (0 == GetModuleFileNameA(nullptr, strModuleFileName, MAX_PATH))
		return;

	char* p = strrchr(strModuleFileName, '\\');
	if (p == nullptr)
		return;
	*p = '\0';
	const char* exeName = p + 1;

	// 直接建目录，避免 system("mkdir ...")：命令行长度溢出 / 路径含空格 / 目录已存在报错
	const std::string logDir = std::string(strModuleFileName) + "\\logs";
	CreateDirectoryA(logDir.c_str(), nullptr);

	const std::string logFile = logDir + "\\" + exeName + ".log";

	plog::init(plog::debug, logFile.c_str(), 1024 * 1024 * 5, 100).addAppender(&consoleAppender);
}
