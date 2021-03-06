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
#include <functional>
using logCallBack = std::function<void(std::string log)>;
#include <myIPC.h>
class ProcessMoniter
{
	std::thread *m_thread = nullptr;
	bool m_bExit = false;
	struct LastIPC {

		MyIPC  myIPC;
		uint64_t lastReadData = 0;
		uint64_t lastGettime = 0;
		void reset()
		{
			lastReadData = 0;
		}
		bool check()
		{
			if (myIPC.getData() != lastReadData)
			{
				lastReadData = myIPC.getData();
				lastGettime = GetTickCount();
				return true;
			}
			if (GetTickCount() - lastGettime > 60 * 1000) {
				return false;
			}
			return true;
		}
	};
	struct processInfo {
		LastIPC lastIPC;
		uint64_t startTime = 0;
		void  reset() {
			startTime = GetTickCount();
			lastIPC.reset();
		}
		bool check() {
			if (GetTickCount() - startTime < 10 * 60 * 1000)
				return true;
			return lastIPC.check();
		}
		std::string processPath;
		int pid;
	};
	std::map<std::string, processInfo*> m_processList;
	logCallBack m_logCallBack=nullptr;

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
public:
	void setLogCallback(logCallBack logCB)
	{
		m_logCallBack = logCB;
	}
	void startMoniter()
	{
		//read ini
		char path[255];
		GetModuleFileNameA(nullptr, path, 255);
		char*p1 = path;
		char* p = strrchr(p1, '\\');
		p++;
		*p = '\0';
		char iniFile[255];
		sprintf(iniFile, "%s\\process.mj", path);
		std::ifstream in(iniFile);

		LOGI << "read from "<< iniFile;
		if(m_logCallBack)
			m_logCallBack("read from " + std::string(iniFile));
		if (in.is_open()) 
		{
			std::string str(path);
			while (!in.eof()) 
			{
				processInfo *pi=new processInfo;
				std::getline(in, pi->processPath);

				std::transform(pi->processPath.begin(), pi->processPath.end(), pi->processPath.begin(), ::tolower);
				int pst = pi->processPath.rfind('\\');
				std::string name = pi->processPath.substr(pst + 1);
				if (pi->processPath[1] != ':')
				{

					char pathA[255];
					sprintf(pathA, "%s%s", path, pi->processPath.data());

					pi->processPath = pathA;
				}

				pi->lastIPC.myIPC.open(name);
				m_processList.emplace(name,pi);

				LOGI << m_processList.size()<<" >> " << pi->processPath;
				if (m_logCallBack)
					m_logCallBack("p>> " + pi->processPath);
			}
			in.close();
		}
		if (m_processList.size() == 0)return;
		
		m_thread =new std::thread(&ProcessMoniter::runThread,this);
	}
	void stopMoniter() {

		LOGI << "stop moniter";

		if (m_logCallBack)
			m_logCallBack("stop moniter");
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
		LOGI << "start moniter thread  "<< GetTickCount();
		
		while (!m_bExit) {
			if (GetTickCount() > 10 *60* 1000)
			{
				for (auto &item : m_processList)
				{
					int pid = findProcessByName(CString(item.first.data()));
					if (pid == -1)
					{
						LOGI << "startProcess:" << item.second->processPath;

						if (m_logCallBack)
							m_logCallBack("startProcess: " + item.second->processPath);
						ShellExecuteA(nullptr, nullptr, item.second->processPath.data(), nullptr, nullptr, SW_SHOWNORMAL);
						item.second->reset();
					}
					else
					{
						if (!item.second->check())
						{
							LOGI << "live cound dont move since 1 minute ,restart it. " << item.first.data();
						//	char cmd[1024];
						//	sprintf_s(cmd, "taskkill /f /im %s.exe", item.first.data());
						//	system(cmd);

							HANDLE explorer;
							explorer = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
							TerminateProcess(explorer, 1);

						}
					}
				}
			}
			Sleep(1000);
		}
	}
};

