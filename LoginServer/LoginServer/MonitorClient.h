#pragma once
#include "CLanClient.h"


class MonitorClient : public CLanClient
{
public:

	MonitorClient();
	~MonitorClient();

	//void Run();
	// ���� �Ϸ��
	void OnConnect() override;
	// ���� �����
	void OnDisconnect() override;
	// ��Ŷ ���Ž�
	void OnRecvPacket(CPacket* packet) override;
	// ���� �߻���
	void OnError(int errorCode, WCHAR* errorMessage) override;


	//static unsigned int __stdcall UpdateThreadStatic(void* arg)
	//{
	//	MonitorClient* pThis = (MonitorClient*)arg;
	//	pThis->UpdateThread();
	//	return 0;
	//}

	//unsigned int __stdcall UpdateThread();

public:
	HANDLE _hUpdateThread;
};

