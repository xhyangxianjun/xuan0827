#pragma once


// AdaboostScaleDlg ��ܤ��

class AdaboostScaleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(AdaboostScaleDlg)

public:
	AdaboostScaleDlg(CWnd* pParent = NULL);   // �зǫغc�禡
	virtual ~AdaboostScaleDlg();

// ��ܤ�����
	enum { IDD = IDD_DIALOG_ADABOOST_SCALE };
	int m_Up;
	int m_Bottom;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	//virtual INT_PTR DoModal();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual INT_PTR DoModal();
	int GetUpBound();
	int GetBottomBound();
};
