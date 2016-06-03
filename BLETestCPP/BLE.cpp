#include "stdafx.h"
#include "BLE.h"
#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothApis.lib")



BLE::BLE()
{
}


BLE::~BLE()
{
}

//this function works to get a handle for a BLE device based on its GUID
//copied from http://social.msdn.microsoft.com/Forums/windowshardware/en-US/e5e1058d-5a64-4e60-b8e2-0ce327c13058/erroraccessdenied-error-when-trying-to-receive-data-from-bluetooth-low-energy-devices?forum=wdk
//credits to Andrey_sh
HANDLE BLE::GetBLEHandle(__in GUID AGuid)
{
	HDEVINFO hDI;
	SP_DEVICE_INTERFACE_DATA did;
	SP_DEVINFO_DATA dd;
	GUID BluetoothInterfaceGUID = AGuid;
	HANDLE hComm = NULL;

	hDI = SetupDiGetClassDevs(&BluetoothInterfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (hDI == INVALID_HANDLE_VALUE) return NULL;

	did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	dd.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDI, NULL, &BluetoothInterfaceGUID, i, &did); i++)
	{
		SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

		DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		DWORD size = 0;

		if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, NULL, 0, &size, 0))
		{
			int err = GetLastError();

			if (err == ERROR_NO_MORE_ITEMS) break;

			PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, size);

			pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, pInterfaceDetailData, size, &size, &dd))
				break;

			hComm = CreateFile(
				pInterfaceDetailData->DevicePath,
				GENERIC_WRITE | GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			GlobalFree(pInterfaceDetailData);
		}
	}

	SetupDiDestroyDeviceInfoList(hDI);
	return hComm;
}

PBTH_LE_GATT_SERVICE BLE::GetServices(__in HANDLE hBLEDevice)
{
	////////////////////////////////////////////////////////////////////////////
	// Determine Services Buffer Size
	////////////////////////////////////////////////////////////////////////////

	HRESULT hr = BluetoothGATTGetServices(
		hBLEDevice,
		0,
		NULL,
		&serviceBufferCount,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		printf("BluetoothGATTGetServices - Buffer Size %d", hr);
	}

	pServiceBuffer = (PBTH_LE_GATT_SERVICE)
		malloc(sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);

	if (NULL == pServiceBuffer) {
		printf("pServiceBuffer out of memory\r\n");
	}
	else {
		RtlZeroMemory(pServiceBuffer,
			sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);
	}

	USHORT numServices;
	hr = BluetoothGATTGetServices(
		hBLEDevice,
		serviceBufferCount,
		pServiceBuffer,
		&numServices,
		BLUETOOTH_GATT_FLAG_NONE);

	if (S_OK != hr) {
		printf("BluetoothGATTGetServices - Buffer Size %d", hr);
	}

	return pServiceBuffer;
}
//this is the notification function
//the way ValueChangedEventParameters is utilized is shown in
//a function in Windows Driver Kit (WDK) 8.0 Samples.zip\C++\WDK 8.0 Samples\Bluetooth Low Energy (LE) Generic Attribute (GATT) Profile Drivers\Solution\WpdHealthHeartRate\HealthHeartRateService.cpp
void SomethingHappened(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

	HRESULT hr;
	if (0 == ValueChangedEventParameters->CharacteristicValue->DataSize) {
		hr = E_FAIL;
	}
	else {
		// Convert Byte Array to String value.
		printf("%s\n", ValueChangedEventParameters->CharacteristicValue->Data);
	}
}

PBTH_LE_GATT_CHARACTERISTIC BLE::GetCharacteristics(__in HANDLE hBLEDevice, __in BTH_LE_GATT_SERVICE* pServiceBuffer, __in FILE *pipe)
{
	union {
		float f;
		char b[4];
	} u;

	//Step 3: now get the list of charactersitics. note how the pServiceBuffer is required from step 2
	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Buffer Size
	////////////////////////////////////////////////////////////////////////////

	USHORT charBufferSize;
	HRESULT hr = BluetoothGATTGetCharacteristics(
		hBLEDevice,
		pServiceBuffer,
		0,
		NULL,
		&charBufferSize,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		printf("BluetoothGATTGetCharacteristics - Buffer Size %d", hr);
	}

	PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = PBTH_LE_GATT_CHARACTERISTIC();
	if (charBufferSize > 0) {
		pCharBuffer = (PBTH_LE_GATT_CHARACTERISTIC)
			malloc(charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));

		if (NULL == pCharBuffer) {
			printf("pCharBuffer out of memory\r\n");
		}
		else {
			RtlZeroMemory(pCharBuffer,
				charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));
		}

		////////////////////////////////////////////////////////////////////////////
		// Retrieve Characteristics
		////////////////////////////////////////////////////////////////////////////
		USHORT numChars;
		hr = BluetoothGATTGetCharacteristics(
			hBLEDevice,
			pServiceBuffer,
			charBufferSize,
			pCharBuffer,
			&numChars,
			BLUETOOTH_GATT_FLAG_NONE);

		//printf("%d\n", numChars);

		if (S_OK != hr) {
			printf("BluetoothGATTGetCharacteristics - Actual Data %d", hr);
		}

		if (numChars != charBufferSize) {
			printf("buffer size and buffer size actual size mismatch\r\n");
		}
	}

	

	PBTH_LE_GATT_CHARACTERISTIC currGattChar;
	for (int ii = 0; ii < charBufferSize; ii++) {
		currGattChar = &pCharBuffer[ii];
		USHORT charValueDataSize;
		PBTH_LE_GATT_CHARACTERISTIC_VALUE pCharValueBuffer;

		/*printf(
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			currGattChar->CharacteristicUuid.Value.LongUuid.Data1,    currGattChar->CharacteristicUuid.Value.LongUuid.Data2,    currGattChar->CharacteristicUuid.Value.LongUuid.Data3,
			currGattChar->CharacteristicUuid.Value.LongUuid.Data4[0], currGattChar->CharacteristicUuid.Value.LongUuid.Data4[1], currGattChar->CharacteristicUuid.Value.LongUuid.Data4[2],
			currGattChar->CharacteristicUuid.Value.LongUuid.Data4[3], currGattChar->CharacteristicUuid.Value.LongUuid.Data4[4], currGattChar->CharacteristicUuid.Value.LongUuid.Data4[5],
			currGattChar->CharacteristicUuid.Value.LongUuid.Data4[6], currGattChar->CharacteristicUuid.Value.LongUuid.Data4[7]);
		*/
		///////////////////////////////////////////////////////////////////////////
		// Determine Descriptor Buffer Size
		////////////////////////////////////////////////////////////////////////////
		USHORT descriptorBufferSize;
		hr = BluetoothGATTGetDescriptors(
			hBLEDevice,
			currGattChar,
			0,
			NULL,
			&descriptorBufferSize,
			BLUETOOTH_GATT_FLAG_NONE);

		if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
			printf("BluetoothGATTGetDescriptors - Buffer Size %d", hr);
		}

		PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;
		if (descriptorBufferSize > 0) {
			pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)
				malloc(descriptorBufferSize
					* sizeof(BTH_LE_GATT_DESCRIPTOR));

			if (NULL == pDescriptorBuffer) {
				printf("pDescriptorBuffer out of memory\r\n");
			}
			else {
				RtlZeroMemory(pDescriptorBuffer, descriptorBufferSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve Descriptors
			////////////////////////////////////////////////////////////////////////////

			USHORT numDescriptors;
			hr = BluetoothGATTGetDescriptors(
				hBLEDevice,
				currGattChar,
				descriptorBufferSize,
				pDescriptorBuffer,
				&numDescriptors,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTGetDescriptors - Actual Data %d", hr);
			}

			if (numDescriptors != descriptorBufferSize) {
				printf("buffer size and buffer size actual size mismatch\r\n");
			}

			for (int kk = 0; kk < numDescriptors; kk++) {
				PBTH_LE_GATT_DESCRIPTOR  currGattDescriptor = &pDescriptorBuffer[kk];
				////////////////////////////////////////////////////////////////////////////
				// Determine Descriptor Value Buffer Size
				////////////////////////////////////////////////////////////////////////////
				USHORT descValueDataSize;
				hr = BluetoothGATTGetDescriptorValue(
					hBLEDevice,
					currGattDescriptor,
					0,
					NULL,
					&descValueDataSize,
					BLUETOOTH_GATT_FLAG_NONE);

				if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
					printf("BluetoothGATTGetDescriptorValue - Buffer Size %d", hr);
				}

				PBTH_LE_GATT_DESCRIPTOR_VALUE pDescValueBuffer = (PBTH_LE_GATT_DESCRIPTOR_VALUE)malloc(descValueDataSize);

				if (NULL == pDescValueBuffer) {
					printf("pDescValueBuffer out of memory\r\n");
				}
				else {
					RtlZeroMemory(pDescValueBuffer, descValueDataSize);
				}

				////////////////////////////////////////////////////////////////////////////
				// Retrieve the Descriptor Value
				////////////////////////////////////////////////////////////////////////////

				hr = BluetoothGATTGetDescriptorValue(
					hBLEDevice,
					currGattDescriptor,
					(ULONG)descValueDataSize,
					pDescValueBuffer,
					NULL,
					BLUETOOTH_GATT_FLAG_NONE);
				if (S_OK != hr) {
					printf("BluetoothGATTGetDescriptorValue - Actual Data %d", hr);
				}
				//you may also get a descriptor that is read (and not notify) andi am guessing the attribute handle is out of limits
				// we set all descriptors that are notifiable to notify us via IsSubstcibeToNotification
				if (currGattDescriptor->AttributeHandle < 255) {
					BTH_LE_GATT_DESCRIPTOR_VALUE newValue;

					RtlZeroMemory(&newValue, sizeof(newValue));

					newValue.DescriptorType = ClientCharacteristicConfiguration;
					newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;

					hr = BluetoothGATTSetDescriptorValue(
						hBLEDevice,
						currGattDescriptor,
						&newValue,
						BLUETOOTH_GATT_FLAG_NONE);
					if (S_OK != hr) {
						printf("BluetoothGATTGetDescriptorValue - Actual Data %d", hr);
					}
					else {
						//printf("setting notification for serivice handle %d\n", currGattDescriptor->ServiceHandle);
					}

				}

			}
		}
		//set the appropriate callback function when the descriptor change value
		BLUETOOTH_GATT_EVENT_HANDLE EventHandle;

		/*if (currGattChar->IsNotifiable) {
			printf("Setting Notification for ServiceHandle %d\n", currGattChar->ServiceHandle);
			BTH_LE_GATT_EVENT_TYPE EventType = CharacteristicValueChangedEvent;

			BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION EventParameterIn;
			EventParameterIn.Characteristics[0] = *currGattChar;
			EventParameterIn.NumCharacteristics = 1;
			hr = BluetoothGATTRegisterEvent(
				hBLEDevice,
				EventType,
				&EventParameterIn,
				SomethingHappened,
				NULL,
				&EventHandle,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTRegisterEvent - Actual Data %d", hr);
			}
		}*/

		
		if (currGattChar->IsReadable) {//currGattChar->IsReadable
									   ////////////////////////////////////////////////////////////////////////////
									   // Determine Characteristic Value Buffer Size
									   ////////////////////////////////////////////////////////////////////////////
			hr = BluetoothGATTGetCharacteristicValue(
				hBLEDevice,
				currGattChar,
				0,
				NULL,
				&charValueDataSize,
				BLUETOOTH_GATT_FLAG_NONE);

			if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Buffer Size %d", hr);
			}

			pCharValueBuffer = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(charValueDataSize);

			if (NULL == pCharValueBuffer) {
				printf("pCharValueBuffer out of memory\r\n");
			}
			else {
				RtlZeroMemory(pCharValueBuffer, charValueDataSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve the Characteristic Value
			////////////////////////////////////////////////////////////////////////////

			hr = BluetoothGATTGetCharacteristicValue(
				hBLEDevice,
				currGattChar,
				(ULONG)charValueDataSize,
				pCharValueBuffer,
				NULL,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Actual Data %d", hr);
			}

			//print the characeteristic Value
			//for an HR monitor this might be the body sensor location
			//printf("\n Printing a read (not notifiable) characterstic");
			//for (int iii = 0; iii < pCharValueBuffer->DataSize; iii++) {// ideally check ->DataSize before printing
																		//printf("\n%d", pCharValueBuffer->Data[iii]);
				//u.b[iii] = pCharValueBuffer->Data[iii];

			//}
			//printf("\n%f", u.f);
			fprintf(pipe, "%s\n", pCharValueBuffer->Data);
			//printf("%s\n", pCharValueBuffer->Data);

			// Free before going to next iteration, or memory leak.
			free(pCharValueBuffer);
			pCharValueBuffer = NULL;
		}

	}
	return pCharBuffer;
}