#include <iostream>
#include <algorithm>
#include <Windows.h>
#include <ntddscsi.h>
#include "IO.h"

LGReneDrive::LGReneDrive(std::string const &drive)
	: drive_path(drive)
{
}

void LGReneDrive::Dump(std::string const &out_file)
{
	std::cout << "dumping " << drive_path << " to " << out_file << std::endl;
	// stop
	cmd.Clear();
	cmd.type = cmd.START_STOP;
	SendCommand();
	/*/ eject
	cmd.Clear();
	cmd.type = cmd.START_STOP;
	// Load/Eject bit
	cmd.descriptor_block[4] = 0x02;
	SendCommand();
	//*/
	// Read
	cmd.Clear();
	cmd.data_length = 0xfffc;
	cmd.data = new uint8_t[cmd.data_length];
	std::fill_n(cmd.data, cmd.data_length, 0);
	cmd.type = cmd.READ;
	uint32_t const position = 0;
	cmd.descriptor_block[1] = REGION_MEMORY;
	cmd.descriptor_block[2] = position >> 24;
	cmd.descriptor_block[2] = (position >> 16) & 0xff;
	cmd.descriptor_block[2] = (position >> 8) & 0xff;
	cmd.descriptor_block[2] = position & 0xff;
	cmd.descriptor_block[7] = cmd.data_length >> 8;
	cmd.descriptor_block[8] = cmd.data_length & 0xff;
	// ???
	cmd.descriptor_block[9] = 0x44;
	SendCommand();
	delete [] cmd.data;

	// Inquiry
	cmd.Clear();
	// The response will include the actual amount read
	cmd.data_length = 0xfffc;
	cmd.data = new uint8_t[cmd.data_length];
	std::fill_n(cmd.data, cmd.data_length, 0);
	cmd.type = cmd.INQUIRY;
	cmd.descriptor_block[3] = cmd.data_length >> 8;
	cmd.descriptor_block[4] = cmd.data_length & 0xff;
	SendCommand();
	// parse inquiry
	InquiryResponse inq;
	inq.peripheral_qualifier	= cmd.data[0] >> 4;
	inq.peripheral_device_type	= cmd.data[0] & 0x1f;
	inq.removable_media			= (cmd.data[1] >> 7) & 1;
	inq.version					= cmd.data[2];
	inq.normal_ACA				= (cmd.data[3] >> 4) & 1;
	inq.hierarchical			= (cmd.data[3] >> 3) & 1;
	inq.response_data_format	= cmd.data[3] & 0xf;
	inq.additional_length		= cmd.data[4];
	inq.SCC						= (cmd.data[5] >> 7) & 1;
	inq.ACC						= (cmd.data[5] >> 6) & 1;
	inq.TPG						= (cmd.data[5] >> 5) & 3;
	inq.third_party_copy		= (cmd.data[5] >> 3) & 1;
	inq.protect					= cmd.data[5] & 1;
	inq.basic_queuing			= (cmd.data[6] >> 7) & 1;
	inq.enclosure_services		= (cmd.data[6] >> 6) & 1;
	inq.multiport				= (cmd.data[6] >> 4) & 1;
	inq.medium_changer			= (cmd.data[6] >> 3) & 1;
	inq.linked_command			= (cmd.data[7] >> 3) & 1;
	inq.command_queuing			= cmd.data[7] & 1;
	std::copy_n(&cmd.data[8], sizeof(inq.vendor_id) - 1, inq.vendor_id);
	std::copy_n(&cmd.data[16], sizeof(inq.product_id) - 1, inq.product_id);
	std::copy_n(&cmd.data[32], sizeof(inq.product_revision) - 1, inq.product_revision);
	std::copy_n(&cmd.data[36], sizeof(inq.drive_serial), inq.drive_serial);
	std::copy_n(&cmd.data[44], sizeof(inq.vendor_unique), inq.vendor_unique);
	delete [] cmd.data;

	if (inq.peripheral_device_type == inq.DEVTYPE_MMC4)
	{
		std::cout << "got inquiry response of " << 4 + inq.additional_length << " bytes\n";
		std::cout <<
			"vendor: " << inq.vendor_id << 
			" product: " << inq.product_id <<
			" revision: " << inq.product_revision << std::endl;
		print_hex_and_ascii("drive serial", inq.drive_serial);
		print_hex_and_ascii("vendor unique", inq.vendor_unique);
	}
	else
	{
		std::cout << drive_path << " is not a MMC4 device\n";
	}
}

void LGReneDrive::SendCommand()
{
	// Open device
	HANDLE device_handle = ::CreateFile(
		(std::wstring(L"//./") + (wchar_t)drive_path.at(0) + L":").c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	if (device_handle == INVALID_HANDLE_VALUE)
		std::cout << "failed to open drive\n";

	// Form ioctl
	PSCSI_PASS_THROUGH_DIRECT sptd = (PSCSI_PASS_THROUGH_DIRECT)operator new(sizeof(SCSI_PASS_THROUGH_DIRECT) + sizeof(cmd.sense_info));
	sptd->Length = sizeof(*sptd);

	sptd->TimeOutValue = 10;

	cmd.descriptor_length = cmd.GetCommandLength(cmd.type);
	cmd.descriptor_block[0] = cmd.GetCommand(cmd.type);
	switch (cmd.GetDirection(cmd.type))
	{
	case TRANSFER_READ:
		sptd->DataIn = SCSI_IOCTL_DATA_IN;
		break;
	case TRANSFER_WRITE:
		sptd->DataIn = SCSI_IOCTL_DATA_OUT;
		break;
	default:
		sptd->DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
		break;
	}

	std::copy(cmd.descriptor_block, cmd.descriptor_block + cmd.descriptor_length, sptd->Cdb);
	sptd->CdbLength = cmd.descriptor_length;
	
	sptd->SenseInfoOffset = sizeof(*sptd);
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

	std::cout << "ioctl returned " <<
		((ioctl_result == TRUE) ? "true" : "false") << std::endl;
	if (ioctl_result == FALSE)
		std::cout << "error: " << GetLastError() << std::endl;

	// Clean up
	delete sptd;
	::CloseHandle(device_handle);
}
