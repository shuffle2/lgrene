#pragma once

#include <Windows.h>

struct LGReneDrive : public LGReneDriveBase
{
	LGReneDrive(std::string const &drive);
	~LGReneDrive();

	void SendCommand();

	HANDLE device_handle;
};
