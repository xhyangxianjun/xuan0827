
// MainFrm.cpp : CMainFrame ���O����@
//

#include "stdafx.h"
#include "Video.h"

#include "MsgBar.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_COMMAND_EX(ID_VIEW_MSGBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MSGBAR, OnUpdateControlBarMenu)
	ON_WM_CREATE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ���A�C���ܾ�
	ID_INDICATOR_VIDEOSIZE,
	ID_INDICATOR_FRAMERATE,
	ID_INDICATOR_FRAMENUMBER,
// 	ID_INDICATOR_CAPS,
// 	ID_INDICATOR_NUM,
// 	ID_INDICATOR_SCRL,
	ID_MOUSE_POS,
	ID_INDICATOR_COLOR,
};

CMsgBar *gpMsgbar;

// CMainFrame �غc/�Ѻc

CMainFrame::CMainFrame()
{
	// TODO: �b���[�J������l�Ƶ{���X
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("�L�k�إߤu��C\n");
		return -1;      // �L�k�إ�
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("�L�k�إߪ��A�C\n");
		return -1;      // �L�k�إ�
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: �p�G�z���n�i�H�T�w�u��C�A�ЧR���o�T��
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	//message bar
	if (!m_Msgbar.Create(_T("MsgBarFrm"), this, CSize(0,200),
		TRUE, ID_VIEW_MSGBAR, CBRS_BOTTOM | WS_VISIBLE | CBRS_SIZE_DYNAMIC))
	{
		TRACE0("Failed to create dialog bar m_Msgbar\n");
		return -1;
	}

	m_Msgbar.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_Msgbar);
	gpMsgbar = &m_Msgbar;

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �b���g�ѭק� CREATESTRUCT cs 
	// �F��ק�������O�μ˦����ت�

	return TRUE;
}

// CMainFrame �E�_

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame �T���B�z�`��
