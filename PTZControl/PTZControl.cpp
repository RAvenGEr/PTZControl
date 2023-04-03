
// PTZControl.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "PTZControl.h"

#include "Configuration.hpp"

#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CPTZControlCommandLineInfo : public CCommandLineInfo
{
public:
	// Construction
	CPTZControlCommandLineInfo(Configuration& conf) : m_conf(conf)
	{
	}

	void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override {
		if (bFlag) {
			CString param{ pszParam };
			if (param.CompareNoCase(L"reset") == 0) {
				m_conf.SetResetOver(true);
			}
			else if (param.CompareNoCase(L"noguard") == 0) {
				m_conf.SetNoGuardOver(true);
			}
			else if (param.CompareNoCase(L"showdevices") == 0) {
				m_conf.SetShowDevices(true);
			}
			else if (param.Left(7).CompareNoCase(L"device:") == 0) {
				CString deviceName = param.Mid(7);
				::PathUnquoteSpaces(CStrBuf(deviceName, 0));

			}
			else
				ParseParamFlag(CT2CA(pszParam));
		}
		else
			ParseParamNotFlag(pszParam);

		// Standard implementation
		ParseLast(bLast);
	}

private:
	Configuration& m_conf;
};

//////////////////////////////////////////////////////////////////////////
// CPTZControlApp

BEGIN_MESSAGE_MAP(CPTZControlApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// CPTZControlApp construction

CPTZControlApp::CPTZControlApp()
	: m_pDlg(nullptr)
{
}

// The one and only CPTZControlApp object
CPTZControlApp theApp;

static constexpr auto CONFIGURATION_PATH{ L"\\MRi-Software\\PTZ" };


//////////////////////////////////////////////////////////////////////////
// CPTZControlApp initialization

BOOL CPTZControlApp::InitInstance()
{
	CWinApp::InitInstance();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Taken from Logitech
	// PTZDemo\ConferenceCamPTZDemoDlg.cpp
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (!SUCCEEDED(hr))
		return FALSE;

	// Load configuration file from AppData local path
	PWSTR pszPath = NULL;
	hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath);
	if (SUCCEEDED(hr)) {
		const std::wstring appData{ pszPath };
		const std::wstring tomlPath = appData + CONFIGURATION_PATH + L"\\config.toml";
		m_conf.LoadFromFile(tomlPath);
		CoTaskMemFree(pszPath);
	}

	//-------------Commandline parsing--------------------------------------

	// Parse command line for standard shell commands, DDE, file open
	CPTZControlCommandLineInfo cmdInfo(m_conf);
	ParseCommandLine(cmdInfo);

	//-------------Main ----------------------------------------------------

	// Create the Dialog
	m_pDlg = new CPTZControlDlg();
	if (!m_pDlg->Create(CPTZControlDlg::IDD))
		return FALSE;

	m_pMainWnd = m_pDlg;

	// Succeeded
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

int CPTZControlApp::ExitInstance()
{
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif
	CoUninitialize();
	return __super::ExitInstance();
}

void CPTZControlApp::WriteConfigurationToFile()
{
	// Load configuration file from AppData local path
	PWSTR pszPath = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath);
	if (SUCCEEDED(hr)) {
		const std::wstring appData{ pszPath };
		const std::wstring configDir = appData + CONFIGURATION_PATH;
		SHCreateDirectory(NULL, configDir.c_str());
		const std::wstring tomlPath = configDir + L"\\config.toml";
		m_conf.WriteToFile(tomlPath);
		CoTaskMemFree(pszPath);
	}
}
