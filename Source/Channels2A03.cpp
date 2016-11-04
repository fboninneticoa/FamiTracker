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

// This file handles playing of 2A03 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "SoundGen.h"
#include "ChannelHandler.h"

void CChannelHandler2A03::PlayNote(stChanNote *NoteData, int EffColumns)
{
	CInstrument2A03 *Inst;
	unsigned int NesFreq;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	int Instrument, Volume, LastInstrument;
	bool Sweeping = false;
	int NewNote;
	//char NewDuty = 0;

	int	InitVolume = 0x0F;

	if (HandleDelay(NoteData, EffColumns))
		return;

	if (!NoteData)
		return;

	Note		= NoteData->Note;
	Octave		= NoteData->Octave;
	Volume		= NoteData->Vol;
	Instrument	= NoteData->Instrument;

	LastInstrument = m_iInstrument;

	if (Note == HALT || Note == RELEASE) {
		Instrument	= MAX_INSTRUMENTS;
		Volume		= 0x10;
		Octave		= 0;
	}

//	m_iDefaultDuty = 0;

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffNum	 = NoteData->EffNumber[n];
		int EffParam = NoteData->EffParam[n];

		if (!CheckCommonEffects(EffNum, EffParam)) {
			switch (EffNum) {
				case EF_VOLUME:
					InitVolume = EffParam;
					if (Note == 0)
						m_iOutVol = InitVolume;
					break;
				case EF_SWEEPUP: {
					Sweep = 0x88 | EffParam;
					m_iLastFrequency = 0xFFFF;
					Sweeping = true;
					break;
					}
				case EF_SWEEPDOWN: {
					Sweep = 0x80 | EffParam;
					m_iLastFrequency = 0xFFFF;
					Sweeping = true;
					break;
					}
				case EF_DUTY_CYCLE:
					m_iDefaultDuty = m_cDutyCycle = /*NewDuty =*/ EffParam;
					break;

#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1) //((((x & 0xF0) >> 4) + 1) | ((x & 0x80) >> 2))

				case EF_SLIDE_UP:
					m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
					m_iEffect = EF_SLIDE_UP;
					m_iNote = m_iNote + (EffParam & 0xF);
					m_iPortaTo = TriggerNote(m_iNote);
					break;
				case EF_SLIDE_DOWN:
					m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
					m_iEffect = EF_SLIDE_DOWN;
					m_iNote = m_iNote - (EffParam & 0xF);
					m_iPortaTo = TriggerNote(m_iNote);
					break;
/*
				case EF_SLIDE_UP:
					m_iPortaSpeed = (EffParam & 0xF0) >> 4;
					m_iEffect = EF_PORTAMENTO;
					m_iPortaTo = TriggerNote(m_iNote + (EffParam & 0xF));
					break;

				case EF_SLIDE_DOWN:
					m_iPortaSpeed = (EffParam & 0xF0) >> 4;
					m_iEffect = EF_PORTAMENTO;
					m_iPortaTo = TriggerNote(m_iNote - (EffParam & 0xF));
					break;*/
			}
		}
	}

	// Change instrument
	if (Instrument != LastInstrument) {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = LastInstrument;
		else
			LastInstrument = Instrument;

		if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
			return;
		if (Inst->GetType() != INST_2A03)
			return;

		for (int i = 0; i < MOD_COUNT; i++) {
			if (ModIndex[i] != Inst->GetModIndex(i)) {
				ModEnable[i]	= Inst->GetModEnable(i);
				ModIndex[i]		= Inst->GetModIndex(i);
				ModPointer[i]	= 0;
				ModDelay[i]		= 1;
			}
		}

		m_iInstrument = Instrument;
	}
	else {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = Instrument;

		if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
			return;
		if (Inst->GetType() != INST_2A03)
			return;
	}

	if (Volume < 0x10)
		m_iVolume = Volume;

	if (Note == 0) {
		if (Sweeping)
			m_cSweep = Sweep;
		return;
	}
	
	if (Note == HALT || Note == RELEASE) {
		KillChannel();
		return;
	}

	if (!Sweeping && (m_cSweep != 0 || Sweep != 0)) {
		Sweep = 0;
		m_cSweep = 0;
		m_iLastFrequency = 0xFFFF;
	}
	else if (Sweeping) {
		m_cSweep = Sweep;
		m_iLastFrequency = 0xFFFF;
	}

	// Trigger instrument
	for (int i = 0; i < MOD_COUNT; i++) {
		ModEnable[i]	= Inst->GetModEnable(i);
		ModIndex[i]		= Inst->GetModIndex(i);
		ModPointer[i]	= 0;
		ModDelay[i]		= 1;
	}

	// And note
	NewNote = MIDI_NOTE(Octave, Note);//(Note - 1) + (Octave * 12);
	NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iFrequency == 0)
			m_iFrequency = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iFrequency = NesFreq;

	m_cDutyCycle	= /*NewDuty*/ m_iDefaultDuty /*0*/;
	m_iNote			= NewNote;
	m_iOutVol		= InitVolume;
	m_bEnabled		= true;

	if (m_iEffect == EF_SLIDE_UP || m_iEffect == EF_SLIDE_DOWN)
		m_iEffect = EF_NONE;

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffNum	 = NoteData->EffNumber[n];
		int EffParam = NoteData->EffParam[n];

		if (!CheckCommonEffects(EffNum, EffParam)) {
			switch (EffNum) {
				case EF_SLIDE_UP:
					m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
					m_iEffect = EF_SLIDE_UP;
					m_iNote = m_iNote + (EffParam & 0xF);
					m_iPortaTo = TriggerNote(m_iNote);
					break;
				case EF_SLIDE_DOWN:
					m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
					m_iEffect = EF_SLIDE_DOWN;
					m_iNote = m_iNote - (EffParam & 0xF);
					m_iPortaTo = TriggerNote(m_iNote);
					break;
			}
		}
	}
}

void CChannelHandler2A03::ProcessChannel()
{
	if (!m_bEnabled && m_iChannelID != 4)
		return;

	// Default effects
	CChannelHandler::ProcessChannel();
	
	// Skip when DPCM
	if (m_iChannelID == 4)
		return;

	// Sequences
	for (int i = 0; i < MOD_COUNT; i++) {
		RunSequence(i, m_pDocument->GetSequence(ModIndex[i], i));
	}
}

void CChannelHandler2A03::RunSequence(int Index, CSequence *pSequence)
{
	int Value;

	if (ModEnable[Index]) {

		Value = pSequence->GetItem(ModPointer[Index]);

		switch (Index) {
			// Volume modifier
			case MOD_VOLUME:
				m_iOutVol = Value;
				break;
			// Arpeggiator
			case MOD_ARPEGGIO:
				m_iFrequency = TriggerNote(m_iNote + Value);
				break;
			// Hi-pitch
			case MOD_HIPITCH:
				m_iFrequency += Value << 4;
				LimitFreq();
				break;
			// Pitch
			case MOD_PITCH:
				m_iFrequency += Value;
				LimitFreq();
				break;
			// Duty cycling
			case MOD_DUTYCYCLE:
				m_cDutyCycle = Value;
				break;
		}

		ModPointer[Index]++;

		if (ModPointer[Index] == pSequence->GetItemCount()) {
			if (pSequence->GetLoopPoint() != -1) {
				// Loop
				ModPointer[Index] = pSequence->GetLoopPoint();
			}
			else {
				// End of sequence
				ModEnable[Index] = 0;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare1Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4000, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4001, m_cSweep);
			m_pAPU->Write(0x4002, HiFreq);
			m_pAPU->Write(0x4003, LoFreq);
			m_cSweep &= 0x7F;
			// Immediately trigger one sweep cycle (by request)
			m_pAPU->Write(0x4017, 0x80);
			m_pAPU->Write(0x4017, 0x00);
		}
	}
	else {
		m_pAPU->Write(0x4001, 0x08);
		// This one was tricky. Manually execute one APU frame sequence to kill the sweep unit
		m_pAPU->Write(0x4017, 0x80);
		m_pAPU->Write(0x4017, 0x00);
		m_pAPU->Write(0x4002, HiFreq);
		
		if (LoFreq != LastLoFreq)
			m_pAPU->Write(0x4003, LoFreq);
	}
}

void CSquare1Chan::ClearRegisters()
{
	m_pAPU->Write(0x4000, 0);
	m_pAPU->Write(0x4001, 0);
	m_pAPU->Write(0x4002, 0);
	m_pAPU->Write(0x4003, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare2Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4004, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4005, m_cSweep);
			m_pAPU->Write(0x4006, HiFreq);
			m_pAPU->Write(0x4007, LoFreq);
			m_cSweep &= 0x7F;
			// Immediately trigger one sweep cycle (by request)
			m_pAPU->Write(0x4017, 0x80);
			m_pAPU->Write(0x4017, 0x00);
		}
	}
	else {
		m_pAPU->Write(0x4005, 0x08);
		// This one was tricky. Manually execute one APU frame sequence to kill the sweep unit
		m_pAPU->Write(0x4017, 0x80);
		m_pAPU->Write(0x4017, 0x00);
		m_pAPU->Write(0x4006, HiFreq);
		
		if (LoFreq != LastLoFreq)
			m_pAPU->Write(0x4007, LoFreq);
	}
}

void CSquare2Chan::ClearRegisters()
{
	m_pAPU->Write(0x4004, 0);
	m_pAPU->Write(0x4005, 0);
	m_pAPU->Write(0x4006, 0);
	m_pAPU->Write(0x4007, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTriangleChan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (DutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	if (m_iOutVol)
		m_pAPU->Write(0x4008, 0xC0);
	else
		m_pAPU->Write(0x4008, 0x00);

	m_pAPU->Write(0x4009, 0x00);
	m_pAPU->Write(0x400A, HiFreq);
	m_pAPU->Write(0x400B, LoFreq);
}

void CTriangleChan::ClearRegisters()
{
	m_pAPU->Write(0x4008, 0);
	m_pAPU->Write(0x4009, 0);
	m_pAPU->Write(0x400A, 0);
	m_pAPU->Write(0x400B, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CNoiseChan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char NoiseMode;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	NoiseMode	= (m_cDutyCycle & 0x01) << 7;
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x400C, 0x30 | Volume);
	m_pAPU->Write(0x400D, 0x00);
	m_pAPU->Write(0x400E, ((HiFreq & 0x0F) ^ 0x0F) | NoiseMode);
	m_pAPU->Write(0x400F, 0x00);
}

void CNoiseChan::ClearRegisters()
{
	m_pAPU->Write(0x400C, 0x30);
	m_pAPU->Write(0x400D, 0);
	m_pAPU->Write(0x400E, 0);
	m_pAPU->Write(0x400F, 0);	
}

unsigned int CNoiseChan::TriggerNote(int Note)
{
	theApp.RegisterKeyState(m_iChannelID, Note);
	return Note;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// DPCM
///////////////////////////////////////////////////////////////////////////////////////////////////////////

int m_iOffset;

void CDPCMChan::RefreshChannel()
{
	unsigned char HiFreq;

	if (m_cDAC != 255) {
		m_pAPU->Write(0x4011, m_cDAC);
		m_cDAC = 255;
	}

	if (!m_bEnabled)
		return;

	HiFreq	= (m_iFrequency & 0xFF);

	m_pAPU->Write(0x4010, 0x00 | (HiFreq & 0x0F) | m_iLoop);
	m_pAPU->Write(0x4012, m_iOffset /*0x00*/);			// load address, start at $C000
	m_pAPU->Write(0x4013, (Length >> 4) - (m_iOffset << 2) /*/ 16*/);		// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);			// fire sample

	m_bEnabled = false;		// don't write to this channel anymore
}

void CDPCMChan::ClearRegisters()
{
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4010, 0);
	
	if (!m_bKeyRelease || !theApp.m_pSettings->General.bNoDPCMReset)
		m_pAPU->Write(0x4011, 0);		// regain full volume for TN

	m_pAPU->Write(0x4012, 0);
	m_pAPU->Write(0x4013, 0);

	m_iOffset = 0;
}

void CDPCMChan::PlayNote(stChanNote *NoteData, int EffColumns)
{
	unsigned int Note, Octave, SampleIndex, LastInstrument;
	CInstrument2A03 *Inst;

	if (HandleDelay(NoteData, EffColumns))
		return;

	Note	= NoteData->Note;
	Octave	= NoteData->Octave;

//	bool bNoOffset = true;

	for (int i = 0; i < EffColumns; i++) {
		if (NoteData->EffNumber[i] == EF_DAC) {
			m_cDAC = NoteData->EffParam[i] & 0x7F;
		}
		else if (NoteData->EffNumber[i] == EF_SAMPLE_OFFSET) {
			m_iOffset = NoteData->EffParam[i];
	//		bNoOffset = false;
		}
	}

	if (Note == 0)
		return;

	//if (bNoOffset)
	//	m_iOffset = 0;

	if (Note == RELEASE)
		m_bKeyRelease = true;
	else
		m_bKeyRelease = false;

	if (Note == HALT || Note == RELEASE) {
		KillChannel();
		return;
	}

	LastInstrument = m_iInstrument;

	if (NoteData->Instrument != 0x40)
		m_iInstrument = NoteData->Instrument;

	if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(m_iInstrument)) == NULL)
		return;

	if (Inst->GetType() != INST_2A03)
		return;

	// Change instrument
	if (NoteData->Instrument != m_iLastInstrument) {
		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = LastInstrument;
		else
			LastInstrument = NoteData->Instrument;

		m_iInstrument = NoteData->Instrument;
	}
	else {
		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = NoteData->Instrument;
	}

	SampleIndex = Inst->GetSample(Octave, Note - 1);

	if (SampleIndex > 0) {
		int Pitch = Inst->GetSamplePitch(Octave, Note - 1);
		if (Pitch & 0x80)
			m_iLoop = 0x40;
		else
			m_iLoop = 0;

		CDSample *DSample = m_pDocument->GetDSample(SampleIndex - 1);

		int SampleSize = DSample->SampleSize;

		m_pSampleMem->SetMem(DSample->SampleData, SampleSize);
		Length = SampleSize;		// this will be adjusted
		m_iFrequency = Pitch & 0x0F;
		m_bEnabled = true;
	}

	theApp.RegisterKeyState(m_iChannelID, (Note - 1) + (Octave * 12));
}

void CDPCMChan::SetSampleMem(CSampleMem *pSampleMem)
{
	m_pSampleMem = pSampleMem;
}