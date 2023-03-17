#include "pch.h"

#include <initguid.h>
#include <comdef.h>
#define NO_DSHOW_STRSAFE	// Avoid more C4995 warnings in intrin.h
#include <DShow.h>
#include <Ks.h>
#include <KsMedia.h>

#pragma comment(lib, "strmiids.lib")

#include "WebcamControl.h"

//////////////////////////////////////////////////////////////////////////

/**
* Checks whether the device represented by the given IMoniker matches the given device path
*/
static bool DevicePathFromMoniker(CComPtr<IMoniker> pMoniker, CString &devicePath)
{
	CComPtr<IPropertyBag> pPropBag;
	// Open the device properties
	HRESULT hr = pMoniker->BindToStorage(NULL, NULL, IID_PPV_ARGS(&pPropBag));
	if (FAILED(hr))
		return false;

	CComVariant varPath;
	// Retrieve the device path
	hr = pPropBag->Read(L"DevicePath", &varPath, 0);
	if (FAILED(hr) || varPath.bstrVal == NULL)
		return false;

	if (FAILED(varPath.ChangeType(VT_BSTR)))
		return false;

	devicePath = varPath.bstrVal;
	return true;
}

static bool ParseDevicePath(CString devicePath, UsbIdentifier& usbId)
{
	if (swscanf_s(devicePath.MakeLower(), L"\\\\?\\usb#vid_%04x&pid_%04x", &usbId.vid, &usbId.pid) != 2)
		return false;
	return true;
}

static bool UsbIdsMatch(const UsbIdentifier value, const UsbIdentifier match)
{
	return (value == match)
		|| (match.pid == 0 && match.vid == match.vid) 
		|| (value.pid == match.pid && match.vid == 0);
}

static bool DeviceMatches(CComPtr<IMoniker> pMoniker, const UsbIdentifier usbId)
{
	if (usbId.vid == 0 && usbId.pid == 0) {
		return true;
	}
	CString devicePath;
	UsbIdentifier idFromPath;
	return DevicePathFromMoniker(pMoniker, devicePath) 
		&& ParseDevicePath(devicePath, idFromPath) 
		&& UsbIdsMatch(idFromPath, usbId);
}

static bool DeviceMatches(CComPtr<IMoniker> pMoniker, const CString devicePath)
{
	CString devicePathMoniker;
	return DevicePathFromMoniker(pMoniker, devicePathMoniker)
		&& (devicePathMoniker == devicePath);
}

template <typename T>
static HRESULT GetDeviceMoniker(T deviceMatch, CComPtr<IMoniker>& pMoniker)
{
	// Create the System Device Enumerator
	CComPtr<ICreateDevEnum> pSysDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, __uuidof(ICreateDevEnum), (void**)&pSysDevEnum);
	if (FAILED(hr))
		return hr;

	// Obtain a class enumerator for the video input device category
	CComPtr<IEnumMoniker> pEnumCat;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);
	if (FAILED(hr))
		return hr;

	// Enumerate the monikers and check if we can find a matching device
	ULONG cFetched;
	while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) {
		CString devicePath;
		
		if (DeviceMatches(pMoniker, deviceMatch)) {
			return S_OK;
		}
		pMoniker.Release();
	}
	return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
}

HRESULT WebcamController::OpenDevice(const CString &devicePath)
{
	CComPtr<IMoniker> pMoniker;
	HRESULT hr = GetDeviceMoniker(devicePath, pMoniker);
	if (FAILED(hr) || !pMoniker)
		return hr;
	return OpenDevice(pMoniker);
}

HRESULT WebcamController::OpenDevice(const UsbIdentifier usbId)
{
	CComPtr<IMoniker> pMoniker;
	HRESULT hr = GetDeviceMoniker(usbId, pMoniker);
	if (FAILED(hr) || !pMoniker)
		return hr;
	return OpenDevice(pMoniker);
}

void WebcamController::CloseDevice()
{
	m_spKsControl.Release();;
	m_spAMCameraControl.Release();

	m_dwXUDeviceInformationNodeId = NONODE;
	m_dwXUVideoPipeControlNodeId = NONODE;
	m_dwXUTestDebugNodeId = NONODE;
	m_dwXUPeripheralControlNodeId = NONODE;

	motorIntervalTime = DEFAULT_MOTOR_INTERVAL;
}

HRESULT WebcamController::IsPeripheralPropertySetSupported() const
{
	if (!m_spKsControl)
		return -1;

	KSP_NODE extProp{};
	extProp.Property.Set = LOGITECH_XU_PERIPHERAL_CONTROL;
	extProp.Property.Id = 0;
	extProp.Property.Flags = KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
	extProp.NodeId = m_dwXUPeripheralControlNodeId;
	extProp.Reserved = 0;
	ULONG ulBytesReturned;
	return m_spKsControl->KsProperty((PKSPROPERTY)&extProp, sizeof(extProp), NULL, 0, &ulBytesReturned);
}


HRESULT WebcamController::SetProperty(LOGITECH_XU_PROPERTYSET lPropertySet, ULONG ulPropertyId, ULONG ulSize, VOID* pValue)
{
	if (!m_spKsControl)
		return -1;

	ASSERT(pValue != 0 && ulSize != 0);

	KSP_NODE extprop{};
	extprop.Property.Id = ulPropertyId;
	extprop.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

	switch (lPropertySet)
	{
	case XU_DEVICE_INFORMATION:
		extprop.NodeId = m_dwXUDeviceInformationNodeId;
		extprop.Property.Set = LOGITECH_XU_DEVICE_INFORMATION;
		break;
	case XU_VIDEOPIPE_CONTROL:
		extprop.NodeId = m_dwXUVideoPipeControlNodeId;
		extprop.Property.Set = LOGITECH_XU_VIDEOPIPE_CONTROL;
		break;
	case XU_TEST_DEBUG:
		extprop.NodeId = m_dwXUTestDebugNodeId;
		extprop.Property.Set = LOGITECH_XU_TEST_DEBUG;
		break;
	case XU_PERIPHERAL_CONTROL:
		extprop.NodeId = m_dwXUPeripheralControlNodeId;
		extprop.Property.Set = LOGITECH_XU_PERIPHERAL_CONTROL;
		break;
	}

	ULONG ulBytesReturned;
	HRESULT hr = m_spKsControl->KsProperty(
		(PKSPROPERTY)&extprop,
		sizeof(extprop),
		pValue,
		ulSize,
		&ulBytesReturned
	);

	return hr;
}

HRESULT WebcamController::OpenDevice(CComPtr<IMoniker> pMoniker)
{
	CComPtr<IKsControl> pKsControl;

	// Get a pointer to the IKsControl interface
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_PPV_ARGS(&pKsControl));
	if (FAILED(hr))
		return hr;

	if (!pKsControl)
		return E_POINTER;

	// Find the H.264 XU node
	hr = InitializeXUNodesArray(pKsControl);
	if (FAILED(hr))
		return hr;

	// save the pointer, we succeeded
	m_spKsControl = pKsControl;
	m_spAMCameraControl = pKsControl;

	long lValue;
	long lFlags;
	hr = m_spAMCameraControl->Get(KSPROPERTY_CAMERACONTROL_PAN_RELATIVE, &lValue, &lFlags);
	m_bMechanicalPanTilt = SUCCEEDED(hr);

#ifdef _DEBUG
	for (unsigned i = 0; i <= 19; ++i)
	{
		hr = m_spAMCameraControl->Get(i, &lValue, &lFlags);
		if (SUCCEEDED(hr))
			TRACE(__FUNCTION__ " m_spAMCameraControl-%d val=%d, flag=%d\n", i, lValue, lFlags);
	}
#endif // _DEBUG

	long lMin;
	long lMax;
	long lSteppingSize;
	long lDefaults;

	hr = m_spAMCameraControl->GetRange(CameraControl_Pan, &lMin, &lMax, &lSteppingSize, &lDefaults, &lFlags);
	if (SUCCEEDED(hr)) {
		m_lDigitalPanMin = lMin;
		m_lDigitalPanMax = lMax;
	}

	hr = m_spAMCameraControl->GetRange(CameraControl_Tilt, &lMin, &lMax, &lSteppingSize, &lDefaults, &lFlags);
	if (SUCCEEDED(hr)) {
		m_lDigitalTiltMin = lMin;
		m_lDigitalTiltMax = lMax;
	}

	return S_OK;
}

void WebcamController::GotoHome()
{
	// Zoom to Home
	{
		DWORD dwValue = 0;
		SetProperty(XU_VIDEOPIPE_CONTROL, XU_VIDEO_FW_ZOOM_CONTROL, sizeof(dwValue), &dwValue);
	}
	// Zoom to Home
	// Home = No Action
	//			1 ?
	//			2 ?
	// Goto Home = 3
	// 8 Presets
	// Preset 1-8 = 4-11, 
	// Goto Preset 1-8 = 12-19
	// Test 22 
	{
		DWORD dwValue(3);
		SetProperty(XU_PERIPHERAL_CONTROL, XU_PERIPHERALCONTROL_PANTILT_MODE_CONTROL, sizeof(DWORD), &dwValue);
	}
	return;
}

void WebcamController::SavePreset(int iNum)
{
	if (iNum < 0 || iNum >= NUM_PRESETS)
		return;

	// Zoom to Home
	// Home = No Action
	//			1 ?
	//			2 ?
	// Goto Home = 3
	// 8 Presets
	// Preset 1-8 = 4-11, 
	// Goto Preset 1-8 = 12-19
	// Test 22 
	DWORD dwValue(iNum + 4);
	SetProperty(XU_PERIPHERAL_CONTROL, XU_PERIPHERALCONTROL_PANTILT_MODE_CONTROL, sizeof(DWORD), &dwValue);
}

void WebcamController::GotoPreset(int iNum)
{
	if (iNum < 0 || iNum >= NUM_PRESETS)
		return;

	// Zoom to Home
	// Home = No Action
	//			1 ?
	//			2 ?
	// Goto Home = 3
	// 8 Presets
	// Preset 1-8 = 4-11, 
	// Goto Preset 1-8 = 12-19
	// Test 22 
	DWORD dwValue(iNum + 12);
	SetProperty(XU_PERIPHERAL_CONTROL, XU_PERIPHERALCONTROL_PANTILT_MODE_CONTROL, sizeof(DWORD), &dwValue);
}

int WebcamController::GetCurrentZoom()
{
	if (!m_spAMCameraControl)
		return -1;

	long oldZoom;
	long flags = CameraControl_Flags_Manual;
	m_spAMCameraControl->Get(CameraControl_Zoom, &oldZoom, &flags);
	return oldZoom;
}

int WebcamController::Zoom(const ZoomDirection zoom)
{
	if (!m_spAMCameraControl)
		return -1;

	long lZoomMin;
	long lZoomMax;
	long lZoomDefault;
	long lZoomStep;
	long lFlags = CameraControl_Flags_Manual;
	m_spAMCameraControl->GetRange(CameraControl_Zoom, &lZoomMin, &lZoomMax, &lZoomStep, &lZoomDefault, &lFlags);

	long lOldZoom = GetCurrentZoom();
	if (lOldZoom < lZoomMin || lOldZoom > lZoomMax)
		lOldZoom = lZoomDefault;

	// Zoom in 150 steps
	long step = std::max((long)1, (lZoomMax - lZoomMin) / 150);

	// calculate new zoom
	long lNewZoom = std::min(lZoomMax, lOldZoom + lZoomStep * to_underlying(zoom) * step);

	m_spAMCameraControl->Set(CameraControl_Zoom, lNewZoom, CameraControl_Flags_Manual);
	return lNewZoom;
}

void WebcamController::Tilt(const TiltDirection tilt)
{
	if (!m_spAMCameraControl)
		return;

	m_spAMCameraControl->Set(KSPROPERTY_CAMERACONTROL_TILT_RELATIVE, to_underlying(tilt), 0);
}

void WebcamController::DigitalTilt(const TiltDirection tilt)
{
	if (!m_spAMCameraControl)
		return;

	long lValue;
	long lFlags;
	HRESULT hResult = m_spAMCameraControl->Get(CameraControl_Tilt, &lValue, &lFlags);
	if (FAILED(hResult))
		return;

	lValue += to_underlying(tilt);
	if (lValue > m_lDigitalTiltMax)
		lValue = m_lDigitalTiltMax;
	if (lValue < m_lDigitalTiltMin)
		lValue = m_lDigitalTiltMin;
	m_spAMCameraControl->Set(CameraControl_Tilt, lValue, lFlags);
}


void WebcamController::Pan(const PanDirection pan)
{
	if (!m_spAMCameraControl)
		return;

	m_spAMCameraControl->Set(KSPROPERTY_CAMERACONTROL_PAN_RELATIVE, to_underlying(pan), 0);
}

void WebcamController::DigitalPan(const PanDirection pan)
{
	if (!m_spAMCameraControl)
		return;

	long lValue;
	long lFlags;
	HRESULT hResult = m_spAMCameraControl->Get(CameraControl_Pan, &lValue, &lFlags);
	if (FAILED(hResult))
		return;

	lValue += to_underlying(pan);
	if (lValue > m_lDigitalPanMax)
		lValue = m_lDigitalPanMax;
	if (lValue < m_lDigitalPanMin)
		lValue = m_lDigitalPanMin;
	m_spAMCameraControl->Set(CameraControl_Pan, lValue, lFlags);
}


/*
* Tries to locate the node that carries H.264 XU extension and saves its ID.
*/
HRESULT WebcamController::InitializeXUNodesArray(CComPtr<IKsControl> pKsControl)
{
	// Get the IKsTopologyInfo interface
	CComQIPtr<IKsTopologyInfo> pKsTopologyInfo = pKsControl;
	if (!pKsTopologyInfo)
		return E_NOINTERFACE;

	// Retrieve the number of nodes in the filter
	DWORD dwNumNodes = 0;
	HRESULT hr = pKsTopologyInfo->get_NumNodes(&dwNumNodes);
	if (FAILED(hr))
		return hr;

	// Go through all extension unit nodes and try to find the required XU node
	hr = E_FAIL;
#ifdef _DEBUG
	std::set<CString> setGuids;
#endif
	for (unsigned int nodeId = 0; nodeId < dwNumNodes; nodeId++)
	{
		GUID guidNodeType;
		hr = pKsTopologyInfo->get_NodeType(nodeId, &guidNodeType);
		if (FAILED(hr))
			continue;

		// All Node types we have
// 		{ 941C7AC0 - C559 - 11D0 - 8A2B - 00A0C9255AC1 } KSNODETYPE_DEV_SPECIFIC
// 		{ DFF229E1 - F70F - 11D0 - B917 - 00A0C9223196 } KSNODETYPE_VIDEO_STREAMING
// 		{ DFF229E5 - F70F - 11D0 - B917 - 00A0C9223196 } KSNODETYPE_VIDEO_PROCESSING
// 		{ DFF229E6 - F70F - 11D0 - B917 - 00A0C9223196 } KSNODETYPE_VIDEO_CAMERA_TERMINAL
#ifdef _DEBUG
		wchar_t szText[100];
		int len = StringFromGUID2(guidNodeType, szText, _countof(szText));
		setGuids.emplace(CString(szText, len));
#endif


		if (!IsEqualGUID(guidNodeType, KSNODETYPE_DEV_SPECIFIC))
			continue;

		if (IsExtensionUnitSupported(pKsControl, LOGITECH_XU_DEVICE_INFORMATION, nodeId))
		{
			m_dwXUDeviceInformationNodeId = nodeId;
		}
		else if (IsExtensionUnitSupported(pKsControl, LOGITECH_XU_VIDEOPIPE_CONTROL, nodeId))
		{
			m_dwXUVideoPipeControlNodeId = nodeId;
		}
		else if (IsExtensionUnitSupported(pKsControl, LOGITECH_XU_TEST_DEBUG, nodeId))
		{
			m_dwXUTestDebugNodeId = nodeId;
		}
		else if (IsExtensionUnitSupported(pKsControl, LOGITECH_XU_PERIPHERAL_CONTROL, nodeId))
		{
			m_dwXUPeripheralControlNodeId = nodeId;
		}
		// 		else if (IsExtensionUnitSupported(pKsControl, PROPSETID_VIDCAP_CAMERACONTROL, nodeId))
		// 		{
		// 			// DWORD dwNodeId = nodeId;
		// 		}
	}
#ifdef _DEBUG

	for (const auto& str : setGuids)
		TRACE(__FUNCTION__ " - %ls\n", str.GetString());
#endif
	return hr;
}

bool WebcamController::IsExtensionUnitSupported(CComPtr<IKsControl> pKsControl, const GUID& guidExtension, unsigned int nodeId)
{
	KSP_NODE extProp{};
	extProp.Property.Set = guidExtension;
	extProp.Property.Id = 0;
	extProp.Property.Flags = KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
	extProp.NodeId = nodeId;
	extProp.Reserved = 0;
	ULONG ulBytesReturned = 0;
	HRESULT hr = pKsControl->KsProperty((PKSPROPERTY)&extProp, sizeof(extProp), NULL, 0, &ulBytesReturned);
	return SUCCEEDED(hr);
}

void WebcamController::LogiMotionTilt(const TiltDirection tilt)
{
	DWORD dwValue = MAKELONG(MAKEWORD(0, 0), MAKEWORD(0, -to_underlying(tilt)));
	SetProperty(XU_PERIPHERAL_CONTROL, XU_PERIPHERALCONTROL_PANTILT_RELATIVE_CONTROL, sizeof(DWORD), &dwValue);
}

void WebcamController::LogiMotionPan(const PanDirection pan)
{
	DWORD dwValue = MAKELONG(MAKEWORD(0, -to_underlying(pan)), MAKEWORD(0, 0));
	SetProperty(XU_PERIPHERAL_CONTROL, XU_PERIPHERALCONTROL_PANTILT_RELATIVE_CONTROL, sizeof(DWORD), &dwValue);
}


std::vector<WebcamDevice> WebcamController::CompatibleDevices(std::vector<CString> deviceNameFilters)
{
	std::vector<WebcamDevice> devices;
	bool not_filtered = deviceNameFilters.empty();
	if (!not_filtered) {
		for (auto name : deviceNameFilters) {
			if (name == _T("*")) {
				not_filtered = true;
			}
		}
	}

	auto deviceNameMatch = [&](const CString& deviceName) {
		for (auto filter : deviceNameFilters) {
			if (deviceName.Find(filter) != -1) {
				return true;
			}
		}
		return false;
	};

	// Get a device list
	CComPtr<ICreateDevEnum> pSysDevEnum;
	HRESULT hr;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pSysDevEnum);
	if (SUCCEEDED(hr))
	{
		//create a device class enumerator
		CComPtr<IEnumMoniker> pIEnumMoniker;
		hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pIEnumMoniker, 0);
		if (SUCCEEDED(hr))
		{
			ULONG	pFetched = NULL;
			CComPtr<IMoniker> pImoniker;
			while (S_OK == pIEnumMoniker->Next(1, &pImoniker, &pFetched))
			{
				CComPtr<IPropertyBag> pPropBag;
				hr = pImoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				if (SUCCEEDED(hr) && pPropBag)
				{
					CComVariant varCameraName;
					CComVariant varDevicePath;
					pPropBag->Read(L"FriendlyName", &varCameraName, 0);
					pPropBag->Read(L"DevicePath", &varDevicePath, 0);

					if (SUCCEEDED(varCameraName.ChangeType(VT_BSTR)) &&
						SUCCEEDED(varDevicePath.ChangeType(VT_BSTR)))
					{
						CString strCameraName(varCameraName.bstrVal);
						CString strDevicePath(varDevicePath.bstrVal);
						if (not_filtered || deviceNameMatch(strCameraName)) {
							devices.emplace_back(WebcamDevice{ strCameraName, strDevicePath });
						}
					}
				}
				pImoniker.Release();
			}
		}
	}
	return devices;
}
