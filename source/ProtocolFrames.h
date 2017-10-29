//
// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#ifndef PROTOCL_FRAMES_H
#define PROTOCL_FRAMES_H

class ProtocolFrame : public Frame
{
public:
	typedef std::shared_ptr<ProtocolFrame> ptr;

	virtual void RenderBubbleText(AnalyzerResults* ar, Channel& channel, DisplayBase display_base) = 0;

protected:
	ProtocolFrame(U32 mChannelIndex, S64 mStartingSample, S64 mEndingSample);

protected:
	U32 _channelIndex;
};

class TextFrame : public ProtocolFrame
{
public:
	static ProtocolFrame::ptr factory(U32 mChannelIndex, const std::string& strShort, S64 mStartingSample, S64 mEndingSample);
	static ProtocolFrame::ptr factory(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, S64 mStartingSample, S64 mEndingSample);
	static ProtocolFrame::ptr factory(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, const std::string& strDetailed, S64 mStartingSample, S64 mEndingSample);

	void RenderBubbleText(AnalyzerResults* ar, Channel& channel, DisplayBase display_base);

private:
	TextFrame(U32 mChannelIndex, const std::string& strShort, S64 mStartingSample, S64 mEndingSample);
	TextFrame(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, S64 mStartingSample, S64 mEndingSample);
	TextFrame(U32 mChannelIndex, const std::string& strShort, const std::string& strMidium, const std::string& strDetailed, S64 mStartingSample, S64 mEndingSample);

private:
	std::string _short;
	std::string _midium;
	std::string _detailed;
};



class ByteFrame : public ProtocolFrame
{
public:
	static ProtocolFrame::ptr factory(U32 mChannelIndex, unsigned char val, S64 mStartingSample, S64 mEndingSample);
	static ProtocolFrame::ptr factory(U32 mChannelIndex, std::string name, char val, S64 mStartingSample, S64 mEndingSample);

	void RenderBubbleText(AnalyzerResults* ar, Channel& channel, DisplayBase display_base);

private:
	ByteFrame(U32 mChannelIndex, unsigned char val, S64 mStartingSample, S64 mEndingSample);
	ByteFrame(U32 mChannelIndex, std::string name, unsigned char val, S64 mStartingSample, S64 mEndingSample);

private:
	unsigned char _val;
	std::string _name;
};

#endif //PROTOCL_FRAMES_H
