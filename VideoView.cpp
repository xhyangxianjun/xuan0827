
// VideoView.cpp : CVideoView ���O����@
//

#include "stdafx.h"

#ifndef SHARED_HANDLERS
#include "Video.h"
#endif

#include "VideoView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoView

IMPLEMENT_DYNCREATE(CVideoView, CScrollView)

	BEGIN_MESSAGE_MAP(CVideoView, CScrollView)
		// �зǦC�L�R�O
		ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
		ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
		ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
		ON_WM_SIZE()
		// 	ON_WM_TIMER()
		ON_WM_DESTROY()
		ON_COMMAND(ID_VIEW_SCALEVIDEO, &CVideoView::OnViewScalevideo)
		ON_UPDATE_COMMAND_UI(ID_VIEW_SCALEVIDEO, &CVideoView::OnUpdateViewScalevideo)
		ON_WM_LBUTTONDOWN()
		ON_WM_LBUTTONUP()
		ON_WM_MOUSEMOVE()
		ON_UPDATE_COMMAND_UI(ID_MOUSE_POS, &CVideoView::OnUpdateStatusbarMousePosition)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_FRAMERATE, &CVideoView::OnUpdateStatusbarAvgFramerate)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_VIDEOSIZE, &CVideoView::OnUpdateStatusbarVideosize)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_FRAMENUMBER, &CVideoView::OnUpdateStatusbarFrameNumber)
		ON_UPDATE_COMMAND_UI(ID_INDICATOR_COLOR, &CVideoView::OnUpdateStatusbarColor)
	END_MESSAGE_MAP()

	// CVideoView �غc/�Ѻc

	CVideoView::CVideoView()
		:m_bScaleVideo(FALSE),IsDown(FALSE),
		IsDraw(true),IsSelect(FALSE)
	{
		// TODO: �b���[�J�غc�{���X
		mMousePt.x = 0;
		mMousePt.y = 0;

	}

	CVideoView::~CVideoView()
	{
	}

	BOOL CVideoView::PreCreateWindow(CREATESTRUCT& cs)
	{
		// TODO: �b���g�ѭק� CREATESTRUCT cs 
		// �F��ק�������O�μ˦����ت�

		return CView::PreCreateWindow(cs);
	}

	// CVideoView �yø

	void CVideoView::OnDraw(CDC* /*pDC*/)
	{
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (!pDoc)
			return;

		// TODO: �b���[�J��͸�ƪ��yø�{���X
	}


	// CVideoView �C�L

	BOOL CVideoView::OnPreparePrinting(CPrintInfo* pInfo)
	{
		// �w�]���ǳƦC�L�{���X
		return DoPreparePrinting(pInfo);
	}

	void CVideoView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
	{
		// TODO: �[�J�C�L�e�B�~����l�]�w
	}

	void CVideoView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
	{
		// TODO: �[�J�C�L�᪺�M���{���X
	}


	// CVideoView �E�_

#ifdef _DEBUG
	void CVideoView::AssertValid() const
	{
		CView::AssertValid();
	}

	void CVideoView::Dump(CDumpContext& dc) const
	{
		CView::Dump(dc);
	}

	CVideoDoc* CVideoView::GetDocument() const // ���O�D��������
	{
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVideoDoc)));
		return (CVideoDoc*)m_pDocument;
	}
#endif //_DEBUG


	// CVideoView �T���B�z�`��


	LRESULT CVideoView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		// TODO: �b���[�J�S�w���{���X�M (��) �I�s�����O
		if(message==WM_SHOWPIC)
		{
			CVideoDoc* pDoc = GetDocument();

			int iFontSize = 18;
			CFont  m_fontLogo;
			m_fontLogo.CreateFont( iFontSize, 0, 0, 0, FF_MODERN, false,FALSE,0,0,0,0,0,0, "�L�n������");
			::SelectObject(pDoc->CompMemDC,m_fontLogo);	
			::SetBkMode(pDoc->CompMemDC,OPAQUE);//�]�w��r�I��
			::SetBkColor(pDoc->CompMemDC,RGB(0,0,255));//�]�w��r�I��
			::SetTextColor(pDoc->CompMemDC,RGB(255,255,255)); //�]�w��r���C��
			//--show video 
			PAINTSTRUCT ps;
			BeginPaint( &ps);
			//---���Video�v�� 
			BYTE* Buffer = pDoc->pData;
			int Width = pDoc->ImgW;
			int Height = pDoc->ImgH;
			int SrcX = pDoc->DrawTextPosition.x;
			int SrcY = pDoc->DrawTextPosition.y;
			CString outtext;

			StretchDIBits(pDoc->CompMemDC, 0, 0,Width, Height,0, 0, Width , Height , Buffer 
				, (BITMAPINFO*) &pDoc->bih,DIB_RGB_COLORS, SRCCOPY ); 

			CClientDC pDC(this);
			CRect rect;

			/*------------------�e��------------------*/
			if(pDoc->SelectRect){

				CPen pen(PS_SOLID,3,RGB(255,0,255));
				CPen *oldpen = pDC.SelectObject(&pen);
				pDC.SelectStockObject(NULL_BRUSH);
				rect.left = pDoc->SelectRect->left; 
				rect.right = pDoc->SelectRect->right;
				rect.top = pDoc->SelectRect->top;
				rect.bottom=pDoc->SelectRect->bottom;
//				CRect rect( CPoint(  pDoc->SelectRect->left , pDoc->SelectRect->top ), CPoint( pDoc->SelectRect->right , pDoc->SelectRect->bottom ));
				pDC.Rectangle(rect);	
			}
		
			if (pDoc->VehicleCandidates.size() > 0)
			{
				for (int i = 0; i < pDoc->VehicleCandidates.size(); i++)
				{
					rect.left = pDoc->VehicleCandidates[i].left + pDoc->ROI_LU.x;
					rect.right = pDoc->VehicleCandidates[i].right + +pDoc->ROI_LU.x;
					rect.top = pDoc->nHeight - pDoc->ROI_RB.y + pDoc->VehicleCandidates[i].top - 1;
					rect.bottom = pDoc->nHeight - pDoc->ROI_RB.y + pDoc->VehicleCandidates[i].bottom - 1;
					pDC.Rectangle(rect);
				}
			}

			/*---------------End of �e��--------------*/

			::SetTextColor(pDoc->CompMemDC,RGB(255,255,255)); //�]�w��r���C��
			::SetBkColor(pDoc->CompMemDC,RGB(0,0,255));//�]�w��r�I��


			BitBlt(pDoc->hdcStill,0,0,pDoc->ImgW, pDoc->ImgH,pDoc->CompMemDC,0,0, SRCCOPY);	
			EndPaint( &ps); 


			CString strTitle;
			strTitle.Format("%s Frame Cnt: %d  ��߮��v�j�Ǽv���h�C������ (%d) ",pDoc->m_filename, pDoc->FrameCnt, pDoc->FrameCnt2 );
			pDoc->SetTitle(strTitle);
		}
		if( message==WM_EMPTY )
		{
			CVideoDoc* pDoc = GetDocument();
			CClientDC pDC(this);

			/*------------------�e��------------------*/
			if(pDoc->SelectRect){

				CPen pen(PS_SOLID,3,RGB(255,0,255));
				CPen *oldpen = pDC.SelectObject(&pen);
				pDC.SelectStockObject(NULL_BRUSH);
				CRect rect( CPoint(  pDoc->SelectRect->left , pDoc->SelectRect->top ), CPoint( pDoc->SelectRect->right , pDoc->SelectRect->bottom ));
				pDC.Rectangle(rect);	
			}

			CString strTitle;
			strTitle.Format("%s Frame Cnt: %d  ��߮��v�j�Ǽv���h�C������ (%d) ",pDoc->m_filename, pDoc->FrameCnt, pDoc->FrameCnt2 );
			pDoc->SetTitle(strTitle);
		}

		return CView::WindowProc(message, wParam, lParam);
	}



	void CVideoView::OnSize(UINT nType, int cx, int cy)
	{
		CScrollView::OnSize(nType, cx, cy);

		// TODO: �b���[�J�z���T���B�z�`���{���X
		SetRenderWindow();
	}

	void CVideoView::SetRenderWindow()
	{
		// Set this view to be the render window
		RECT rec;
		::GetClientRect(this->m_hWnd, &rec); // Get the client area

		// Get the graph manager in the Document
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);

		if(m_bScaleVideo)  // display video in the whole window
			pDoc->SetRenderWindow(this->m_hWnd, &rec);
		else                // display video at original size
			pDoc->SetRenderWindow(this->m_hWnd);

		// 	// Render video at video size but offset left-up corner to (x, y)
		// 	pDoc->SetRenderWindow(this->m_hWnd, 100, 100);
	}

	void CVideoView::OnInitialUpdate()
	{
		CScrollView::OnInitialUpdate();

		// TODO: �b���[�J�S�w���{���X�M (��) �I�s�����O
		CSize SCSize(0,0);
		SetScrollSizes(MM_TEXT,SCSize);

		// Add the devices' names to the menu "Device"
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		// YT add
		pDoc->hWin = GetSafeHwnd();
	}

	void CVideoView::OnDestroy()
	{
		CScrollView::OnDestroy();

		// TODO: �b���[�J�z���T���B�z�`���{���X
		// 	::KillTimer(m_hWnd, ID_TIMER);
		// Need to distroy the filter graph in Document 
		// before the view is destroyed
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		pDoc->ReleaseFilterGraph();
	}


	void CVideoView::OnViewScalevideo()
	{
		// TODO: �b���[�J�z���R�O�B�z�`���{���X
		m_bScaleVideo = !m_bScaleVideo;
		SetRenderWindow();
	}


	void CVideoView::OnUpdateViewScalevideo(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		pCmdUI->SetCheck(m_bScaleVideo);
	}


	void CVideoView::OnLButtonDown(UINT nFlags, CPoint point)
	{
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		CPoint Position = GetScrollPosition() + point;

		if( pDoc->pData  &&  pDoc->IsPause ){
			SetCapture();//�]�w�ƹ���Цb���b���檺����
			pDoc->UpdateAllViews(NULL);
			if( this == GetCapture() && (unsigned)Position.x < pDoc->ImgW && (unsigned)Position.y < pDoc->ImgH ){
				CClientDC pDC(this);

				if(IsSelect){
					if(pDoc->SelectRect != NULL)
					{
						pDoc->SelectRect->Is_Move_Resize(Position.x,Position.y);
					}
				}
				if(IsDraw){
					if( pDoc->SelectRect) delete pDoc->SelectRect;
					pDoc->SelectRect = new Retangle(Position.x,Position.y,0);
					pDoc->SelectRect->Draw(pDC);
					pDoc->SelectRect->ShowSize(pDC);
				}
			}
			IsDown = true;
		}

		CScrollView::OnLButtonDown(nFlags, point);
	}


	void CVideoView::OnLButtonUp(UINT nFlags, CPoint point)
	{
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		CPoint Position = GetScrollPosition() + point;

		if(  pDoc->pData  &&  pDoc->IsPause )
		{
			if(this == GetCapture() && (unsigned)Position.x < pDoc->ImgW && (unsigned)Position.y < pDoc->ImgH )
			{
				CClientDC pDC(this);

				if(IsSelect)
				{
					if(pDoc->SelectRect != NULL)
					{
						pDoc->SelectRect->SetSelect(false);
						pDoc->SelectRect->SetResize(false);
						pDoc->SelectRect->SetLock(false);
						pDoc->SelectRect->ShowSize(pDC);
					}
				}

				if(IsDraw)
				{
					pDoc->SelectRect->SetRB(Position.x,Position.y);
					pDoc->SelectRect->Draw(pDC);
					pDoc->SelectRect->ShowSize(pDC);
					IsSelect = true; 
				}
				IsDown = false;
				//pDoc->UpdateAllViews(NULL);
			}
			if(pDoc->SelectRect)
			{
				pDoc->ROI_LU.x=pDoc->SelectRect->left;
				pDoc->ROI_LU.y=pDoc->nHeight - pDoc->SelectRect->bottom - 1;
				if( pDoc->ROI_LU.y < 0 ) 
					pDoc->ROI_LU.y=0;
				pDoc->ROI_RB.x=pDoc->SelectRect->right;
				pDoc->ROI_RB.y=pDoc->nHeight - pDoc->SelectRect->top -1;
				//////////////////////////////////////////////////////////////////////////
// 				gpMsgbar->ShowMessage("(%d,%d)..(%d,%d)\n", pDoc->ROI_LU.x, pDoc->ROI_LU.y, pDoc->ROI_RB.x, pDoc->ROI_RB.y);
				//ROI�����////////////////////////////////////////////////////////////////////////
				pDoc->_height = abs(pDoc->ROI_LU.y - pDoc->ROI_RB.y) + 1;
				pDoc->_width = abs(pDoc->ROI_LU.x - pDoc->ROI_RB.x) + 1;
				pDoc->_pixel = pDoc->_width * pDoc->_height;
				pDoc->_size = pDoc->_pixel * 3;
				pDoc->_effw = pDoc->_width * 3;

				pDoc->ROI_RGB = new BYTE[pDoc->_size];
				pDoc->ROI_GRAY = new BYTE[pDoc->_pixel];
				pDoc->ROI_GRADIENT = new BYTE[pDoc->_pixel];
				//////////////////////////////////////////////////////////////////////////
			}
			
			if(pDoc->UsingSmallFrame){
				pDoc->ROI_LU.x/=2;
				pDoc->ROI_LU.y/=2;
				pDoc->ROI_RB.x/=2;
				pDoc->ROI_RB.y/=2;
			}

			ReleaseCapture();
		}
		CScrollView::OnLButtonUp(nFlags, point);
	}


	void CVideoView::OnMouseMove(UINT nFlags, CPoint point)
	{
		CVideoDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		CPoint Position = GetScrollPosition() + point;
		pMouse_pos = point;

		if(IsDown && (unsigned)Position.x < pDoc->ImgW && (unsigned)Position.y < pDoc->ImgH )
		{		
			CClientDC pDC(this);
			pDC.SetROP2(R2_NOTXORPEN );
			pDC.SetBkMode(TRANSPARENT);

			if(IsSelect)
			{	
				//for(int i = 0 ; i < 10 ; i++)
				if(pDoc->SelectRect != NULL)
				{
					if(pDoc->SelectRect->GetSelect())
					{
						pDoc->SelectRect->Draw(pDC);
						pDoc->SelectRect->Move(Position.x,Position.y, pDoc->ImgW , pDoc->ImgH );
						pDoc->SelectRect->Draw(pDC);
					}
					if(pDoc->SelectRect->GetResize())
					{
						pDoc->SelectRect->Draw(pDC);
						pDoc->SelectRect->SetRB(Position.x,Position.y);
						pDoc->SelectRect->Draw(pDC);
						pDoc->SelectRect->ShowSize(pDC);
					}
				}
			}

			if(IsDraw)
			{
				pDoc->SelectRect->Draw(pDC);
				pDoc->SelectRect->SetRB(Position.x,Position.y);
				pDoc->SelectRect->Draw(pDC);
			}	
		}

		//mMousePt = Position;
		CScrollView::OnMouseMove(nFlags, point);
	}


	void CVideoView::OnUpdateStatusbarMousePosition(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		pCmdUI->Enable(); 
		CString str;
		str.Format(_T("(%d,%d)"), pMouse_pos.x, pMouse_pos.y ); 
		pCmdUI->SetText(str); 
	}


	void CVideoView::OnUpdateStatusbarAvgFramerate(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		CVideoDoc *pDoc = GetDocument();
		pCmdUI->Enable(); 
		CString str;
		str.Format(_T("%4.2lf fps"), pDoc->GetFrameRate()); 
		pCmdUI->SetText(str);
	}


	void CVideoView::OnUpdateStatusbarVideosize(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		CVideoDoc *pDoc = GetDocument();
		pCmdUI->Enable(); 
		CString str;
		SIZE s = pDoc->GetVideoSizeNumber();
		SetScrollSizes(MM_TEXT, s);
		str.Format(_T("%d x %d"), s.cx, s.cy); 
		pCmdUI->SetText(str);
	}


	void CVideoView::OnUpdateStatusbarFrameNumber(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		CVideoDoc *pDoc = GetDocument();
		pCmdUI->Enable(); 
		CString str;
		str.Format(_T("%d"), pDoc->GetFrameNum()); 
		pCmdUI->SetText(str);
	}


	void CVideoView::OnUpdateStatusbarColor(CCmdUI *pCmdUI)
	{
		// TODO: �b���[�J�z���R�O��s UI �B�z�`���{���X
		int r, g, b, y, k;
		CVideoDoc *pDoc = GetDocument();

		r = g = b = 0;
		if (mMousePt.x > 0 && mMousePt.x < pDoc->nWidth && mMousePt.y >= 0 && mMousePt.y < pDoc->nHeight)
		{
			y = pDoc->nHeight - mMousePt.y - 1;
			k = y * pDoc->nEffw + mMousePt.x * pDoc->nBPP;

				b = pDoc->pData[k];
				g = pDoc->pData[k+1];
				r = pDoc->pData[k+2];
		

		}

		pCmdUI->Enable(); 
		CString str;
		str.Format(_T("(%d,%d,%d)"), r, g, b); 
		pCmdUI->SetText(str);
	}
