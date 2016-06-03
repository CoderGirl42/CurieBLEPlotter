// How to get notification values from a HR BLE monitor
//Ensure that you have paired the HR BLE monitor with the computer


#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "BLE.h"
#define TO_SEARCH_DEVICE_UUID "{19B10000-E8F2-537E-4F6C-D104768A1214}" //we use UUID for an HR BLE device 
using namespace std;


/*void GUIDToString(GUID* guid) {

	printf(
		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2],
		guid->Data4[3], guid->Data4[4], guid->Data4[5],
		guid->Data4[6], guid->Data4[7]);

}

int main(int argc, char *argv[], char *envp[])
{
	BLE * bleDevice = new BLE();
	GUID AGuid;
	CLSIDFromString(TEXT(TO_SEARCH_DEVICE_UUID), &AGuid);

	HANDLE hLEDevice = bleDevice->GetBLEHandle(AGuid);

	BTH_LE_GATT_SERVICE* pServiceBuffer = bleDevice->GetServices(hLEDevice);

	//for(int i = 0; i < 10000; i++)
	PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = bleDevice->GetCharacteristics(hLEDevice, pServiceBuffer);

	//go into an inf loop that sleeps. you will ideally see notifications from the HR device
	while (1) {
		Sleep(1000);
		//printf("look for notification");
	}

	CloseHandle(hLEDevice);

	if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS)
	{
		// Insert error handling here.
		return 1;
	}

return 0;
}

*/

// Tested on:
// 1. Visual Studio 2012 on Windows
// 2. Mingw gcc 4.7.1 on Windows
// 3. gcc 4.6.3 on GNU/Linux

// Note that gnuplot binary must be on the path
// and on Windows we need to use the piped version of gnuplot
#ifdef WIN32
#define GNUPLOT_NAME "C:/gnuplot/bin/gnuplot.exe -persist"
#else 
#define GNUPLOT_NAME "gnuplot"
#endif

int main()
{
#ifdef WIN32
	FILE *pipe = _popen(GNUPLOT_NAME, "w");
#else
	FILE *pipe = popen(GNUPLOT_NAME, "w");
#endif
	if (pipe != NULL)
	{
		fprintf(pipe, "set term wx\n");         // set the terminal
		//fprintf(pipe, "set hidden3d\n");
		//fprintf(pipe, "set dgrid3d 10, 10 qnorm 2\n");

		fprintf(pipe, "splot '-' with linespoints\n"); // plot type
		
		BLE * bleDevice = new BLE();
		GUID AGuid;
		CLSIDFromString(TEXT(TO_SEARCH_DEVICE_UUID), &AGuid);

		HANDLE hLEDevice = bleDevice->GetBLEHandle(AGuid);

		BTH_LE_GATT_SERVICE* pServiceBuffer = bleDevice->GetServices(hLEDevice);

		for(int i = 0; i < 50; i++)
			PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = bleDevice->GetCharacteristics(hLEDevice, pServiceBuffer, pipe);

		
		//for (int i = 0; i < 10; i++)             // loop over the data [0,...,9]
		//	fprintf(pipe, "%d\n", i);           // data terminated with \n
		
		
		
		fprintf(pipe, "%s\n", "e");             // termination character
		fflush(pipe);                           // flush the pipe

												// wait for key press
		std::cin.clear();
		std::cin.ignore(std::cin.rdbuf()->in_avail());
		std::cin.get();

#ifdef WIN32
		_pclose(pipe);
#else
		pclose(pipe);
#endif

		CloseHandle(hLEDevice);

		if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			// Insert error handling here.
			return 1;
		}
	}
	else
		std::cout << "Could not open pipe" << std::endl;
	return 0;
}

