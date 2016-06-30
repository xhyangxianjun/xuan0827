
// VideoView.h : CVideoView ���O������
//

#pragma once
#include "Retangle.h"
#include "MainFrm.h"
#include "VideoDoc.h"
#include "Retangle.h"

class CVideoView : public CScrollView
{
protected: // �ȱq�ǦC�ƫإ�
	CVideoView();
	DECLARE_DYNCREATE(CVideoView)

	// �ݩ�
public:
	CVideoDoc* GetDocument() const;

	bool m_bScaleVideo;  // scale the video the the size of the view

	CPoint mMousePt;
	CPoint pMouse_pos;

	
	//for ROI
	bool IsSelect;
	bool IsDraw;
	bool IsDown;

	// �@�~
public:
	// 	void CreateMemory();
	// 	void FreeMemory();
	// The function sets this view to be the renderer window
	void SetRenderWindow();

	// �мg
public:
	virtual void OnDraw(CDC* pDC);  // �мg�H�yø���˵�
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

	// �{���X��@
public:
	virtual ~CVideoView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// ���ͪ��T�������禡
protected:
	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnInitialUpdate();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnViewScalevideo();
	afx_msg void OnUpdateViewScalevideo(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnUpdateStatusbarMousePosition(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStatusbarAvgFramerate(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStatusbarVideosize(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStatusbarFrameNumber(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStatusbarColor(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // VideoView.cpp ������������
inline CVideoDoc* CVideoView::GetDocument() const
{ return reinterpret_cast<CVideoDoc*>(m_pDocument); }
#endif

