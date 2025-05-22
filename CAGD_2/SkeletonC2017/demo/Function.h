#pragma once


// Function dialog

class Function : public CDialogEx
{
	DECLARE_DYNAMIC(Function)

public:
	Function(CWnd* pParent = nullptr);   // standard constructor
	virtual ~Function();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPPAGE_MEDIUM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
