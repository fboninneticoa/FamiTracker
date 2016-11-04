/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerView.h"
#include "InstrumentEditDlg.h"

const int KEYBOARD_TOP		= 313;
const int KEYBOARD_LEFT		= 12;
const int KEYBOARD_WIDTH	= 561;
const int KEYBOARD_HEIGHT	= 58;

// CInstrumentEditDlg dialog

IMPLEMENT_DYNAMIC(CInstrumentEditDlg, CDialog)

CInstrumentEditDlg::CInstrumentEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentEditDlg::IDD, pParent)
{
	m_bOpened = false;
}

CInstrumentEditDlg::~CInstrumentEditDlg()
{
}

void CInstrumentEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_INST_TAB, OnTcnSelchangeInstTab)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


// CInstrumentEditDlg message handlers

void CInstrumentEditDlg::OnBnClickedClose()
{
	EndDialog(0x00);
}

BOOL CInstrumentEditDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_iSelectedInstType = -1;
	m_iLastNote = -1;
	m_bOpened = true;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditDlg::Load2A03()
{
	CRect Rect, ParentRect;
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));

	pTabControl->GetWindowRect(&ParentRect);
	pTabControl->DeleteAllItems();
	pTabControl->InsertItem(0, "Instrument settings");
	pTabControl->InsertItem(1, "DPCM samples");

	InstrumentSettings.Create(IDD_INSTRUMENT_INTERNAL, this);
	InstrumentSettings.GetWindowRect(&Rect);
	Rect.MoveToXY(ParentRect.left - Rect.left + 1, ParentRect.top - Rect.top + 21);
	Rect.bottom -= 2;
	Rect.right += 1;
	InstrumentSettings.MoveWindow(Rect);

	InstrumentDPCM.Create(IDD_INSTRUMENT_DPCM, this);
	InstrumentDPCM.GetWindowRect(&Rect);
	Rect.MoveToXY(ParentRect.left - Rect.left + 1, ParentRect.top - Rect.top + 21);
	Rect.bottom -= 2;
	Rect.right += 1;
	InstrumentDPCM.MoveWindow(Rect);

	InstrumentSettings.ShowWindow(SW_SHOW);
	InstrumentDPCM.ShowWindow(SW_HIDE);
}

void CInstrumentEditDlg::LoadVRC6()
{
	CRect Rect, ParentRect;
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));

	pTabControl->GetWindowRect(&ParentRect);
	pTabControl->DeleteAllItems();
	pTabControl->InsertItem(0, "Konami VRC6");

	InstrumentVRC6.Create(IDD_INSTRUMENT_INTERNAL, this);
	InstrumentVRC6.GetWindowRect(&Rect);
	Rect.MoveToXY(ParentRect.left - Rect.left + 1, ParentRect.top - Rect.top + 21);
	Rect.bottom -= 2;
	Rect.right += 1;
	InstrumentVRC6.MoveWindow(Rect);
	InstrumentVRC6.ShowWindow(SW_SHOW);
}

void CInstrumentEditDlg::SetCurrentInstrument(int Index)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(theApp.GetDocument());
	CString Title;
	char Name[256];

	pDoc->GetInstrumentName(Index, Name);
	Title.Format("Instrument editor - %s", Name);
	SetWindowText(Title);

	CInstrument *pInst = pDoc->GetInstrument(Index);
	int InstType = pInst->GetType();

	if (m_iSelectedInstType != InstType) {
		switch (m_iSelectedInstType) {
			case INST_2A03:
				InstrumentSettings.DestroyWindow();
				InstrumentDPCM.DestroyWindow();
				break;
			case INST_VRC6:
				InstrumentVRC6.DestroyWindow();
				break;
		}
		switch (InstType) {
			case INST_2A03:
				Load2A03();
				break;
			case INST_VRC6:
				LoadVRC6();
				break;
		}
	}

	switch (InstType) {
		case INST_2A03:
			InstrumentSettings.SetCurrentInstrument(Index);
			InstrumentDPCM.SetCurrentInstrument(Index);
			break;
		case INST_VRC6:
			InstrumentVRC6.SetCurrentInstrument(Index);
			break;
	}

	m_iSelectedInstType = InstType;
}

void CInstrumentEditDlg::OnTcnSelchangeInstTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTabCtrl *pTabControl = (CTabCtrl*)GetDlgItem(IDC_INST_TAB);
	int Selection;

	Selection = pTabControl->GetCurSel();

	switch (Selection) {
		case 0:
			InstrumentSettings.ShowWindow(SW_SHOW);
			InstrumentDPCM.ShowWindow(SW_HIDE);
			break;
		case 1:
			InstrumentSettings.ShowWindow(SW_HIDE);
			InstrumentDPCM.ShowWindow(SW_SHOW);
			break;
	}

	*pResult = 0;
}

void CInstrumentEditDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CDialog::OnPaint() for painting messages

	const int WHITE_KEY_W	= 10;
	const int BLACK_KEY_W	= 8;

	CBitmap WhiteKeyBmp, BlackKeyBmp, *OldWhite;
	CBitmap WhiteKeyMarkBmp, BlackKeyMarkBmp, *OldBlack;

	CDC WhiteKey, BlackKey;
	CDC BackDC;

	CBitmap Bmp, *OldBmp;

	Bmp.CreateCompatibleBitmap(&dc, 800, 800);
	BackDC.CreateCompatibleDC(&dc);
	OldBmp = BackDC.SelectObject(&Bmp);

	WhiteKeyBmp.LoadBitmap(IDB_KEY_WHITE);
	BlackKeyBmp.LoadBitmap(IDB_KEY_BLACK);
	WhiteKeyMarkBmp.LoadBitmap(IDB_KEY_WHITE_MARK);
	BlackKeyMarkBmp.LoadBitmap(IDB_KEY_BLACK_MARK);

	WhiteKey.CreateCompatibleDC(&dc);
	BlackKey.CreateCompatibleDC(&dc);

	int i, j;

	OldWhite = WhiteKey.SelectObject(&WhiteKeyBmp);
	OldBlack = BlackKey.SelectObject(&BlackKeyBmp);

	int Pos;
	int NoteIndex;

	const int WHITE[]	= {0, 2, 4, 5, 7, 9, 11};
	const int BLACK_1[] = {1, 3};
	const int BLACK_2[] = {6, 8, 10};

	int Note	= m_iLightNote % 12;
	int Octave	= m_iLightNote / 12;

	for (j = 0; j < 8; j++) {
		Pos = /*KEYBOARD_LEFT +*/ ((WHITE_KEY_W * 7) * j);

		for (i = 0; i < 7; i++) {
			NoteIndex = (j * 7 + i);
			if ((Note == WHITE[i]) && (Octave == j) && m_iLightNote != -1)
				WhiteKey.SelectObject(WhiteKeyMarkBmp);
			else
				WhiteKey.SelectObject(WhiteKeyBmp);

			BackDC.BitBlt(i * WHITE_KEY_W + Pos, 0, 100, 100, &WhiteKey, 0, 0, SRCCOPY);
		}

		for (i = 0; i < 2; i++) {
			NoteIndex = (j * 7 + i);
			if ((Note == BLACK_1[i]) && (Octave == j) && m_iLightNote != -1)
				BlackKey.SelectObject(BlackKeyMarkBmp);
			else
				BlackKey.SelectObject(BlackKeyBmp);

			BackDC.BitBlt(i * WHITE_KEY_W + WHITE_KEY_W / 2 + 1 + Pos, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}

		for (i = 0; i < 3; i++) {
			if ((Note == BLACK_2[i]) && (Octave == j) && m_iLightNote != -1)
				BlackKey.SelectObject(BlackKeyMarkBmp);
			else
				BlackKey.SelectObject(BlackKeyBmp);

			BackDC.BitBlt((i + 3) * WHITE_KEY_W + WHITE_KEY_W / 2 + 1 + Pos, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}
	}

	WhiteKey.SelectObject(OldWhite);
	BlackKey.SelectObject(OldBlack);

	dc.BitBlt(KEYBOARD_LEFT, KEYBOARD_TOP, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, &BackDC, 0, 0, SRCCOPY);

	BackDC.SelectObject(OldBmp);
}

void CInstrumentEditDlg::ChangeNoteState(int Note)
{
	m_iLightNote = Note;

	if (m_hWnd)
		RedrawWindow(CRect(KEYBOARD_LEFT, KEYBOARD_TOP, 580, KEYBOARD_TOP + 100), 0, RDW_INVALIDATE);
}

void CInstrumentEditDlg::SwitchOnNote(int x, int y)
{
	stChanNote NoteData;
	int Octave;
	int Note;
	int KeyPos;

	int Channel = ((CFamiTrackerView*)theApp.GetView())->GetSelectedChannel();

	Octave = (x - KEYBOARD_LEFT) / 70;

	if (y > KEYBOARD_TOP && y < (KEYBOARD_TOP + 58) && x > KEYBOARD_LEFT && x < (KEYBOARD_LEFT + 560)) {
		if (y > KEYBOARD_TOP + 38) {
			
			// Only white keys
			KeyPos = (x - KEYBOARD_LEFT) % 70;

			if (KeyPos >= 0 && KeyPos < 10)				// C
				Note = 0;
			else if (KeyPos >= 10 && KeyPos < 20)		// D
				Note = 2;
			else if (KeyPos >= 20 && KeyPos < 30)		// E
				Note = 4;
			else if (KeyPos >= 30 && KeyPos < 40)		// F
				Note = 5;
			else if (KeyPos >= 40 && KeyPos < 50)		// G
				Note = 7;
			else if (KeyPos >= 50 && KeyPos < 60)		// A
				Note = 9;
			else if (KeyPos >= 60 && KeyPos < 70)		// B
				Note = 11;
		}
		else {
			// Black and white keys
			KeyPos = (x - KEYBOARD_LEFT) % 70;

			if (KeyPos >= 0 && KeyPos < 7)			// C
				Note = 0;
			else if (KeyPos >= 7 && KeyPos < 13) 	// C#
				Note = 1;
			else if (KeyPos >= 13 && KeyPos < 16) 	// D
				Note = 2;
			else if (KeyPos >= 16 && KeyPos < 23) 	// D#
				Note = 3;
			else if (KeyPos >= 23 && KeyPos < 30) 	// E
				Note = 4;
			else if (KeyPos >= 30 && KeyPos < 37) 	// F
				Note = 5;
			else if (KeyPos >= 37 && KeyPos < 43) 	// F#
				Note = 6;
			else if (KeyPos >= 43 && KeyPos < 46) 	// G
				Note = 7;
			else if (KeyPos >= 46 && KeyPos < 53) 	// G#
				Note = 8;
			else if (KeyPos >= 53 && KeyPos < 56) 	// A
				Note = 9;
			else if (KeyPos >= 56 && KeyPos < 62) 	// A#
				Note = 10;
			else if (KeyPos >= 62 && KeyPos < 70) 	// B
				Note = 11;
		}

		if (Note + (Octave * 12) != m_iLastNote) {
			NoteData.Note			= Note + 1;
			NoteData.Octave			= Octave;
			NoteData.Vol			= 0x0F;
			NoteData.Instrument		= ((CFamiTrackerView*)theApp.GetView())->GetInstrument();
			NoteData.EffNumber[0]	= 0;
			NoteData.EffParam[0]	= 0;

			((CFamiTrackerView*)theApp.GetView())->FeedNote(Channel, &NoteData);
		}

		m_iLastNote = Note + (Octave * 12);
	}
	else {
		NoteData.Note			= HALT;
		NoteData.Octave			= 0;
		NoteData.Vol			= 0;
		NoteData.Instrument		= 0;
		NoteData.EffNumber[0]	= 0;
		NoteData.EffParam[0]	= 0;

		((CFamiTrackerView*)theApp.GetView())->FeedNote(Channel, &NoteData);

		m_iLastNote = -1;
	}
}

void CInstrumentEditDlg::SwitchOffNote()
{
	stChanNote NoteData;

	int Channel = ((CFamiTrackerView*)theApp.GetView())->GetSelectedChannel();

	NoteData.Note			= HALT;
	NoteData.Octave			= 0;
	NoteData.Vol			= 0;
	NoteData.Instrument		= 0;
	NoteData.EffNumber[0]	= 0;
	NoteData.EffParam[0]	= 0;

	((CFamiTrackerView*)theApp.GetView())->FeedNote(Channel, &NoteData);

	m_iLastNote = -1;
}

void CInstrumentEditDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);

	CDialog::OnLButtonDown(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	SwitchOffNote();
	CDialog::OnLButtonUp(nFlags, point);
}

void CInstrumentEditDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
		SwitchOnNote(point.x, point.y);

	CDialog::OnMouseMove(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);
	CDialog::OnLButtonDblClk(nFlags, point);
}

BOOL CInstrumentEditDlg::DestroyWindow()
{
	if (m_bOpened) {
		switch (m_iSelectedInstType) {
			case INST_2A03:
				InstrumentSettings.DestroyWindow();
				InstrumentDPCM.DestroyWindow();
				break;
			case INST_VRC6:
				InstrumentVRC6.DestroyWindow();
				break;
		}
	}
	
	m_iSelectedInstType = -1;
	m_bOpened = false;
	
	return CDialog::DestroyWindow();
}

void CInstrumentEditDlg::OnOK()
{
	DestroyWindow();
	CDialog::OnOK();
}

void CInstrumentEditDlg::OnCancel()
{
	DestroyWindow();
}