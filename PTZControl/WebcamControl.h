#pragma once

#include <vector>

#include <Ks.h>
#include <KsProxy.h>		// For IKsControl
#include <vidcap.h>			// For IKsNodeControl

#include <afxstr.h>

#include "LogitechTypes.h"

struct WebcamDevice
{
	const CString name;
	const CString path;
};

enum class PanDirection : long {
	Stop = 0,
	Left = -1,
	Right = 1,
};

enum class TiltDirection : long {
	Stop = 0,
	Down = -1,
	Up = 1,
};

enum class ZoomDirection : long {
	Stop = 0,
	Out = -1,
	In = 1,
};

template <typename T>
constexpr auto to_underlying(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

/** Retrieve webcam devices, optionally filtering by name */
std::vector<WebcamDevice> WebcamDevices(std::vector<CString> deviceNameFilters = {});

/**
* (see https://msdn.microsoft.com/en-us/library/windows/hardware/ff568656(v=vs.85).aspx).
*/
class WebcamController
{
public:
	static constexpr size_t NUM_PRESETS{ 8 };

	WebcamController() = default;
	WebcamController(WebcamController&) = delete;
	WebcamController(WebcamController&&) = default;

	WebcamController& operator=(WebcamController&) = delete;
	WebcamController& operator=(WebcamController&&) = default;

	WebcamController(const CString& name, const CString& path) :m_device{ name, path } {}

	HRESULT OpenDevice(const WebcamDevice& devicePath);
	HRESULT ReOpenDevice();
	void CloseDevice();
	HRESULT IsPeripheralPropertySetSupported() const;

	int GetCurrentZoom();
	int Zoom(ZoomDirection zoom);

	/** Start tilt in supplied direction or stop */
	void Tilt(TiltDirection tilt);

	/** Start pan in supplied direction or stop */
	void Pan(PanDirection pan);

	/** Tilt camera step at a time using Logitech control */
	void LogiMotionTilt(TiltDirection tilt);

	/** Pan camera step at a time using Logitech control */
	void LogiMotionPan(PanDirection pan);

	/** Digital tilt step at a time */
	void DigitalTilt(TiltDirection tilt);

	/** Digital pan step at a time */
	void DigitalPan(PanDirection pan);

	void GotoHome();
	void SavePreset(int iNum);
	void GotoPreset(int iNum);

	const CString& Path() const {
		return m_device.path;
	}

	const CString& Name() const {
		return m_device.name;
	}

	bool invertLogitech{ true };

private:
	static constexpr DWORD NONODE{ 0xFFFFFF };

	HRESULT OpenDevice(CComPtr<IMoniker> pMoniker);
	HRESULT InitializeXUNodesArray(CComPtr<IKsControl> pKsControl);
	bool IsExtensionUnitSupported(CComPtr<IKsControl> pKsControl, const GUID& guidExtension, unsigned int nodeId);

	HRESULT SetProperty(LOGITECH_XU_PROPERTYSET lPropertySet, ULONG ulPropertyId, ULONG ulSize, VOID* pValue);

	CComPtr<IKsControl> m_spKsControl{};
	CComQIPtr<IAMCameraControl> m_spAMCameraControl{};

	WebcamDevice m_device{};

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
