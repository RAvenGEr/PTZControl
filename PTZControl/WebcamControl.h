#pragma once

#include <vector>

#include <Ks.h>
#include <KsProxy.h>		// For IKsControl
#include <vidcap.h>			// For IKsNodeControl

#include <afxstr.h>

#include "LogitechTypes.h"

struct WebcamDevice
{
	const CString deviceName;
	const CString devicePath;
};

struct UsbIdentifier
{
	DWORD vid;
	DWORD pid;
};

/**
* (see https://msdn.microsoft.com/en-us/library/windows/hardware/ff568656(v=vs.85).aspx).
*/
class WebcamController
{
public:
	static constexpr size_t NUM_PRESETS{ 8 };
	static constexpr int DEFAULT_MOTOR_INTERVAL{ 70 };

	/** Retrieve compatible devices, optionally filtering by name */
	static std::vector<WebcamDevice> CompatibleDevices(std::vector<CString> deviceNameFilters = {});

	WebcamController() {}

	HRESULT OpenDevice(const CString &devicePath);
	HRESULT OpenDevice(const UsbIdentifier usbId);
	void CloseDevice();
	HRESULT IsPeripheralPropertySetSupported();

	int GetCurrentZoom();
	int Zoom(int direction);
	void MoveTilt(int yDirection);
	void MovePan(int xDirection);
	void Tilt(int yDirection);
	void Pan(int xDirection);

	void GotoHome();
	void SavePreset(int iNum);
	void GotoPreset(int iNum);

	int motorIntervalTime{ DEFAULT_MOTOR_INTERVAL };
	bool useLogitechMotionControl{ false };

private:
	static constexpr DWORD NONODE{ 0xFFFFFF };

	HRESULT OpenDevice(CComPtr<IMoniker> pMoniker);
	HRESULT InitializeXUNodesArray(CComPtr<IKsControl> pKsControl);
	bool IsExtensionUnitSupported(CComPtr<IKsControl> pKsControl, const GUID& guidExtension, unsigned int nodeId);

	//HRESULT GetProperty(LOGITECH_XU_PROPERTYSET lPropertySet, ULONG ulPropertyId, ULONG ulSize, VOID* pValue);
	HRESULT SetProperty(LOGITECH_XU_PROPERTYSET lPropertySet, ULONG ulPropertyId, ULONG ulSize, VOID* pValue);

	CComPtr<IKsControl> m_spKsControl{};
	CComQIPtr<IAMCameraControl> m_spAMCameraControl{};

	DWORD m_dwXUDeviceInformationNodeId{ NONODE };
	DWORD m_dwXUVideoPipeControlNodeId{ NONODE };
	DWORD m_dwXUTestDebugNodeId{ NONODE };
	DWORD m_dwXUPeripheralControlNodeId{ NONODE };

	bool m_bMechanicalPanTilt{ false };
	long m_lDigitalTiltMin{ -1 };
	long m_lDigitalTiltMax{ -1 };
	long m_lDigitalPanMin{ -1 };
	long m_lDigitalPanMax{ -1 };
};
