
// PTZControlDlg.h : header file
//

#pragma once

#include <stddef.h>

#include "resource.h"
#include "WebcamControl.h"

//////////////////////////////////////////////////////////////////////////////////////////
// CPTZButton

class CPTZButton : public CMFCButton
{
DECLARE_DYNAMIC(CPTZButton)
public:
	CPTZButton() {}

	void SetCheckStyle()
	{
		m_bCheckButton = TRUE;
		m_bAutoCheck = FALSE;
	}

	COLORREF GetFaceColor()
	{
		return m_clrFace;
	}

protected:
	void PreSubclassWindow() override;
	void OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState) override;
	void OnDrawFocusRect(CDC* pDC, const CRect& rectClient) override;
};


class CPTZRepeatingButton : public CPTZButton
{
	DECLARE_DYNAMIC(CPTZRepeatingButton)
public:
	CPTZRepeatingButton() {}

	bool InAutoRepeat()
	{
		return m_uiSent != 0;
	}

	COLORREF GetFaceColor()
	{
		return m_clrFace;
	}

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

private:
	UINT	m_uiSent{ 0 };
};

//////////////////////////////////////////////////////////////////////////////////////////
// CPTZControlDlg dialog

class CPTZControlDlg : public CDialogEx
{
// Construction
public:
	CPTZControlDlg(CWnd* pParent = nullptr);	// standard constructor
	~CPTZControlDlg();
// Dialog Data
	enum { IDD = IDD_PTZCONTROL_DIALOG };

	static constexpr size_t NUM_MAX_WEBCAMS{ 3 };

protected:
	CPTZRepeatingButton m_btZoomIn;
	CPTZRepeatingButton m_btZoomOut;
	CPTZRepeatingButton m_btUp;
	CPTZRepeatingButton m_btDown;
	CPTZButton m_btHome;
	CPTZRepeatingButton m_btLeft;
	CPTZRepeatingButton m_btRight;
	CPTZButton m_btMemory;
	CPTZButton m_btPreset[WebcamController::NUM_PRESETS];
	CPTZButton m_btExit;
	CPTZButton m_btSettings;
	CPTZButton m_btWebCam[NUM_MAX_WEBCAMS];

	std::vector<WebcamController> m_webCams;

// Map to save the colors of the buttons per Webcam
	typedef std::map<UINT,COLORREF> TMAP_BTNCOLORS;
	TMAP_BTNCOLORS m_aMapBtnColors[NUM_MAX_WEBCAMS];

// Tooltips per WebCam
	CString m_strTooltips[NUM_MAX_WEBCAMS][WebcamController::NUM_PRESETS];

	HACCEL m_hAccel;
	CString m_strCameraDeviceNames;

	size_t m_currentCam;

	void FindCompatibleCams();

	void ResetAllColors();
	WebcamController &GetCurrentWebCam();
	void SetActiveCam(size_t cam);

	// Guard thread
	CEvent	m_evTerminating;
	CWinThread* m_pGuardThread;
	static UINT AFX_CDECL GuardThread(LPVOID hWnd); // AFX_THREADPROC

// Implementation

	HICON m_hIcon;

	// Generated message map functions
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBtExit();
	afx_msg void OnClose();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnBtMemory();
	afx_msg BOOL OnBtPreset(UINT nId);

	void ResetMemButton();

	afx_msg BOOL OnBtWebCam(UINT nId);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnBtZoomIn();
	afx_msg void OnBtZoomOut();
	afx_msg void OnBtUp();
	afx_msg void OnBtLeft();
	afx_msg void OnBtHome();
	afx_msg void OnBtRight();
	afx_msg void OnBtDown();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBtUnpushed();
	afx_msg void OnBtSettings();
};
