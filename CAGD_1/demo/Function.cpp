// Function.cpp : implementation file
//

#include "pch.h"
#include "Function.h"
#include "afxdialogex.h"


// Function dialog

IMPLEMENT_DYNAMIC(Function, CDialogEx)

Function::Function(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PROPPAGE_MEDIUM, pParent)
{

}

Function::~Function()
{
}

void Function::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(Function, CDialogEx)
END_MESSAGE_MAP()


// Function message handlers
