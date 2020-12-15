
#pragma once
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include <plog/Appenders/ColorConsoleAppender.h>
#include <windows.h>
inline void initLogger()
{
	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender; // Create the 2nd appender.
	char strModuleFileName[MAX_PATH];
	char strPath[MAX_PATH];
	GetModuleFileNameA(nullptr, strModuleFileName, MAX_PATH);
	char*p = strrchr(strModuleFileName, '\\');
	*p = '\0';
	p++;
	sprintf_s(strPath, "mkdir %s\\logs\\", strModuleFileName);
	system(strPath);
	sprintf_s(strPath, "%s\\logs\\%s.log", strModuleFileName,p);

	plog::init(plog::debug, strPath, 1024 * 1024 * 5, 100).addAppender(&consoleAppender);
}