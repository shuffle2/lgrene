#include <iostream>
#include <fstream>
#include <algorithm>
#include "IO.h"
#include "Utils.h"

LGReneDriveBase::LGReneDriveBase(std::string const &drive)
	: drive_path(drive)
{
}

void LGReneDriveBase::Stop()
{
	cmd.Clear();
	cmd.type = cmd.START_STOP;
	SendCommand();
}

void LGReneDriveBase::Eject()
{
	cmd.Clear();
	cmd.type = cmd.START_STOP;
	// Load/Eject bit
	cmd.descriptor_block[4] = 0x02;
	SendCommand();
}

void LGReneDriveBase::ReadBuffer(Region const region, uint32_t const offset, uint16_t const size)
{
	cmd.Clear();
	cmd.data_length = size;
	cmd.type = cmd.READ;
	cmd.descriptor_block[1] = region;
	cmd.descriptor_block[2] = offset >> 24;
	cmd.descriptor_block[3] = (offset >> 16) & 0xff;
	cmd.descriptor_block[4] = (offset >> 8) & 0xff;
	cmd.descriptor_block[5] = offset & 0xff;
	cmd.descriptor_block[7] = cmd.data_length >> 8;
	cmd.descriptor_block[8] = cmd.data_length & 0xff;
	// ???
	cmd.descriptor_block[9] = 0x44;
	SendCommand();
}

void LGReneDriveBase::Inquiry()
{
	cmd.Clear();
	// The response will include the actual amount read
	cmd.data_length = cmd.data_length_max;
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
	inq.vendor_id				= std::string((const char *)&cmd.data[8], 8);
	inq.product_id				= std::string((const char *)&cmd.data[16], 16);
	inq.product_revision		= std::string((const char *)&cmd.data[32], 4);
	std::copy_n(&cmd.data[36], sizeof(inq.drive_serial), inq.drive_serial);
	std::copy_n(&cmd.data[44], sizeof(inq.vendor_unique), inq.vendor_unique);

	if (inq.peripheral_device_type == inq.DEVTYPE_MMC4)
	{
		int const total_length = 4 + inq.additional_length;
		std::cout << "got inquiry response of " << total_length << " bytes\n";
		if (total_length >= 16)
			std::cout << "vendor: " << inq.vendor_id;
		if (total_length >= 32)
			std::cout << " product: " << inq.product_id;
		if (total_length >= 36)
			std::cout << " revision: " << inq.product_revision;
		std::cout << std::endl;
		if (total_length >= 44)
			print_hex_and_ascii("drive serial", inq.drive_serial);
		if (total_length >= 56)
			print_hex_and_ascii("vendor unique", inq.vendor_unique);
	}
	else
	{
		std::cout << drive_path << " is not a MMC4 device\n";
	}
}

void LGReneDriveBase::Dump(std::string const &out_file)
{	
	// Poke the drive to wake it up
	Stop();

	// See if it's valid / show pretty info
	Inquiry();
	
	// read stuff to show it works
	ReadBuffer(REGION_MEMORY, 0x400000, cmd.data_length_max);
	std::cout << "here's some mem @ 0x400000\n";
	hexdump_n(cmd.data, 0x800);

	std::cout << "dumping " << drive_path << " to " << out_file << std::endl;

	std::ofstream output_file(out_file, std::ios::binary);
	uint32_t const increment = cmd.data_length_max;
	// have to hard reset drive after reading a bit above 0x04e00000 :'(
	for (uint32_t offset = 0; offset + increment < 0x04e00000; offset += increment)
	{
		std::cout << std::fixed << std::setprecision(2) <<
			'\r' << offset / (double)0x04e00000 * 100. << "% " <<
			'(' << std::showbase << std::hex << offset << ')';
		ReadBuffer(REGION_MEMORY, offset, increment);
		output_file.write((char const *const)cmd.data, cmd.data_length);
	}
	std::cout << std::endl;
	output_file.close();
}
