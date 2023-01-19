#pragma once
#include <ntddk.h>

void PloadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) 
{
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageInfo);
	DbgPrint("[+] Module loaded: %wZ\n", FullImageName);

	wchar_t* moduleName = L"\\csgo\\bin\\client.dll";

	if (FullImageName) {
		if (wcswcs(FullImageName->Buffer, moduleName)) {
			DbgPrint("[+] Found client.dll");			
		}
	}
}
