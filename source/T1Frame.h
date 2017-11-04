// Copyright � 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <memory>
#include <iomanip>
#include "Convert.hpp"
#include "TxFrame.h"

#ifndef T1FRAME_HPP
#define T1FRAME_HPP

class T1Frame : public TxFrame
{
public:
	// definitions
	enum Definitions
	{
		BLOCK_MASK = 0xC0,
		TAG_I_BLOCK1 = 0x00,
		TAG_I_BLOCK2 = 0x40,
		TAG_R_BLOCK = 0x80,
		TAG_S_BLOCK = 0xc0
	};
	enum BlockType
	{
		Unknown,
		I_BLOCK,
		S_BLOCK,
		R_BLOCK
	};
	enum Position
	{
		NAD = 0,
		PCB,
		LEN,
		INF,
		LRC,
		Complete
	};

private:
	BlockType _blockType = Unknown;
	Position _pos = Position::NAD;
	unsigned char _nad = 0;
	unsigned char _pcb = 0;
	unsigned char _len = 0;
	std::vector<unsigned char> _inf;
	unsigned char _lrc = 0;
	unsigned char _xor = 0;
	std::string _lastElementName;

public:
	static TxFrame::ptr factory()
	{
		return TxFrame::ptr(new T1Frame());
	}

	virtual ~T1Frame()
	{
	}

	virtual void PushData(unsigned char data)
	{
		_lastElementName = "";
		if (_pos == NAD)
		{
			_xor ^= data;
			OnNAD(data);
		}
		else if (_pos == PCB)
		{
			_xor ^= data;
			OnPCB(data);
		}
		else if (_pos == LEN)
		{
			_xor ^= data;
			OnLEN(data);
		}
		else if (_pos == INF)
		{
			_xor ^= data;
			OnINF(data);
		}
		else if (_pos == LRC)
		{
			OnLRC(data);
		}
		else
		{
			throw std::runtime_error("T1 frame has been already parsed, no need for more data!");
		}
	}

	virtual bool Valid()
	{
		if (!Completed()) return false;
		if (_xor != _lrc) return false;
		return true;
	}

	virtual bool Completed()
	{
		return _pos == Complete;
	}

	virtual std::string GetLastElementName()
	{
		return _lastElementName;
	}

	virtual std::string ToString()
	{
		std::stringstream ss;
		RenderBlockType(ss, _blockType);
		RenderTokenWithHexValue(ss, std::string("NAD"), _nad);
		ss << " ";
		RenderTokenWithHexValue(ss, std::string("PCB"), _pcb);
		ss << " ";
		RenderTokenWithHexValue(ss, std::string("LEN"), _len);
		ss << " ";
		RenderTokenWithHexValues(ss, std::string("INF"), _inf);
		ss << " ";
		RenderTokenWithHexValue(ss, std::string("LRC"), _lrc);
		return ss.str();
	}

protected:
	void OnNAD(unsigned char data)
	{
		_nad = data;
		_pos = PCB;
		_lastElementName = "NAD";
	}

	void OnPCB(unsigned char data)
	{
		DetermineBlockType(data);
		_pcb = data;
		_pos = LEN;
		_lastElementName = "PCB";
	}

	void OnLEN(unsigned char data)
	{
		_len = data;
		_pos = _len > 0 ? INF : LRC;
		_lastElementName = "LEN";
	}

	void OnINF(unsigned char data)
	{
		_inf.push_back(data);
		{
			std::stringstream ss;
			ss << "INF" << _inf.size();
			_lastElementName = ss.str();
		}
		if (_len == _inf.size())
		{
			_pos = LRC;
		}
	}

	void OnLRC(unsigned char data)
	{
		_lrc = data;
		_pos = Complete;
		{
			std::stringstream ss;
			ss << "LRC-" << ((_lrc == _xor) ? "OK" : "ERR");			
			_lastElementName = ss.str();
		}
	}

private:
	T1Frame()
	{
	}

	void DetermineBlockType(unsigned char data)
	{
		unsigned char tmp = data & Definitions::BLOCK_MASK;
		switch (tmp)
		{
		case Definitions::TAG_I_BLOCK1:
		case Definitions::TAG_I_BLOCK2:
			_blockType = BlockType::I_BLOCK;
			break;
		case Definitions::TAG_R_BLOCK:
			_blockType = BlockType::R_BLOCK;
			break;
		case Definitions::TAG_S_BLOCK:
			_blockType = BlockType::S_BLOCK;
			break;
		}
	}

	void RenderBlockType(std::stringstream& ss, BlockType bt)
	{
		switch (bt)
		{
		case BlockType::Unknown:
			ss << "Unknown ";
			break;
		case BlockType::I_BLOCK:
			ss << "I-BLOCK ";
			break;
		case BlockType::R_BLOCK:
			ss << "R-BLOCK ";
			break;
		case BlockType::S_BLOCK:
			ss << "S-BLOCK ";
			break;
		}
	}

	void RenderTokenWithHexValue(std::stringstream& ss, const std::string& name, unsigned char val)
	{
        RenderTokenWithHexValue(ss, name.c_str(), val);
	}

    void RenderTokenWithHexValue(std::stringstream& ss, const char* name, unsigned char val)
    {
        ss << name << "(";
        AppendHexByte(ss, val);
        ss << "h)";
    }
    
    void RenderTokenWithHexValues(std::stringstream& ss, const std::string& name, std::vector<unsigned char>& buff)
    {
        RenderTokenWithHexValues(ss, name.c_str(), buff);
    }
    
	void RenderTokenWithHexValues(std::stringstream& ss, const char* name, std::vector<unsigned char>& buff)
	{
		ss << name << "(";
        for (auto var : buff)
		{
			AppendHexByte(ss, var);
		}
		ss << "h)";
	}

	void AppendHexByte(std::stringstream& ss, unsigned char val)
	{
		ss << std::noshowbase << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)val;
	}

};

#endif //T1FRAME_HPP
