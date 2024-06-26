#define _CRT_SECURE_NO_WARNINGS
#include "Profiler.h"
#include <conio.h>
#include <iostream>
#include <Windows.h>


LARGE_INTEGER Freq;
const int MAX_DATA_SIZE = 50;
const int MAX_FUNCNAME_SIZE = 64;
const int minSize = 2;
const int maxSize = 2;

#define NON_USING 0
#define USING 1
#define RESETTED 2
#define MAX_THREAD_NUM 50

struct st_PROFILE_DATA
{
	SRWLOCK srwLock;
	long lFlag; //���������� ��� ���� (�迭�ÿ���) //0 : �����ϰ����� // 1: ����� // 2: ���µ�
	WCHAR szName[MAX_FUNCNAME_SIZE]; // �������� ���� �̸�
	LARGE_INTEGER lStartTime; //�������� ���� ���� �ð�
	__int64 iTotalTime; // ��ü ���ð� ī���� Time;
	__int64 iMin[minSize]; // �ּ� ���ð� ī���� TIme
	__int64 iMax[maxSize]; //�ִ� ���ð� ī���� Time
	__int64 iCall; //���� ȣ�� Ƚ��
};

#ifdef PROFILE
st_PROFILE_DATA threadProfileData[MAX_THREAD_NUM][MAX_DATA_SIZE];

#else
st_PROFILE_DATA** threadProfileData;
#endif

thread_local int threadIndex = -1;
long perThreadIndex = -1;

void ProfileBegin(const WCHAR* funcName)
{
	if (threadIndex == -1)
	{
		threadIndex = InterlockedIncrement(&perThreadIndex);
		if (threadIndex >= MAX_THREAD_NUM)
		{
			__debugbreak();
		}
		InitializeSRWLock(&threadProfileData[threadIndex]->srwLock);
	}

	int index = -1;
	st_PROFILE_DATA* profileData = threadProfileData[threadIndex];
	AcquireSRWLockExclusive(&profileData->srwLock);

	for (int i = 0; i < MAX_DATA_SIZE; i++)
	{
		if (profileData[i].lFlag == USING || profileData[i].lFlag == RESETTED)
		{
			if (wcscmp(profileData[i].szName, funcName) == 0)
			{
				if (profileData[i].lFlag == RESETTED)
				{
					profileData[i].lFlag = USING;
				}
				index = i;
				break;
			}
		}
		else if (profileData[i].lFlag == NON_USING)
		{
			index = i;
			profileData[i].lFlag = USING;
			wcscpy_s(profileData[i].szName, MAX_FUNCNAME_SIZE, funcName);
			profileData[i].iMin[0] = INT64_MAX;
			profileData[i].iMin[1] = INT64_MAX;
			profileData[i].iMax[0] = 0;
			profileData[i].iMax[1] = 0;
			break;
		}

	}
	if (index == -1)
	{
		DebugBreak();
	}
	st_PROFILE_DATA& pd = profileData[index];
	if (pd.lStartTime.QuadPart != 0)
	{
		DebugBreak();
	}

	LARGE_INTEGER Start;
	QueryPerformanceCounter(&Start);
	pd.lStartTime = Start; // ���� �ð� ����
	ReleaseSRWLockExclusive(&profileData->srwLock);

}


// end�� �ι� �ߴ���
// begin �� �ι� �ߴ���
// begin�� ���ϰ� end�ߴ���

// end�� �ι�
// begin �� �ι�
// begin�� ���ϰ� end
// end�� ���ϰ� begin
// flag�δ°Ÿ��� ������ֳ�

// lStartTime�� end�Ҷ� 0�� �����θ�ǳ�

void ProfileEnd(const WCHAR* funcName)
{
	if (threadIndex == -1)
	{
		__debugbreak();
	}


	st_PROFILE_DATA* profileData = threadProfileData[threadIndex];
	AcquireSRWLockExclusive(&profileData->srwLock);
	int index = -1;
	for (int i = 0; i < MAX_DATA_SIZE; i++)
	{
		long& lFlag = profileData[i].lFlag;
		if (lFlag == USING || lFlag == RESETTED)
		{
			if (wcscmp(profileData[i].szName, funcName) == 0)
			{
				if (lFlag == RESETTED)
				{
					ReleaseSRWLockExclusive(&profileData->srwLock);
					return;
				}
				index = i;
				break;
			}
		}
	}
	if (index == -1)
	{
		DebugBreak();
	}

	st_PROFILE_DATA& pd = profileData[index];

	if (pd.lStartTime.QuadPart == 0)
	{
		DebugBreak();
	}

	LARGE_INTEGER End;
	QueryPerformanceCounter(&End);
	LONGLONG timeDiff = End.QuadPart - pd.lStartTime.QuadPart;
	pd.iTotalTime += timeDiff;
	pd.lStartTime.QuadPart = 0;
	pd.iCall++; // ȣ�� Ƚ�� ����

	//[0] ���� �������� �� ����
	//[1] ���� �ι�° ���� ���� �� ����
	// �ּҰ� 2�� ����

	for (int i = minSize - 1; i >= 0; i--)
	{
		if (pd.iMin[i] > timeDiff)
		{
			pd.iMin[i] = timeDiff;
			break;
		}
	}

	//[0] ���� �ִ밪 ����
	//[1] ���� �ι��� ���� ū �� ����
	for (int i = maxSize - 1; i >= 0; i--)
	{
		if (pd.iMax[i] < timeDiff)
		{
			pd.iMax[i] = timeDiff;
			break;
		}
	}

	// �ִ밪 2�� ����
	// totalTime ����
	ReleaseSRWLockExclusive(&profileData->srwLock);

}

// �������ϸ� �� �����͸� Text ���Ϸ� ����Ѵ�


void ProfileDataOutText(const WCHAR* fileName)
{
	for (int i = 0; i <= perThreadIndex; i++)
	{
		AcquireSRWLockExclusive(&threadProfileData[i]->srwLock);
	}

	FILE* file;
	_wfopen_s(&file, fileName, L"wb");
	if (file == NULL)
	{
		printf("file open failed\n");
		return;
	}

	WCHAR subject[] = L"Name         		  | Average   | Min1      | Min2      | Max1      | Max2      | Call\n";
	fwrite(subject, sizeof(WCHAR), wcslen(subject), file);

	for (int i = 0; i <= perThreadIndex; i++)
	{
		st_PROFILE_DATA* profileData = threadProfileData[i];

		for (int j = 0; j < MAX_DATA_SIZE; j++)
		{
			if (profileData[j].lFlag == 1)
			{
				WCHAR* funcName = profileData[j].szName;
				double min1 = profileData[j].iMin[0] / (double)Freq.QuadPart * 1'000'000;
				double min2 = profileData[j].iMin[1] / (double)Freq.QuadPart * 1'000'000;
				double max1 = profileData[j].iMax[0] / (double)Freq.QuadPart * 1'000'000;
				double max2 = profileData[j].iMax[1] / (double)Freq.QuadPart * 1'000'000;
				LONGLONG iCall = profileData[j].iCall;
				double avg = 0;
				if (iCall != 0)
				{
					avg = profileData[j].iTotalTime / iCall / (double)Freq.QuadPart * 1'000'000;
				}

				fwprintf(file, L"%-25s | %-10.2lfus | %-10.2lfus | %-10.2lfus | %-10.2lfus | %-10.2lfus | %-10lld\n", funcName, avg, min1, min2, max1, max2, iCall);
			}
		}
		fwprintf(file, L"====================================\n");
	}


	int success = fclose(file);
	if (success != 0)
	{
		printf("file close failed");
	}

	for (int i = 0; i <= perThreadIndex; i++)
	{
		ReleaseSRWLockExclusive(&threadProfileData[i]->srwLock);
	}
}

void ProfileReset()
{
	//WCHAR szName[MAX_FUNCNAME_SIZE]; // �������� ���� �̸�
	//LARGE_INTEGER lStartTime; //�������� ���� ���� �ð�
	//__int64 iTotalTime; // ��ü ���ð� ī���� Time;
	//__int64 iMin[minSize]; // �ּ� ���ð� ī���� TIme
	//__int64 iMax[maxSize]; //�ִ� ���ð� ī���� Time
	//__int64 iCall; //���� ȣ�� Ƚ��

	for (int i = 0; i <= perThreadIndex; i++)
	{
		AcquireSRWLockExclusive(&threadProfileData[i]->srwLock);
	}

	for (int i = 0; i <= perThreadIndex; i++)
	{
		st_PROFILE_DATA* profileData = threadProfileData[i];
		for (int j = 0; j < MAX_DATA_SIZE; j++)
		{
			if (profileData[j].lFlag == USING)
			{
				profileData[j].lFlag = RESETTED;
				profileData[j].lStartTime.QuadPart = 0;
				WCHAR blankString[] = L"";
				profileData[j].iTotalTime = 0;
				profileData[j].iCall = 0;
				for (int k = 0; k < minSize; k++)
				{
					profileData[j].iMin[k] = INT64_MAX;
					profileData[j].iMax[k] = INT64_MIN;
				}
			}
		}
	}

	for (int i = 0; i <= perThreadIndex; i++)
	{
		ReleaseSRWLockExclusive(&threadProfileData[i]->srwLock);
	}


}

void ProfileInit()
{
	QueryPerformanceFrequency(&Freq);
}
