

#pragma once
 


class BLE
{
private:
	//Step 2: Get a list of services that the device advertises
	// first send 0,NULL as the parameters to BluetoothGATTServices inorder to get the number of
	// services in serviceBufferCount
	USHORT serviceBufferCount;



	PBTH_LE_GATT_SERVICE pServiceBuffer;
public:
	BLE();
	~BLE();

	HANDLE GetBLEHandle(__in GUID AGuid);

	PBTH_LE_GATT_SERVICE GetServices(__in HANDLE hBLEHandle);
	PBTH_LE_GATT_CHARACTERISTIC GetCharacteristics(__in HANDLE hBLEDevice, __in BTH_LE_GATT_SERVICE* pServiceBuffer, FILE* pipe);
};

