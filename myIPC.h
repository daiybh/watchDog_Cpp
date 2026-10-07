#pragma once
#include <windows.h>
#include <stdint.h>
#include <string>

// 基于命名文件映射（共享内存）的跨进程心跳封装。
// 被监控程序周期性写入计数，看门狗只读该计数并判断是否超时。
class MyIPC
{

public:
	MyIPC() {}
	MyIPC(const MyIPC&) = delete;
	MyIPC& operator=(const MyIPC&) = delete;

	// 打开（不存在时创建）共享内存，成功返回 true
	bool open(const std::string& appName)
	{
		close(); // 允许重复 open，先释放上一次的映射

		strMapName = "MYIPC." + appName + ".ShareMemory";
		hMap = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, (LPCSTR)strMapName.c_str());
		const bool bCreated = (NULL == hMap);
		if (bCreated)
		{   // 不存在则创建
			hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE,
				0,
				100,
				(LPCSTR)strMapName.c_str());
		}
		if (NULL == hMap)
		{   // 创建/打开都失败（例如权限不足），保持未打开状态
			pBuffer = nullptr;
			return false;
		}

		// 映射到进程地址空间，得到指向共享内存的指针
		pBuffer = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (NULL == pBuffer)
		{
			::CloseHandle(hMap);
			hMap = nullptr;
			return false;
		}

		if (bCreated)
			setData(100);
		return true;
	}

	bool isOpened() const
	{
		return pBuffer != nullptr;
	}

	void setData(uint64_t tCount)
	{
		if (!pBuffer)
			return;
		*((uint64_t*)pBuffer) = tCount;
	}

	uint64_t getData()
	{
		if (!pBuffer)
			return 0;
		return *((uint64_t*)pBuffer);
	}

	~MyIPC() {
		close();
	}
private:
	void close()
	{
		if (pBuffer)
		{
			::UnmapViewOfFile(pBuffer);
			pBuffer = nullptr;
		}
		if (hMap)
		{
			::CloseHandle(hMap);
			hMap = nullptr;
		}
	}

	std::string strMapName = ("xieTongAppDlg.ShareMemory"); // 内存映射对象名
	HANDLE hMap = nullptr;      // 文件映射句柄
	LPVOID pBuffer = nullptr;   // 映射视图指针，未打开时为 nullptr
};
