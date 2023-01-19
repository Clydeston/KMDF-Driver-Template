#include <ntddk.h>
#include "ioctl.h"
#include "callbacks.h"

UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\DRIVERNAME");
UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(L"\\??\\DRIVERNAME");

void DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS HandleRequest(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS IOCTLDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DbgPrint("[+] Loaded Driver");

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = HandleRequest;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = HandleRequest;	
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IOCTLDeviceControl;

	// SYMBOLIC LINK CREATION
	PDEVICE_OBJECT pDevice;
	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		false,
		&pDevice);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("[+] Device Creation Failed (0x%08X)\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&symLinkName, &deviceName);

	if (!NT_SUCCESS(status)) {
		DbgPrint("[+] Symbolic Link Creation Failed (0x%08X)\n", status);
		// device cleanup
		IoDeleteDevice(pDevice);
		return status;
	}

	// CALLBACKS
	NTSTATUS imageLoadNotify = PsSetLoadImageNotifyRoutine(PloadImageNotifyRoutine);
	if (!NT_SUCCESS(imageLoadNotify))
	{
		DbgPrint("PsSetCreateProcessNotifyRoutine failed (0x%08X)\n", imageLoadNotify);
	}

	// MUST RETURN SUCCESSFUL STATUS 
	// OTHERWISE UNLOAD WON'T GET CALLED
	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	DbgPrint("[+] Driver Unload \n");
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(DriverObject->DeviceObject);
	/* REMOVE CALLBACKS */	
	PsRemoveLoadImageNotifyRoutine(PloadImageNotifyRoutine);
	DbgPrint("[+] Driver Unload Completed \n");
}

NTSTATUS HandleRequest(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	DbgPrint("[+] Handle Request Received\n");
	UNREFERENCED_PARAMETER(DeviceObject);	

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS IOCTLDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR dataLength = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	// IOCTL CODES HERE
	case TEST_CODE:
	{
		// DATA RECEIVED 
		DataReceived* dataReceived = (DataReceived*)Irp->AssociatedIrp.SystemBuffer;
		dataReceived->DATA = (PULONG)01011;
		dataLength = stack->Parameters.DeviceIoControl.InputBufferLength;
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		DbgPrint("[+] Invalid IOCTL Request\n");
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dataLength;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}