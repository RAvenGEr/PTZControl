
// PTZControlDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include <algorithm>
#include <dbt.h>
#include "PTZControl.h"
#include "PTZControlDlg.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//	Array of supported cameras
//		Allow the cameras with the following tags in the device name.
//		This will match all Logitech PRT Pro, PTZ Pro 2 and Rally cameras.
//		Also the ConferenceCam CC3000e Camera will be detected.
//		Remember: Just a partial token must match the name.
static const LPCTSTR g_aCameras[] = 
{
	_T("PTZ Pro"),
	_T("Logi Rally"),
	_T("ConferenceCam"),
	_T("Logi Group"),
};

//////////////////////////////////////////////////////////////////////////////////////////
// We have several layouts for the Buttons
// 0-1 cameras (dialog will shrink by one column)
//		E |
//		S |
// 2 cameras
//		S E 
//		- -
//		1 2
// 3 cameras
//		1 E 
//		2 S
//		3 

struct SPosition
{
	UINT	nId;					// Id of button 
	bool	bShow;					// Show or hide
	int		x, y;					// outside raster, left two columns
};

struct SLayout
{
	int cxDelta;					// Num to shrink / grow the dialog
	const SPosition* pButtons;		// List of entries terminated with nId==0.
};

static const SPosition layoutBtns1[]	// 0 or 1 camera (uses only 1 column)
{
	IDC_BT_EXIT,		true,  0, 0,	// one columns
	IDC_BT_SETTINGS,	true,  0, 1,
	// Invisible
	IDC_BT_WEBCAM1,		false, 0, 0,
	IDC_BT_WEBCAM2,		false, 0, 0,
	IDC_BT_WEBCAM3,		false, 0, 0,
	0
};

static const SPosition layoutBtns2[]	// 2 cameras (uses 2 columns)
{
	IDC_BT_SETTINGS,	true,	0, 0,	// Top row
	IDC_BT_EXIT,		true,	1, 0,
	IDC_BT_WEBCAM1,		true,	0, 2,	// bottom row
	IDC_BT_WEBCAM2,		true,	1, 2,
	// Invisible
	IDC_BT_WEBCAM3,		false,  0, 0,
	0
};

static const SPosition layoutBtns3[]	// 3 cameras
{
	IDC_BT_EXIT,		true,	1, 0,	// Outer left column
	IDC_BT_SETTINGS,	true,	1, 1,
	IDC_BT_WEBCAM1,		true,	0, 0,	// all in one column left (top down)
	IDC_BT_WEBCAM2,		true,	0, 1,
	IDC_BT_WEBCAM3,		true,   0, 2,
	0
};

const SLayout g_layout[CPTZControlDlg::NUM_MAX_WEBCAMS+1] = 
{
	-1,	layoutBtns1,		// 0 or 1 camera (shrink dialog)
	-1,	layoutBtns1,		// 0 or 1 camera (shrink dialog)
	0,  layoutBtns2,		// 2 cameras	 (keep size)
	0,  layoutBtns3,		// 3 cameras	 (keep size)
};


//////////////////////////////////////////////////////////////////////////////////////////
//	Special new notification 

#define ON_BN_UNPUSHED(id, memberFxn) \
	ON_CONTROL(BN_UNPUSHED, id, memberFxn)

//////////////////////////////////////////////////////////////////////////////////////////
//	CPTZButton

IMPLEMENT_DYNAMIC(CPTZButton,CMFCButton)

void CPTZButton::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	SetTooltip(_T(""));
	// Setting delay time doesn't seam to work.
	GetToolTipCtrl().SetDelayTime(0);
}

void CPTZButton::OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState)
{
	__super::OnDrawBorder(pDC,rectClient,uiState);

	// Give the button our button face. But only when the mouse isn't over
	if (m_bWinXPTheme && m_clrFace!=COLORREF(-1)/* && !m_bHover && !m_bHighlighted*/)
		pDC->FillSolidRect(rectClient, m_clrFace);
}

void CPTZButton::OnDrawFocusRect(CDC* pDC, const CRect& rectClient)
{
	UNUSED_ALWAYS(pDC); UNUSED_ALWAYS(rectClient);
}

IMPLEMENT_DYNAMIC(CPTZRepeatingButton, CMFCButton)

void CPTZRepeatingButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetTimer(TIMER_AUTO_REPEAT, AUTO_REPEAT_INITIAL_DELAY, NULL);
	m_uiSent = 0;
	__super::OnLButtonDown(nFlags, point);
}

void CPTZRepeatingButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	KillTimer(TIMER_AUTO_REPEAT);

	if (GetCapture() != NULL)
	{
		// If we never sent a message we do it once.
		if (m_uiSent==0 && (GetState() & BST_PUSHED) != 0)
			GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
		else
			GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_UNPUSHED), (LPARAM)m_hWnd);
		// release capture
		ReleaseCapture();
	}
}

void CPTZRepeatingButton::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent==TIMER_AUTO_REPEAT)
	{
		if ((GetState() & BST_PUSHED) == 0)
			return;
		++m_uiSent;
		SetTimer(TIMER_AUTO_REPEAT, AUTO_REPEAT_DELAY, NULL);
		GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
	}

	__super::OnTimer(nIDEvent);
}

BEGIN_MESSAGE_MAP(CPTZRepeatingButton, CMFCButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////

BOOL AdjustVisibleWindowRect(LPRECT lpRect, HWND hWndParent=NULL)
{
	RECT rcOrg(*lpRect);

	RECT rcBase;
	if (hWndParent)
		// Any parent given?
		::GetClientRect(hWndParent,&rcBase);
	else
	{
		::MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (!AfxGetMainWnd())
			// If we have no main window we just try to use the main screen point 0,0
			::GetMonitorInfo(::MonitorFromPoint(CPoint(0,0), MONITOR_DEFAULTTONEAREST), &mi);
		else
			// Get the main window monitor
			::GetMonitorInfo(::MonitorFromWindow(AfxGetMainWnd()->m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		rcBase = mi.rcWork;
	}

	// To large?
	if (lpRect->bottom-lpRect->top>rcBase.bottom-rcBase.top)
		lpRect->bottom -= (lpRect->bottom-lpRect->top)-(rcBase.bottom-rcBase.top);
	if (lpRect->right-lpRect->left>rcBase.right-rcBase.left)
		lpRect->right -= (lpRect->right-lpRect->left)-(rcBase.right-rcBase.left);

	// Check if rect is visible
	if (lpRect->bottom>rcBase.bottom)
		::OffsetRect(lpRect,0,rcBase.bottom-lpRect->bottom);
	if (lpRect->top<rcBase.top)
		::OffsetRect(lpRect,0,rcBase.top-lpRect->top);
	if (lpRect->right>rcBase.right)
		::OffsetRect(lpRect,rcBase.right-lpRect->right,0);
	if (lpRect->left<rcBase.left)
		::OffsetRect(lpRect,rcBase.left-lpRect->left,0);

	// Check if coords changes
	return !EqualRect(lpRect,&rcOrg);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CPTZControlDlg dialog

CPTZControlDlg::CPTZControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PTZCONTROL_DIALOG, pParent)
	, m_hAccel(NULL)
	, m_evTerminating(FALSE,TRUE)
	, m_pGuardThread(nullptr)
	, m_currentCam(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hAccel = ::LoadAccelerators(AfxFindResourceHandle(IDR_ACCELERATOR, RT_ACCELERATOR), MAKEINTRESOURCE(IDR_ACCELERATOR));
}

CPTZControlDlg::~CPTZControlDlg()
{}

void CPTZControlDlg::PostNcDestroy()
{
	__super::PostNcDestroy();

	// Cleanup the guard thread.
	m_evTerminating.SetEvent();
	if (m_pGuardThread)
		::WaitForSingleObject(m_pGuardThread->m_hThread,INFINITE);
	m_pGuardThread = nullptr;

	// Finally delete the application.
	delete this;
}

BOOL CPTZControlDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return __super::OnCommand(wParam,lParam);
}


BOOL CPTZControlDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message>=WM_KEYFIRST && pMsg->message<=WM_KEYLAST)
	{
		if (m_hAccel)
		{
			if (::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
				return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPTZControlDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BT_ZOOM_IN, m_btZoomIn);
	DDX_Control(pDX, IDC_BT_ZOOM_OUT, m_btZoomOut);
	DDX_Control(pDX, IDC_BT_UP, m_btUp);
	DDX_Control(pDX, IDC_BT_DOWN, m_btDown);
	DDX_Control(pDX, IDC_BT_HOME, m_btHome);
	DDX_Control(pDX, IDC_BT_LEFT, m_btLeft);
	DDX_Control(pDX, IDC_BT_RIGHT, m_btRight);
	DDX_Control(pDX, IDC_BT_MEMORY, m_btMemory);
	DDX_Control(pDX, IDC_BT_PRESET1, m_btPreset[0]);
	DDX_Control(pDX, IDC_BT_PRESET2, m_btPreset[1]);
	DDX_Control(pDX, IDC_BT_PRESET3, m_btPreset[2]);
	DDX_Control(pDX, IDC_BT_PRESET4, m_btPreset[3]);
	DDX_Control(pDX, IDC_BT_PRESET5, m_btPreset[4]);
	DDX_Control(pDX, IDC_BT_PRESET6, m_btPreset[5]);
	DDX_Control(pDX, IDC_BT_PRESET7, m_btPreset[6]);
	DDX_Control(pDX, IDC_BT_PRESET8, m_btPreset[7]);
	DDX_Control(pDX, IDC_BT_EXIT, m_btExit);
	DDX_Control(pDX, IDC_BT_SETTINGS, m_btSettings);
	DDX_Control(pDX, IDC_BT_WEBCAM1, m_btWebCam[0]);
	DDX_Control(pDX, IDC_BT_WEBCAM2, m_btWebCam[1]);
	DDX_Control(pDX, IDC_BT_WEBCAM3, m_btWebCam[2]);
}

BEGIN_MESSAGE_MAP(CPTZControlDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_DEVICECHANGE()
	ON_WM_NCHITTEST()
	ON_BN_CLICKED(IDC_BT_EXIT, &CPTZControlDlg::OnBtExit)
	ON_WM_SETFOCUS()
	ON_BN_CLICKED(IDC_BT_MEMORY, &CPTZControlDlg::OnBtMemory)
	ON_COMMAND_EX(IDC_BT_WEBCAM1, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_WEBCAM2, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_WEBCAM3, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_PRESET1, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET2, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET3, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET4, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET5, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET6, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET7, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET8, &CPTZControlDlg::OnBtPreset)
	ON_BN_CLICKED(IDC_BT_ZOOM_IN, &CPTZControlDlg::OnBtZoomIn)
	ON_BN_CLICKED(IDC_BT_ZOOM_OUT, &CPTZControlDlg::OnBtZoomOut)
	ON_BN_CLICKED(IDC_BT_UP, &CPTZControlDlg::OnBtUp)
	ON_BN_CLICKED(IDC_BT_DOWN, &CPTZControlDlg::OnBtDown)
	ON_BN_CLICKED(IDC_BT_LEFT, &CPTZControlDlg::OnBtLeft)
	ON_BN_CLICKED(IDC_BT_HOME, &CPTZControlDlg::OnBtHome)
	ON_BN_CLICKED(IDC_BT_RIGHT, &CPTZControlDlg::OnBtRight)
	ON_BN_CLICKED(IDC_BT_SETTINGS, &CPTZControlDlg::OnBtSettings)
	ON_BN_UNPUSHED(IDC_BT_UP, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_DOWN, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_LEFT, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_RIGHT, &CPTZControlDlg::OnBtUnpushed)
	ON_WM_TIMER()
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////////////////
// CPTZControlDlg message handlers

void CPTZControlDlg::ResetAllColors()
{
	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
	{
		auto *pButton = DYNAMIC_DOWNCAST(CPTZButton, pWnd);

		// Don't reset color for web cam button
		bool bIsWebCamBtn = false;
		for (auto &btn : m_btWebCam)
			bIsWebCamBtn |= pButton==&btn;

		// Reset color for all other buttons
		if (pButton && !bIsWebCamBtn)
			pButton->SetFaceColor(COLORREF(-1), TRUE);
	}
}

void CPTZControlDlg::FindCompatibleCams()
{
	std::vector<WebcamController> camsFound;
	std::vector<CString> camDevNames;
	//---------------------------------------------------------------------
	// INIT AND FIND WEB CAMS
	// 
	// Try to find the Device list
	// Find the devices to search for. We have some default devices if no other is set in 
	// the registry or on the command line.
	std::vector<CString> deviceNameFilters;
	for (const auto* p : g_aCameras)
		deviceNameFilters.emplace_back(p);
	CString strCameraNameToSearch = theApp.GetProfileString(REG_DEVICE, REG_DEVICENAME, _T(""));
	if (!strCameraNameToSearch.IsEmpty())
		deviceNameFilters.push_back(strCameraNameToSearch);
	if (!theApp.m_strDevName.IsEmpty())
		deviceNameFilters.push_back(theApp.m_strDevName);

	auto aDevices{ WebcamController::CompatibleDevices(deviceNameFilters) };

	auto OpenWebCam = [](WebcamController& webCam, CString strDevToken)->bool
	{
		HRESULT hr = webCam.OpenDevice(strDevToken);
		if (FAILED(hr))
		{
			AfxMessageBox(IDP_ERR_OPENFAILED);
			return false;
		}

		//	Load the default setting
		webCam.useLogitechMotionControl = theApp.GetProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, FALSE) != 0;
		webCam.motorIntervalTime = theApp.GetProfileInt(REG_DEVICE, REG_MOTORINTERVALTIMER, WebcamController::DEFAULT_MOTOR_INTERVAL);
		return true;
	};

	for (auto device : aDevices) {
		camsFound.emplace_back();
		if (OpenWebCam(camsFound.back(), device.devicePath)) {
			camDevNames.push_back(device.devicePath);
		} else {
			camsFound.pop_back();
		}
	}

	// Check how many web cams we found
	if (m_webCams.empty() && camsFound.empty())
	{
		// If we have no cam, show message
		AfxMessageBox(IDP_ERR_NO_CAMERA, MB_ICONERROR);
	}
	m_webCams.insert(m_webCams.end(), std::make_move_iterator(camsFound.begin()), std::make_move_iterator(camsFound.end()));
	m_camDevPaths.insert(m_camDevPaths.end(), std::make_move_iterator(camDevNames.begin()), std::make_move_iterator(camDevNames.end()));
	if (m_webCams.size() != m_camDevPaths.size()) {
		AfxMessageBox(L"Unexpected result of device scan");
	}
}

void CPTZControlDlg::ResetMemButton()
{
	// Clear the mem button
	KillTimer(TIMER_CLEAR_MEMORY);
	m_btMemory.SetCheck(0);
	m_btMemory.SetFaceColor(COLORREF(-1));
}

WebcamController& CPTZControlDlg::GetCurrentWebCam()
{
	return m_webCams[m_currentCam];
}

void CPTZControlDlg::SetActiveCam(size_t cam)
{
	if (cam < m_webCams.size())
	{
		// Clear mem button
		ResetMemButton();

		// Save the colors of the current buttons and set the old ones
		for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
		{
			auto* pButton = DYNAMIC_DOWNCAST(CPTZButton, pWnd);
			if (pButton)
			{
				// Save the color of the current buttons for the current web cam and get 
				// the saved color from the map we have for the new cam.
				auto nId = pButton->GetDlgCtrlID();
				m_aMapBtnColors[m_currentCam][nId] = pButton->GetFaceColor();
				auto it = m_aMapBtnColors[cam].find(nId);
				if (it!=m_aMapBtnColors[cam].end())
					pButton->SetFaceColor(it->second);
			}
		}

		// Set the tooltips
		for (int i=0; i<WebcamController::NUM_PRESETS; ++i)
			m_btPreset[i].SetTooltip(m_strTooltips[cam][i]);

		// Set the new webcam
		m_currentCam = cam;
		auto Enable = [&](CPTZButton &btn, bool bActive)
		{
			btn.SetCheck(bActive);
			btn.SetFaceColor(bActive ? COLOR_ORANGE : -1, TRUE);
		};
		size_t iWebCam = 0;
		for (auto &btn : m_btWebCam)
			Enable(btn,m_currentCam==iWebCam++);
	}
}

BOOL CPTZControlDlg::OnInitDialog()
{
	__super::OnInitDialog();

	//---------------------------------------------------------------------
	// INIT MFC STUFF

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Load the bitmap
	{
		CMFCToolBarImages	images;
		images.SetImageSize(CSize(16, 16));
		images.Load(IDB_BUTTONS);

		int iImage = 0;
		CPTZButton* apButtons[] =
		{
			&m_btDown,
			&m_btLeft,
			&m_btRight,
			&m_btUp,
			&m_btHome,
			&m_btZoomIn,
			&m_btZoomOut,
			&m_btMemory,
			&m_btPreset[0],
			&m_btPreset[1],
			&m_btPreset[2],
			&m_btPreset[3],
			&m_btPreset[4],
			&m_btPreset[5],
			&m_btPreset[6],
			&m_btPreset[7],
			&m_btExit,
			&m_btSettings,
			&m_btWebCam[0],
			&m_btWebCam[1],
			&m_btWebCam[2],
		};
		for (auto* pBtn : apButtons)
		{
			HICON hIcon = images.ExtractIcon(iImage++);
			pBtn->SetImage(hIcon);
			// we have a graphic now, we skip the text
			pBtn->SetWindowText(_T(""));
		}
	}

	// This is a check box style
	m_btMemory.SetCheckStyle();
	for (auto &btn : m_btWebCam)
		btn.SetCheckStyle();

	FindCompatibleCams();

	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = KSCATEGORY_CAPTURE;

	m_hDeviceNotify = RegisterDeviceNotification(
		GetSafeHwnd(),
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
	);

	//---------------------------------------------------------------------
	// ADJUST THE DIALOG

	// Calculate the with an position of the Grid.
	CRect rectBtn11, rectBtn12, rectBtn21;
	m_btWebCam[0].GetWindowRect(rectBtn11);
	m_btWebCam[1].GetWindowRect(rectBtn12);
	m_btExit.GetWindowRect(rectBtn21);
	ScreenToClient(rectBtn11);
	ScreenToClient(rectBtn12);
	ScreenToClient(rectBtn21);

	CPoint pointBase { rectBtn11.TopLeft() };
	CSize sizeRaster { rectBtn21.left-rectBtn11.left, rectBtn12.top-rectBtn11.top };

	// Get the required layout
	const auto &layout = g_layout[m_webCams.size()];

	for (const auto* pLayoutBtn = layout.pButtons; pLayoutBtn->nId; ++pLayoutBtn)
	{
		CWnd *pWnd = GetDlgItem(pLayoutBtn->nId);
		// Move the button and hide or show the button
		pWnd->SetWindowPos(nullptr, 
						   pointBase.x + pLayoutBtn->x * sizeRaster.cx, pointBase.y + pLayoutBtn->y * sizeRaster.cy, 
						   0, 0, SWP_NOSIZE|SWP_NOZORDER|(pLayoutBtn->bShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)			
		);
		pWnd->EnableWindow(pLayoutBtn->bShow);
	}

	// First Center
	CenterWindow();

	// Adjust the window
	CRect rect;
	GetWindowRect(rect);
	CPoint pt = rect.TopLeft();
	rect.OffsetRect(-pt);
	pt.x = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_POSX, pt.x);
	pt.y = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_POSY, pt.y);
	rect.OffsetRect(pt);
	AdjustVisibleWindowRect(rect);

	// Move it
	SetWindowPos(&CWnd::wndTopMost, rect.left, rect.top, rect.Width()+layout.cxDelta*sizeRaster.cx, rect.Height(), 0);

	// Set the tooltips for all presets
	for (int i = 0; i < WebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			CString str;
			str.Format(REG_TOOLTIP, i + 1 + j*100);
			m_strTooltips[j][i] = theApp.GetProfileString(REG_WINDOW, str);
		}
	}

	EnableToolTips(TRUE);

	// Start a guard thread that takes care about a blocking app.
	if (!theApp.m_bNoGuard)
		m_pGuardThread = AfxBeginThread(&GuardThread,this);

	// Set a time to move the focus to the parent window
	SetTimer(TIMER_FOCUS_CHECK,FOCUS_CHECK_DELAY,nullptr);
	SetFocus();

	// Move all cams to home position. And WebCam 0 will be the active one.
	if (theApp.m_bNoReset)
	{
		// Activate camera 0.
		if (!m_webCams.empty())
			SetActiveCam(0);
	}
	else
	{
		// Reset the cameras to home position. and leave the first camera 
		// (index 0) the active one ((loop backwards).
		for (size_t i = m_webCams.size(); i > 0; --i)
		{
			SetActiveCam(i - 1);
			OnBtHome();
		}
	}

	return FALSE;  
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon. For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPTZControlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		__super::OnPaint();
	}
}

BOOL CPTZControlDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	BOOL result = __super::OnDeviceChange(nEventType, dwData);
	auto broadcast = reinterpret_cast<DEV_BROADCAST_HDR*>(dwData);
	switch (nEventType)
	{
	case DBT_DEVICEARRIVAL:
		if (broadcast->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
			auto devint = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(dwData);
			CString path{ devint->dbcc_name };
			path.MakeLower();
			auto res = std::find(m_camDevPaths.begin(), m_camDevPaths.end(), path);
			if (res != m_camDevPaths.end()) {
				size_t pos = res - m_camDevPaths.begin();
				HRESULT hr = m_webCams[pos].OpenDevice(path);
				if (SUCCEEDED(hr))
					m_btWebCam[pos].EnableWindow(TRUE);
			}
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		if (broadcast->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
			auto devint = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(dwData);
			CString path{ devint->dbcc_name };
			path.MakeLower();
			auto res = std::find(m_camDevPaths.begin(), m_camDevPaths.end(), path);
			if (res != m_camDevPaths.end()) {
				size_t pos = res - m_camDevPaths.begin();
				if (m_webCams.size() > 1) {
					if (m_currentCam == pos) {
						SetActiveCam(pos == 0 ? 1 : 0);
					}
					m_btWebCam[pos].EnableWindow(FALSE);
				}
				m_webCams[pos].CloseDevice();
			}
		}
		break;
	}
	return result;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPTZControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CPTZControlDlg::OnNcHitTest(CPoint point)
{
	// Allow drag & drop
	LRESULT lResult = __super::OnNcHitTest(point);
	if (lResult == HTCLIENT)
		lResult = HTCAPTION;
	return lResult;
}

void CPTZControlDlg::OnClose()
{
	CRect rect;
	GetWindowRect(rect);
	theApp.WriteProfileInt(REG_WINDOW,REG_WINDOW_POSX,rect.left);
	theApp.WriteProfileInt(REG_WINDOW,REG_WINDOW_POSY,rect.top);

	UnregisterDeviceNotification(m_hDeviceNotify);

	DestroyWindow();
}

void CPTZControlDlg::OnBtExit()
{
	PostMessage(WM_CLOSE);
}

void CPTZControlDlg::OnOK()
{
	// Ignore
}

void CPTZControlDlg::OnCancel()
{
	// Ignore
}

void CPTZControlDlg::OnBtMemory()
{
	ResetAllColors();
	bool bCheck = !m_btMemory.GetCheck();
	m_btMemory.SetCheck(bCheck);
	if (bCheck)
	{
		// Remember the mem button, but clear it after some delay.
		m_btMemory.SetFaceColor(COLOR_RED, TRUE);
		SetTimer(TIMER_CLEAR_MEMORY,CLEAR_MEMORY_DELAY,nullptr);
	}
	else
		m_btMemory.SetFaceColor(COLORREF(-1),TRUE);
}

BOOL CPTZControlDlg::OnBtPreset(UINT nId)
{
	UINT uiPreset = 0;
	while (uiPreset<WebcamController::NUM_PRESETS)
	{
		if (static_cast<UINT>(m_btPreset[uiPreset].GetDlgCtrlID())==nId)
			break;
		++uiPreset;
	}

	if (uiPreset<WebcamController::NUM_PRESETS)
	{
		ResetAllColors();
		bool bStore = m_btMemory.GetCheck();
		if (bStore)
		{
			// Save as new preset
			GetCurrentWebCam().SavePreset(uiPreset);
			ResetMemButton();
		}
		else
			GetCurrentWebCam().GotoPreset(uiPreset);

		// Set color
		STATIC_DOWNCAST(CPTZButton,GetDlgItem(nId))->SetFaceColor(COLOR_GREEN,TRUE);
	}
	return TRUE;
}

BOOL CPTZControlDlg::OnBtWebCam(UINT nId)
{
	SetActiveCam(nId==IDC_BT_WEBCAM1 ? 0 :
				 nId==IDC_BT_WEBCAM2 ? 1 : 2);
	return 1;
}

void CPTZControlDlg::OnBtHome()
{
	ResetAllColors();
	GetCurrentWebCam().GotoHome();
	m_btHome.SetFaceColor(COLOR_GREEN,TRUE);
}


void CPTZControlDlg::OnBtZoomIn()
{
	ResetAllColors();
	GetCurrentWebCam().Zoom(ZoomDirection::In);
}


void CPTZControlDlg::OnBtZoomOut()
{
	ResetAllColors();
	GetCurrentWebCam().Zoom(ZoomDirection::Out);
}


void CPTZControlDlg::OnBtDown()
{
	ResetAllColors();
	if (m_btDown.InAutoRepeat()) {
		GetCurrentWebCam().Tilt(TiltDirection::Down);
		KillTimer(TIMER_TILT_STOP);
	}
	else if (GetCurrentWebCam().useLogitechMotionControl) {
		GetCurrentWebCam().LogiMotionTilt(TiltDirection::Down);
	}
	else {
		GetCurrentWebCam().Tilt(TiltDirection::Down);
		SetTimer(TIMER_TILT_STOP, GetCurrentWebCam().motorIntervalTime, NULL);
	}
}

void CPTZControlDlg::OnBtUp()
{
	ResetAllColors();
	if (m_btUp.InAutoRepeat()) {
		GetCurrentWebCam().Tilt(TiltDirection::Up);
		KillTimer(TIMER_TILT_STOP);
	}
	else if (GetCurrentWebCam().useLogitechMotionControl) {
		GetCurrentWebCam().LogiMotionTilt(TiltDirection::Up);
	}
	else {
		GetCurrentWebCam().Tilt(TiltDirection::Up);
		SetTimer(TIMER_TILT_STOP, GetCurrentWebCam().motorIntervalTime, NULL);
	}
}


void CPTZControlDlg::OnBtLeft()
{
	ResetAllColors();
	if (m_btLeft.InAutoRepeat()) {
		GetCurrentWebCam().Pan(PanDirection::Left);
		KillTimer(TIMER_PAN_STOP);
	}
	else if (GetCurrentWebCam().useLogitechMotionControl) {
		GetCurrentWebCam().LogiMotionPan(PanDirection::Left);
	}
	else {
		GetCurrentWebCam().Pan(PanDirection::Left);
		SetTimer(TIMER_PAN_STOP, GetCurrentWebCam().motorIntervalTime, NULL);
	}
}


void CPTZControlDlg::OnBtRight()
{
	ResetAllColors();
	if (m_btRight.InAutoRepeat()) {
		GetCurrentWebCam().Pan(PanDirection::Right);
		KillTimer(TIMER_PAN_STOP);
	}
	else if (GetCurrentWebCam().useLogitechMotionControl) {
		GetCurrentWebCam().LogiMotionPan(PanDirection::Right);
	}
	else {
		GetCurrentWebCam().Pan(PanDirection::Right);
		SetTimer(TIMER_PAN_STOP, GetCurrentWebCam().motorIntervalTime, NULL);	
	}
}


void CPTZControlDlg::OnSetFocus(CWnd* pOldWnd)
{
	// Do nothing. Never set the focus to a child.
	UNUSED_ALWAYS(pOldWnd);
}



void CPTZControlDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_FOCUS_CHECK)
	{
		CWnd* pWndFocus = GetFocus();
		if (pWndFocus && 
			pWndFocus->GetParent()==this && 
			(GetAsyncKeyState(VK_LBUTTON) & 0x8000)==0)
			// only move the focus, when a button has the focus and the mouse is not down.
			SetFocus();
	}
	else if (nIDEvent == TIMER_CLEAR_MEMORY)
	{
		// Clear the mem button after some delay
		ResetMemButton();
	}
	else if (nIDEvent == TIMER_PAN_STOP) {
		GetCurrentWebCam().Pan(PanDirection::Stop);
	}
	else if (nIDEvent == TIMER_TILT_STOP) {
		GetCurrentWebCam().Tilt(TiltDirection::Stop);
	}
	__super::OnTimer(nIDEvent);
}

void CPTZControlDlg::OnBtUnpushed()
{
	GetCurrentWebCam().Pan(PanDirection::Stop);
	GetCurrentWebCam().Tilt(TiltDirection::Stop);
}

void CPTZControlDlg::OnBtSettings()
{
	CSettingsDlg dlg;
	dlg.m_strCameraName = m_strCameraDeviceNames;
	dlg.m_strCameraName.Replace(_T("\r\n"), _T(", "));
	dlg.m_bLogitechCameraControl = GetCurrentWebCam().useLogitechMotionControl;
	dlg.m_iMotorIntervalTimer = GetCurrentWebCam().motorIntervalTime;

	// Get a copy of the tooltips
	for (int i = 0; i < WebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			dlg.m_strTooltip[j][i] = m_strTooltips[j][i];
		}
	}

	if (dlg.DoModal()!=IDOK)
		return;

	// Copy back and save
	for (int i = 0; i < WebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			m_strTooltips[j][i] = dlg.m_strTooltip[j][i];
			CString str;
			str.Format(REG_TOOLTIP, i + 1 + j*100);
			theApp.WriteProfileString(REG_WINDOW, str, m_strTooltips[j][i]);
		}
	}

	// Camera control
	GetCurrentWebCam().useLogitechMotionControl = dlg.m_bLogitechCameraControl!=0;
	theApp.WriteProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, dlg.m_bLogitechCameraControl);
	GetCurrentWebCam().motorIntervalTime = dlg.m_iMotorIntervalTimer;
	theApp.WriteProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, dlg.m_bLogitechCameraControl);

	// Set tooltips again
	SetActiveCam(m_currentCam);
}

//////////////////////////////////////////////////////////////////////////
//	It seams that in some cases a camera my block.
//	This thread should help that the blocking thread is detected. and the
//	application terminates.

UINT AFX_CDECL CPTZControlDlg::GuardThread(LPVOID p)
{
	auto* pWnd = static_cast<CPTZControlDlg*>(p);

	// This thread end when the application exists.
	// For ever all 2000 seconds check if the application still runs and accepts messages.
	for (;;)
	{
		// Check if we have an event, wait for 1 sec
		CSingleLock lock(&pWnd->m_evTerminating, FALSE);
		if (lock.Lock(1000))
			// Event is set.
			return 0;

		// Check if the application is blocking
		if (::SendMessageTimeout(pWnd->GetSafeHwnd(), WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NORMAL, 1000, NULL) == 0)
		{
			// Exit the current process
			::ExitProcess(10);
		}
	}
}
