
// PTZControl.h : main header file for the PROJECT_NAME application
//
#pragma once

#include "PTZControlDlg.h"

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

//////////////////////////////////////////////////////////////////////////

#define TIMER_FOCUS_CHECK	4711
#define TIMER_AUTO_REPEAT	4712
#define TIMER_CLEAR_MEMORY	4713
#define TIMER_TILT_STOP		4714
#define TIMER_PAN_STOP		4715

#define FOCUS_CHECK_DELAY			250		// After 250msec we move the focus away from a button.
#define AUTO_REPEAT_DELAY			50		// Autorepeat is on the fastest possible delay of 50msec
#define AUTO_REPEAT_INITIAL_DELAY	500		// after 1/2 second we start autorepeat
#define CLEAR_MEMORY_DELAY			5000	// After 5 seconds clear the memory

#define COLOR_GREEN		RGB(0,240,0)
#define COLOR_RED		RGB(240,0,0)
#define COLOR_ORANGE	RGB(255,140,0)

//////////////////////////////////////////////////////////////////////////
// CPTZControlApp:
// See PTZControl.cpp for the implementation of this class
//

class CPTZControlApp : public CWinApp
{
public:
	CPTZControlApp();

	virtual BOOL InitInstance();

	virtual int ExitInstance();

	const Configuration& conf() const {
		return m_conf;
	}

	Configuration& mut_conf() {
		return m_conf;
	}

	void WriteConfigurationToFile();

	DECLARE_MESSAGE_MAP()

private:
	Configuration m_conf;
	CPTZControlDlg* m_pDlg;
};

extern CPTZControlApp theApp;
