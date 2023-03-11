#pragma once

#include "PTZControlDlg.h"

// CSettingsDlg dialog

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);
	virtual ~CSettingsDlg() {}

	afx_msg void OnChLogitechcontrol();
	virtual BOOL OnInitDialog();

	CString m_strCameraName;
	CString m_strTooltip[CPTZControlDlg::NUM_MAX_WEBCAMS][WebcamController::NUM_PRESETS];
	BOOL m_bLogitechCameraControl;
	int m_iMotorIntervalTimer;
	CEdit m_edMotorInterval;
	CButton m_chLogitechControl;

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
};
