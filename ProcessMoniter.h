#pragma once
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <TlHelp32.h>
#include <map>
#include <algorithm>
#include <windows.h>
#include <winnt.h>
#include "myLogger.h"
class ProcessMoniter
{
	std::thread *m_thread = nullptr;
	bool m_bExit = false;
	struct processInfo {
		std::string processPath;
		int pid;
	};
	std::map<std::string, processInfo> m_processList;
public:

	int execmd_z(char* cmd, char* result, int pos)
	{
		char buffer[128]; // 定义缓冲区                          
		FILE* pipe = _popen(cmd, "r"); // 打开管道，并执行命令   
		if (!pipe)
			return 0; // 返回0表示运行失败   

		int i = 1;
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe)) { // 将管道输出到result中 
				if (buffer[0] == '\n')
					continue;
				if (pos == 0) {
					strcat(result, buffer);
				}
				else if (pos == i)
				{
					strcat(result, buffer);
					break;
				}
				i++;
			}
		}
		_pclose(pipe); // 关闭管道 
		return 1;
	}
	
	int findProcessByName(CString szName) 
	{
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
			return -1;

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hProcessSnap, &pe32))
		{
			CloseHandle(hProcessSnap);
			return -1;
		}

		bool bFind = false;
		int pid = -1;
		do
		{
			CString name = pe32.szExeFile;
			name.MakeLower();
			if (name==szName)
			{
				bFind = true;		
				pid = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hProcessSnap, &pe32));

		if (hProcessSnap != INVALID_HANDLE_VALUE)
			CloseHandle(hProcessSnap);
		return pid;
	}

	void startMoniter() 
	{
		char *buffer=new char[10 * 1024 * 1024];
		char cmd[128];
		memset(cmd, 0, sizeof(cmd));

		memset(buffer, 0, sizeof(10 * 1024 * 1024));

		std::sprintf(cmd, "%s%s%s", "tasklist /fi \"imagename eq ", "qq.exe", "\" /fo list");
		execmd_z(cmd, buffer, 2);
		//read ini
		std::ifstream in("process.mj");

		LOGI << "read from process.mj";
		if (in.is_open()) 
		{
			std::string str;
			while (!in.eof()) 
			{
				processInfo pi;
				std::getline(in, pi.processPath);

				std::transform(pi.processPath.begin(), pi.processPath.end(), pi.processPath.begin(), ::tolower);
				int pst = pi.processPath.rfind('\\');
				m_processList.emplace(pi.processPath.substr(pst+1),pi);

				LOGI << m_processList.size()<<" >> " << pi.processPath;
			}
		}
		if (m_processList.size() == 0)return;
		
		m_thread =new std::thread(&ProcessMoniter::runThread,this);
	}
	void stopMoniter() {

		LOGI << "stop moniter";
		m_bExit = true;
		if (m_thread)
		{
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
		}
	}
public:
	void runThread() {
		LOGI << "start moniter thread";

		while (!m_bExit) {
			for (auto &item : m_processList	)
			{
				int pid = findProcessByName(CString(item.first.data()));
				if (pid == -1)
				{
					LOGI << "startProcess:" << item.second.processPath;
					ShellExecuteA(nullptr, nullptr, item.second.processPath.data(), nullptr, nullptr, SW_SHOWNORMAL);
				}
			}
			Sleep(1000);
		}
	}
};

