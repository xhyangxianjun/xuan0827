
// Video.h : Video ���ε{�����D���Y��
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'"
#endif

#include "resource.h"       // �D�n�Ÿ�
#include "MsgBar.h"

// CVideoApp:
// �аѾ\��@�����O�� Video.cpp
//

class CVideoApp : public CWinApp
{
public:
	CVideoApp();


// �мg
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// �{���X��@
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CVideoApp theApp;
extern CMsgBar *gpMsgbar;