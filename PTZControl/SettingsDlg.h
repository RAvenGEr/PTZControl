#pragma once

#include "PTZControlDlg.h"

// CSettingsDlg dialog

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(const std::vector<WebcamController>& cameras, CWnd* pParent = nullptr)
		: CDialogEx(IDD_SETTINGS, pParent)
		, m_cameras(cameras)
	{
	}

	virtual ~CSettingsDlg() {}

	afx_msg void OnChLogitechcontrol();
	afx_msg void OnEdMotorInterval();
	afx_msg void OnCamComboSel();
	virtual BOOL OnInitDialog();
	
	std::map<std::wstring, std::vector<std::wstring>> m_tooltips;
	std::map<std::wstring, bool> m_useLogitechControl;
	std::map<std::wstring, int> m_motorTime;
	CEdit m_edMotorInterval;
	CButton m_chLogitechControl;

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
	const std::vector<WebcamController>& m_cameras;
	CComboBox m_camCombo;
	CSpinButtonCtrl m_motorSpin;

	CWnd* m_pTipEdit[WebcamController::NUM_PRESETS];
};
