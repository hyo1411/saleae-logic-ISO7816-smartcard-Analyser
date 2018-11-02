//
// Copyright © 2013 Dirk-Willem van Gulik <dirkx@webweaving.org>, all rights reserved.
// Copyright © 2016 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this 
// file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under 
// the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF 
// ANY KIND, either express or implied. See the License for the specific language 
// governing permissions and limitations under the License.
//


#include <algorithm>
#include "iso7816Analyzer.h"
#include "iso7816AnalyzerSettings.h"

#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>
#include <AnalyzerResults.h>
#include "Logging.hpp"
#include "Convert.hpp"
#include "Util.h"
#include "Exceptions.hpp"
#include "SaleaeHelper.hpp"
#include "Definitions.hpp"
#include "ISO7816Pps.hpp"

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

	Iso7816BitDecoder::ptr decoder = Iso7816BitDecoder::factory(mIo, mReset, mVcc, mClk);

	int resetCounter = 0;
	for (; ; )
	{
		try {
			// seek for a RESET going high.
			Logging::Write(std::string("Looking for RST going high..."));

			U64 fallingIoEdge = 0;
			bool high = false;
			U64 pos = decoder->SeekForResetEdge(high);
			resetCounter++;

			{
				std::string msg = std::string("R:") + Convert::ToDec(resetCounter);
				Logging::Write(msg);
				ProtocolFrame::ptr frame = TextFrame::factory(mSettings->mResetChannel.mChannelIndex, msg, pos, pos + 100);
				mResults->AddProtocolFrame(frame);
			}

			if (!high)
			{
				Logging::Write(std::string("Not this time..."));
				continue;
			}

			// log event
			AddMarker(pos, AnalyzerResults::UpArrow, mSettings->mResetChannel);
			LogEvent(pos, std::string("Reset detected"));

			// discard all serial data until now.
			decoder->Sync(pos);

			// 6.2.2 Cold reset
			Iso7816Session::ptr session;
			while (true)
			{
				/*	At time Tb, RST is put to state H. The answer on I/O shall begin between 400 and 40 000 clock cycles (delay
					tc) after the rising edge of the signal on RST (at time Tb + tc). If the answer does not begin within 40 000 clock
					cycles with RST at state H, the interface device shall perform a deactivation.
				*/
				pos = decoder->AdvanceClkCycles(400);
				decoder->Sync(pos);

				// search for first start bit - falling edge
				LogEvent(pos, std::string("Seeking for start bit..."));
				fallingIoEdge = decoder->SeekForIoFallingEdge();
				DumpLines();
				decoder->Sync(fallingIoEdge);
				LogEvent(fallingIoEdge, std::string("Falling I/O edge found"));

				// sync lines
				U64 risingIoEdge = decoder->AdvanceToNextIoEdge();
				LogEvent(risingIoEdge, std::string("Rising I/O edge found"));
				DumpLines();

				// We can use the first up/down dip to measure the baud rate.
				U64 defaultEtu = decoder->CountClkCyclesToPosition(risingIoEdge);
				LogEvent(fallingIoEdge, std::string("Found the start bit, initial ETU: ") + Convert::ToDec(defaultEtu) + std::string(" clocks..."));

				// default ETU shoud be 372 
				if (!IsValidETU(defaultEtu))
				{
					LogEvent(fallingIoEdge, std::string("This is not a valid start bit: ") + Convert::ToDec(defaultEtu) + std::string(" clocks..."));
					session.reset();
					continue;
				}

				// sync lines at the I/O rising edge
				DumpLines();
				decoder->Sync(risingIoEdge);


				session = Iso7816Session::factory(mResults, defaultEtu, mSettings->mIoChannel.mChannelIndex, mSettings->mResetChannel.mChannelIndex);

				AddMarker(fallingIoEdge, AnalyzerResults::DownArrow, mSettings->mIoChannel);
				AddMarker(fallingIoEdge + ((risingIoEdge - fallingIoEdge) / 2), AnalyzerResults::Start, mSettings->mIoChannel);
				AddMarker(risingIoEdge, AnalyzerResults::UpArrow, mSettings->mIoChannel);				
				break;
			}

			// decode TS byte
			unsigned char data = DecodeByte(decoder, session, true);

			U64 endOfByte = decoder->GetIoPosition();
			decoder->Sync(endOfByte);
			session->PushByte(static_cast<unsigned char>(data & 0xFF), fallingIoEdge, endOfByte);

			// now we keep waiting for the next 'down'; start bit
			// and then read our 10 bits, etc, etc.
			for (;;)
			{
				try
				{
					SeekForNextStartBit(decoder, session);
					U64 startPos = decoder->GetIoPosition();
					unsigned char bt = DecodeByte(decoder, session);

					session->PushByte(bt, startPos, decoder->GetIoPosition());
				}
				catch (OutOfSyncException& ex)
				{
					AddMarker(ex.getPosition(), AnalyzerResults::ErrorDot, mSettings->mIoChannel);
					LogEvent(ex.getPosition(), std::string("Out of sync with start bit."));
					continue;
				}
				catch (ParityException& ex)
				{
					AddMarker(ex.getPosition(), AnalyzerResults::ErrorDot, mSettings->mIoChannel);
					LogEvent(ex.getPosition(), std::string("Parity error"));
					break;
				}
				catch (ErrorSignalException& ex)
				{
					LogEvent(ex.getPosition(), std::string("Stop bit not high."));
					AddMarker(ex.getPosition(), AnalyzerResults::ErrorDot, mSettings->mIoChannel);
					break;
				}
			}
		}
		catch (ResetException& ex)
		{
			LogEvent(ex.getPosition(), std::string("Found RESET line change"));
		}
		catch (std::exception& ex2)
		{
			LogEvent(0, ex2.what());
		}
	}
}

void iso7816Analyzer::SeekForNextStartBit(Iso7816BitDecoder::ptr decoder, Iso7816Session::ptr session)
{
	// falling edge -- beginning of the start bit
	U64 fallingIoEdge = decoder->SeekForIoFallingEdge();
	decoder->Sync(fallingIoEdge);
	AddMarker(fallingIoEdge, AnalyzerResults::DownArrow, mSettings->mIoChannel);
	U64 endOfStartBit = decoder->AdvanceClkCycles(session->GetEtu());
	AddMarker(fallingIoEdge + ((endOfStartBit - fallingIoEdge) / 2), AnalyzerResults::Start, mSettings->mIoChannel);
	AddMarker(endOfStartBit, AnalyzerResults::UpArrow, mSettings->mIoChannel);
	LogEvent(fallingIoEdge, std::string("Found a new start bit"));
	decoder->Sync(endOfStartBit);
}


unsigned char iso7816Analyzer::DecodeByte(Iso7816BitDecoder::ptr decoder, Iso7816Session::ptr session, bool initialTs)
{
	// advance to the middle of first bit
	U64 pos = decoder->AdvanceClkCycles(session->GetEtu() / 2);
	LogEvent(pos, std::string("Mooving to the middle of first bit"));
	decoder->Sync(pos);

	unsigned char data = 0;
	for (int i = 0; i <= 7; i++) {
		U8 bit = decoder->GetIoState() ? 1 : 0;
		AddMarker(pos, bit ? AnalyzerResults::One : AnalyzerResults::Zero, mSettings->mIoChannel);
		LogEvent(decoder->GetIoPosition(), std::string("Found bit: ") + Convert::ToDec(bit ? 1 : 0));

		// next bit
		pos = decoder->AdvanceClkCycles(session->GetEtu());
		decoder->Sync(pos);
		LogEvent(pos, std::string("Advanced to the next bit"));

		data = (data << 1) | bit;
	};

	// now we are right on parity bit
	bool p = decoder->GetIoState() == BitState::BIT_HIGH ? true : false;

	LogEvent(pos, std::string("Before parity check..."));
	LogEvent(pos, std::string("Data: ") + Convert::ToHex(data));
	LogEvent(pos, std::string("Parity: ") + (p ? std::string("true") : std::string("false")));

	bool expectedParity = initialTs ? initialTs : Util::Parity(data);
	AddMarker(pos, expectedParity != p ? AnalyzerResults::ErrorX : AnalyzerResults::X, mSettings->mIoChannel);

	// 7.3 Error signal and character repetition
	/*	As shown in Figure 9, when character parity is incorrect, the receiver shall transmit an error signal on the
		electrical circuit I/O. Then the receiver shall expect a repetition of the character.
	*/
	pos = decoder->AdvanceClkCycles(session->GetEtu());
	decoder->Sync(pos);
	if (mIo->GetBitState() != BIT_HIGH)
	{
		throw ErrorSignalException(mIo->GetSampleNumber());
	};

	mResults->AddMarker(pos, AnalyzerResults::Stop, mSettings->mIoChannel);
	return static_cast<unsigned char>(data);
}

bool iso7816Analyzer::IsValidETU(U64 ea)
{
	return ea > DEF_ETU_MIN && ea < DEF_ETU_MAX;
}

void iso7816Analyzer::LogEvent(U64 position, const std::string& msg)
{
	Logging::Write(std::string("[") + Convert::ToDec(position) + std::string("] ") + msg);
}

void iso7816Analyzer::LogEvent(U64 position, const char* msg)
{
    Logging::Write(std::string("[") + Convert::ToDec(position) + std::string("] ") + std::string(msg));
}

void iso7816Analyzer::AddMarker(U64 position, AnalyzerResults::MarkerType mt, Channel& channel)
{
	mResults->AddMarker(position, mt, channel);
	mResults->CommitResults();
}

void iso7816Analyzer::DumpLines()
{
	std::string msg =
		std::string("I/O: ") + Convert::ToDec(mIo->GetSampleNumber()) + std::string("(") + (mIo->GetBitState() == BIT_HIGH ? "1" : "0") + std::string("), ") +
		std::string("RST: ") + Convert::ToDec(mReset->GetSampleNumber()) + std::string("(") + (mReset->GetBitState() == BIT_HIGH ? "1" : "0") + std::string("), ") +
		std::string("CLK: ") + Convert::ToDec(mClk->GetSampleNumber()) + std::string("(") + (mClk->GetBitState() == BIT_HIGH ? "1" : "0") + std::string("), ") +
		std::string("Vcc: ") + Convert::ToDec(mVcc->GetSampleNumber()) + std::string("(") + (mVcc->GetBitState() == BIT_HIGH ? "1" : "0") + std::string(")");
    
	LogEvent(mIo->GetSampleNumber(), msg);
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
