//
// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#include "Iso7816BitDecoder.h"
#include "SaleaeHelper.hpp"
#include "Exceptions.hpp"
#include "Logging.hpp"

Iso7816BitDecoder::ptr Iso7816BitDecoder::factory(AnalyzerChannelData* io, AnalyzerChannelData* reset, AnalyzerChannelData* vcc, AnalyzerChannelData* clk)
{
	Iso7816BitDecoder::ptr ret(new Iso7816BitDecoder(io, reset, vcc, clk));
	return ret;
}

Iso7816BitDecoder::Iso7816BitDecoder(AnalyzerChannelData* io, AnalyzerChannelData* reset, AnalyzerChannelData* vcc, AnalyzerChannelData* clk)
{
	_io = io;
	_reset = reset;
	_vcc = vcc;
	_clk = clk;
}

Iso7816BitDecoder::~Iso7816BitDecoder()
{
}

void Iso7816BitDecoder::Sync(u64 pos)
{
	SaleaeHelper::AdvanceToAbsPositionOrThrow(_io, pos, std::string("I/O"));
	SaleaeHelper::AdvanceToAbsPositionOrThrow(_reset, pos, std::string("RESET"));
	SaleaeHelper::AdvanceToAbsPositionOrThrow(_vcc, pos, std::string("Vcc"));
	SaleaeHelper::AdvanceToAbsPositionOrThrow(_clk, pos, std::string("CLK"));
}

Iso7816BitDecoder::u64 Iso7816BitDecoder::SeekForResetEdge(bool& high)
{
	Logging::Write(std::string("Looking for RST going high..."));
	_reset->AdvanceToNextEdge();

	high = _reset->GetBitState() == BIT_HIGH;
	return _reset->GetSampleNumber();
}

Iso7816BitDecoder::u64 Iso7816BitDecoder::SeekForIoFallingEdge()
{
	// ensure line is HIGH
	while (_io->GetBitState() != BIT_HIGH)
	{
		AdvanceToNextEdgeWithResetDetection(_io);
	}
	// if so the next edge is falling down
	AdvanceToNextEdgeWithResetDetection(_io);
	return _io->GetSampleNumber();
}

Iso7816BitDecoder::u64 Iso7816BitDecoder::SkipClkCycles(std::size_t cycles)
{
	for (std::size_t i = 0; i < cycles; i++)
	{
		AdvanceToNextEdgeWithResetDetection(_clk);
		AdvanceToNextEdgeWithResetDetection(_clk);
	}
	return _clk->GetSampleNumber();
}

Iso7816BitDecoder::u64 Iso7816BitDecoder::AdvanceToNextIoEdge()
{
	AdvanceToNextEdgeWithResetDetection(_io);
	return _io->GetSampleNumber();
}

BitState Iso7816BitDecoder::GetIoState()
{
	return _io->GetBitState();
}

Iso7816BitDecoder::u64 Iso7816BitDecoder::GetIoPosition()
{
	return _io->GetSampleNumber();
}

std::size_t Iso7816BitDecoder::CountClkCyclesToPosition(u64 pos)
{
	std::size_t ret = 0;
	while (_clk->GetSampleOfNextEdge() < pos)
	{
		AdvanceToNextEdgeWithResetDetection(_clk);
		AdvanceToNextEdgeWithResetDetection(_clk);
		ret++;
	}
	return ret;
}


void Iso7816BitDecoder::AdvanceToNextEdgeWithResetDetection(AnalyzerChannelData* channel)
{
	u64 pos = channel->GetSampleOfNextEdge();
	if (_reset->WouldAdvancingToAbsPositionCauseTransition(pos))
	{
		throw ResetException(_reset->GetSampleOfNextEdge());
	}
	channel->AdvanceToNextEdge();
}