#pragma once
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <TlHelp32.h>
#include <map>
#include <algorithm>
#include <atomic>
#include <windows.h>
#include <winnt.h>
#include "myLogger.h"
#include <functional>
using logCallBack = std::function<void(std::string log)>;
#include <myIPC.h>
class ProcessMoniter
{
	std::thread *m_thread = nullptr;
	std::atomic<bool> m_bExit{ false };   // 跨线程退出标志，必须是原子的
	struct LastIPC {

		MyIPC  myIPC;
		uint64_t lastReadData = 0;
		uint64_t lastGettime = 0;
		void reset()
		{
			lastReadData = 0;
			lastGettime = 0;
		}
		bool check()
		{
			// 共享内存没打开时读不到心跳，按“正常”处理，避免误杀进程
			if (!myIPC.isOpened())
				return true;

			const uint64_t now = GetTickCount64();
			const uint64_t data = myIPC.getData();
			if (data != lastReadData)
			{
				lastReadData = data;
				lastGettime = now;
				return true;
			}
			if (lastGettime == 0)
			{   // 第一次采样，还没有基准时间，先记下时间
				lastGettime = now;
				return true;
			}
			// 心跳 1 分钟没有变化，判定为假死
			return (now - lastGettime) <= 60 * 1000;
		}
	};
	struct processInfo {
		LastIPC lastIPC;
		uint64_t startTime = 0;
		void  reset() {
			startTime = GetTickCount64();
			lastIPC.reset();
		}
		bool check() {
			// 启动后 10 分钟内给足启动时间，不做心跳检查
			if (GetTickCount64() - startTime < 10 * 60 * 1000)
				return true;
			return lastIPC.check();
		}
		std::string processPath;
		int pid = -1;
		uint64_t lastStartAttempt = 0;   // 上次拉起进程的时间，用于失败重试限流
	};
	std::map<std::string, processInfo*> m_processList;
	logCallBack m_logCallBack = nullptr;

	// 返回: >=0 找到的 pid；-1 未找到；-2 枚举进程失败
	int findProcessByName(CString szName) 
	{
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
			return -2;

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hProcessSnap, &pe32))
		{
			CloseHandle(hProcessSnap);
			return -2;
		}

		int pid = -1;
		szName.MakeLower();
		do
		{
			CString name = pe32.szExeFile;
			name.MakeLower();
			if (name==szName)
			{
				pid = (int)pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hProcessSnap, &pe32));

		CloseHandle(hProcessSnap);
		return pid;
	}

	void clearProcessList()
	{
		for (auto &item : m_processList)
			delete item.second;
		m_processList.clear();
	}
public:
	ProcessMoniter() = default;
	~ProcessMoniter()
	{
		m_bExit = true;
		if (m_thread)
		{
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
		}
		clearProcessList();
	}

	void setLogCallback(logCallBack logCB)
	{
		m_logCallBack = logCB;
	}
	bool isMonitering() const
	{
		return m_thread != nullptr;
	}
	void startMoniter()
	{
		if (m_thread)
			return;   // 已在监控，避免重复读配置和重复开线程

		//read ini
		char path[MAX_PATH] = { 0 };
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string strDir(path);
		size_t nPos = strDir.rfind('\\');
		if (nPos == std::string::npos)
		{
			LOGE << "invalid module path: " << path;
			return;
		}
		strDir.resize(nPos + 1);   // 保留结尾的 '\'
		std::string iniFile = strDir + "process.mj";

		LOGI << "read from "<< iniFile;
		if(m_logCallBack)
			m_logCallBack("read from " + iniFile);

		std::ifstream in(iniFile);
		if (in.is_open()) 
		{
			std::string line;
			while (std::getline(in, line)) 
			{
				// 去掉行尾的 \r，跳过空行（原来的 while(!eof()) 会多读一行空记录）
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				if (line.empty())
					continue;

				std::transform(line.begin(), line.end(), line.begin(),
					[](unsigned char c) { return (char)::tolower(c); });

				// 可执行文件名（配置行里最后一段）
				std::string name = line;
				size_t nSlash = name.rfind('\\');
				if (nSlash != std::string::npos)
					name = name.substr(nSlash + 1);

				processInfo *pi = new processInfo;
				// 非绝对路径时按程序所在目录补全（原实现直接访问 [1] 会越界）
				if (line.size() < 2 || line[1] != ':')
					pi->processPath = strDir + line;
				else
					pi->processPath = line;

				pi->lastIPC.myIPC.open(name);

				if (!m_processList.emplace(name, pi).second)
				{   // 配置里有重名项，避免泄漏
					LOGI << "duplicated entry ignored: " << name;
					delete pi;
					continue;
				}

				LOGI << m_processList.size()<<" >> " << pi->processPath;
				if (m_logCallBack)
					m_logCallBack("p>> " + pi->processPath);
			}
			in.close();
		}
		if (m_processList.empty())
		{
			LOGE << "process.mj not found or empty: " << iniFile;
			if (m_logCallBack)
				m_logCallBack("process.mj not found or empty: " + iniFile);
			return;
		}
		
		m_bExit = false;
		m_thread =new std::thread(&ProcessMoniter::runThread,this);
	}
	void stopMoniter() {

		if (!m_thread)
		{   // 没有在运行，只需要释放配置
			clearProcessList();
			return;
		}

		LOGI << "stop moniter";

		if (m_logCallBack)
			m_logCallBack("stop moniter");
		m_bExit = true;
		m_thread->join();
		delete m_thread;
		m_thread = nullptr;
		clearProcessList();
	}
public:
	void runThread() {
		LOGI << "start moniter thread  "<< GetTickCount64();
		
		while (!m_bExit) {
			// 系统启动 10 分钟内不做任何干预
			if (GetTickCount64() > 10 *60* 1000)
			{
				for (auto &item : m_processList)
				{
					int pid = findProcessByName(CString(item.first.data()));
					if (pid == -2)
					{   // 枚举失败不能当成“没运行”，否则会重复拉起进程
						LOGE << "enum process failed, skip this round.";
						if (m_logCallBack)
							m_logCallBack("enum process failed, skip this round.");
						continue;
					}
					if (pid == -1)
					{
						// 拉起后 1 分钟内不再重复拉起：
						// 否则路径错误/权限不足时 ShellExecute 会每秒失败一次，日志被刷爆
						const uint64_t nowTick = GetTickCount64();
						if (nowTick - item.second->lastStartAttempt < 60 * 1000)
							continue;
						item.second->lastStartAttempt = nowTick;

						LOGI << "startProcess:" << item.second->processPath;

						if (m_logCallBack)
							m_logCallBack("startProcess: " + item.second->processPath);
						HINSTANCE hRet = ShellExecuteA(nullptr, nullptr, item.second->processPath.data(), nullptr, nullptr, SW_SHOWNORMAL);
						if ((INT_PTR)hRet <= 32)
						{   // 启动失败（文件不存在/无权限…）要记日志，否则会每秒重试且毫无提示
							LOGE << "ShellExecute failed, code=" << (INT_PTR)hRet
								 << " path=" << item.second->processPath;
							if (m_logCallBack)
								m_logCallBack("startProcess failed: " + item.second->processPath);
						}
						item.second->reset();
					}
					else
					{
						if (!item.second->check())
						{
							LOGI << "live cound dont move since 1 minute ,restart it. " << item.first;
							if (m_logCallBack)
								m_logCallBack("restart: " + item.first);

							HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
							if (hProcess)
							{
								if (!TerminateProcess(hProcess, 1))
									LOGE << "TerminateProcess failed, err=" << GetLastError();
								CloseHandle(hProcess);   // 原实现每次重启都泄漏一个句柄
							}
							else
							{
								LOGE << "OpenProcess failed, pid=" << pid << " err=" << GetLastError();
							}
							// 重新计时，避免对正在退出的进程反复 TerminateProcess
							item.second->reset();
						}
					}
				}
			}
			// 分段睡眠，停止时不用等满 1 秒
			for (int i = 0; i < 10 && !m_bExit; ++i)
				Sleep(100);
		}
	}
};
