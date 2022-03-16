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
#define LOGIC2
#include <iostream>
#include <fstream>
#include <algorithm>
#include <AnalyzerHelpers.h>
#include "iso7816AnalyzerResults.h"
#include "iso7816Analyzer.h"
#include "iso7816AnalyzerSettings.h"

iso7816AnalyzerResults::iso7816AnalyzerResults( iso7816Analyzer* analyzer, iso7816AnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

iso7816AnalyzerResults::~iso7816AnalyzerResults()
{
}

void iso7816AnalyzerResults::AddProtocolFrame(ProtocolFrame::ptr frame, const char* str)
{
	_frames.push_back(frame);
	AddFrame(*(frame.get()));
	// New FrameV2 code.
	FrameV2 frame_v2;
// you can add any number of key value pairs. Each will get it's own column in the data table.
	frame_v2.AddString( "t1", str );
// This actually saves your new FrameV2. In this example, we just copy the same start and end sample number from Frame V1 above.
// The second parameter is the frame "type". Any string is allowed.
	AddFrameV2( frame_v2, "t1", frame.get()->mStartingSampleInclusive, frame.get()->mEndingSampleInclusive );


	CommitResults();
}


void iso7816AnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();

	Frame frame = GetFrame(frame_index);
	ProtocolFrame::ptr _frame = FindProtocolFrame(frame.mData1);
	if (_frame)
	{
		_frame->RenderBubbleText(this, channel, display_base);
	}
}

void iso7816AnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void iso7816AnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddResultString( number_str );
}

void iso7816AnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void iso7816AnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

ProtocolFrame::ptr iso7816AnalyzerResults::FindProtocolFrame(U64 mData1)
{
	auto it = std::find_if(_frames.begin(), _frames.end(), [=](ProtocolFrame::ptr i) {return i->mData1 == mData1; });
	return (it == _frames.end()) ? ProtocolFrame::ptr() : (*it);
}
