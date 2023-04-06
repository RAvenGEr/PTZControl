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
	DDX_Control(pDX, IDC_EDIT_DISPLAY, m_edDisplayName);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_1, m_tipEdit[0]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_2, m_tipEdit[1]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_3, m_tipEdit[2]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_4, m_tipEdit[3]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_5, m_tipEdit[4]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_6, m_tipEdit[5]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_7, m_tipEdit[6]);
	DDX_Control(pDX, IDC_ED_TOOLTIP_1_8, m_tipEdit[7]);

	if (pDX->m_bSaveAndValidate) {
		StoreCameraData();
	}
}

void CSettingsDlg::StoreCameraData()
{
	if (std::find(begin(m_modifiedCams), end(m_modifiedCams), m_currentCamPath) == end(m_modifiedCams)) {
		m_modifiedCams.push_back(m_currentCamPath);
	}
	CString temp;
	m_edDisplayName.GetWindowTextW(temp);
	m_displayName[m_currentCamPath] = temp;
	m_useLogitechControl[m_currentCamPath] = m_chLogitechControl.GetCheck();
	m_edMotorInterval.GetWindowTextW(temp);
	const auto val = _wtoi(temp);
	m_motorTime[m_currentCamPath] = val;
	for (size_t i = 0; i < WebcamController::NUM_PRESETS; ++i) {
		m_tipEdit[i].GetWindowTextW(temp);
		m_tooltips[m_currentCamPath][i] = temp;
	}
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
	CString str;
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
	if (!m_currentCamPath.empty()) {
		StoreCameraData();
	}
	const WebcamController* cam = reinterpret_cast<WebcamController*>(m_camCombo.GetItemDataPtr(m_camCombo.GetCurSel()));
	m_currentCamPath = cam->Path();
	const auto &name = theApp.conf().DisplayName(m_currentCamPath);
	m_edDisplayName.SetWindowTextW(name.c_str());
	for (size_t i = 0; i < WebcamController::NUM_PRESETS; ++i) {
		const auto& tip = theApp.conf().TooltipForPreset(m_currentCamPath, i);
		m_tipEdit[i].SetWindowTextW(tip.c_str());
	}
	m_motorSpin.SetPos(theApp.conf().MotorTime(m_currentCamPath));
	m_chLogitechControl.SetCheck(theApp.conf().UseLogitechControl(m_currentCamPath));
	
}

BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_motorSpin.SetRange(10, 1000);
	m_motorSpin.SetPos(70);

	for (const auto& cam : m_cameras) {
		CString name = cam.Name();
		try {
			const auto& display = theApp.conf().DisplayName(static_cast<LPCWSTR>(name));
			name = L" (" + name + L")";
			name = display.c_str() + name;
		}
		catch (std::out_of_range) {
		}
		int pos = m_camCombo.AddString(name);
		m_camCombo.SetItemDataPtr(pos, reinterpret_cast<void*>(const_cast<WebcamController*>(&cam)));
	}

	m_camCombo.SetCurSel(0);
	OnCamComboSel();
	OnChLogitechcontrol();

	CenterWindow();
	return TRUE;
}
