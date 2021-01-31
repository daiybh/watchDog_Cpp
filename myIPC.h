#pragma once
#include <windows.h>
class MyIPC {

public:
	MyIPC() {}
	
	void open(std::string appName)
	{
		strMapName = "MYIPC." + appName + ".ShareMemory";
		// ������ͼ��һ���������ڴ�ӳ���ļ�����  
		hMap = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, (LPCSTR)strMapName.c_str());
		if (NULL == hMap)
		{    // ��ʧ�ܣ�����֮
			hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE,
				0,
				100,
				(LPCSTR)strMapName.c_str());
			//�����������L"name"��ʽ
			 // ӳ������һ����ͼ���õ�ָ�����ڴ��ָ�룬�������������
			pBuffer = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			//strcpy((char*)pBuffer, strComData.c_str());
			setData(100);
		}
		else
		{    // �򿪳ɹ���ӳ������һ����ͼ���õ�ָ�����ڴ��ָ�룬��ʾ�����������
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
		// ����ļ�ӳ�䣬�ر��ڴ�ӳ���ļ�������
		::UnmapViewOfFile(pBuffer);
		::CloseHandle(hMap);
	}
private:
	std::string strMapName = ("xieTongAppDlg.ShareMemory");                // �ڴ�ӳ���������
	std::string strComData = ("This is common data!");        // �����ڴ��е�����
	LPVOID pBuffer;                                    // �����ڴ�ָ��
	HANDLE hMap = nullptr;
};