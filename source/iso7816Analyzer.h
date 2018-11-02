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

#ifndef ISO7816_ANALYZER_H
#define ISO7816_ANALYZER_H

#include <Analyzer.h>

#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>
#include <AnalyzerResults.h>

#include "iso7816AnalyzerResults.h"
#include "iso7816SimulationDataGenerator.h"
#include "ByteBuffer.hpp"
#include "ISO7816Pps.hpp"
#include "ISO7816Atr.hpp"
#include "ProtocolFrames.h"
#include "Iso7816Session.h"
#include "Iso7816BitDecoder.h"

typedef enum {
	DIRECT	= 0x02,
	INVERSE	= 0x04,
} iso7816_mode_t;

#ifdef WIN32
#pragma warning (push)
#pragma warning( disable: 4251 )
#endif //WIN32

class iso7816AnalyzerSettings;
class ANALYZER_EXPORT iso7816Analyzer : public Analyzer2
{
public:
	iso7816Analyzer();
	virtual ~iso7816Analyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

private:
	virtual void _WorkerThread();
	void SeekForNextStartBit(Iso7816BitDecoder::ptr decoder, Iso7816Session::ptr session);
	unsigned char DecodeByte(Iso7816BitDecoder::ptr decoder, Iso7816Session::ptr session, bool initialTs = false);
	bool IsValidETU(U64 ea);

	void LogEvent(U64 position, const std::string& msg);
    void LogEvent(U64 position, const char* msg);
	void AddMarker(U64 position, AnalyzerResults::MarkerType mt, Channel& channel);
	void DumpLines();

private: //vars
	std::unique_ptr<iso7816AnalyzerSettings> mSettings;
	iso7816AnalyzerResults::ptr mResults;

	bool ppsFound;
	bool apduStarted;
	ByteBuffer pps;
	ISO7816Atr::ptr _atr;

	AnalyzerChannelData* mIo;
	AnalyzerChannelData* mReset;
	AnalyzerChannelData* mVcc;
	AnalyzerChannelData* mClk;

	iso7816SimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;
};

#ifdef WIN32
#pragma warning (pop)
#endif //WIN32

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //ISO7816_ANALYZER_H
