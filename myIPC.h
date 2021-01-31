#pragma once
#include <windows.h>
class MyIPC {

public:
	MyIPC() {}
	
	void open(std::string appName)
	{
		strMapName = "MYIPC." + appName + ".ShareMemory";
		// 首先试图打开一个命名的内存映射文件对象  
		hMap = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, (LPCSTR)strMapName.c_str());
		if (NULL == hMap)
		{    // 打开失败，创建之
			hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE,
				0,
				100,
				(LPCSTR)strMapName.c_str());
			//如果有问题用L"name"形式
			 // 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
			pBuffer = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			//strcpy((char*)pBuffer, strComData.c_str());
			setData(100);
		}
		else
		{    // 打开成功，映射对象的一个视图，得到指向共享内存的指针，显示出里面的数据
			pBuffer = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		}
	}
	void setData(uint64_t tCount)
	{
		*((uint64_t*)pBuffer) = tCount;
	}

	uint64_t  getData()
	{
		uint64_t* pI = (uint64_t*)pBuffer;
		return *(pI);
	}
	~MyIPC() {
		// 解除文件映射，关闭内存映射文件对象句柄
		::UnmapViewOfFile(pBuffer);
		::CloseHandle(hMap);
	}
private:
	std::string strMapName = ("xieTongAppDlg.ShareMemory");                // 内存映射对象名称
	std::string strComData = ("This is common data!");        // 共享内存中的数据
	LPVOID pBuffer;                                    // 共享内存指针
	HANDLE hMap = nullptr;
};