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

	const std::vector<std::wstring> ModifiedCameras() const 
	{
		return m_modifiedCams;
	}

	const std::array<std::wstring, WebcamController::NUM_PRESETS>& Tooltips(const std::wstring& camPath) const
	{
		return m_tooltips.at(camPath);
	}

	const std::wstring& DisplayName(const std::wstring& camPath) const
	{
		return m_displayName.at(camPath);
	}

	bool UseLogitechControl(const std::wstring& camPath) const
	{
		return m_useLogitechControl.at(camPath);
	}

	int MotorTime(const std::wstring& camPath) const
	{
		return m_motorTime.at(camPath);
	}

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
	void StoreCameraData();
	const std::vector<WebcamController>& m_cameras;

	std::map<std::wstring, std::array<std::wstring, WebcamController::NUM_PRESETS>> m_tooltips{};
	std::map<std::wstring, std::wstring> m_displayName{};
	std::map<std::wstring, bool> m_useLogitechControl{};
	std::map<std::wstring, int> m_motorTime{};

	std::vector<std::wstring> m_modifiedCams{};

	std::wstring m_currentCamPath;

	CEdit m_edMotorInterval;
	CButton m_chLogitechControl;
	CComboBox m_camCombo;
	CSpinButtonCtrl m_motorSpin;
	CEdit m_edDisplayName;
	CEdit m_tipEdit[WebcamController::NUM_PRESETS];
};
