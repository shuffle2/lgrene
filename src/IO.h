#pragma once

#include <string>
#include <cstdint>
#include "Utils.h"

struct LGReneDriveBase
{
	enum Region
	{
		REGION_CACHE_SIZE = 1,
		REGION_CACHE = 2,
		REGION_MEMORY = 5,
		REGION_DEBUG = 6
	};

	struct Command
	{
		enum TransferDirection
		{
			TRANSFER_READ,
			TRANSFER_WRITE,
			TRANSFER_UNKNOWN,
		};

		enum
		{
			LENGTH_6,
			LENGTH_10,
			LENGTH_10_,
			LENGTH_RESERVED,
			LENGTH_16,
			LENGTH_12,
			LENGTH_VENDOR,
			LENGTH_VENDOR_
		};

		enum TypeInfo
		{
			INQUIRY		= (TRANSFER_READ << 8) | (LENGTH_6   << 5) | 0x12,
			START_STOP	= (TRANSFER_READ << 8) | (LENGTH_6   << 5) | 0x1b,
			WRITE		= (TRANSFER_WRITE<< 8) | (LENGTH_10  << 5) | 0x1b,
			READ		= (TRANSFER_READ << 8) | (LENGTH_10  << 5) | 0x1c,
			READ_TOC	= (TRANSFER_READ << 8) | (LENGTH_10_ << 5) | 0x03,
			DISC_INFO	= (TRANSFER_READ << 8) | (LENGTH_10_ << 5) | 0x11,
			TRACK_INFO	= (TRANSFER_READ << 8) | (LENGTH_10_ << 5) | 0x12,
			MODE_SENSE	= (TRANSFER_READ << 8) | (LENGTH_10_ << 5) | 0x1a
		};

		TypeInfo type;
		uint8_t descriptor_block[16];
		uint8_t descriptor_length;
		// TODO on windows this may need to be aligned. Care if/when it matters
		uint8_t *data;
		uint32_t data_length;
		// < 0xffff and multiple of 2
		static uint32_t const data_length_max = 0xffff & ~1;
		uint8_t sense_info[24];

		Command()
		{
			data = new uint8_t[data_length_max];
		}

		~Command()
		{
			delete [] data;
		}
		
		void Clear()
		{
			memzero(descriptor_block);
			descriptor_length = 0;
			memzero_n(data, data_length_max);
			data_length = 0;
			memzero(sense_info);
		}

		uint8_t const GetCommand(TypeInfo const ti) const
		{
			return ti & 0xff;
		}

		uint8_t const GetCommandLength(TypeInfo const ti) const
		{
			switch ((ti >> 5) & 3)
			{
			case LENGTH_6:
				return 6;
			case LENGTH_10:
			case LENGTH_10_:
				return 10;
			case LENGTH_12:
				return 12;
			case LENGTH_16:
				return 16;
			default:
				std::cerr << "TypeInfo has no length\n";
				return 0;
			}
		}

		TransferDirection const GetDirection(TypeInfo const ti) const
		{
			return (TransferDirection)(ti >> 8);
		}
	};

	// Standard Inquiry
	struct InquiryResponse
	{
		enum
		{
			DEVTYPE_MMC4 = 5
		};

		uint8_t peripheral_qualifier;
		uint8_t peripheral_device_type;
		bool removable_media;
		uint8_t version;
		bool normal_ACA;
		bool hierarchical;
		uint8_t response_data_format;
		uint8_t additional_length;
		bool SCC;
		// Access Controls Coordinator
		bool ACC;
		// Target Port Group
		uint8_t TPG;
		bool third_party_copy;
		bool protect;
		bool basic_queuing;
		bool enclosure_services;
		bool multiport;
		bool medium_changer;
		bool linked_command;
		bool command_queuing;
		std::string vendor_id;
		std::string product_id;
		std::string product_revision;
		uint8_t drive_serial[8];
		uint8_t vendor_unique[12];
	};

	LGReneDriveBase(std::string const &drive);

	void Dump(std::string const &out_file = "./dump");
	virtual void SendCommand() = 0;
	void Stop();
	void Eject();
	void ReadBuffer(Region const region, uint32_t const offset, uint16_t const size);
	void Inquiry();

	std::string drive_path;
	Command cmd;
};

#if defined(_MSC_VER)
#include "IOWin.h"
#else
#error Platform not implemented
#endif
