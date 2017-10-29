//
// Copyright © 2013 Dirk-Willem van Gulik <dirkx@webweaving.org>, all rights reserved.
// Copyright © 2016 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//


#include <algorithm>
#include "iso7816Analyzer.h"
#include "iso7816AnalyzerSettings.h"

#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>
#include <AnalyzerResults.h>
#include "Logging.hpp"
#include "Convert.hpp"
#include "SaleaeHelper.hpp"
#include "Definitions.hpp"
#include "ISO7816Pps.hpp"
#include "Iso7816Session.h"

static const bool parity[256] = 
{
#   define P2(n) n, n^1, n^1, n
#   define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#   define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
    P6(0), P6(1), P6(1), P6(0)
};

iso7816Analyzer::iso7816Analyzer()
:	Analyzer2(),  
	mSettings( new iso7816AnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

iso7816Analyzer::~iso7816Analyzer()
{
	KillThread();
}


void iso7816Analyzer::WorkerThread()
{
	try
	{
		Logging::Write("Start");
		_WorkerThread();
		Logging::Write("Stop");
	}
	catch (std::exception &e)
	{
		Logging::Write(std::string("[Exception] ") + e.what());
	}
	catch (...)
	{
		Logging::Write("[Exception] Unknown error");
	}
}

void iso7816Analyzer::SetupResults()
{
	mResults.reset(new iso7816AnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mIoChannel);
	mResults->AddChannelBubblesWillAppearOn(mSettings->mResetChannel);

	Logging::Write(std::string("SimulationSampleRate: ") + Convert::ToDec(GetSimulationSampleRate()));
	Logging::Write(std::string("SampleRate: ") + Convert::ToDec(GetSampleRate()));
	Logging::Write(std::string("TriggerSample: ") + Convert::ToDec(GetTriggerSample()));
}

void iso7816Analyzer::_WorkerThread()
{
	mIo = GetAnalyzerChannelData( mSettings->mIoChannel );
	mReset = GetAnalyzerChannelData( mSettings->mResetChannel );
	mVcc = GetAnalyzerChannelData(mSettings->mVccChannel);
	mClk = GetAnalyzerChannelData(mSettings->mClkChannel);

	for( ; ; )
	{
		// seek for a RESET going high.
		//
		Logging::Write(std::string("Looking for RST going high..."));
		mReset->AdvanceToNextEdge();

		if (mReset->GetBitState() != BIT_HIGH)
		{
			Logging::Write(std::string("Not this time..."));
			continue;
		}

		// discard all serial data until now.
		U64 reset = mReset->GetSampleNumber();
		Logging::Write(std::string("Reset detected: ") + Convert::ToDec(reset));
		SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, reset, std::string("I/O"));

		// mark the start in pretty much all channels.
		//
		// mResults->AddMarker(mReset->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mResetChannel);
		// mResults->AddMarker(mIo->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mIoChannel);

		// We know expect a TS (After some 400 to 40k clock cycles of idleness XX).
		// We can use the first up/down dip to measure the baud rate.
		//
		//  S01234567P     S01234567P
		// HLHHLLLLLLH or HLHHLHHHLLH
		//   11000000 1     11011100 1
 		//   xC0            xDC
		// inverse        direct convention
		//
		// search for first stop bit.
		//

		Iso7816Session::ptr _session;
		U64 a0 = 0;
		while (true)
		{
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mClk, mIo->GetSampleNumber(), std::string("CLK"));
			U64 startBit = SaleaeHelper::AdvanceClkCycles(mClk, 400);
			Logging::Write(std::string("Start bit seeking: ") + Convert::ToDec(startBit));
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, startBit, std::string("I/O"));
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, startBit, std::string("RST"));
			mResults->AddMarker(reset, AnalyzerResults::UpArrow, mSettings->mResetChannel);

			// seeking for falling edge
			while (mIo->GetBitState() != BIT_HIGH)
			{
				mIo->AdvanceToNextEdge();
			}

			mIo->AdvanceToNextEdge();
			a0 = mIo->GetSampleNumber();
			mIo->AdvanceToNextEdge();
			U64 a1 = mIo->GetSampleNumber();

			SaleaeHelper::AdvanceToAbsPositionOrThrow(mClk, a0, std::string("CLK"));
			U64 clocks = SaleaeHelper::AdvanceToAbsPositionOrThrow(mClk, a1, std::string("CLK")) / 2;
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, a1, std::string("RST"));

			// default ETU shoud be 372 
			if (!IsValidETU(clocks))
			{
				Logging::Write(std::string("This is not a valid start bit: ") + Convert::ToDec(clocks) + std::string(" clocks..."));
				mIo->AdvanceToNextEdge();
				_session.reset();
				continue;
			}
			_session = Iso7816Session::factory(mResults, clocks, mSettings->mIoChannel.mChannelIndex, mSettings->mResetChannel.mChannelIndex);

			mResults->AddMarker(a0, AnalyzerResults::DownArrow, mSettings->mIoChannel);
			mResults->AddMarker(a0 + (a1 - a0) / 2, AnalyzerResults::Start, mSettings->mIoChannel);
			mResults->AddMarker(a1, AnalyzerResults::UpArrow, mSettings->mIoChannel);
			Logging::Write(std::string("Found the start bit, initial ETU: ") + Convert::ToDec(clocks) + std::string(" clocks..."));
			break;
		}

		if (!_session || !IsValidETU(_session->GetEtu()))
		{
			Logging::Write("Valid start bit not found, skipinng analysis...");
			mIo->AdvanceToNextEdge();
			_session.reset();
			continue;
		}

		if (mIo->GetBitState() != BIT_HIGH) {
			mResults->AddMarker(mIo->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mIoChannel);
			mResults->CommitResults();
			Logging::Write("Start bit end error");
			continue;
		}


		U16 data = 0; 

		// move to the center of first data bit
		U64 bitPos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu() / 2);
		U64 pausePos = 0;
		for (U32 i = 0; i <= 8; i++)
		{
			// move to the center of what we're guessing as best as we can
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, bitPos, std::string("I/O"));
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, bitPos, std::string("RST"));
			U8 bit = mIo->GetBitState() ? 1 : 0;
			mResults->AddMarker(mIo->GetSampleNumber(), bit ? AnalyzerResults::One : AnalyzerResults::Zero, mSettings->mIoChannel);
			data = (data <<1) | bit;
			if (i == 8)
			{
				pausePos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu());
			}
			else
			{
				bitPos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu());
			}
		}
		bool p = 1 == (data & 1);
		data >>= 1;

		if (parity[data] != p) {
			mResults->AddMarker(bitPos, AnalyzerResults::ErrorDot, mSettings->mIoChannel);
			mResults->CommitResults();
			Logging::Write("Parity error");
			continue;
		};
		
		mResults->AddMarker(bitPos, AnalyzerResults::Stop, mSettings->mIoChannel);
		if (mIo->GetBitState() != BIT_HIGH) {
			Logging::Write("Stop bit not high.");
			mResults->AddMarker(pausePos, AnalyzerResults::ErrorDot, mSettings->mIoChannel);
			mResults->CommitResults();
			continue;
		};

		_session->PushByte(static_cast<unsigned char>(data & 0xFF), a0, mIo->GetSampleNumber());

		// now we keep waiting for the next 'down'; start bit
		// and then read our 10 bits, etc, etc.
		for(;;)
		{
			U64 _nextIo = mIo->GetSampleOfNextEdge();	//without moving, get the sample of the next transition.
			if (mReset->WouldAdvancingToAbsPositionCauseTransition(_nextIo))
			{
				Logging::Write(std::string("A new possible reset detected..."));
				break;
			}

			mIo->AdvanceToNextEdge(); //falling edge -- beginning of the start bit
			U64 _current = mIo->GetSampleNumber();

			if (mIo->GetBitState() != BIT_LOW) {
				mResults->AddMarker(mIo->GetSampleNumber(),
					AnalyzerResults::ErrorDot, mSettings->mIoChannel);
				mResults->CommitResults();
				Logging::Write(std::string("Out of sync with start bit."));
				continue;
			};

			U64 starting_sample = mIo->GetSampleNumber();
			// synchronise CLK position
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mClk, starting_sample, std::string("CLK"));

			U64 bitPos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu() / 2);
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, bitPos, std::string("I/O"));
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, bitPos, std::string("RST"));
			mResults->AddMarker(mIo->GetSampleNumber(), AnalyzerResults::Start, mSettings->mIoChannel);

			U16 data = 0;
			for(U32 i = 0; i <= 8; i++) {

				bitPos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu());
				SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, bitPos, std::string("I/O"));
				SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, bitPos, std::string("RST"));

				U8 bit = mIo->GetBitState() ? 1 : 0;
				mResults->AddMarker(mIo->GetSampleNumber(), bit ? AnalyzerResults::One : AnalyzerResults::Zero, mSettings->mIoChannel);

				data = (data <<1) | bit;
			};

			bool p = 1 == (data  & 1);
			data >>= 1;

			if (parity[ data ] != p) {
				mResults->AddMarker(mIo->GetSampleNumber(),AnalyzerResults::ErrorDot, mSettings->mIoChannel);
				mResults->CommitResults();
				Logging::Write(std::string("Parity error"));
				break;
			};

			bitPos = SaleaeHelper::AdvanceClkCycles(mClk, _session->GetEtu());
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mIo, bitPos, std::string("I/O"));
			SaleaeHelper::AdvanceToAbsPositionOrThrow(mReset, bitPos, std::string("RST"));
			if (mIo->GetBitState() != BIT_HIGH) {
				Logging::Write(std::string("Stop bit not high."));
				mResults->AddMarker(mIo->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mIoChannel);
				mResults->CommitResults();
				break;
			};

			mResults->AddMarker(mIo->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mIoChannel);

			_session->PushByte(static_cast<unsigned char>(data & 0xFF), starting_sample, mIo->GetSampleNumber());
		}
	}
}

bool iso7816Analyzer::IsValidETU(U64 ea)
{
	return ea > DEF_ETU_MIN && ea < DEF_ETU_MAX;
}

bool iso7816Analyzer::NeedsRerun()
{
	return false;
}

U32 iso7816Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 iso7816Analyzer::GetMinimumSampleRateHz()
{
	return 0;
}

const char* iso7816Analyzer::GetAnalyzerName() const
{
	return "ISO 7816 SmartCard";
}

const char* GetAnalyzerName()
{
	return "ISO 7816 SmartCard";
}

Analyzer* CreateAnalyzer()
{
	return new iso7816Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
