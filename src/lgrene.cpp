#include <iostream>
#include <cstdlib>

#include "IO.h"

int main(int const argc, char const *const *const argv)
{
	if (argc < 1)
		return EXIT_FAILURE;

	LGReneDrive device(argv[1]);
	device.Dump();

	return EXIT_SUCCESS;
}
