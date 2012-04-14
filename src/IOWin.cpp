#include <Windows.h>
#include <ntddscsi.h>
#include "Utils.h"
#include "IO.h"
#include "IOWin.h"

LGReneDrive::LGReneDrive(std::string const &drive)
	: LGReneDriveBase(drive)
{
	// Open device
	device_handle = ::CreateFile(
		(std::wstring(L"//./") + string_to_wstring(drive)).c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	if (device_handle == INVALID_HANDLE_VALUE)
		std::cout << "failed to open drive\n";
}

LGReneDrive::~LGReneDrive()
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
	}
}

void LGReneDrive::SendCommand()
{
	// Form ioctl
	PSCSI_PASS_THROUGH_DIRECT sptd = (PSCSI_PASS_THROUGH_DIRECT)operator new(sizeof(SCSI_PASS_THROUGH_DIRECT) + sizeof(cmd.sense_info));
	sptd->Length = sizeof(*sptd);

	sptd->TimeOutValue = 10;

	cmd.descriptor_length = cmd.GetCommandLength(cmd.type);
	cmd.descriptor_block[0] = cmd.GetCommand(cmd.type);
	switch (cmd.GetDirection(cmd.type))
	{
	case cmd.TRANSFER_READ:
		sptd->DataIn = SCSI_IOCTL_DATA_IN;
		break;
	case cmd.TRANSFER_WRITE:
		sptd->DataIn = SCSI_IOCTL_DATA_OUT;
		break;
	default:
		sptd->DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
		break;
	}

	std::copy(cmd.descriptor_block, cmd.descriptor_block + cmd.descriptor_length, sptd->Cdb);
	sptd->CdbLength = cmd.descriptor_length;
	
	sptd->SenseInfoOffset = sptd->Length;
	sptd->SenseInfoLength = sizeof(cmd.sense_info);

	sptd->DataTransferLength = cmd.data_length;
	sptd->DataBuffer = cmd.data;
	

	// Send ioctl
	DWORD bytes_returned;
	DWORD const transfer_size = sptd->Length + sptd->SenseInfoLength;
	BOOL ioctl_result = ::DeviceIoControl(
		device_handle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		sptd,
		transfer_size,
		sptd,
		transfer_size,
		&bytes_returned,
		NULL
		);

	if (ioctl_result == FALSE)
		std::cout << "error: " << GetLastError() << std::endl;

	// Clean up
	delete sptd;
}
