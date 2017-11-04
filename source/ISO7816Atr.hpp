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
#include <stdexcept>
#include <iomanip>
#include "Convert.hpp"

#ifndef ISO7816ATR_HPP
#define ISO7816ATR_HPP

class ISO7816Atr
{
public:
	// definitions
	const int MAX_Yi = 4;

	enum Order
	{
		INVERSE = 0x3F,
		DIRECT = 0x3B
	};

	enum Position
	{
		TS = 0,
		T0,
		TXi,
		TK,
		TCK,
		Complete
	};

	enum Tx
	{
		None = 0,
		TA = 0x10,
		TB = 0x20,
		TC = 0x40,
		TD = 0x80,
		TDMask = 0x0f,
		Mask = 0xf0
	};

private:
	std::vector<unsigned char> _historical;
	std::map<unsigned char, unsigned char> _params;
	Position _pos = Position::TS;
	Order _order = Order::DIRECT;
	int _historical_num = 0;
	unsigned char _yi = 0;
	int _txi = 1;
	unsigned char _tck = 0;
	unsigned char _xor = 0;
	bool _hasTCK = false;
	std::string _lastElementName;

public:
	typedef std::shared_ptr<ISO7816Atr> ptr;

public:
	static ISO7816Atr::ptr factory()
	{
		return ISO7816Atr::ptr(new ISO7816Atr());
	}

	virtual ~ISO7816Atr()
	{
	}

	void PushData(unsigned char data)
	{
		_lastElementName = "";
		if (_pos == TS)
		{
			OnTS(data);
		}
		else if (_pos == T0)
		{
			_xor ^= data;
			OnT0(data);
		}
		else if (_pos == TXi)
		{
			_xor ^= data;
			OnTx(data);
		}
		else if (_pos == TK)
		{
			_xor ^= data;
			OnTK(data);
		}
		else if (_pos == TCK)
		{
			OnTCK(data);
		}
		else
		{
			throw std::runtime_error("ATR has been already parsed, no need for more data!");
		}
	}

	bool InterfaceByteExists(Tx tx, int idx)
	{
		unsigned char _key = GetKey(tx, idx);
		return _params.end() != _params.find(_key);
	}

	unsigned char GetInterfaceByte(Tx tx, int idx)
	{
		unsigned char _key = GetKey(tx, idx);
		auto _val = _params.find(_key);
		if (_val == _params.end())
		{
			throw std::runtime_error("Interface byte does not exist!");
		}
		return _val->second;
	}

	bool Valid()
	{
		if (!Completed()) return false;
		if (_hasTCK && (_xor != _tck)) return false;
		return true;
	}

	bool Completed()
	{
		return _pos == Complete;
	}

	std::string GetLastElementName()
	{
		return _lastElementName;
	}

	std::string ToString()
	{
		std::stringstream ss;
        std::string tmpStr((_order == Order::DIRECT) ? "DIRECT" : "INVERSE");
		RenderTokenWithHexValue(ss, tmpStr, _order);
		ss << " ";
		
		for (int i = 1; i <= MAX_Yi; i++)
		{
			RenderTxIfExists(ss, Tx::TA, i);
			RenderTxIfExists(ss, Tx::TB, i);
			RenderTxIfExists(ss, Tx::TC, i);
			RenderTxIfExists(ss, Tx::TD, i);
		}

		if (_historical.size() > 0)
		{
            tmpStr = std::string("No. of hist.");
			RenderTokenWithHexValue(ss, tmpStr, static_cast<unsigned char>(_historical.size()));
			ss << " '";
			for (auto it= _historical.begin(); it != _historical.end(); it++)
			{
				AppendHexByte(ss, *it);
			}
			ss << "' ";
		}

		if (_hasTCK)
		{
            tmpStr = std::string("TCK");
			RenderTokenWithHexValue(ss, tmpStr, _tck);
		}

		return ss.str();
	}

protected:
	void OnTS(unsigned char data)
	{
		// HLHHLLLLLLH or HLHHLHHHLLH
		//   11000000 1     11011100 1
		//   xC0            xDC
		// inverse        direct convention
		if (data != Order::DIRECT && data != Order::INVERSE)
		{
			throw std::runtime_error("Unsupported order byte!");
		}
		_order = (Order)data;
		_pos = T0;
		_lastElementName = "TS";
	}

	void OnT0(unsigned char data)
	{
		_historical_num = data & 0x0f;
		_yi = data & Tx::Mask;
		_pos = TXi;
		_lastElementName = "T0";
	}

	void OnTx(unsigned char data)
	{
		Tx tx = ExpectedTx();
		_lastElementName = std::string(GetTxName(tx)) + Convert::ToDec(_txi);
		ProcessTxi(_txi, tx, data);
		if (tx == Tx::TD)
		{
			_yi = data & Tx::Mask;
			if (_yi != 0)
			{
				// go to TX(y+1)
				_txi++;
				return;
			}
		}
		if (_yi == 0)
		{
			// all TXi bytes processed, next historical bytes or checksum
			if (_historical_num > 0)
			{
				_pos = TK;
			}
			else
			{
				_hasTCK = NeedTCK();
				_pos = _hasTCK ? TCK : Complete;
			}
		}
	}

	void ProcessTxi(int idx, Tx tx, unsigned char data)
	{
		// clear the flag
		_yi = _yi & ~tx;
		unsigned char _key = GetKey(tx, idx);
		_params[_key] = (tx == Tx::TD) ? (data & Tx::TDMask) : data;
	}

	Tx ExpectedTx()
	{
		if ((_yi & Tx::TA) != 0) return Tx::TA;
		if ((_yi & Tx::TB) != 0) return Tx::TB;
		if ((_yi & Tx::TC) != 0) return Tx::TC;
		if ((_yi & Tx::TD) != 0) return Tx::TD;
		return Tx::None;
	}

	void OnTK(unsigned char data)
	{
		if (_historical_num > 0)
		{
			_historical.push_back(data);
			_lastElementName = std::string("H") + Convert::ToDec(static_cast<unsigned int>(_historical.size()));
			_historical_num--;
		}

		if (_historical_num == 0)
		{
			_hasTCK = NeedTCK();
			_pos = _hasTCK ? TCK : Complete;
		}
	}

	bool NeedTCK()
	{
		unsigned char tmp = 0;
		for (int i = 1; i <= 3; i++)
		{
			if (InterfaceByteExists(Tx::TD, i))
			{
				tmp |= GetInterfaceByte(Tx::TD, i);
			}
		}
		// 8.2.5 Check byte TCK
		// If only T = 0 is indicated, possibly by default, then TCK shall be absent. If T = 0 and T = 15 are present and in all
		// the other cases, TCK shall be present.
		return tmp != 0;
	}

	void OnTCK(unsigned char data)
	{
		_lastElementName = std::string("TCK");
		_tck = data;
		_pos = Complete;
	}

	unsigned char GetKey(Tx tx, int idx)
	{
		return (idx & 0x0f) | tx;
	}

	void RenderTxIfExists(std::stringstream& ss, Tx tx, int idx)
	{
		if (InterfaceByteExists(tx, idx))
		{
			std::stringstream tmp;
			tmp << GetTxName(tx) << idx;
            std::string str = tmp.str();
			RenderTokenWithHexValue(ss, str, GetInterfaceByte(tx, idx));
			ss << " ";
		}
	}

	const char* GetTxName(Tx tx)
	{
		switch (tx)
		{
			case Tx::TA:
				return "TA";
			case Tx::TB:
				return "TB";
			case Tx::TC:
				return "TC";
			case Tx::TD:
				return "TD";
            default:
                break;
		}
		return "Tx";
	}

	void RenderTokenWithHexValue(std::stringstream& ss, std::string& name, unsigned char val)
	{
		ss << name << "(";
		AppendHexByte(ss, val);
		ss << "h)";
	}

	void AppendHexByte(std::stringstream& ss, unsigned char val)
	{
		ss << std::noshowbase << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)val;
	}

private:
	ISO7816Atr()
	{
	}
};

#endif //ISO7816ATR_HPP
