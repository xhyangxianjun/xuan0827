// AdaboostScaleDlg.cpp : ��@��
//

#include "stdafx.h"
#include "video.h"
#include "AdaboostScaleDlg.h"
#include "afxdialogex.h"


// AdaboostScaleDlg ��ܤ��

IMPLEMENT_DYNAMIC(AdaboostScaleDlg, CDialogEx)

AdaboostScaleDlg::AdaboostScaleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(AdaboostScaleDlg::IDD, pParent)
{
	m_Up = 0;
	m_Bottom = 0;
}

AdaboostScaleDlg::~AdaboostScaleDlg()
{
}

void AdaboostScaleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
// 	DDX_Text(pDX,IDC_TEXT_UP,m_Up);
// 	DDX_Text(pDX,IDC_TEXT_BOTTOM,m_Bottom);
}


BEGIN_MESSAGE_MAP(AdaboostScaleDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &AdaboostScaleDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// AdaboostScaleDlg �T���B�z�`��


afx_msg void AdaboostScaleDlg::OnBnClickedOk()
{
	// TODO: �b���[�J����i���B�z�`���{���X
	CDialogEx::OnOK();
	char szText[10];
	CEdit *pUp = (CEdit*)(GetDlgItem(IDC_TEXT_UP));
	CEdit *pBottom = (CEdit*)(GetDlgItem(IDC_TEXT_BOTTOM));
	pUp->GetWindowText(szText,10);
	m_Up = atoi(szText);
	pBottom->GetWindowText(szText,10);
	m_Bottom = atoi(szText);
	//m_Up = atoi()
	//CDialogEx::DoDataExchange(pDX);
}

INT_PTR AdaboostScaleDlg::DoModal()
{
	// TODO: �b���[�J�S�w���{���X�M (��) �I�s�����O

	return CDialogEx::DoModal();
}

int AdaboostScaleDlg::GetUpBound(){
	return m_Up;
}

int AdaboostScaleDlg::GetBottomBound(){
	return m_Bottom;
}