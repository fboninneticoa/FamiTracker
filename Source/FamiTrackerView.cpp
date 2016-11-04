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
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "MIDI.h"
#include "InstrumentEditDlg.h"
#include "SpeedDlg.h"
#include "SoundGen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_CHANNEL(x)	((x - LEFT_OFFSET - 40) / CHANNEL_WIDTH)
#define GET_COLUMN(x)	((x - LEFT_OFFSET - 40) - (GET_CHANNEL(x) * CHANNEL_WIDTH))

const char CLIPBOARD_ID[] = _T("FamiTracker");

enum { COL_NOTE, COL_INSTRUMENT, COL_VOLUME, COL_EFFECT };

const unsigned int PAGE_SIZE = 4;

const unsigned int TEXT_POSITION		= 10;
const unsigned int TOP_OFFSET			= 46;
const unsigned int LEFT_OFFSET			= 10;
const unsigned int ROW_HEIGHT			= 15;

const unsigned int TEXT_HEIGHT			= 18;

const unsigned int COLUMN_SPACING		= 4;
const unsigned int CHAR_WIDTH			= 12;

const unsigned int ROW_NUMBER_WIDTH		= 20;
const unsigned int CHANNEL_SPACING		= 10;

const unsigned int CHANNEL_START		= LEFT_OFFSET * 2 + ROW_NUMBER_WIDTH - 3;

const unsigned int CHANNEL_WIDTH		= CHAR_WIDTH * 9 + COLUMN_SPACING * 3;
const unsigned int CHAN_AREA_WIDTH		= CHANNEL_WIDTH + CHANNEL_SPACING * 2;

const unsigned int COLUMN_SPACE[]		= {CHAR_WIDTH * 3 + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING, 
										   CHAR_WIDTH + COLUMN_SPACING,  
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING};

const unsigned int CURSOR_SPACE[]		= {CHAR_WIDTH * 3,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH};

const unsigned int COLUMN_SEL[]			= {0, CHAR_WIDTH * 3 + COLUMN_SPACING,
											CHAR_WIDTH * 4 + COLUMN_SPACING + 2,
											CHAR_WIDTH * 5 + COLUMN_SPACING * 2, 
											CHAR_WIDTH * 6 + COLUMN_SPACING * 3,
											CHAR_WIDTH * 7 + COLUMN_SPACING * 4 - 2,
											CHAR_WIDTH * 8 + COLUMN_SPACING * 4 - 2, 
											CHAR_WIDTH * 9 + COLUMN_SPACING * 4,
											CHAR_WIDTH * 10 + COLUMN_SPACING * 5 - 2,
											CHAR_WIDTH * 11 + COLUMN_SPACING * 5 - 2, 
											CHAR_WIDTH * 12 + COLUMN_SPACING * 5,
											CHAR_WIDTH * 13 + COLUMN_SPACING * 6 - 2,
											CHAR_WIDTH * 14 + COLUMN_SPACING * 6 - 2, 
											CHAR_WIDTH * 15 + COLUMN_SPACING * 6,
											CHAR_WIDTH * 16 + COLUMN_SPACING * 7 - 2,
											CHAR_WIDTH * 17 + COLUMN_SPACING * 7 - 2, 
											CHAR_WIDTH * 18 + COLUMN_SPACING * 7};

const unsigned int GET_APPR_START_SEL[]	= {0, 1, 1, 3, 4, 4, 4, 7, 7, 7, 10, 10, 10, 13, 13, 13};
const unsigned int GET_APPR_END_SEL[]	= {0, 2, 2, 3, 6, 6, 6, 9, 9, 9, 12, 12, 12, 15, 15, 15};

const char *EFFECT_TEXTS[] = {
	{_T("FXX - Set speed/tempo")},
	{_T("BXX - Jump to frame")},
	{_T("DXX - Halt")},
	{_T("CXX - Skip to next frame")},
	{_T("EXX - Set volume")},
	{_T("3XX - Automatic portamento")},
	{_T("(not used)")},
	{_T("HXY - Hardware sweep up, x = speed, y = shift")},
	{_T("IXY - Hardware sweep down, x = speed, y = shift")},
	{_T("0XX - Arpeggio")},
	{_T("4XY - Vibrato, x = speed, y = depth")},
	{_T("7XY - Tremolo, x = speed, y = depth")},
	{_T("PXX - Fine pitch")},
	{_T("GXX - Row delay")},
	{_T("ZXX - DPCM delta counter setting")},
	{_T("1XX - Portamento up")},
	{_T("2XX - Portamento down")},
	{_T("VXX - Square duty / Noise mode")},
	{_T("YXX - DPCM sample offset")}
};

int FadeCol[5];

// fix this shit!
int ArpeggioNotes[128];
int ArpeggioPtr = 0, LastArpPtr = 0;
bool DoArpeggiate = false;
int KeyCount = 0;

// CFamiTrackerView

IMPLEMENT_DYNCREATE(CFamiTrackerView, CView)

BEGIN_MESSAGE_MAP(CFamiTrackerView, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteoverwrite)
	ON_COMMAND(ID_EDIT_INSTRUMENTMASK, OnEditInstrumentMask)
	ON_COMMAND(ID_FRAME_INSERT, OnFrameInsert)
	ON_COMMAND(ID_FRAME_REMOVE, OnFrameRemove)
	ON_COMMAND(ID_TRANSPOSE_DECREASENOTE, OnTransposeDecreasenote)
	ON_COMMAND(ID_TRANSPOSE_DECREASEOCTAVE, OnTransposeDecreaseoctave)
	ON_COMMAND(ID_TRANSPOSE_INCREASENOTE, OnTransposeIncreasenote)
	ON_COMMAND(ID_TRANSPOSE_INCREASEOCTAVE, OnTransposeIncreaseoctave)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
	ON_COMMAND(ID_TRACKER_EDIT, OnTrackerEdit)
	ON_COMMAND(ID_TRACKER_PAL, OnTrackerPal)
	ON_COMMAND(ID_TRACKER_NTSC, OnTrackerNtsc)
	ON_COMMAND(ID_SPEED_CUSTOM, OnSpeedCustom)
	ON_COMMAND(ID_SPEED_DEFAULT, OnSpeedDefault)
	ON_COMMAND(ID_TRACKER_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_TRACKER_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_EDIT_PASTEMIX, OnEditPastemix)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveframedown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveframeup)
	ON_COMMAND(ID_CMD_OCTAVE_NEXT, OnNextOctave)
	ON_COMMAND(ID_CMD_OCTAVE_PREVIOUS, OnPreviousOctave)
	ON_COMMAND(ID_CMD_PASTEOVERWRITE, OnPasteOverwrite)
	ON_COMMAND(ID_CMD_PASTEMIXED, OnPasteMixed)	
	ON_COMMAND(ID_EDIT_GRADIENT, OnEditGradient)
	ON_COMMAND(ID_CMD_NEXT_INSTRUMENT, OnNextInstrument)
	ON_COMMAND(ID_CMD_PREV_INSTRUMENT, OnPrevInstrument)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INSTRUMENTMASK, OnUpdateEditInstrumentMask)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFAULT, OnUpdateSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteoverwrite)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_FRAME_REMOVE, OnUpdateFrameRemove)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEMIX, OnUpdateEditPastemix)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEDOWN, OnUpdateModuleMoveframedown)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEUP, OnUpdateModuleMoveframeup)
	ON_WM_MENUCHAR()
END_MESSAGE_MAP()

struct stClipData {
	unsigned int		Size;
	bool				FirstColumn;
	unsigned int		NumCols;
	char				Notes[MAX_PATTERN_LENGTH];
	char				Octaves[MAX_PATTERN_LENGTH];
	int					ColumnData[COLUMNS + 3 * 4][MAX_PATTERN_LENGTH];
};

volatile static int iStillAlive = 20;

bool bInsertedEffect = false;
int iLastEffect = 0;

bool IsColumnSelected(int Start, int End, int Column)
{
	int SelColStart = Start;
	int SelColEnd	= End;

	if (SelColStart > SelColEnd) {
		SelColStart = End; 
		SelColEnd	= Start;
	}

	if (SelColStart == 2)
		SelColStart = 1;
	if (SelColStart == 5 || SelColStart == 6)
		SelColStart = 4;
	if (SelColStart == 8 || SelColStart == 9)
		SelColStart = 7;
	if (SelColStart == 11 || SelColStart == 12)
		SelColStart = 10;
	if (SelColStart == 14 || SelColStart == 15)
		SelColStart = 13;

	if (SelColEnd == 1)
		SelColEnd = 2;
	if (SelColEnd == 4 || SelColEnd == 5)
		SelColEnd = 6;
	if (SelColEnd == 7 || SelColEnd == 8)
		SelColEnd = 9;
	if (SelColEnd == 10 || SelColEnd == 11)
		SelColEnd = 12;
	if (SelColEnd == 13 || SelColEnd == 14)
		SelColEnd = 15;

	switch (Column) {
		case 0:
			return (SelColStart == 0);
		case 1:
			return (SelColStart <= 1 && SelColEnd >= 2);
		case 2:
			return (SelColStart <= 3 && SelColEnd >= 3);
		case 3:
			return (SelColStart <= 4 && SelColEnd >= 6);
		case 4:
			return (SelColStart <= 7 && SelColEnd >= 9);
		case 5:
			return (SelColStart <= 10 && SelColEnd >= 12);
		case 6:
			return (SelColEnd == 15);
	}

	return false;
}

int GetMax(int x, int y)
{
	return (x > y ? x : y);
}

int GetMin(int x, int y)
{
	return (x < y ? x : y);
}

int GetColumnAtPoint(int PointX, int MaxColumns)
{
	int ColumnOffset, Offset = 0;
	int i;
	int Column = 0;

	ColumnOffset = PointX;

	for (i = 0; i < MaxColumns; i++) {
		Offset += COLUMN_SPACE[i];
		if (ColumnOffset < Offset) {
			Column = i;
			break;
		}
	}

	return Column;
}

// Convert keys 0-F to numbers
int ConvertKeyToHex(int Key) {

	switch (Key) {
		case 48: return 0x00;
		case 49: return 0x01;
		case 50: return 0x02;
		case 51: return 0x03;
		case 52: return 0x04;
		case 53: return 0x05;
		case 54: return 0x06;
		case 55: return 0x07;
		case 56: return 0x08;
		case 57: return 0x09;
		case 65: return 0x0A;
		case 66: return 0x0B;
		case 67: return 0x0C;
		case 68: return 0x0D;
		case 69: return 0x0E;
		case 70: return 0x0F;
	}

	return -1;
}

// Convert keys 0-9 to numbers
int ConvertKeyToDec(int Key)
{
	if (Key > 47 && Key < 58)
		return Key - 48;

	return -1;
}


// CFamiTrackerView construction/destruction

CFamiTrackerView::CFamiTrackerView()
{
	int i;

	m_bInitialized		= false;

	m_iCurrentRow		= 0;
	m_iCurrentFrame		= 0;
	m_iCursorRow		= 0;
	m_iKeyStepping		= 1;
	m_iRealKeyStepping	= 1;
	m_bEditEnable		= false;

	m_bShiftPressed		= false;
	m_bControlPressed	= false;
	m_bChangeAllPattern	= false;
	m_iPasteMode		= PASTE_MODE_NORMAL;

	m_bMaskInstrument	= false;
	m_bSwitchToInstrument = false;

	m_iCursorChannel	= 0;
	m_iCursorRow		= 0;
	m_iCursorColumn		= 0;
	m_iStartChan		= 0;

	m_iSelectStart		= -1;
	m_iSelectEnd		= 0;
	m_iSelectChannel	= 0;
	m_iSelectColStart	= 0;
	m_iSelectColEnd		= 0;

	m_iOctave			= 3;
	m_iInstrument		= 0;

	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;

	m_iKeyboardNote		= -1;
	m_iSoloChannel		= -1;

	m_iTempo			= 150;
	m_iSpeed			= 6;
	m_iTickPeriod		= 6;

	m_iPlayTime			= 0;

	m_pBackDC			= NULL;
	m_bUpdateBackground = true;

	memset(m_iActiveNotes, 0, sizeof(int) * MAX_CHANNELS);

	for (i = 0; i < MAX_CHANNELS; i++) {
		m_bMuteChannels[i] = false;

		CurrentNotes[i].EffNumber[0] = 0;
		CurrentNotes[i].EffParam[0] = 0;
		CurrentNotes[i].Instrument	= 0;
		CurrentNotes[i].Octave		= 0;
		CurrentNotes[i].Note		= 0;
		CurrentNotes[i].Vol			= 0;
	}

	for (i = 0; i < 256; i++)
		m_cKeyList[i] = 0;

	memset(Arpeggiate, 0, sizeof(int) * MAX_CHANNELS);
}

CFamiTrackerView::~CFamiTrackerView()
{
}

BOOL CFamiTrackerView::PreCreateWindow(CREATESTRUCT& cs)
{
	m_iClipBoard = RegisterClipboardFormat(CLIPBOARD_ID);

	if (m_iClipBoard == 0)
		theApp.DisplayError(IDS_CLIPBOARD_ERROR);

	return CView::PreCreateWindow(cs);
}


// CFamiTrackerView diagnostics

#ifdef _DEBUG
void CFamiTrackerView::AssertValid() const
{
	CView::AssertValid();
}

void CFamiTrackerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFamiTrackerDoc* CFamiTrackerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)));
	return (CFamiTrackerDoc*)m_pDocument;
}
#endif //_DEBUG

// CFamiTrackerView message handlers

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker drawing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PrepareBackground(CDC *dc)
{	
	const unsigned int EFF_COL_WIDTH	= ((CHAR_WIDTH * 3) + COLUMN_SPACING);
	const unsigned int GRAY				= 0x606060;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int ColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText			= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection		= theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor			= theApp.m_pSettings->Appearance.iColCursor;

	const unsigned int CHANNEL_LINE_COLOR	= 0x806060;
	const unsigned int CHANNEL_LINE_TOP		= TOP_OFFSET;

	unsigned int BorderColor;

	CBrush Brush, ArrowBrush(0x80B0B0), *OldBrush;
	CPen Pen, ArrowPen(PS_SOLID, 1, 0x606060), *OldPen;

	CPoint ArrowPoints[3];

	unsigned int AvailableChannels = pDoc->GetAvailableChannels();
	unsigned int CurrentColumn = 0, ColumnCount = 0;
	unsigned int i, Offset, TotalWidth = 0, HiddenChannels = 0, VisibleWidth = 0;

	// Determine weither the view needs to be scrolled horizontally
	Offset = CHANNEL_START + CHANNEL_SPACING;

	m_iChannelsVisible = AvailableChannels;

	// Don't begin after the cursor
	if (m_iStartChan > m_iCursorChannel)
		m_iStartChan = m_iCursorChannel;

	// Calculate channel widths
	for (i = 0; i < AvailableChannels; i++) {		
		m_iChannelWidths[i] = CHANNEL_WIDTH + pDoc->GetEffColumns(i) * EFF_COL_WIDTH + CHANNEL_SPACING * 2;
		m_iChannelColumns[i] = COLUMNS + pDoc->GetEffColumns(i) * 3;

		if (m_iCursorChannel == i)
			CurrentColumn = ColumnCount + m_iCursorColumn;

		ColumnCount += COLUMNS + pDoc->GetEffColumns(i) * 3;
		TotalWidth += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	// Find out the first visible channel on screen
	for (i = m_iStartChan; i < AvailableChannels; i++) {
		if (m_iCursorChannel == i && (Offset + m_iChannelWidths[i]) > m_iWindowWidth) {
			m_iStartChan++;
			i = m_iStartChan;
			Offset = CHANNEL_START + CHANNEL_SPACING;
			continue;
		}
		Offset += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	for (i = m_iStartChan; i < AvailableChannels; i++) {
		if ((Offset + m_iChannelWidths[i]) <= m_iWindowWidth)
			VisibleWidth += m_iChannelWidths[i];
		Offset += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	// Create colors
	Brush.CreateSolidBrush(ColBackground);

	// Border color
	if (m_bEditEnable)
		BorderColor = COLOR_SCHEME.BORDER_EDIT_A;
	else
		BorderColor = 0x00F82060;//COLOR_SCHEME.BORDER_NORMAL_A;

	// Dark border
	if (!m_bHasFocus)
		BorderColor	= DIM(BorderColor, 40) + 0x404040;

	OldPen = dc->SelectObject(&Pen);
	OldBrush = dc->SelectObject(&Brush);

	// Begin drawing

	// Clear background
	dc->FillSolidRect(0, 0, m_iWindowWidth - 4, m_iWindowHeight - 4, ColBackground);
	dc->SetBkMode(TRANSPARENT);

	// Draw frame, changes color in editing mode
	for (i = 0; i < 4; i++) {
		unsigned int iCol = DIM(BorderColor, (100 - i * 10));
		dc->Draw3dRect(i, i, m_iWindowWidth - 4 - (i << 1), m_iWindowHeight - 4 - (i << 1), iCol, iCol);
	}

	// Draw the channel header background, faded
	for (i = 0; i < TOP_OFFSET && VisibleWidth > 0; i++) {
		int level = ((TOP_OFFSET - i) * 100) / TOP_OFFSET;
		dc->FillSolidRect(4, 4 + i, m_iWindowWidth - 12, 1, DIM_TO(0x404040, ColBackground, level));
	}

	if (ColHiBackground < 0x808080) {
		for (i = 0; i < TOP_OFFSET - 4 && VisibleWidth > 0; i++) {
			float sin_level = float((((TOP_OFFSET - i) * 60) + 10) / TOP_OFFSET) / 100.0f * 3.14f;
			int level = (int)abs(sinf(sin_level + 0.2f) * 50);
			int Color = DIM_TO((ColHiBackground | BorderColor), ColBackground, level);
			float Angle = (float(i) / float(TOP_OFFSET)) * 3.14f / 2 + 1.6f;
			int Edge = (int)(sinf(Angle) * (TOP_OFFSET - 25));
			dc->FillSolidRect(CHANNEL_START - Edge, 4 + i, TotalWidth - 4 + Edge * 2, 1, Color);
		}
	}

	dc->SelectObject(ArrowPen);
	dc->SelectObject(ArrowBrush);

	// Print channel headers
	for (i = m_iStartChan; i < AvailableChannels; i++) {
		// Draw vertical line
		dc->FillSolidRect(Offset - CHANNEL_SPACING - 3, /*4*/CHANNEL_LINE_TOP, 1, m_iWindowHeight - 10, CHANNEL_LINE_COLOR);

		// If channel will go outside the window, break
		if (Offset + m_iChannelWidths[i] > m_iWindowWidth) {
			m_iChannelsVisible = i;
			break;
		}

		// Display the fx columns with some flashy text
		dc->SetTextColor(DIM(ColHiText, 60));

		if (pDoc->GetEffColumns(i) > 0)
			dc->TextOut(Offset + CHANNEL_WIDTH + 8, TEXT_POSITION + 12, "fx2");
		if (pDoc->GetEffColumns(i) > 1)
			dc->TextOut(Offset + CHANNEL_WIDTH + 48, TEXT_POSITION + 12, "fx3");
		if (pDoc->GetEffColumns(i) > 2)
			dc->TextOut(Offset + CHANNEL_WIDTH + 88, TEXT_POSITION + 12, "fx4");

		// Arrows for expanding/removing fx columns
		if (pDoc->GetEffColumns(i) > 0) {			
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 17, 12);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 17, 12 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 17 - 5, 12 + 5);
			dc->Polygon(ArrowPoints, 3);
		}

		if (pDoc->GetEffColumns(i) < (MAX_EFFECT_COLUMNS - 1)) {
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 11, 12);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 11, 12 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 11 + 5, 12 + 5);
			dc->Polygon(ArrowPoints, 3);
		}

		Offset += m_iChannelWidths[i];
	}

	// Draw the last vertical channel line
	dc->FillSolidRect(Offset - CHANNEL_SPACING - 3, CHANNEL_LINE_TOP/*3*/, 1, m_iWindowHeight - 10, CHANNEL_LINE_COLOR);

	// Display if there's channels outside the screen
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);

	// Before
	if (m_iStartChan != 0)
		dc->TextOut(10, TEXT_POSITION, "<<", 2);

	// After
	if (m_iChannelsVisible != AvailableChannels && (m_iWindowWidth - Offset) > 24)
		dc->TextOut(Offset, TEXT_POSITION, ">>", 2);

	Offset -= 15;

	if (Offset > (m_iWindowWidth - 2))
		Offset = (m_iWindowWidth - 2);

	// Draw the horizontal line between the channel names and tracker field
//	dc->FillSolidRect(CHANNEL_START - 2, TOP_OFFSET - 2, Offset - CHANNEL_START + 4, 1, 0x808080);

	// Delete objects
	dc->SelectObject(OldPen);
	dc->SelectObject(OldBrush);

	// Update horizontal scrollbars
	SetScrollRange(SB_HORZ, 0, ColumnCount - 1);
	SetScrollPos(SB_HORZ, CurrentColumn);
}

void CFamiTrackerView::CreateBackground()
{
	m_bUpdateBackground = true;
	Invalidate();
}

void CFamiTrackerView::CacheBackground(CDC *pDC)
{
	// Saves a DC
	if (m_pBackDC)
		m_pBackDC->DeleteDC();

	m_pBackDC = new CDC;

	m_bmpCache.DeleteObject();

	m_bmpCache.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
	m_pBackDC->CreateCompatibleDC(pDC);
	m_pOldCacheBmp = m_pBackDC->SelectObject(&m_bmpCache);

	m_pOldCacheBmp->DeleteObject();

//	m_pBackDC->CreateCompatibleDC(pDC);
	m_pBackDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, pDC, 0, 0, SRCCOPY);
}

void CFamiTrackerView::RestoreBackground(CDC *pDC)
{
	// Copies the background to screen
	pDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, m_pBackDC, 0, 0, SRCCOPY);
}

void CFamiTrackerView::DrawPatternRow(CDC *pDC, unsigned int Row, unsigned int Line, unsigned int Frame, bool Preview)
{
	// Draw a single pattern row

	const int PREVIEW_FADE_LEVEL = 60;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int ColBackground	 = theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground = theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText		 = theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText		 = theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection	 = theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor		 = theApp.m_pSettings->Appearance.iColCursor;
	unsigned int SelectStart	 = GetSelectStart();
	unsigned int SelectEnd		 = GetSelectEnd();
	unsigned int SelectColStart	 = GetSelectColStart();
	unsigned int SelectColEnd	 = GetSelectColEnd();

	unsigned int CurrentBgCol, CurrentTextCol;
	unsigned int RowsInField, Width;
	unsigned int Offset, ColOffset;
	unsigned int x, c;

	int CursorDifference = signed(m_iCursorRow - m_iCurrentRow);

	unsigned int RowCursor = m_iCursorRow;
	unsigned int MiddleRow = m_iCurrentRow;

	bool bHighlight;

	stChanNote	Note;
	CString		RowText;

	if (!theApp.m_pSettings->General.bFreeCursorEdit)
		RowCursor = MiddleRow;

	RowsInField = m_iActualLengths[Frame];

	if ((Row & 3) == 0) {
		// Highlight
		CurrentTextCol = ColHiText;
		CurrentBgCol = ColHiBackground;
		bHighlight = true;

		pDC->FillSolidRect(4, TOP_OFFSET + (ROW_HEIGHT * Line), CHANNEL_START - 7, TEXT_HEIGHT - 3, CurrentBgCol);
	}
	else {
		CurrentTextCol = ColText;
		CurrentBgCol = ColBackground;
		bHighlight = false;
	}

	if (Preview) {
		CurrentTextCol = DIM_TO(CurrentTextCol, CurrentBgCol, PREVIEW_FADE_LEVEL);
	}

	pDC->SetTextColor(CurrentTextCol);

	if (theApp.m_pSettings->General.bRowInHex) {
		RowText.Format(_T("%02X"), Row);
		pDC->TextOut(LEFT_OFFSET, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
	}
	else {
		RowText.Format(_T("%03i"), Row);
		pDC->TextOut(LEFT_OFFSET - 3, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	for (x = m_iStartChan; x < m_iChannelsVisible; x++) {
		ColOffset = 0;
		// These are only on currently active frame
		if (!Preview) {
			// Highlighted row, draw bg
			if (!(Row & 3) &&  /*x < 5 &&*/ Row != RowCursor) {
				Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
				pDC->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, TEXT_HEIGHT - 3, CurrentBgCol);
			}
			else if (Row == RowCursor) {
				Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
				pDC->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, TEXT_HEIGHT - 3, DIM_TO(ColCursor, CurrentBgCol, 20));
			}
			// Selection
			if (m_iSelectStart != -1 && x == m_iSelectChannel && Row >= SelectStart && Row <= SelectEnd) {
				int SelStart	= TOP_OFFSET + ROW_HEIGHT * Line ;
				int SelChan		= Offset + COLUMN_SEL[SelectColStart] - 4;
				int SelChanEnd	= Offset + COLUMN_SEL[SelectColEnd + 1] - SelChan - 4;
				pDC->FillSolidRect(SelChan, SelStart, SelChanEnd, ROW_HEIGHT, ColSelection);
			}
		}

		// Draw all columns
		for (c = 0; c < m_iChannelColumns[x]; c++) {
			// Cursor box
			if (Row == RowCursor && x == m_iCursorChannel && c == m_iCursorColumn && !Preview) {
				unsigned int BoxTop	= TOP_OFFSET + ROW_HEIGHT * Line;
				unsigned int SelChan = Offset + ColOffset - 2;

				pDC->FillSolidRect(SelChan, BoxTop, CURSOR_SPACE[m_iCursorColumn], TEXT_HEIGHT - 3, DIM(ColCursor, 80));
				pDC->FillSolidRect(SelChan + 1, BoxTop + 1, CURSOR_SPACE[m_iCursorColumn] - 2, TEXT_HEIGHT - 5, ColCursor);
			}

			pDoc->GetNoteData(Frame, x, Row, &Note);
			DrawLine(Offset + ColOffset, &Note, Line, c, x, CurrentTextCol, bHighlight, Preview, pDC);
			ColOffset += COLUMN_SPACE[c];
		}
		Offset += m_iChannelWidths[x];
	}
	Row++;
}

void CFamiTrackerView::DrawPatternField(CDC* dc)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Row, PatternRow, RowsInField;
	unsigned int i;

	ScanActualLengths();

	// Make sure cursor isn't out of frame
	RowsInField = m_iActualLengths[m_iCurrentFrame];		// Number of rows in current frame

	if (m_iCursorRow > (RowsInField - 1))
		m_iCursorRow = RowsInField - 1;

	int CursorDifference = signed(m_iCursorRow - m_iCurrentRow);

	dc->SetBkMode(TRANSPARENT);

	// Adjust if cursor is locked or the tracker is playing
	if (!theApp.m_pSettings->General.bFreeCursorEdit) {
		m_iCurrentRow = m_iCursorRow;
	}
	else {
		// Adjust if cursor is out of screen (if free cursor)
		while (CursorDifference >= signed(m_iVisibleRows / 2) && CursorDifference > 0) {
			// Change these if you want one whole page to scroll instead of single lines
			m_iCurrentRow += /*m_iVisibleRows */ 1;
			CursorDifference = signed(m_iCursorRow - m_iCurrentRow);
		}

		while (-CursorDifference > signed(m_iVisibleRows / 2) && CursorDifference < 0) {
			m_iCurrentRow -= /*m_iVisibleRows */ 1;
			CursorDifference = signed(m_iCursorRow - m_iCurrentRow);
		}
	}

	unsigned int RowCursor = m_iCursorRow;
	unsigned int MiddleRow = m_iCurrentRow;

	// Choose start row
	if (MiddleRow > (m_iVisibleRows / 2))
		Row = MiddleRow - (m_iVisibleRows / 2);
	else 
		Row = 0;

	for (i = 0; i < m_iVisibleRows; i++) {
		if ((MiddleRow - (m_iVisibleRows / 2) + i) >= 0 && (MiddleRow - (m_iVisibleRows / 2) + i) < RowsInField) {
			// Current
			PatternRow = int(MiddleRow - (m_iVisibleRows / 2) + i);
			DrawPatternRow(dc, PatternRow, i, m_iCurrentFrame, false);
			Row++;
		}
		else if ((m_iCurrentFrame > 0) && int(MiddleRow - (m_iVisibleRows / 2) + i) < 0 && int(MiddleRow - (m_iVisibleRows / 2) + i) >= -int(m_iActualLengths[m_iCurrentFrame - 1]) && theApp.m_pSettings->General.bFramePreview) {
			// Previous
			PatternRow = int(m_iActualLengths[m_iCurrentFrame - 1]) + int(MiddleRow - (m_iVisibleRows / 2) + i);
			DrawPatternRow(dc, PatternRow, i, m_iCurrentFrame - 1, true);
			Row++;
		}
		else if ((m_iCurrentFrame < (pDoc->GetFrameCount() - 1)) && int(MiddleRow - (m_iVisibleRows / 2) + i) >= int(RowsInField) && int(MiddleRow - (m_iVisibleRows / 2) + i) < int(RowsInField + m_iActualLengths[m_iCurrentFrame + 1]) && theApp.m_pSettings->General.bFramePreview) {
			// Next
			PatternRow = int(MiddleRow - (m_iVisibleRows / 2) + i) - RowsInField;
			DrawPatternRow(dc, PatternRow, i, m_iCurrentFrame + 1, true);
			Row++;
		}
	}

	/*
	unsigned int CursorCol1 = DIM(ColCursor, 100);
	unsigned int CursorCol2 = DIM(ColCursor, 80);
	unsigned int CursorCol3 = DIM(ColCursor, 60);

	// Selected row
	if (!theApp.m_pSettings->General.bFreeCursorEdit) {
		dc->Draw3dRect(3, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT - 2, m_iWindowWidth - 10, ROW_HEIGHT + 2, CursorCol3, CursorCol3);
		dc->Draw3dRect(4, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT - 1, m_iWindowWidth - 12, ROW_HEIGHT, CursorCol2, CursorCol2);
		dc->Draw3dRect(5, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT, m_iWindowWidth - 14, ROW_HEIGHT - 2, CursorCol1, CursorCol1);
	}
	*/

	// Update vertical scrollbar
	if (RowsInField > 1)
		SetScrollRange(SB_VERT, 0, RowsInField - 1);
	else
		SetScrollRange(SB_VERT, 0, 1);

	SetScrollPos(SB_VERT, m_iCurrentRow);

	if (bInsertedEffect) {
		/*
		const char *Text = EFFECT_TEXTS[iLastEffect];
		int x = CHANNEL_START + m_iCursorChannel * CHANNEL_WIDTH + COLUMN_SEL[m_iCursorColumn];
		int y = TOP_OFFSET + ((m_iVisibleRows / 2) + 1) * ROW_HEIGHT + 5;
		int Height = dc->GetTextExtent(Text).cy + 2;
		int Width = dc->GetTextExtent(Text).cx + 3;
		dc->FillRect(CRect(x - 1, y - 1, x + 1 + Width, y + 1 + Height), &CBrush(0x808080));
		dc->FillRect(CRect(x, y, x + Width, y + Height), &CBrush(0x303030));
		dc->SetTextColor(0xFFFFFF);
		dc->TextOut(x + 1, y + 1, Text);
		*/
	}
/*
	dc->SetBkColor(COLOR_SCHEME.CURSOR);
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dc->TextOut(m_iWindowWidth - 47, m_iWindowHeight - 22, _T("BETA"));
*/
#ifdef WIP
	CString Wip;
	Wip.Format("WIP %i", VERSION_WIP);
	dc->SetBkColor(COLOR_SCHEME.CURSOR);
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dc->TextOut(m_iWindowWidth - 47, m_iWindowHeight - 22, Wip);
#endif
#ifdef _DEBUG
	dc->SetBkColor(COLOR_SCHEME.CURSOR);
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dc->TextOut(m_iWindowWidth - 47, 42, _T("DEBUG"));
	dc->TextOut(m_iWindowWidth - 47, 62, _T("DEBUG"));
	dc->TextOut(m_iWindowWidth - 47, 82, _T("DEBUG"));
#endif
}

void CFamiTrackerView::OnDraw(CDC* pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	LOGFONT LogFont;
	CBitmap Bmp, *OldBmp;
	CFont	Font, *OldFont;
	CDC		*BackDC;

	LPCSTR	FontName = theApp.m_pSettings->General.strFont;

	// Check document
	if (!pDoc->IsFileLoaded()) {
		pDC->FillSolidRect(0, 0, m_iWindowWidth, m_iWindowHeight, 0x000000);
		pDC->SetTextColor(0xFFFFFF);
		pDC->TextOut(m_iWindowWidth / 2 - 84, m_iWindowHeight / 2 - 4, _T("No document is loaded"));
		return;
	}

	// Set up the drawing area
	BackDC = new CDC;

	// Create the font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	// Setup dc
	Bmp.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
	BackDC->CreateCompatibleDC(pDC);
	OldBmp = BackDC->SelectObject(&Bmp);
	OldFont = BackDC->SelectObject(&Font);

	// Create the background
	if (m_bUpdateBackground) {
		PrepareBackground(BackDC);
		m_bUpdateBackground = false;
	}

	RestoreBackground(BackDC);

	// Draw the field
	DrawPatternField(BackDC);

	DrawMeters(BackDC);
	DrawChannelHeaders(BackDC);

	// Display area and clean up
	pDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, BackDC, 0, 0, SRCCOPY);

	BackDC->SelectObject(OldBmp);
	BackDC->SelectObject(OldFont);

	delete BackDC;
}

void CFamiTrackerView::DrawLine(int Offset, stChanNote *NoteData, int Line, int Column, int Channel, int Color, bool bHighlight, bool bShaded, CDC *dc)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	int LineY		= TOP_OFFSET + Line * ROW_HEIGHT;
	int OffsetX		= Offset;

	int Vol			= NoteData->Vol;
	int Note		= NoteData->Note;
	int Octave		= NoteData->Octave;
	int Instrument	= NoteData->Instrument;

	if (Note < 0 || Note > HALT)
		return;

	int EffNumber, EffParam;

	switch (Column) {
		case 4: case 5: case 6:
			EffNumber	= NoteData->EffNumber[0];
			EffParam	= NoteData->EffParam[0];
			break;
		case 7: case 8: case 9:
			EffNumber	= NoteData->EffNumber[1];
			EffParam	= NoteData->EffParam[1];
			break;
		case 10: case 11: case 12:
			EffNumber	= NoteData->EffNumber[2];
			EffParam	= NoteData->EffParam[2];
			break;
		case 13: case 14: case 15:
			EffNumber	= NoteData->EffNumber[3];
			EffParam	= NoteData->EffParam[3];
			break;
	}

	unsigned int InstCol = theApp.m_pSettings->Appearance.iColPatternInstrument;
	unsigned int VolumeCol = theApp.m_pSettings->Appearance.iColPatternVolume;
	unsigned int EffCol = theApp.m_pSettings->Appearance.iColPatternEffect;

	if (!theApp.m_pSettings->General.bPatternColor) {
		InstCol = VolumeCol = EffCol = Color;
	}

	unsigned int ShadedCol = DIM(Color, 50);

	if (bHighlight) {
		/*
		VolumeCol	= 0xFFa020;
		EffCol		= 0x2080FF;
		InstCol		= 0x20FF80;
		*/
	}

	if (bShaded) {
		VolumeCol	= DIM(VolumeCol, 50);
		EffCol		= DIM(EffCol, 50);
		InstCol		= DIM(InstCol, 50);
	}

	switch (Column) {
		case 0:
			// Note and octave
			switch (Note) {
				case 0:
				case HALT:
					if (Note == 0) {
						DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '-', ShadedCol, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '-', ShadedCol, dc);
					}
					else 
					{
						DrawChar(OffsetX, LineY, '*', Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '*', Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '*', Color, dc);
					}
					break;
				default:
					if (Channel == 3) {
						char NoiseFreq = (Note - 1 + Octave * 12) & 0x0F;
						DrawChar(OffsetX, LineY, HEX[NoiseFreq], Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '-', Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '#', Color, dc);
					}
					else {
						DrawChar(OffsetX, LineY, NOTES_A[Note - 1], Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, NOTES_B[Note - 1], Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, NOTES_C[Octave], Color, dc);
					}
			}
			break;
		case 1:
			// Instrument x0
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument >> 4], InstCol, dc);
			break;
		case 2:
			// Instrument 0x
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument & 0x0F], InstCol, dc);
			break;
		case 3:
			// Volume (skip triangle)
			if (Vol == 0x10 || Channel == 2 || Channel == 4)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Vol & 0x0F], VolumeCol, dc);
			break;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, EFF_CHAR[EffNumber - 1], EffCol, dc);
			break;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[(EffParam >> 4) & 0x0F], Color, dc);
			break;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[EffParam & 0x0F], Color, dc);
			break;
	}

}

void CFamiTrackerView::DrawChar(int x, int y, char c, int Color, CDC *dc) 
{
	CString Text;
	Text.Format(_T("%c"), c);
	dc->SetTextColor(Color);
	dc->TextOut(x, y, Text);
}

void CFamiTrackerView::DrawChannelHeaders(CDC *pDC)
{
	// Only draw the channel names
	//
	
	const char *CHANNEL_NAMES[]		 = {"Square 1", "Square 2", "Triangle", "Noise", "DPCM"};
	const char *CHANNEL_NAMES_VRC6[] = {"Square 1", "Square 2", "Sawtooth"};

	CFamiTrackerDoc* pDoc = GetDocument();

	unsigned int Offset = CHANNEL_START + CHANNEL_SPACING;
	unsigned int AvailableChannels = pDoc->GetAvailableChannels();
	unsigned int CurTopColor, i;

	LOGFONT LogFont;
	CFont	Font, *OldFont;
	LPCSTR	FontName = theApp.m_pSettings->General.strFont;

	pDC->SetBkMode(TRANSPARENT);

	// Create the font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	OldFont = pDC->SelectObject(&Font);

	for (i = m_iStartChan; i < AvailableChannels; i++) {

		// If channel will go outside the window, break
		if (Offset + m_iChannelWidths[i] > m_iWindowWidth) {
			m_iChannelsVisible = i;
			break;
		}

		CurTopColor = m_bMuteChannels[i] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE;

		if (m_bMuteChannels[i]) {
			if (FadeCol[i] < 100)
				FadeCol[i] += 20;
			if (FadeCol[i] > 100)
				FadeCol[i] = 100;
		}
		else {
			if (FadeCol[i] > 0)
				FadeCol[i] -= 20;
			if (FadeCol[i] < 0)
				FadeCol[i] = 0;
		}

		CurTopColor = DIM_TO(COLOR_SCHEME.TEXT_MUTED, COLOR_SCHEME.TEXT_HILITE, FadeCol[i]);
		
		if (i > 4) {	// expansion names
			switch (pDoc->GetExpansionChip()) {
				case CHIP_VRC6: 
					pDC->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES_VRC6[i - 5]);
					break;
			}
		}
		else { // internal names
			pDC->SetTextColor(DIM(CurTopColor, 50));
			pDC->TextOut(Offset + 1, TEXT_POSITION + 1, CHANNEL_NAMES[i]);

			pDC->SetTextColor(CurTopColor);
			pDC->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES[i]);
		}

		Offset += m_iChannelWidths[i];
	}

	pDC->SelectObject(OldFont);
}

void CFamiTrackerView::DrawMeters(CDC *pDC)
{
	const unsigned int COL_DARK = 0x485848;
	const unsigned int COL_LIGHT = 0x10E020;
	
	const unsigned int COL_DARK_SHADOW = DIM(COL_DARK, 70);
	const unsigned int COL_LIGHT_SHADOW = DIM(COL_LIGHT, 60);

	const int BAR_TOP	 = TEXT_POSITION + 21;
	const int BAR_LEFT	 = CHANNEL_START + CHANNEL_SPACING - 2;
	const int BAR_SIZE	 = 8;
	const int BAR_SPACE	 = 1;
	const int BAR_HEIGHT = 5;

	static unsigned int colors[15];
	static unsigned int colors_dark[15];
	static unsigned int colors_shadow[15];
	static unsigned int colors_dark_shadow[15];

	unsigned int Offset = BAR_LEFT;
	unsigned int i, c;

	CPoint	points[5];

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (colors[0] == 0) {
		for (i = 0; i < 15; i++) {
			colors[i] = DIM_TO(COL_LIGHT, 0x00F0F0, (100 - (i * i) / 3));
			colors_dark[i] = DIM_TO(COL_DARK, 0x00F0F0, (100 - (i * i) / 3));
			colors_shadow[i] = DIM(colors[i], 60);
			colors_dark_shadow[i] = DIM(colors_dark[i], 70);
		}
	}

	if (!pDoc->IsFileLoaded())
		return;

	for (c = m_iStartChan; c < m_iChannelsVisible; c++) {
		for (i = 0; i < 15; i++) {
			if (i < m_iVolLevels[c]) {
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + BAR_SIZE - 1, BAR_TOP + 1, 1, BAR_HEIGHT, colors_shadow[i]);
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, colors_shadow[i]);
				pDC->FillSolidRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), colors[i]);
			}
			else {
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + BAR_SIZE - 1, BAR_TOP + 1, BAR_SPACE, BAR_HEIGHT, COL_DARK_SHADOW);
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, COL_DARK_SHADOW);
				pDC->FillSolidRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), COL_DARK);
			}
		}
		Offset += CHANNEL_WIDTH + pDoc->GetEffColumns(c) * (CHAR_WIDTH * 3 + COLUMN_SPACING) + CHANNEL_SPACING * 2;
	}

	CString Text;
//	Text.Format("DPCM thing %i", theApp.GetDPCM());
//	pDC->TextOut(Offset + 50, 100, Text);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker playing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CFamiTrackerView::PlayerCommand(char Command, int Value)
{
	// Handle commands from the player

	CFamiTrackerDoc* pDoc = GetDocument();
	stChanNote NoteData;

	switch (Command) {
		case CMD_MOVE_TO_TOP:
			m_iCursorRow = 0;
			m_bForceRedraw = true;
			break;

		case CMD_STEP_DOWN:
			m_iCursorRow++;
			if (m_iCursorRow >= pDoc->GetPatternLength()) {
				m_iCursorRow = 0;
				if (Value == 0) {
					m_iCurrentFrame++;
					if (m_iCurrentFrame < 0) {
						m_iCurrentFrame += pDoc->GetFrameCount();
					}
					else if (m_iCurrentFrame > pDoc->GetFrameCount() - 1) {
						m_iCurrentFrame -= pDoc->GetFrameCount();
					}
				}
			}
			m_bForceRedraw = true;
			break;

		case CMD_JUMP_TO:
			m_iCurrentFrame = Value + 1;
			m_iCursorRow = 0;
			if (m_iCurrentFrame > signed(pDoc->GetFrameCount() - 1))
				m_iCurrentFrame = 0;
			m_bForceRedraw = true;
			break;

		case CMD_SKIP_TO:
			m_iCurrentFrame++;
			if (m_iCurrentFrame < 0) {
				m_iCurrentFrame += pDoc->GetFrameCount();
			}
			else if (m_iCurrentFrame > signed(pDoc->GetFrameCount() - 1)) {
				m_iCurrentFrame -= pDoc->GetFrameCount();
			}
			m_iCursorRow = Value;
			m_bForceRedraw = true;
			break;

		case CMD_UPDATE_ROW:
			for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
				pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &NoteData);
				if (!m_bMuteChannels[i]) {
					if (NoteData.Instrument < 0x40 && NoteData.Note > 0 && i == m_iCursorChannel && m_bSwitchToInstrument) {
						m_iInstrument = NoteData.Instrument;
					}
					FeedNote(i, &NoteData);
				}
				else {
					// These effects will pass even if the channel is muted
					const int PASS_EFFECTS[] = {EF_HALT, EF_JUMP, EF_SPEED, EF_SKIP};
					bool ValidCommand = false;
					unsigned int j, k;
					NoteData.Note		= 0;
					NoteData.Octave		= 0;
					NoteData.Instrument = 0;
					for (j = 0; j < (pDoc->GetEffColumns(i) + 1); j++) {
						for (k = 0; k < 4; k++) {
							if (NoteData.EffNumber[j] == PASS_EFFECTS[k])
								ValidCommand = true;
						}
						if (!ValidCommand)
							NoteData.EffNumber[j] = EF_NONE;
					}
					if (ValidCommand)
						FeedNote(i, &NoteData);
				}
			}
			break;

		case CMD_TIME:
			m_iPlayTime = Value;
			break;

		case CMD_TICK:
			if (KeyCount == 1 || !theApp.m_pSettings->Midi.bMidiArpeggio)
				return 0;

			// auto arpeggio
			int OldPtr = ArpeggioPtr;
			do {
				ArpeggioPtr = (ArpeggioPtr + 1) & 127;				
				if (ArpeggioNotes[ArpeggioPtr] == 1) {
					LastArpPtr = ArpeggioPtr;
					Arpeggiate[m_iCursorChannel] = ArpeggioPtr;
					break;
				}
 				else if (ArpeggioNotes[ArpeggioPtr] == 2) {
					ArpeggioNotes[ArpeggioPtr] = 0;
				}
			}
			while (ArpeggioPtr != OldPtr);

			break;
	}

	return 0;
}

void CFamiTrackerView::GetRow(CFamiTrackerDoc *pDaoc)
{
	stChanNote NoteData;
	CFamiTrackerDoc* pDoc = GetDocument();

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		if (!m_bMuteChannels[i]) {
			pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &NoteData);
			FeedNote(i, &NoteData);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	CDC TempDC;
	CBitmap Bmp;
	CFamiTrackerDoc* pDoc = GetDocument();

	if (!pDoc->IsFileLoaded())
		return FALSE;

	if (m_bUpdateBackground) {
		Bmp.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
		TempDC.CreateCompatibleDC(pDC);
		TempDC.SelectObject(&Bmp)->DeleteObject();

		PrepareBackground(&TempDC);
		CacheBackground(&TempDC);

		TempDC.DeleteDC();

		m_bUpdateBackground = false;
	}

	return FALSE;
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	m_iWindowWidth	= (lpClientRect->right - lpClientRect->left) - 17;
	m_iWindowHeight	= (lpClientRect->bottom - lpClientRect->top) - 17;
	m_iVisibleRows	= (((m_iWindowHeight - 8) + (ROW_HEIGHT / 2)) / ROW_HEIGHT) - 3;

	CreateBackground();

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

BOOL CFamiTrackerView::PreTranslateMessage(MSG* pMsg)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (pMsg->message) {
		case WM_USER + 1:
			if (m_bInitialized)
				TranslateMidiMessage();
			break;
	}

	return CView::PreTranslateMessage(pMsg);
}

void CFamiTrackerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	CRect WinRect;
	CMenu *PopupMenu, PopupMenuBar;
	int Item = 0;

	GetWindowRect(WinRect);
	PopupMenuBar.LoadMenu(IDR_POPUP);

	PopupMenu = PopupMenuBar.GetSubMenu(0);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, this);

	CView::OnRButtonUp(nFlags, point);
}

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nSBCode) {
		case SB_LINEDOWN:	MoveCursorDown();		break;
		case SB_LINEUP:		MoveCursorUp();			break;
		case SB_PAGEDOWN:	MoveCursorPageDown();	break;
		case SB_PAGEUP:		MoveCursorPageUp();		break;
		case SB_TOP:		MoveCursorToTop();		break;
		case SB_BOTTOM:		MoveCursorToBottom();	break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iCursorRow = nPos;
			ForceRedraw();
			break;
		case SB_ENDSCROLL:
			return;
	}

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x, count = 0;

	switch (nSBCode) {
		case SB_LINERIGHT:	MoveCursorRight();			break;
		case SB_LINELEFT:	MoveCursorLeft();			break;
		case SB_PAGERIGHT:	MoveCursorNextChannel();	break;
		case SB_PAGELEFT:	MoveCursorPrevChannel();	break;
		case SB_RIGHT:		MoveCursorFirstChannel();	break;
		case SB_LEFT:		MoveCursorLastChannel();	break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
				for (x = 0; x < signed(COLUMNS + pDoc->GetEffColumns(i) * 3); x++) {
					if (count++ == nPos) {
						m_iCursorColumn = x;
						m_iCursorChannel = i;
						ForceRedraw();
						CView::OnHScroll(nSBCode, nPos, pScrollBar);
						return;
					}
				}
			}
			break;
		case SB_ENDSCROLL:
			return;
	}

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::CheckSelectionBeforeMove()
{
	if (m_bShiftPressed) {
		// If there was no selection
		if (m_iSelectStart == -1) {
			m_iSelectStart = m_iCursorRow;
			m_iSelectColStart = m_iCursorColumn;
		}
	}
}

void CFamiTrackerView::CheckSelectionAfterMove()
{
	if (m_bShiftPressed) {
		m_iSelectEnd = m_iCursorRow;
		m_iSelectColEnd	= m_iCursorColumn;
		m_iSelectChannel = m_iCursorChannel;
	}
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int ScrollLength;

	if (zDelta != 0) {
		if (zDelta < 0)
			ScrollLength = theApp.m_pSettings->General.iPageStepSize;
		else
			ScrollLength = -theApp.m_pSettings->General.iPageStepSize;
		
		if (theApp.m_pSettings->General.bFreeCursorEdit) {
			int NewRow = m_iCurrentRow + ScrollLength;			
			if (NewRow >= 0 && NewRow <= (signed)GetCurrentPatternLength()) {
				m_iCurrentRow = NewRow;

				if (abs((int)(m_iCurrentRow - m_iCursorRow)) > (signed)(m_iVisibleRows / 2)) {
					m_iCursorRow += ScrollLength;
				}
			}
		}
		else {
			m_iCursorRow += ScrollLength;

			if ((m_iCursorRow + ScrollLength) >= GetCurrentPatternLength()) {	// it's unsigned so this should be ok
				if (theApp.m_pSettings->General.bWrapCursor)
					m_iCursorRow %= GetCurrentPatternLength();
				else
					m_iCursorRow = 0;
			}
		}
	}
	ForceRedraw();
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

unsigned int CFamiTrackerView::GetChannelAtPoint(unsigned int PointX)
{
	unsigned int i, ChanSize, Offset = 0, Channel = -1;

	for (i = m_iStartChan; i < m_iChannelsVisible; i++) {
		ChanSize = m_iChannelWidths[i];
		Offset += ChanSize;
		if ((PointX - CHANNEL_START) < Offset) {
			Channel = i;
			break;
		}
	}

	return Channel;
}

unsigned int CFamiTrackerView::GetColumnAtPoint(unsigned int PointX, unsigned int MaxColumns)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, Offset;
	int ColumnOffset = -1, Column = -1;
	int ChanSize, Channel = -1;

	Offset = CHANNEL_START + CHANNEL_SPACING;

	if (PointX < Offset)
		return -1;

	for (i = m_iStartChan; i < pDoc->GetAvailableChannels(); i++) {
		
		ChanSize = CHANNEL_WIDTH + (pDoc->GetEffColumns(i) * (CHAR_WIDTH * 3 + COLUMN_SPACING));
		Offset += ChanSize;
		if (PointX < Offset) {
			ColumnOffset = ChanSize - (Offset - PointX);
			Channel = i;
			break;
		}
		Offset += CHANNEL_SPACING * 2;
	}

	if (ColumnOffset < 0)
		return -1;

	if (Channel == -1)
		return -1;

	Offset = 0;

	for (i = 0; i < MaxColumns; i++) {
		Offset += COLUMN_SPACE[i];
		if (ColumnOffset < signed(Offset)) {
			Column = i;
			break;
		}
	}

	return Column;
}

bool HitTest(int rangeX, int rangeY, int rangeWidth, int rangeHeight, int pointX, int pointY)
{
	if (pointX >= rangeX && pointX <= (rangeX + rangeWidth) && pointY >= rangeY && pointY <= (rangeY + rangeWidth))
		return true;

	return false;
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel = 0, Column = 0;
	int	DeltaY, DeltaRow;

	IgnoreFirst = true;

	Channel = GetChannelAtPoint(point.x);

	if (Channel == -1)
		return;

	DeltaY = point.y - (m_iWindowHeight / 2);

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (Column == -1)
		return;
	
	m_iSelectColStart = Column;
	m_iSelectColEnd = Column;

	// The top row (channel name and meters)
	if (point.y < signed(ROW_HEIGHT * 2) && Channel < signed(pDoc->GetAvailableChannels())) {
		// Silence channel
		if (Column < 5) {
			ToggleChannel(Channel);
			return;
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (pDoc->GetEffColumns(Channel) > 0)
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) - 1);
			CreateBackground();
		}
		// Add one track effect column
		else if (Column == 6) {
			if (pDoc->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) + 1);
			CreateBackground();
		}
		ForceRedraw();
	}

	int Sel = signed(m_iCurrentRow) + DeltaRow;

	if (m_iSelectStart != -1) {
		if (m_bShiftPressed) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = -1;
			ForceRedraw();
		}
	}

	CView::OnLButtonDown(nFlags, point);
}

int CFamiTrackerView::GetRowAtPoint(unsigned int PointY)
{
	int DeltaRow, NewRow;

	if (PointY < TOP_OFFSET)
		return -1;

	DeltaRow = ((PointY - TOP_OFFSET) / ROW_HEIGHT) - (m_iVisibleRows / 2);
	NewRow = m_iCurrentRow + DeltaRow;

	if (NewRow < 0)
		return -1;

	return NewRow;
}

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column, Row;

	Row		= GetRowAtPoint(point.y);
	Channel = GetChannelAtPoint(point.x);

	// Skip if in the channel headers
	if (point.y < signed(ROW_HEIGHT * 2) && Channel < signed(pDoc->GetAvailableChannels()))
		return;

	if (Channel != -1)
		Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (m_iSelectStart == -1) {
		if (Row != -1)
			m_iCursorRow = Row;
		if (Channel != -1) {
			m_iCursorChannel = Channel;
			if (Column != -1)
				m_iCursorColumn	= Column;
		}
		ForceRedraw();
	}

	CView::OnLButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column;

	IgnoreFirst = true;

	Channel = GetChannelAtPoint(point.x);
	
	if (Channel == -1)
		return;

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (Column == -1)
		return;

	if (point.y < ROW_HEIGHT * 2 && Channel < signed(pDoc->GetAvailableChannels())) {
		// Mute/unmute
		if (Column < 5) {
			SoloChannel(Channel);
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (pDoc->GetEffColumns(Channel) > 0)
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) - 1);
		}
		// Add one track effect column
		else if (Column == 6) {
			if (pDoc->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) + 1);
		}
		ForceRedraw();
	}
	else
		SelectWholePattern(Channel);

	ForceRedraw();

	CView::OnLButtonDblClk(nFlags, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	CString String;
	int DeltaRow, DeltaY = point.y - (m_iWindowHeight / 2) - 20;
	int Channel, Column;

	static int LastX, LastY;

	if (!(nFlags & MK_LBUTTON))
		return;

	if (IgnoreFirst) {
		IgnoreFirst = false;
		LastX = point.x;
		LastY = point.y;
		return;
	}

	if (point.y < ROW_HEIGHT * 2)
		return;

	if ((LastX == point.x) && (LastY == point.y))
		return;

	if ((Channel = GetChannelAtPoint(point.x)) == -1)
		return;

	if ((Column = GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3)) == -1)
		return;

	if (Channel == m_iSelectChannel)
		m_iSelectColEnd = Column;

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);
	
	int Sel = m_iCurrentRow + DeltaRow;

	if (((DeltaRow + m_iCurrentRow) >= 0 && (DeltaRow + m_iCurrentRow) < signed(GetCurrentPatternLength())) && Channel >= 0) {
		
		if (m_iSelectStart != -1) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = Sel;
			m_iSelectChannel = Channel;
		}
		
		// Scroll if top or bottom is reached
		if (DeltaRow >= signed((m_iWindowHeight / ROW_HEIGHT) / 2) - 2)
			MoveCursorDown();
		else if (DeltaRow <= (-signed((m_iWindowHeight / ROW_HEIGHT) / 2) + 2))
			MoveCursorUp();
	}

	if (m_iSelectStart != -1) {
		String.Format(_T("Selection {%i; %i}"), m_iSelectStart, m_iSelectEnd);
		SetMessageText(String);
	}

	CView::OnMouseMove(nFlags, point);
}

void CFamiTrackerView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bHasFocus = false;
//	Invalidate(TRUE);
	ForceRedraw();
}

void CFamiTrackerView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bHasFocus = true;
//	Invalidate(TRUE);
	ForceRedraw();
}

void CFamiTrackerView::PostNcDestroy()
{
	// Called at exit
	m_bInitialized = false;
	CView::PostNcDestroy();
}

void CFamiTrackerView::OnTimer(UINT nIDEvent)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame *pMainFrm;
	static int LastNoteState;
	int TicksPerSec = pDoc->GetFrameRate();

	static_cast<CMainFrame*>(GetParentFrame())->SetIndicatorTime(m_iPlayTime / 600, (m_iPlayTime / 10) % 60, m_iPlayTime % 10);

	if (nIDEvent == 0) {
		pMainFrm = (CMainFrame*)GetParentFrame();

		// Fetch channel levels
		for (int i = 0; i < MAX_CHANNELS; i++) {
			m_iVolLevels[i] = theApp.GetOutput(i);
		}

		// DPCM
		m_iVolLevels[4] /= 8;

		if (m_bForceRedraw) {
			m_bForceRedraw = false;
	//		Invalidate(FALSE);
			RedrawWindow();
			((CMainFrame*)GetParent())->RefreshPattern();
		}
		else {
			if (pDoc->IsFileLoaded()) {
				CDC *pDC = this->GetDC();
				DrawMeters(pDC);
				DrawChannelHeaders(pDC);
				ReleaseDC(pDC);
			}
		}

		if (LastNoteState != m_iKeyboardNote)
			pMainFrm->ChangeNoteState(m_iKeyboardNote);

		LastNoteState = m_iKeyboardNote;
	}

	CView::OnTimer(nIDEvent);
}

int CFamiTrackerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Install a timer for screen updates
	SetTimer(0, 20, NULL);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnEditCopy()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stClipData	*ClipData;
	stChanNote	Note;
	HGLOBAL		hMem;
	int			Ptr = 0;
	int			i;
	int			SelStart, SelEnd, Columns = 0;
	int			SelColStart, SelColEnd;

	if (m_iSelectStart == -1)
		return;

	SelStart	= GetMin(m_iSelectStart, m_iSelectEnd);
	SelEnd		= GetMax(m_iSelectStart, m_iSelectEnd);
	SelColStart	= GetMin(m_iSelectColStart, m_iSelectColEnd);
	SelColEnd	= GetMax(m_iSelectColStart, m_iSelectColEnd);

	if (!OpenClipboard()) {
		theApp.DisplayError(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	EmptyClipboard();

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(stClipData) * 2);

	if (hMem == NULL)
		return;

	ClipData				= (stClipData*)GlobalLock(hMem);
	ClipData->Size			= SelEnd - SelStart + 1;
	ClipData->NumCols		= 0;
	ClipData->FirstColumn	= false;

	if (SelColStart == 0)
		ClipData->FirstColumn = true;

	int ColPtr = 0;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->GetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &Note);

		if (SelColStart == 0) {
			ClipData->Notes[Ptr]	= Note.Note;
			ClipData->Octaves[Ptr]	= Note.Octave;
		}

		ColPtr = 0;
		for (int n = 1; n < COLUMNS + MAX_EFFECT_COLUMNS * 3; n++) {
			if (n >= SelColStart && n <= SelColEnd) {
				switch (n - 1) {
					case 0: ClipData->ColumnData[ColPtr][Ptr] = Note.Instrument >> 4;		break;		// Instrument
					case 1: ClipData->ColumnData[ColPtr][Ptr] = Note.Instrument & 0x0F;		break;
					case 2: ClipData->ColumnData[ColPtr][Ptr] = Note.Vol;					break;		// Volume
					case 3: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[0];			break;		// Effect column 1
					case 4: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[0] >> 4;		break;
					case 5: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[0] & 0x0F;	break;
					case 6: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[1];			break;		// Effect column 2
					case 7: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[1] >> 4;		break;
					case 8: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[1] & 0x0F;	break;
					case 9: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[2];			break;		// Effect column 3
					case 10: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[2] >> 4;		break;
					case 11: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[2] & 0x0F;	break;
					case 12: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[3];			break;		// Effect column 4
					case 13: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[3] >> 4;		break;
					case 14: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[3] & 0x0F;	break;
				}
				ColPtr++;
				ClipData->NumCols = ColPtr;
			}
		}
		
		Ptr++;
	}

	GlobalUnlock(hMem);
	SetClipboardData(m_iClipBoard, hMem);
	CloseClipboard();
}

void CFamiTrackerView::OnEditCut()
{
	if (m_iSelectStart == -1)
		return;

	AddUndo(m_iSelectChannel);
	OnEditCopy();
	OnEditDelete();

	ForceRedraw();
}

void CFamiTrackerView::OnEditPaste()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	HGLOBAL			hMem;
	stChanNote		ChanNote;
	stClipData		*ClipData;
	unsigned int	i;

//	if (m_bEditEnable == false)
//		return;

	OpenClipboard();

	if (!IsClipboardFormatAvailable(m_iClipBoard)) {
		theApp.DisplayError(IDS_CLIPBOARD_NOT_AVALIABLE);
	}

	hMem = GetClipboardData(m_iClipBoard);

	if (hMem == NULL)
		return;

	AddUndo(m_iCursorChannel);

	ClipData = (stClipData*)GlobalLock(hMem);

	for (i = 0; i < ClipData->Size; i++) {
		if ((m_iCursorRow + i) < GetCurrentPatternLength()) {

			//if (!m_bPasteOverwrite && !m_bPasteMix)
			if (m_iPasteMode == PASTE_MODE_NORMAL)
				pDoc->InsertNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i);

			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i, &ChanNote);

			bool Change = true;

			if (ClipData->FirstColumn) {
				if (/*m_bPasteMix*/ m_iPasteMode == PASTE_MODE_MIX) {
					if (ChanNote.Note == 0) {
						ChanNote.Octave	= ClipData->Octaves[i];
						ChanNote.Note	= ClipData->Notes[i];
					}
					else
						Change = false;
				}
				else {
					ChanNote.Octave	= ClipData->Octaves[i];
					ChanNote.Note	= ClipData->Notes[i];
				}
			}

			unsigned int ColPtr = 0;

			for (int n = m_iCursorColumn; n < (COLUMNS + MAX_EFFECT_COLUMNS * 3); n++) {
				if (ColPtr < ClipData->NumCols && n > 0) {
					switch (n - 1) {
						case 0: 		// Instrument
							if (/*!m_bPasteMix*/ Change || ChanNote.Instrument == 0x40)
								ChanNote.Instrument = (ChanNote.Instrument & 0x0F) | ClipData->ColumnData[ColPtr][i] << 4;
							if (ChanNote.Instrument != 64)
								ChanNote.Instrument &= 0x3F;
							break;
						case 1: 
							if (/*!m_bPasteMix*/ Change || ChanNote.Instrument == 0x40)
								ChanNote.Instrument = (ChanNote.Instrument & 0xF0) | ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote.Instrument != 64)
								ChanNote.Instrument &= 0x3F; 
							break;
						case 2:			// Volume
							if (/*!m_bPasteMix*/ Change && ChanNote.Vol == 0x10)
								ChanNote.Vol = ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote.Vol != 0x10)
								ChanNote.Vol &= 0x0F;
							break;
						case 3: ChanNote.EffNumber[0]	= ClipData->ColumnData[ColPtr][i]; break;											// First effect column
						case 4: ChanNote.EffParam[0]	= (ChanNote.EffParam[0] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 5: ChanNote.EffParam[0]	= (ChanNote.EffParam[0] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 6: ChanNote.EffNumber[1]	= ClipData->ColumnData[ColPtr][i]; break;											// Second effect column
						case 7: ChanNote.EffParam[1]	= (ChanNote.EffParam[1] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 8: ChanNote.EffParam[1]	= (ChanNote.EffParam[1] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 9: ChanNote.EffNumber[2]	= ClipData->ColumnData[ColPtr][i]; break;											// Third effect column
						case 10: ChanNote.EffParam[2]	= (ChanNote.EffParam[2] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 11: ChanNote.EffParam[2]	= (ChanNote.EffParam[2] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 12: ChanNote.EffNumber[3]	= ClipData->ColumnData[ColPtr][i]; break;											// Fourth effect column
						case 13: ChanNote.EffParam[3]	= (ChanNote.EffParam[3] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 14: ChanNote.EffParam[3]	= (ChanNote.EffParam[3] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
					}
					ColPtr++;
				}
			}

			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i, &ChanNote);
		}
	}

	GlobalUnlock(hMem);
	CloseClipboard();
	ForceRedraw();
}

void CFamiTrackerView::OnEditDelete()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int	i;
	int	SelStart, SelEnd;
	int SelColStart, SelColEnd;

	if (m_iSelectStart == -1 || m_bEditEnable == false)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart	= m_iSelectEnd;
		SelEnd		= m_iSelectStart;
	}
	else {
		SelStart	= m_iSelectStart;
		SelEnd		= m_iSelectEnd;
	}

	if (m_iSelectColStart > m_iSelectColEnd) {
		SelColStart = m_iSelectColEnd;
		SelColEnd	= m_iSelectColStart;
	}
	else {
		SelColStart = m_iSelectColStart;
		SelColEnd	= m_iSelectColEnd;
	}

	AddUndo(m_iSelectChannel);

	stChanNote NoteData;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->GetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &NoteData);
		if (IsColumnSelected(SelColStart, SelColEnd, 0)) {
			NoteData.Note = 0; NoteData.Octave = 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 1))
			NoteData.Instrument = MAX_INSTRUMENTS;
		if (IsColumnSelected(SelColStart, SelColEnd, 2))
			NoteData.Vol = 0x10;
		if (IsColumnSelected(SelColStart, SelColEnd, 3)) {
			NoteData.EffNumber[0]	= EF_NONE;
			NoteData.EffParam[0]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 4)) {
			NoteData.EffNumber[1]	= EF_NONE;
			NoteData.EffParam[1]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 5)) {
			NoteData.EffNumber[2]	= EF_NONE;
			NoteData.EffParam[2]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 6)) {
			NoteData.EffNumber[3]	= EF_NONE;
			NoteData.EffParam[3]	= 0;
		}

		pDoc->SetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &NoteData);
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		SetMessageText(_T("Changed to edit mode"));
	else
		SetMessageText(_T("Changed to normal mode"));
	
	ForceRedraw();
}

void CFamiTrackerView::OnTrackerPal()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame*	pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	int Speed = (pDoc->GetEngineSpeed() == 0) ? 50 : pDoc->GetEngineSpeed();
	pDoc->SetMachine(PAL);
	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnTrackerNtsc()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame*	pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	int Speed = (pDoc->GetEngineSpeed() == 0) ? 60 : pDoc->GetEngineSpeed();
	pDoc->SetMachine(NTSC);
	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnSpeedCustom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSpeedDlg SpeedDlg;
	int Speed;

	Speed = pDoc->GetEngineSpeed();

	if (Speed == 0)
		Speed = (pDoc->GetMachine() == NTSC ? 60 : 50);

	Speed = SpeedDlg.GetSpeedFromDlg(Speed);

	if (Speed == 0)
		return;

	pDoc->SetEngineSpeed(Speed);
	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnSpeedDefault()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->SetEngineSpeed(0);
	theApp.SetMachineType(pDoc->GetMachine(), (pDoc->GetMachine() == NTSC) ? 60 : 50);
}

void CFamiTrackerView::OnEditPasteoverwrite()
{
	if (m_iPasteMode == PASTE_MODE_OVERWRITE)
		m_iPasteMode = PASTE_MODE_NORMAL;
	else
		m_iPasteMode = PASTE_MODE_OVERWRITE;
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Note > 0 && Note.Note != HALT) {
			if (Note.Note > 1) 
				Note.Note--;
			else {
				if (Note.Octave > 0) {
					Note.Note = B;
					Note.Octave--;
				}
			}
		}
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);
		
		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Note > 0 && Note.Note != HALT) {
				if (Note.Note > 1)
					Note.Note--;
				else {
					if (Note.Octave > 0) {
						Note.Note = B;
						Note.Octave--;
					}
				}
			}
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Octave > 0) 
			Note.Octave--;
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Octave > 0)
				Note.Octave--;
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Note > 0 && Note.Note != HALT) {
			if (Note.Note < B)
				Note.Note++;
			else {
				if (Note.Octave < 8) {
					Note.Note = C;
					Note.Octave++;
				}
			}
		}
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Note > 0 && Note.Note != HALT) {
				if (Note.Note < B)
					Note.Note++;
				else {
					if (Note.Octave < 8) {
						Note.Note = C;
						Note.Octave++;
					}
				}
			}
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Octave < 8) 
			Note.Octave++;
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Octave < 8)
				Note.Octave++;
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnEditInstrumentMask()
{
	m_bMaskInstrument = !m_bMaskInstrument;
}

void CFamiTrackerView::OnEditUndo()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Pattern, x, Track = pDoc->GetSelectedTrack();
	CString Text;

	if (!m_iUndoLevel)
		return;

	if (!m_iRedoLevel) {
		int Channel = m_UndoStack[m_iUndoLevel - 1].Channel;
		int Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel);
		int x;

		m_UndoStack[m_iUndoLevel].Channel	= Channel;
		m_UndoStack[m_iUndoLevel].Pattern	= Pattern;
		m_UndoStack[m_iUndoLevel].Row		= m_iCursorRow;
		m_UndoStack[m_iUndoLevel].Column	= m_iCursorColumn;
		m_UndoStack[m_iUndoLevel].Frame		= m_iCurrentFrame;

		for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
			pDoc->GetDataAtPattern(Track, Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
		}
	}

	m_iRedoLevel++;
	m_iUndoLevel--;

	Channel = m_UndoStack[m_iUndoLevel].Channel;
	Pattern = m_UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->SetDataAtPattern(Track, Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
	}

	m_iCursorChannel	= m_UndoStack[m_iUndoLevel].Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->SetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel, Pattern);

	m_iCurrentRow = m_iCursorRow;

	Text.Format(_T("Undo (%i / %i)"), m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	SetMessageText(Text);

	ForceRedraw();
}

void CFamiTrackerView::OnEditRedo()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int Track = pDoc->GetSelectedTrack();

	if (!m_iRedoLevel)
		return;

	m_iUndoLevel++;
	m_iRedoLevel--;

	int Channel, Pattern, x;

	Channel = m_UndoStack[m_iUndoLevel].Channel;
	Pattern = m_UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->SetDataAtPattern(Track, Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
	}

	m_iCursorChannel	= Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->SetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel, Pattern);

	m_iCurrentRow = m_iCursorRow;

	CString Text;
	Text.Format(_T("Redo (%i / %i)"), m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	SetMessageText(Text);

	ForceRedraw();
}

void CFamiTrackerView::OnEditSelectall()
{
	SelectWholePattern(m_iCursorChannel);
}

void CFamiTrackerView::OnFrameInsert()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x, frames = pDoc->GetFrameCount(), channels = pDoc->GetAvailableChannels();;

	if (frames == MAX_FRAMES) {
		return;
	}

	pDoc->SetFrameCount(frames + 1);

	for (x = (frames + 1); x > (m_iCurrentFrame + 1); x--) {
		for (i = 0; i < channels; i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x - 1, i));
		}
	}

	for (i = 0; i < channels; i++) {
		pDoc->SetPatternAtFrame(m_iCurrentFrame + 1, i, 0);
	}
}

void CFamiTrackerView::OnFrameRemove()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x;

	if (pDoc->GetFrameCount() == 1) {
		return;
	}

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, 0);
	}

	for (x = m_iCurrentFrame; x < pDoc->GetFrameCount(); x++) {
		for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x + 1, i));
		}
	}

	pDoc->SetFrameCount(pDoc->GetFrameCount() - 1);

	if (m_iCurrentFrame > (pDoc->GetFrameCount() - 1))
		m_iCurrentFrame = (pDoc->GetFrameCount() - 1);
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	stChanNote Note;

	ASSERT_VALID(pDoc);

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &Note);
		if (!m_bMuteChannels[i])
			FeedNote(i, &Note);
	
	}

	MoveCursorDown();
}

void CFamiTrackerView::OnEditPastemix()
{
	if (m_iPasteMode == PASTE_MODE_MIX)
		m_iPasteMode = PASTE_MODE_NORMAL;
	else
		m_iPasteMode = PASTE_MODE_MIX;
}

void CFamiTrackerView::OnModuleMoveframedown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Pattern;

	if (m_iCurrentFrame == (pDoc->GetFrameCount() - 1))
		return;

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, i);
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, pDoc->GetPatternAtFrame(m_iCurrentFrame + 1, i));
		pDoc->SetPatternAtFrame(m_iCurrentFrame + 1, i, Pattern);
	}

	m_iCurrentFrame++;
	ForceRedraw();
}

void CFamiTrackerView::OnModuleMoveframeup()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Pattern, i;

	if (m_iCurrentFrame == 0)
		return;

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, i);
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, pDoc->GetPatternAtFrame(m_iCurrentFrame - 1, i));
		pDoc->SetPatternAtFrame(m_iCurrentFrame - 1, i, Pattern);
	}

	m_iCurrentFrame--;
	ForceRedraw();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI updates
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnUpdateEditInstrumentMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskInstrument ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(m_iClipBoard) ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnUpdateTrackerEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bEditEnable ? 1 : 0);
}

void CFamiTrackerView::OnUpdateTrackerPal(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->SetCheck(pDoc->GetMachine() == PAL);
}

void CFamiTrackerView::OnUpdateTrackerNtsc(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->SetCheck(pDoc->GetMachine() == NTSC);
}

void CFamiTrackerView::OnUpdateSpeedDefault(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() == 0);
}

void CFamiTrackerView::OnUpdateSpeedCustom(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);	
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() != 0);
}

void CFamiTrackerView::OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iPasteMode == PASTE_MODE_OVERWRITE);
}

void CFamiTrackerView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iUndoLevel > 0 ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iRedoLevel > 0 ? 1 : 0);
}

void CFamiTrackerView::OnUpdateFrameRemove(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	if (!pDoc->IsFileLoaded())
		return;
	pCmdUI->Enable(pDoc->GetFrameCount() > 1);
}

void CFamiTrackerView::OnUpdateEditPastemix(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iPasteMode == PASTE_MODE_MIX);
}

void CFamiTrackerView::OnUpdateModuleMoveframedown(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	if (!pDoc->IsFileLoaded())
		return;
	pCmdUI->Enable(!(m_iCurrentFrame == (pDoc->GetFrameCount() - 1)));
}

void CFamiTrackerView::OnUpdateModuleMoveframeup(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iCurrentFrame > 0);
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	switch (lHint) {
		case UPDATE_SONG_TRACK:		// changed song
			m_iCurrentFrame		= 0;
			m_iCurrentRow		= 0;
			m_iCursorChannel	= 0;
			m_iCursorColumn		= 0;
			m_iCursorRow		= 0;
			break;
		case UPDATE_SONG_TRACKS:
			((CMainFrame*)GetParentFrame())->UpdateTrackBox();
			break;				 
		case UPDATE_CLEAR:
			((CMainFrame*)GetParentFrame())->ClearInstrumentList();
			((CMainFrame*)GetParentFrame())->CloseInstrumentSettings();
			break;
		case UPDATE_NEW_DOC:
			Invalidate();
			RedrawWindow();
			break;
		default:
			// Document has changed, redraw the view
			break;
	}

	CreateBackground();

	m_bForceRedraw = true;
}

void CFamiTrackerView::OnInitialUpdate()
{
	// Called when a new document is loaded
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame	*MainFrame = static_cast<CMainFrame*>(GetParentFrame());

	unsigned int i;
	char Text[256];

	CView::OnInitialUpdate();

	m_iLastCursorColumn = 0;
	m_iLastRowState		= 0;
	m_bInitialized		= true;
	m_bForceRedraw		= false;
	m_iInstrument		= 0;
	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;

	m_iCurrentFrame		= 0;
	m_iCurrentRow		= 0;
	m_iCursorChannel	= 0;
	m_iCursorColumn		= 0;
	m_iCursorRow		= 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i)) {
			pDoc->GetInstrumentName(i, Text);
			MainFrame->AddInstrument(i, Text, pDoc->GetInstrument(i)->GetType());
		}
	}

	if (pDoc->GetEngineSpeed() == 0) {
		if (pDoc->GetMachine() == NTSC)
			theApp.SetMachineType(pDoc->GetMachine(), 60);
		else
			theApp.SetMachineType(pDoc->GetMachine(), 50);
	}
	else
		theApp.SetMachineType(pDoc->GetMachine(), pDoc->GetEngineSpeed());

	MainFrame->SetSongInfo(pDoc->GetSongName(), pDoc->GetSongArtist(), pDoc->GetSongCopyright());
	MainFrame->UpdateTrackBox();
	MainFrame->DisplayOctave();

	RedrawWindow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::SetMessageText(LPCSTR lpszText)
{
	GetParentFrame()->SetMessageText(lpszText);
}

void CFamiTrackerView::ForceRedraw()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->UpdateAllViews(NULL);
}

void CFamiTrackerView::RemoveWithoutDelete()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote	Note;

	int	i;
	int	SelStart, SelEnd;

	if (m_iSelectStart == -1 || m_bEditEnable == false)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart = m_iSelectEnd;
		SelEnd = m_iSelectStart;
	}
	else {
		SelStart = m_iSelectStart;
		SelEnd = m_iSelectEnd;
	}

	AddUndo(m_iSelectChannel);

	Note.Note = 0;
	Note.Octave = 0;
	Note.Instrument = 0;
	Note.EffNumber[0] = 0;
	Note.EffParam[0] = 0;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->SetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &Note);
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::SelectWholePattern(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iSelectStart == -1) {
		m_iSelectChannel	= Channel;
		m_iSelectStart		= 0;
		m_iSelectEnd		= GetCurrentPatternLength() - 1;
		m_iSelectColStart	= 0;
		m_iSelectColEnd		= COLUMNS - 1 + (pDoc->GetEffColumns(Channel) * 3);
	}
	else
		m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::RegisterKeyState(int Channel, int Note)
{
	if (Channel == m_iCursorChannel)
		m_iKeyboardNote	= Note;
}

void CFamiTrackerView::GetStartSpeed()
{
	unsigned int TempoAccum;
	CFamiTrackerDoc* pDoc = GetDocument();

	TempoAccum		= 60 * pDoc->GetFrameRate();		// 60 as in 60 seconds per minute
	m_iTickPeriod	= pDoc->GetSongSpeed();
	m_iSpeed		= DEFAULT_SPEED;
	
	m_iTempo = TempoAccum / 24;

	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
}

void CFamiTrackerView::SelectFrame(unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Frame < GetDocument()->GetFrameCount());

	m_iCurrentFrame = Frame;
	ForceRedraw();
}

void CFamiTrackerView::SelectNextFrame()
{
	if (m_iCurrentFrame < (GetDocument()->GetFrameCount() - 1))
		m_iCurrentFrame++;
	else
		m_iCurrentFrame = 0;

	ForceRedraw();
}

void CFamiTrackerView::SelectPrevFrame()
{
	if (m_iCurrentFrame > 0)
		m_iCurrentFrame--;
	else
		m_iCurrentFrame = GetDocument()->GetFrameCount() - 1;

	ForceRedraw();
}

void CFamiTrackerView::SelectFirstFrame()
{
	m_iCurrentFrame = 0;
	ForceRedraw();
}

void CFamiTrackerView::SelectLastFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCurrentFrame = pDoc->GetFrameCount() - 1;
	ForceRedraw();
}

void CFamiTrackerView::SelectChannel(unsigned int Channel)
{
	ASSERT(Channel < MAX_CHANNELS);
	m_iCursorChannel = Channel;
	m_iCursorColumn = 0;
	ForceRedraw();
}

// Moves cursor one step to the left
void CFamiTrackerView::MoveCursorLeft()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int MaxColumns = GetCurrentColumnCount();

	if (m_iCursorColumn > 0) {
		m_iCursorColumn--;
	}
	else {
		if (m_iCursorChannel > 0) {
			int PrevColCount = COLUMNS + pDoc->GetEffColumns(m_iCursorChannel - 1) * 3;
			m_iCursorColumn = PrevColCount - 1;
			m_iCursorChannel--;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorChannel = pDoc->GetAvailableChannels() - 1;
				m_iCursorColumn = GetCurrentColumnCount() - 1;
			}
		}
	}

	ForceRedraw();
}

// Moves cursor one step to the right
void CFamiTrackerView::MoveCursorRight()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int MaxColumns = GetCurrentColumnCount();

	if (m_iCursorColumn < (MaxColumns - 1)) {
		m_iCursorColumn++;
	}
	else {
		if (m_iCursorChannel < (pDoc->GetAvailableChannels() - 1)) {
			m_iCursorColumn = 0;
			m_iCursorChannel++;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorColumn	= 0;
				m_iCursorChannel = 0;
			}
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorUp()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Step = theApp.m_pSettings->General.bNoStepMove ? 1 : m_iKeyStepping;

	if (m_iCursorRow >= Step)
		m_iCursorRow -= Step;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow = GetCurrentPatternLength() - (Step - m_iCursorRow);

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Step = theApp.m_pSettings->General.bNoStepMove ? 1 : m_iKeyStepping;

	if (m_iCursorRow < (GetCurrentPatternLength() - Step ))
		m_iCursorRow += Step ;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow = Step  - (GetCurrentPatternLength() - m_iCursorRow) ;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPageUp()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int PageSize = theApp.m_pSettings->General.iPageStepSize;

	if (m_iCursorRow > (PageSize - 1))
		m_iCursorRow -= PageSize;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow += GetCurrentPatternLength() - PageSize;
	else
		m_iCursorRow = 0;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPageDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int PageSize = theApp.m_pSettings->General.iPageStepSize;

	if (m_iCursorRow < GetCurrentPatternLength() - PageSize)
		m_iCursorRow += PageSize; 
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow -= GetCurrentPatternLength() - PageSize;
	else 
		m_iCursorRow = GetCurrentPatternLength() - 1;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorToTop()
{
	m_iCursorRow = 0;
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorToBottom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	m_iCursorRow = GetCurrentPatternLength() - 1;
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorNextChannel()
{
	m_iCursorColumn = 0;

	if (m_iCursorChannel < GetDocument()->GetAvailableChannels() - 1)
		m_iCursorChannel++;
	else
		m_iCursorChannel = 0;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_iCursorColumn = 0;

	if (m_iCursorChannel > 0)
		m_iCursorChannel--;
	else
		m_iCursorChannel = GetDocument()->GetAvailableChannels() - 1;
	
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorFirstChannel()
{
	m_iCursorChannel = 0;
	m_iCursorColumn = 0;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorLastChannel()
{
	m_iCursorChannel = GetDocument()->GetAvailableChannels() - 1;
	m_iCursorColumn	= 0;

	ForceRedraw();
}

void CFamiTrackerView::ToggleChannel(unsigned int Channel)
{
	m_bMuteChannels[Channel] = !m_bMuteChannels[Channel];
	HaltNote(Channel, true);
	ForceRedraw();
}

void CFamiTrackerView::SoloChannel(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (IsChannelSolo(Channel)) {
		// Revert channels
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			m_bMuteChannels[i] = false;
		}
	}
	else {
		// Solo selected channel
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			if (i != Channel)
				m_bMuteChannels[i] = true;
		}
		m_bMuteChannels[Channel] = false;
	}

	ForceRedraw();
}

bool CFamiTrackerView::IsChannelSolo(unsigned int Channel)
{
	// Returns true if Channel is the only active channel 
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		if (m_bMuteChannels[i] == false && i != Channel)
			return false;
	}
	return true;
}

void CFamiTrackerView::SetInstrument(int Instrument)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (Instrument < 0 || Instrument >= (MAX_INSTRUMENTS - 1))
		return;

	if (pDoc->IsInstrumentUsed(Instrument))
		m_iInstrument = Instrument;
}

void CFamiTrackerView::IncreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->IncreasePattern(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->IncreasePattern(m_iCurrentFrame, m_iCursorChannel);
}

void CFamiTrackerView::DecreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->DecreasePattern(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->DecreasePattern(m_iCurrentFrame, m_iCursorChannel);
}

void CFamiTrackerView::SetSongSpeed(unsigned int Speed)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	ASSERT(Speed <= MAX_TEMPO);

	m_iTickPeriod = Speed;
	
	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
		
	pDoc->SetSongSpeed(Speed);
}

void CFamiTrackerView::StepDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Step = m_iKeyStepping;

	if (m_iCursorRow < (GetCurrentPatternLength() - Step))
		m_iCursorRow += Step ;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow = Step  - (GetCurrentPatternLength() - m_iCursorRow) ;

	ForceRedraw();
}

void CFamiTrackerView::AddUndo(unsigned int Channel)
{
	// Channel - Channel that contains the pattern that should be added ontop of undo stack
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, Channel);
	unsigned int Track = pDoc->GetSelectedTrack();
	int x;

	stUndoBlock UndoBlock;
	
	UndoBlock.Channel	= Channel;
	UndoBlock.Pattern	= Pattern;
	UndoBlock.Row		= m_iCursorRow;
	UndoBlock.Column	= m_iCursorColumn;
	UndoBlock.Frame		= m_iCurrentFrame;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->GetDataAtPattern(Track, Pattern, Channel, x, &UndoBlock.ChannelData[x]);
	}

	if (m_iUndoLevel < UNDO_LEVELS)
		m_iUndoLevel++;
	else {
		if (m_iUndoLevel == (UNDO_LEVELS - 0)) {
			for (x = 1; x < (UNDO_LEVELS+1); x++) {
				m_UndoStack[x - 1] = m_UndoStack[x];
			}
		}
	}

	m_UndoStack[m_iUndoLevel - 1] = UndoBlock;

	m_iRedoLevel = 0;
}

void CFamiTrackerView::StopNote(int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	memset(&Note, 0, sizeof(stChanNote));

	Note.Note = HALT;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->SetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Note);
		pDoc->SetModifiedFlag();
		ForceRedraw();
	}

	FeedNote(Channel, &Note);
}

void CFamiTrackerView::InsertNote(int Note, int Octave, int Channel, int Velocity)
{
	// Inserts a note

	stChanNote Cell;

	if (m_bMuteChannels[Channel]) {
		memset(&Cell, 0, sizeof(stChanNote));
		Cell.Vol = 0x0F;
	}
	else
		GetDocument()->GetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Cell);

	Cell.Note = Note;

	if (Note != HALT) {
		Cell.Octave	= Octave;

		if (!m_bMaskInstrument)
			Cell.Instrument = m_iInstrument;

		if (Velocity < 128)
			Cell.Vol = (Velocity / 8);
	}	

	if (m_bEditEnable) {
		AddUndo(Channel);
		GetDocument()->SetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Cell);
		if (m_iCursorColumn == 0 && !theApp.IsPlaying() && m_iRealKeyStepping > 0)
			StepDown();
	}
}

void CFamiTrackerView::ScanActualLengths()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	unsigned int FrameCount		= pDoc->GetFrameCount();
	unsigned int Channels		= pDoc->GetAvailableChannels();
	unsigned int PatternLength	= pDoc->GetPatternLength();

	for (unsigned int i = 0; i < FrameCount; i++) {
		m_iActualLengths[i] = PatternLength;
		m_iNextPreviewFrame[i] = i + 1;
		if (theApp.m_pSettings->General.bFramePreview) {
			for (unsigned int j = 0; j < Channels; j++) {
				for (unsigned int k = 0; k < PatternLength; k++) {
					pDoc->GetNoteData(i, j, k, &Note);
					for (unsigned int l = 0; l < pDoc->GetEffColumns(j) + 1; l++) {
						if (Note.EffNumber[l] == EF_SKIP || 
							Note.EffNumber[l] == EF_JUMP ||
							Note.EffNumber[l] == EF_HALT && (k + 1) > m_iCursorRow) {
							m_iActualLengths[i] = k + 1;
							switch (Note.EffNumber[l]) {
								case EF_SKIP:
									m_iNextPreviewFrame[i] = i + 1;
									break;
								case EF_JUMP:
									m_iNextPreviewFrame[i] = Note.EffParam[l];
									break;
								case EF_HALT:
									m_iNextPreviewFrame[i] = i + 1;
									break;
							}
							if (m_iNextPreviewFrame[i] >= FrameCount)
								m_iNextPreviewFrame[i] = FrameCount - 1;
							break;
						}
					}
				}
			}
		}
	}
}

unsigned int CFamiTrackerView::GetCurrentPatternLength()
{
	return m_iActualLengths[m_iCurrentFrame];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Note playing routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity)
{
	// Play a note in a channel
	stChanNote NoteData;

	NoteData.Note		= Note;
	NoteData.Octave		= Octave;
	NoteData.Vol		= Velocity;
	NoteData.Instrument	= GetInstrument();
	
	FeedNote(Channel, &NoteData);
}

void CFamiTrackerView::HaltNote(unsigned int Channel, bool HardHalt)
{
	// Stop a channel
	stChanNote NoteData;
	memset(&NoteData, 0, sizeof(stChanNote));

	if (HardHalt)
		NoteData.Note = HALT;
	else
		NoteData.Note = RELEASE;

	FeedNote(Channel, &NoteData);
}

void CFamiTrackerView::FeedNote(int Channel, stChanNote *NoteData)
{
	// Store a note for playing
	CurrentNotes[Channel] = *NoteData;
	NewNoteData[Channel] = true;
	theApp.GetMIDI()->WriteNote(Channel, NoteData->Note, NoteData->Octave, NoteData->Vol);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MIDI note handling functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

int m_iMultipleNotes = 0;
int m_iNoteArray[20];

void CFamiTrackerView::TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert)
{
	// Play a MIDI note

	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	m_iActiveNotes[Channel] = MidiNote;

	if (!theApp.m_pSettings->Midi.bMidiVelocity)
		Velocity = 127;

	if (Insert)
		InsertNote(Note, Octave, Channel, Velocity + 1);

	PlayNote(Channel, Note, Octave, Velocity + 1);

	ArpeggioNotes[MidiNote] = 1;
	ArpeggioPtr = MidiNote;
	LastArpPtr = ArpeggioPtr;

	UpdateArpDisplay();

	m_iLastMIDINote = MidiNote;

	KeyCount = 0;

	for (int i = 0; i < 128; i++) {
		if (ArpeggioNotes[i] == 1)
			KeyCount++;
	}
}

void CFamiTrackerView::ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut)
{
	// Release a MIDI note

	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	m_iActiveNotes[Channel] = 0;
	ArpeggioNotes[MidiNote] = 2;

	UpdateArpDisplay();

	// Cut note
	if (m_iLastMIDINote == MidiNote)
		HaltNote(Channel, false);

	if (InsertCut)
		InsertNote(HALT, 0, Channel, 0);
}

void CFamiTrackerView::UpdateArpDisplay()
{
	if (theApp.m_pSettings->Midi.bMidiArpeggio == true) {
		int Base = -1;
		char Text[256];
		strcpy(Text, _T("Arpeggio "));

		for (int i = 0; i < 128; i++) {
			if (ArpeggioNotes[i] == 1) {
				if (Base == -1)
					Base = i;
				sprintf(Text, _T("%s %i"), Text, i - Base);
			}
		}

		if (Base != -1)
			GetParentFrame()->SetMessageText(Text);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tracker input routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Keyboard handling routines
//

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{	
	// Called when a key is pressed
	 
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CAccelerator *pAccel = theApp.GetAccelerator();

	bInsertedEffect = false;

	if (pAccel->GetAction(nChar) != 0)
		return;

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9) {
		if (m_bControlPressed)
			SetStepping(nChar - VK_NUMPAD0);
		else {
			SetInstrument(nChar - VK_NUMPAD0);
			((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
		}
		return;
	}
	
	switch (nChar) {
		// Shift		
		case VK_SHIFT:
			m_bShiftPressed = true;
			break;
		case VK_CONTROL:
			m_bControlPressed = true;
			break;
		// Movement
		case VK_UP:
			CheckSelectionBeforeMove();
			MoveCursorUp();
			CheckSelectionAfterMove();
			break;
		case VK_DOWN:
			CheckSelectionBeforeMove();
			MoveCursorDown();
			CheckSelectionAfterMove();
			break;
		case VK_HOME:
			CheckSelectionBeforeMove();
			OnKeyHome();
			CheckSelectionAfterMove();
			break;
		case VK_END:	
			CheckSelectionBeforeMove();
			MoveCursorToBottom();	
			CheckSelectionAfterMove();
			break;
		case VK_PRIOR:	
			CheckSelectionBeforeMove();
			MoveCursorPageUp();		
			CheckSelectionAfterMove();
			break;
		case VK_NEXT:	
			CheckSelectionBeforeMove();
			MoveCursorPageDown();	
			CheckSelectionAfterMove();
			break;
		case VK_LEFT:
			CheckSelectionBeforeMove();
			MoveCursorLeft();
			CheckSelectionAfterMove();
			break;
		case VK_RIGHT:
			CheckSelectionBeforeMove();
			MoveCursorRight();
			CheckSelectionAfterMove();
			break;
		case VK_TAB:
			if (m_bShiftPressed)
				MoveCursorPrevChannel();
			else
				MoveCursorNextChannel();
			ForceRedraw();
			break;
		// Pattern editing
		case VK_ADD:
			KeyIncreaseAction();
			break;
		case VK_SUBTRACT:
			KeyDecreaseAction();
			break;
		case VK_DELETE:
			if (m_iSelectStart != -1) {
				if (PreventRepeat(VK_DELETE, m_bEditEnable))
					return;
				OnEditDelete();
			}
			else {
				if (PreventRepeat(VK_DELETE, m_bEditEnable) || !m_bEditEnable)
					return;
				AddUndo(m_iCursorChannel);
				if (pDoc->DeleteNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, m_iCursorColumn)) {
					StepDown();
					ForceRedraw();
				}
			}
			break;

		case VK_INSERT:
			if (!m_bEditEnable || PreventRepeat(VK_INSERT, m_bEditEnable))
				return;
			AddUndo(m_iCursorChannel);
			pDoc->InsertNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);
			ForceRedraw();
			break;

		case VK_BACK:
			if (m_iSelectStart != -1)
				RemoveWithoutDelete();
			else {
				if (m_iCursorRow == 0 || !m_bEditEnable || PreventRepeat(VK_BACK, m_bEditEnable))
					return;
				AddUndo(m_iCursorChannel);
				if (pDoc->RemoveNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow)) {
					MoveCursorUp();
					ForceRedraw();
				}
			}
			break;

			// Octaves
		case VK_F2: SetOctave(0); break;
		case VK_F3: SetOctave(1); break;
		case VK_F4: SetOctave(2); break;
		case VK_F5: SetOctave(3); break;
		case VK_F6: SetOctave(4); break;
		case VK_F7: SetOctave(5); break;
		case VK_F8: SetOctave(6); break;
		case VK_F9: SetOctave(7); break;

		case VK_ESCAPE:
			m_iSelectStart = -1;
			ForceRedraw();
			break;

		case 255:
			return;

		default:
			HandleKeyboardInput(nChar);
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is released

	if (nChar == VK_SHIFT)
		m_bShiftPressed = false;
	else if (nChar == VK_CONTROL)
		m_bControlPressed = false;

	HandleKeyboardNote(nChar, false);

	m_cKeyList[nChar] = 0;

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::KeyIncreaseAction()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	AddUndo(m_iCursorChannel);

	switch (m_iCursorColumn) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2: 
			pDoc->IncreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	
			ForceRedraw(); 
			break;
		case C_VOLUME: 
			pDoc->IncreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);	
			ForceRedraw(); 
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 0);
			ForceRedraw(); 
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 1);
			ForceRedraw(); 
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 2);
			ForceRedraw(); 
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 
			pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 3);
			ForceRedraw(); 
			break;
	}
}

void CFamiTrackerView::KeyDecreaseAction()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	AddUndo(m_iCursorChannel);

	switch (m_iCursorColumn) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			pDoc->DecreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	
			ForceRedraw(); 
			break;
		case C_VOLUME: 
			pDoc->DecreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);	
			ForceRedraw(); 
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 0);
			ForceRedraw(); 
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 1);
			ForceRedraw(); 
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 2); 
			ForceRedraw(); 
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 					
			pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 3); 
			ForceRedraw(); 
			break;
	}
}

bool CFamiTrackerView::EditInstrumentColumn(stChanNote &Note, int Value)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	unsigned char Mask, Shift;
	
	if (!m_bEditEnable || Value == -1)
		return false;

	if (m_iCursorColumn == C_INSTRUMENT1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	if (Note.Instrument == MAX_INSTRUMENTS)
		Note.Instrument = 0;

	if (EditStyle == EDIT_STYLE1) {
		Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
	}
	else if (EditStyle == EDIT_STYLE2) {
		if (Note.Instrument == (MAX_INSTRUMENTS - 1))
			Note.Instrument = 0;
		Note.Instrument = ((Note.Instrument & Mask) << 4) | Value & 0x0F;
	}
	else if (EditStyle == EDIT_STYLE3) {
		Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
		if (m_iCursorColumn == C_INSTRUMENT1)
			m_iCursorColumn = C_INSTRUMENT2;
		else if (m_iCursorColumn == C_INSTRUMENT2) {
			m_iCursorColumn = C_INSTRUMENT1;
			MoveCursorDown();
		}
	}

	if (Note.Instrument > (MAX_INSTRUMENTS - 1))
		Note.Instrument = (MAX_INSTRUMENTS - 1);

	return true;
}

bool CFamiTrackerView::EditVolumeColumn(stChanNote &Note, int Value)
{
	if (!m_bEditEnable || Value == -1)
		return false;

	Note.Vol = Value;

	if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE3)
		MoveCursorDown();

	return true;
}

bool CFamiTrackerView::EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	bool Found = false;

	if (!m_bEditEnable)
		return false;
		
	for (int i = 0; i < EF_COUNT; i++) {
		if (nChar == EFF_CHAR[i]) {
			Note.EffNumber[EffectIndex] = i + 1;
			Found = true;
			if (EditStyle == EDIT_STYLE3)
				MoveCursorDown();
			bInsertedEffect = true;
			iLastEffect = i;
			break;
		}
	}

	return Found;
}

bool CFamiTrackerView::EditEffParamColumn(stChanNote &Note, int Value, int EffectIndex)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	unsigned char Mask, Shift;
	
	if (!m_bEditEnable || Value == -1)
		return false;

	if (m_iCursorColumn == C_EFF_PARAM1 || 
		m_iCursorColumn == C_EFF2_PARAM1 ||
		m_iCursorColumn == C_EFF3_PARAM1 ||
		m_iCursorColumn == C_EFF4_PARAM1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}
	
	if (EditStyle == EDIT_STYLE1) {
		Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
	}
	else if (EditStyle == EDIT_STYLE2) {
		Note.EffParam[EffectIndex] = ((Note.EffParam[EffectIndex] & Mask) << 4) | Value & 0x0F;
	}
	else if (EditStyle == EDIT_STYLE3) {
		Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
		if (Mask == 0x0F)
			MoveCursorRight();
		else {
			MoveCursorLeft();
			MoveCursorDown();
		}
	}

	return true;
}

void CFamiTrackerView::HandleKeyboardInput(char nChar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static bool FirstChar = false;

	stChanNote Note;
	int EditedColumn = m_iCursorColumn;
	int EditedRow	 = m_iCursorRow;
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	int Index;

	// Watch for repeating keys
	if (PreventRepeat(nChar, m_bEditEnable))
		return;

	TRACE("Note down: %i\n", nChar);

	// Get the note data
	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, EditedRow, &Note);

	// Make all effect columns look the same, save an index instead
	switch (EditedColumn) {
		case C_EFF_NUM:		EditedColumn = C_EFF_NUM;	Index = 0; break;
		case C_EFF2_NUM:	EditedColumn = C_EFF_NUM;	Index = 1; break;
		case C_EFF3_NUM:	EditedColumn = C_EFF_NUM;	Index = 2; break;
		case C_EFF4_NUM:	EditedColumn = C_EFF_NUM;	Index = 3; break;
		case C_EFF_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 0; break;
		case C_EFF2_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 1; break;
		case C_EFF3_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 2; break;
		case C_EFF4_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 3; break;
		case C_EFF_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 0; break;
		case C_EFF2_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 1; break;
		case C_EFF3_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 2; break;
		case C_EFF4_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 3; break;			
	}

	// Determine what to change and do it. If nothing happened, return.
	switch (EditedColumn) {
		case C_NOTE:
			// This is special
			HandleKeyboardNote(nChar, true);
			return;
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			if (!EditInstrumentColumn(Note, ConvertKeyToHex(nChar)))
				return;
			break;
		case C_VOLUME:
			if (!EditVolumeColumn(Note, ConvertKeyToHex(nChar)))
				return;
			break;
		case C_EFF_NUM:
			if (!EditEffNumberColumn(Note, nChar, Index))
				return;
			break;
		case C_EFF_PARAM1:
		case C_EFF_PARAM2:
			if (!EditEffParamColumn(Note, ConvertKeyToHex(nChar), Index))
				return;
			break;
	}

	// If something was edited, store it in the document again
	if (m_bEditEnable) {
		// Save an undo for this channel
		AddUndo(m_iCursorChannel);
		// Store it
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, EditedRow, &Note);
		// If it should be stepped down
		if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE1)
			StepDown();
	}
}

void CFamiTrackerView::HandleKeyboardNote(char nChar, bool Pressed) 
{
	// Play a note from the keyboard
	int Note = TranslateKey(nChar);

	if (Pressed) {
		if (CheckHaltKey(nChar))
			ReleaseMIDINote(m_iCursorChannel, Note, true);
		else {
			// Invalid key
			if (Note == -1)
				return;
			TriggerMIDINote(m_iCursorChannel, Note, 0x7F, m_bEditEnable);
		}
	}
	else {
		if (Note == -1)
			return;
		ReleaseMIDINote(m_iCursorChannel, Note, false);
	}
}

bool CFamiTrackerView::CheckHaltKey(unsigned char Key)
{
	switch (Key) {
		case 226: return true;	// < /stop
		case 186: return true;	// � /stop (for american keyboards)
		//case 49: return true;	// 1 /stop (again)
	}

	// 1, disable when ModPlug is selected
	if (theApp.m_pSettings->General.iEditStyle != EDIT_STYLE2 && Key == 49)
		return true;

	return false;
}

int CFamiTrackerView::TranslateKey2(unsigned char Key)
{
	// Translates a keyboard character into a MIDI note
	int	KeyNote = 0, KeyOctave = 0;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote NoteData;

	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &NoteData);	

	// Convert key to a note, Modplug style
	switch (Key) {
		case 49:	KeyNote = NoteData.Note;	KeyOctave = 0;	break;	// 1
		case 50:	KeyNote = NoteData.Note;	KeyOctave = 1;	break;	// 2
		case 51:	KeyNote = NoteData.Note;	KeyOctave = 2;	break;	// 3
		case 52:	KeyNote = NoteData.Note;	KeyOctave = 3;	break;	// 4
		case 53:	KeyNote = NoteData.Note;	KeyOctave = 4;	break;	// 5
		case 54:	KeyNote = NoteData.Note;	KeyOctave = 5;	break;	// 6
		case 55:	KeyNote = NoteData.Note;	KeyOctave = 6;	break;	// 7
		case 56:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 8
		case 57:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 9
		case 48:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 0

		case 81:	KeyNote = C;	KeyOctave = m_iOctave;	break;	// Q
		case 87:	KeyNote = Cb;	KeyOctave = m_iOctave;	break;	// W
		case 69:	KeyNote = D;	KeyOctave = m_iOctave;	break;	// E
		case 82:	KeyNote = Db;	KeyOctave = m_iOctave;	break;	// R
		case 84:	KeyNote = E;	KeyOctave = m_iOctave;	break;	// T
		case 89:	KeyNote = F;	KeyOctave = m_iOctave;	break;	// Y
		case 85:	KeyNote = Fb;	KeyOctave = m_iOctave;	break;	// U
		case 73:	KeyNote = G;	KeyOctave = m_iOctave;	break;	// I
		case 79:	KeyNote = Gb;	KeyOctave = m_iOctave;	break;	// O
		case 80:	KeyNote = A;	KeyOctave = m_iOctave;	break;	// P
		case 221:	KeyNote = Ab;	KeyOctave = m_iOctave;	break;	// �
		case 219:	KeyNote = B;	KeyOctave = m_iOctave;	break;	// �

		case 65:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// A
		case 83:	KeyNote = Cb;	KeyOctave = m_iOctave + 1;	break;	// S
		case 68:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// D
		case 70:	KeyNote = Db;	KeyOctave = m_iOctave + 1;	break;	// F
		case 71:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// G
		case 72:	KeyNote = F;	KeyOctave = m_iOctave + 1;	break;	// H
		case 74:	KeyNote = Fb;	KeyOctave = m_iOctave + 1;	break;	// J
		case 75:	KeyNote = G;	KeyOctave = m_iOctave + 1;	break;	// K
		case 76:	KeyNote = Gb;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = A;	KeyOctave = m_iOctave + 1;	break;	// �
		case 222:	KeyNote = Ab;	KeyOctave = m_iOctave + 1;	break;	// �
		case 191:	KeyNote = B;	KeyOctave = m_iOctave + 1;	break;	// '

		case 90:	KeyNote = C;	KeyOctave = m_iOctave + 2;	break;	// Z
		case 88:	KeyNote = Cb;	KeyOctave = m_iOctave + 2;	break;	// X
		case 67:	KeyNote = D;	KeyOctave = m_iOctave + 2;	break;	// C
		case 86:	KeyNote = Db;	KeyOctave = m_iOctave + 2;	break;	// V
		case 66:	KeyNote = E;	KeyOctave = m_iOctave + 2;	break;	// B
		case 78:	KeyNote = F;	KeyOctave = m_iOctave + 2;	break;	// N
		case 77:	KeyNote = Fb;	KeyOctave = m_iOctave + 2;	break;	// M
		case 188:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// ,
		case 190:	KeyNote = Gb;	KeyOctave = m_iOctave + 2;	break;	// .
		case 189:	KeyNote = A;	KeyOctave = m_iOctave + 2;	break;	// -
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKey(unsigned char Key)
{
	// Translates a keyboard character into a MIDI note
	int	KeyNote = 0, KeyOctave = 0;

	// For modplug users
	if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE2)
		return TranslateKey2(Key);

	// Convert key to a note
	switch (Key) {
		case 50:	KeyNote = Cb;	KeyOctave = m_iOctave + 1;	break;	// 2
		case 51:	KeyNote = Db;	KeyOctave = m_iOctave + 1;	break;	// 3
		case 53:	KeyNote = Fb;	KeyOctave = m_iOctave + 1;	break;	// 5
		case 54:	KeyNote = Gb;	KeyOctave = m_iOctave + 1;	break;	// 6
		case 55:	KeyNote = Ab;	KeyOctave = m_iOctave + 1;	break;	// 7
		case 57:	KeyNote = Cb;	KeyOctave = m_iOctave + 2;	break;	// 9
		case 48:	KeyNote = Db;	KeyOctave = m_iOctave + 2;	break;	// 0
		case 81:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// Q
		case 87:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// W
		case 69:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// E
		case 82:	KeyNote = F;	KeyOctave = m_iOctave + 1;	break;	// R
		case 84:	KeyNote = G;	KeyOctave = m_iOctave + 1;	break;	// T
		case 89:	KeyNote = A;	KeyOctave = m_iOctave + 1;	break;	// Y
		case 85:	KeyNote = B;	KeyOctave = m_iOctave + 1;	break;	// U
		case 73:	KeyNote = C;	KeyOctave = m_iOctave + 2;	break;	// I
		case 79:	KeyNote = D;	KeyOctave = m_iOctave + 2;	break;	// O
		case 80:	KeyNote = E;	KeyOctave = m_iOctave + 2;	break;	// P
		case 221:	KeyNote = F;	KeyOctave = m_iOctave + 2;	break;	// �
		case 219:	KeyNote = Fb;	KeyOctave = m_iOctave + 2;	break;	// �
		//case 186:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// �
		case 83:	KeyNote = Cb;	KeyOctave = m_iOctave;		break;	// S
		case 68:	KeyNote = Db;	KeyOctave = m_iOctave;		break;	// D
		case 71:	KeyNote = Fb;	KeyOctave = m_iOctave;		break;	// G
		case 72:	KeyNote = Gb;	KeyOctave = m_iOctave;		break;	// H
		case 74:	KeyNote = Ab;	KeyOctave = m_iOctave;		break;	// J
		case 76:	KeyNote = Cb;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = Db;	KeyOctave = m_iOctave + 1;	break;	// �
		case 90:	KeyNote = C;	KeyOctave = m_iOctave;		break;	// Z
		case 88:	KeyNote = D;	KeyOctave = m_iOctave;		break;	// X
		case 67:	KeyNote = E;	KeyOctave = m_iOctave;		break;	// C
		case 86:	KeyNote = F;	KeyOctave = m_iOctave;		break;	// V
		case 66:	KeyNote = G;	KeyOctave = m_iOctave;		break;	// B
		case 78:	KeyNote = A;	KeyOctave = m_iOctave;		break;	// N
		case 77:	KeyNote = B;	KeyOctave = m_iOctave;		break;	// M
		case 188:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// ,
		case 190:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// .
		case 189:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// -
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

bool CFamiTrackerView::PreventRepeat(unsigned char Key, bool Insert)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!theApp.m_pSettings->General.bKeyRepeat || !Insert))
			return true;
	}

	return false;
}

void CFamiTrackerView::RepeatRelease(unsigned char Key)
{
	memset(m_cKeyList, 0, 256);
}

//
// Note preview
//

void CFamiTrackerView::PreviewNote(unsigned char Key)
{
	if (PreventRepeat(Key, false))
		return;

	int Note = TranslateKey(Key);

	if (Note > 0)
		TriggerMIDINote(m_iCursorChannel, Note, 0x7F, false);
}

void CFamiTrackerView::PreviewRelease(unsigned char Key)
{
	memset(m_cKeyList, 0, 256);

	int Note = TranslateKey(Key);

	if (Note > 0)
		ReleaseMIDINote(m_iCursorChannel, Note, false);
}

//
// MIDI handling routines
//

void CFamiTrackerView::TranslateMidiMessage()
{
	// Check and handle MIDI messages

	unsigned char Message, Channel, Note, Octave, Velocity;
	static unsigned char LastNote, LastOctave;
	CString Status;

	CMIDI *pMIDI = theApp.GetMIDI();

	if (!m_bInitialized)
		return;

	while (pMIDI->ReadMessage(Message, Channel, Note, Octave, Velocity)) {
		if (!theApp.m_pSettings->Midi.bMidiChannelMap)
			Channel = m_iCursorChannel;
		switch (Message) {
			case MIDI_MSG_NOTE_ON:
			//	if (Channel > 4)
			//		return;

				if (Velocity == 0)
					ReleaseMIDINote(Channel, MIDI_NOTE(Octave, Note), false);
				else
					TriggerMIDINote(Channel, MIDI_NOTE(Octave, Note), Velocity, true);

				Status.Format(_T("MIDI message: Note on (note = %02i, octave = %02i, velocity = %02X)"), Note, Octave, Velocity);
				SetMessageText(Status);
				break;

			case MIDI_MSG_NOTE_OFF:
		//		if (Channel > 4)
		//			return;

				ReleaseMIDINote(Channel, MIDI_NOTE(Octave, Note), false);

				Status.Format(_T("MIDI message: Note off"));
				SetMessageText(Status);
				break;
			
			case 0x0F:
				if (Channel == 0x08)
					StepDown();
				break;
		}
	}
}

//
// Effects menu
//

void CFamiTrackerView::InsertEffect(char Effect) 
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	if (!m_bEditEnable)
		return;

	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);

	Note.EffNumber[0] = Effect;
	Note.EffParam[0] = 0;

	pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
}

void CFamiTrackerView::OnTrackerToggleChannel()
{
	ToggleChannel(m_iCursorChannel);
}

void CFamiTrackerView::OnTrackerSoloChannel()
{
	SoloChannel(m_iCursorChannel);
}

void CFamiTrackerView::OnNextOctave()
{
	if (m_iOctave < 7)
		SetOctave(m_iOctave + 1);
}

void CFamiTrackerView::OnPreviousOctave()
{
	if (m_iOctave > 0)
		SetOctave(m_iOctave - 1);
}

void CFamiTrackerView::OnPasteOverwrite()
{
	int SavedMode = m_iPasteMode;
	m_iPasteMode = PASTE_MODE_OVERWRITE;
	OnEditPaste();
	m_iPasteMode = SavedMode;
}

void CFamiTrackerView::OnPasteMixed()
{
	int SavedMode = m_iPasteMode;
	m_iPasteMode = PASTE_MODE_MIX;
	OnEditPaste();
	m_iPasteMode = SavedMode;
}

void CFamiTrackerView::OnEditGradient()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int SelStart, SelEnd, SelColStart, SelColEnd, i;
	
	bool VolActive = true, EffActive[MAX_EFFECT_COLUMNS] = {true, true, true, true};

	float StartVol, EndVol, VolGrad;
	float StartEff[MAX_EFFECT_COLUMNS], EndEff[MAX_EFFECT_COLUMNS], EffGrad[MAX_EFFECT_COLUMNS];

	stChanNote Note;

	if (m_iSelectStart == -1)
		return;

	SelStart	= GetMin(m_iSelectStart, m_iSelectEnd);
	SelEnd		= GetMax(m_iSelectStart, m_iSelectEnd);
	SelColStart	= GetMin(m_iSelectColStart, m_iSelectColEnd);
	SelColEnd	= GetMax(m_iSelectColStart, m_iSelectColEnd);

	// Start values
	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, SelStart, &Note);
	
	StartVol = (float)Note.Vol;

	if (StartVol == 16)
		VolActive = false;
	for (i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		StartEff[i] = (float)Note.EffParam[i];
		if (Note.EffNumber[i] == EF_NONE)
			EffActive[i] = false;
	}

	// End values
	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, SelEnd, &Note);
	EndVol = (float)Note.Vol;
	if (EndVol == 16)
		VolActive = false;
	for (i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		EndEff[i] = (float)Note.EffParam[i];
		if (Note.EffNumber[i] == EF_NONE)
			EffActive[i] = false;
	}

	// Create delta values
	VolGrad = (EndVol - StartVol) / float(SelEnd - SelStart);
	for (i = 0; i < MAX_EFFECT_COLUMNS; i++)
		EffGrad[i] = (EndEff[i] - StartEff[i]) / float(SelEnd - SelStart);

	// Calculate
	for (int i = SelStart; i < SelEnd; i++) {
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);

		// Effects
		for (int x = 0; x < 4; x++) {
			if (IsColumnSelected(SelColStart, SelColEnd, x + 3) && EffActive[x] && Note.EffParam[x] != EF_NONE) {
				Note.EffParam[x] = (int)StartEff[x];
				StartEff[x] += EffGrad[x];
			}
		}

		// Volume
		if (IsColumnSelected(SelColStart, SelColEnd, COL_VOLUME) && VolActive) {
			Note.Vol = (int)StartVol;
			StartVol += VolGrad;
		}

		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
	}

	ForceRedraw();
}

void CFamiTrackerView::OnNextInstrument()
{
	SetInstrument(GetInstrument() + 1);
	((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
}

void CFamiTrackerView::OnPrevInstrument()
{
	SetInstrument(GetInstrument() - 1);
	((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
}

void CFamiTrackerView::SetOctave(unsigned int iOctave)
{
	m_iOctave = iOctave;
	((CMainFrame*)GetParentFrame())->DisplayOctave();
}

int CFamiTrackerView::GetCurrentChannelType()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorChannel < 5)
		return CHIP_NONE;
	else
		return pDoc->GetExpansionChip();
}

void CFamiTrackerView::OnKeyHome()
{
	if (m_bControlPressed) {
		MoveCursorToTop();
	}
	else {
		if (m_iCursorColumn != 0) {
			m_iCursorColumn = 0;
			ForceRedraw();
		}
		else if (m_iCursorChannel != 0) {
			m_iCursorChannel = 0;
			ForceRedraw();
		}
		else {
			MoveCursorToTop();
		}
	}
}