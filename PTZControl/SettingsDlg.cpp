// SettingsDlg.cpp : implementation file
//

#include "pch.h"
#include "PTZControl.h"
#include "SettingsDlg.h"
#include "afxdialogex.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_ED_MOTORTIME, m_edMotorInterval);
	DDX_Control(pDX, IDC_CH_LOGITECHCONTROL, m_chLogitechControl);
	DDX_Control(pDX, IDC_COMBOCAMERA, m_camCombo);
	DDX_Control(pDX, IDC_SPIN1, m_motorSpin);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CH_LOGITECHCONTROL, &CSettingsDlg::OnChLogitechcontrol)
	ON_EN_KILLFOCUS(IDC_ED_MOTORTIME, &CSettingsDlg::OnEdMotorInterval)
	ON_CBN_SELCHANGE(IDC_COMBOCAMERA, &CSettingsDlg::OnCamComboSel)
END_MESSAGE_MAP()

// CSettingsDlg message handlers

void CSettingsDlg::OnChLogitechcontrol()
{
	m_edMotorInterval.EnableWindow(!m_chLogitechControl.GetCheck());
}

void CSettingsDlg::OnEdMotorInterval()
{
	if (!m_edMotorInterval) {
		return;
	}
	CString str = _T("");
	m_edMotorInterval.GetWindowTextW(str);
	const auto val = _wtoi(str);
	int lower;
	int upper;
	m_motorSpin.GetRange(lower, upper);
	if (val < lower) {
		wchar_t buff[32];
		_itow_s(lower, buff, 32, 10);
		m_edMotorInterval.SetWindowTextW(buff);
	}
	else if (val > upper) {
		wchar_t buff[32];
		_itow_s(lower, buff, 32, 10);
		m_edMotorInterval.SetWindowTextW(buff);
	}
}

void CSettingsDlg::OnCamComboSel()
{
	const WebcamController* cam = reinterpret_cast<WebcamController*>(m_camCombo.GetItemDataPtr(m_camCombo.GetCurSel()));
}

BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_motorSpin.SetRange(10, 1000);
	m_motorSpin.SetPos(70);

	for (int i = 0; i < WebcamController::NUM_PRESETS; ++i) {
		m_pTipEdit[i] = GetDlgItem(IDC_ED_TOOLTIP_1_1 + i);
	}

	for (const auto& cam : m_cameras) {
		const CString name = cam.Name().IsEmpty() ?L"Demo":cam.Name();
		int pos = m_camCombo.AddString(name);
		m_camCombo.SetItemDataPtr(pos, reinterpret_cast<void*>(const_cast<WebcamController*>(&cam)));
	}

	m_camCombo.SetCurSel(0);

	OnChLogitechcontrol();

	CenterWindow();
	return TRUE;
}
