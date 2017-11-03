//
// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#ifndef ISO7816_BIT_DECODER
#define ISO7816_BIT_DECODER

#include <memory>
#include <AnalyzerChannelData.h>

class Iso7816BitDecoder
{
public:
	typedef std::shared_ptr<Iso7816BitDecoder> ptr;
	typedef unsigned long long int u64;

public:
	static Iso7816BitDecoder::ptr factory(AnalyzerChannelData* io, AnalyzerChannelData* reset, AnalyzerChannelData* vcc, AnalyzerChannelData* clk);
	virtual ~Iso7816BitDecoder();

	void Sync(u64 pos);
	u64 SeekForResetEdge(bool& high);
	u64 SeekForIoFallingEdge();
	u64 SkipClkCycles(std::size_t cycles);
	u64 AdvanceToNextIoEdge();
	BitState GetIoState();
	u64 GetIoPosition();
	std::size_t CountClkCyclesToPosition(u64 pos);

protected:
	Iso7816BitDecoder(AnalyzerChannelData* io, AnalyzerChannelData* reset, AnalyzerChannelData* vcc, AnalyzerChannelData* clk);

	void AdvanceToNextEdgeWithResetDetection(AnalyzerChannelData* channel);

	AnalyzerChannelData* _io;
	AnalyzerChannelData* _reset;
	AnalyzerChannelData* _vcc;
	AnalyzerChannelData* _clk;
};

#endif //ISO7816_BIT_DECODER
