//
// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#include <memory>
#include <AnalyzerResults.h>
#include <AnalyzerHelpers.h>
#include "ProtocolFrames.h"


ProtocolFrame::ProtocolFrame(U32 mChannelIndex, S64 mStartingSample, S64 mEndingSample)
{
	this->mData1 = reinterpret_cast<U64>(this);

	this->mStartingSampleInclusive = mStartingSample;
	this->mEndingSampleInclusive = mEndingSample;
	this->_channelIndex = mChannelIndex;
}


ProtocolFrame::ptr TextFrame::factory(U32 mChannelIndex, const std::string& strShort, S64 mStartingSample, S64 mEndingSample)
{
	ProtocolFrame::ptr ret(new TextFrame(mChannelIndex, strShort, mStartingSample, mEndingSample));
	return ret;
}
ProtocolFrame::ptr TextFrame::factory(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, S64 mStartingSample, S64 mEndingSample)
{
	ProtocolFrame::ptr ret(new TextFrame(mChannelIndex, strShort, strMidium, mStartingSample, mEndingSample));
	return ret;
}
ProtocolFrame::ptr TextFrame::factory(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, const std::string& strDetailed, S64 mStartingSample, S64 mEndingSample)
{
	ProtocolFrame::ptr ret(new TextFrame(mChannelIndex, strShort, strMidium, strDetailed, mStartingSample, mEndingSample));
	return ret;
}


TextFrame::TextFrame(U32 mChannelIndex, const std::string& strShort, S64 mStartingSample, S64 mEndingSample)
	: ProtocolFrame(mChannelIndex, mStartingSample, mEndingSample)
{
	this->mData1 = reinterpret_cast<U64>(this);
	_short = strShort;
}
TextFrame::TextFrame(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, S64 mStartingSample, S64 mEndingSample)
	: ProtocolFrame(mChannelIndex, mStartingSample, mEndingSample)
{
	_short = strShort;
	_midium = strMidium;
}
TextFrame::TextFrame(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, const std::string& strDetailed, S64 mStartingSample, S64 mEndingSample)
	: ProtocolFrame(mChannelIndex, mStartingSample, mEndingSample)
{
	_short = strShort;
	_midium = strMidium;
	_detailed = strDetailed;
}

void TextFrame::RenderBubbleText(AnalyzerResults* ar, Channel& channel, DisplayBase display_base)
{
	if (channel.mChannelIndex != this->_channelIndex) return;

	ar->AddResultString(_short.c_str());
	if (!_midium.empty())
	{
		ar->AddResultString(_midium.c_str());
	}
	if (!_detailed.empty())
	{
		ar->AddResultString(_detailed.c_str());
	}
}


ProtocolFrame::ptr ByteFrame::factory(U32 mChannelIndex, unsigned char val, S64 mStartingSample, S64 mEndingSample)
{
	ProtocolFrame::ptr ret(new ByteFrame(mChannelIndex, val, mStartingSample, mEndingSample));
	return ret;
}

ProtocolFrame::ptr ByteFrame::factory(U32 mChannelIndex, std::string name, char val, S64 mStartingSample, S64 mEndingSample)
{
	ProtocolFrame::ptr ret(new ByteFrame(mChannelIndex, name, val, mStartingSample, mEndingSample));
	return ret;
}


void ByteFrame::RenderBubbleText(AnalyzerResults* ar, Channel& channel, DisplayBase display_base)
{
	if (channel.mChannelIndex != this->_channelIndex) return;

	char number_str[128];
	AnalyzerHelpers::GetNumberString(_val, display_base, 8, number_str, sizeof(number_str));
	ar->AddResultString(number_str);
	if (!_name.empty())
	{
		std::string tmp = _name + std::string(" ") + std::string(number_str);
		ar->AddResultString(tmp.c_str());
	}
}

ByteFrame::ByteFrame(U32 mChannelIndex, unsigned char val, S64 mStartingSample, S64 mEndingSample)
	: ProtocolFrame(mChannelIndex, mStartingSample, mEndingSample)
{
	this->_val = val;
}

ByteFrame::ByteFrame(U32 mChannelIndex, std::string name, unsigned char val, S64 mStartingSample, S64 mEndingSample)
	: ProtocolFrame(mChannelIndex, mStartingSample, mEndingSample)
{
	this->_name = name;
	this->_val = val;
}
