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
#include "ISO7816Session.h"
#include "Convert.hpp"
#include "Logging.hpp"
#include "Definitions.hpp"
#include "iso7816AnalyzerResults.h"
#include "T1Frame.h"

Iso7816Session::ptr Iso7816Session::factory(iso7816AnalyzerResults::ptr results, Iso7816Session::u64 initialEtu, unsigned int chlBytes, unsigned int chlFrames)
{
	Iso7816Session::ptr ret(new Iso7816Session(results, initialEtu, chlBytes, chlFrames));
	return ret;
}

void Iso7816Session::PushByte(unsigned char val, unsigned long long startPos, unsigned long long endPos)
{
	if (_state == SessionState::Start)
	{
		switch (val)
		{
		case Mode::DIRECT:
			break;
		case Mode::INVERSE:
			break;
		default:
			throw std::runtime_error("The first byte shoud be C0h (INVERSE) or DCh (DIRECT) only!");
		}
		_mode = (Mode)val;
		_state = SessionState::Atr;
	}

	unsigned char tmp = Transform(val);
	_buff.push_back(ByteElement(tmp, startPos, endPos));

	Logging::Write(std::string("data: ") + Convert::ToHex(tmp) + std::string("h"));
	switch (_state)
	{
	case SessionState::Atr:
		OnAtr();
		break;
	case SessionState::Pps:
		OnPps();
		break;
	case SessionState::Transmission:
		OnTransmission();
		break;
	case SessionState::Unknown:
		OnUnknown();
		break;
    default:
        break;
	}
}

Iso7816Session::Iso7816Session(iso7816AnalyzerResults::ptr results, Iso7816Session::u64 initialEtu, unsigned int chlBytes, unsigned int chlFrames)
{
	_results = results;
	_etu = initialEtu;
	_chlBytes = chlBytes;
	_chlFrames = chlFrames;
}

unsigned char Iso7816Session::Transform(unsigned char val)
{
	if (_mode == Mode::DIRECT)
	{
		return Convert::Msb2Lsb(val);
	}
	else if (_mode == Mode::INVERSE)
	{
		return ~val;
	}
	else
	{
		throw std::runtime_error("Unsupported mode: INVERSE or DIRECT only!");
	}
}

void Iso7816Session::OnAtr()
{
	if (!_atr)
	{
		_atr = ISO7816Atr::factory();
	}
	_atr->PushData(_buff.back().GetValue());

	{
		std::string _name = _atr->GetLastElementName();
		ProtocolFrame::ptr frame = ByteFrame::factory(_chlBytes, _name, _buff.back().GetValue(), _buff.back().GetStartPos(), _buff.back().GetEndPos());
		_results->AddProtocolFrame(frame);
	}

	if (_atr->Completed())
	{
		{
			ProtocolFrame::ptr frame = TextFrame::factory(_chlFrames, "A", "ATR", _atr->ToString(), _buff[0].GetStartPos(), _buff.rbegin()->GetEndPos());
			_results->AddProtocolFrame(frame);
		}

		// 6.3.1 Selection of transmission parameters and protocol
		if (_atr->InterfaceByteExists(ISO7816Atr::Tx::TA, 2))
		{
			Logging::Write(std::string("Card is in 'specific' mode"));
			// If TA2 (see 8.3) is present in the Answer-to-Reset (card in specific mode), then the interface device shall
			// start the specific transmission protocol using the specific values of the transmission parameters.
			unsigned char _ta1 = _atr->GetInterfaceByte(ISO7816Atr::Tx::TA, 1);
			unsigned char _fi = (_ta1 >> 4) & 0x0f;
			unsigned char _di = _ta1 & 0x0f;
			_etu = static_cast<u64>(ISO7816Pps::CalculateETU(_fi, _di));
			Logging::Write(std::string("The new ETU value is: ") + Convert::ToDec(_etu));

			unsigned char _ta2 = _atr->GetInterfaceByte(ISO7816Atr::Tx::TA, 2);
			_prot = (Protocol)(_ta2 & 0x0f);
			Logging::Write(std::string("Selected protocol is: T") + Convert::ToDec(_prot));

			_state = SessionState::Transmission;
		}
		else
		{
			_state = SessionState::Pps;
		}
		_buff.clear();
	}
}

void Iso7816Session::OnPps()
{
	// 6.3.1 Selection of transmission parameters and protocol
	/*
	- If the value of the first character received by the card is 'FF', then the interface device shall have
	started a PPS exchange(see 9); the default values of the transmission parameters shall continue to
	apply until completion of a successful PPS exchange(see 9.3), after what the interface device shall
	start the negotiated transmission protocol using the negotiated values of the transmission parameters.
	- Otherwise, the interface device shall have started the “first offered transmission protocol” (see TD1 in
	8.2.3). The interface device shall do so when the card offers only one transmission protocol and only
	the default values of the transmission parameters. Such a card need not support PPS exchange.
	*/
	if (_buff.size() == 1)
	{
		if (PPS_HEADER != _buff[0].GetValue())
		{
			_state = SessionState::Transmission;
			OnTransmission();
			return;
		}
	}

	{
		ProtocolFrame::ptr frame = ByteFrame::factory(_chlBytes, _buff.back().GetValue(), _buff.back().GetStartPos(), _buff.back().GetEndPos());
		_results->AddProtocolFrame(frame);
	}

	// PPS request
	int res = ISO7816Pps::IsPpsFrame(_buff.ToBytes(), 0);
	if (res > 0)
	{
		ISO7816Pps::ptr frm1 = ISO7816Pps::DecodeFrame(_buff.ToBytes(), 0);
		// PPS response
		int res2 = ISO7816Pps::IsPpsFrame(_buff.ToBytes(), res);
		if (res2 > 0)
		{
			ISO7816Pps::ptr frm2 = ISO7816Pps::DecodeFrame(_buff.ToBytes(), res);

			// we are not able to decode frame 2 - not enough data?
			if (!frm2) return;

			// check if the frames are equal
			if (!frm1->Equal(frm2))
			{
				// not able to decode PPS, just report bytes
				_state = SessionState::Unknown;
				return;
			}
			// they are the same
			Logging::Write(std::string("PPS detected, fi: ") + Convert::ToDec(frm1->GetFi()) + std::string(", di: ") + Convert::ToDec(frm1->GetDi()));
			_etu = static_cast<u64>(ISO7816Pps::CalculateETU(frm1->GetFi(), frm1->GetDi()));
			Logging::Write(std::string("New ETU: ") + Convert::ToDec(_etu));
			_prot = (Protocol)frm1->GetProtocol();
			Logging::Write(std::string("Selected protocol is: T") + Convert::ToDec(_prot));

			{
				ProtocolFrame::ptr frame = TextFrame::factory(_chlFrames, "P", "PPS", frm1->ToString(), _buff[0].GetStartPos(), _buff.rbegin()->GetEndPos());
				_results->AddProtocolFrame(frame);
			}

			_buff.erase(_buff.begin(), _buff.begin() + (size_t)res2);
			_state = SessionState::Transmission;
			return;
		}
	}
}

void Iso7816Session::OnTransmission()
{
	if (_prot == Protocol::T1)
	{
		if (!_txframe)
		{
			_txframe = T1Frame::factory();
		}
		_txframe->PushData(_buff.rbegin()->GetValue());
		{
			ProtocolFrame::ptr frame = ByteFrame::factory(_chlBytes, _txframe->GetLastElementName(), _buff.back().GetValue(), _buff.back().GetStartPos(), _buff.back().GetEndPos());
			_results->AddProtocolFrame(frame);
		}
		if (_txframe->Completed())
		{
			ProtocolFrame::ptr frame = TextFrame::factory(_chlFrames, _txframe->ToString(), _buff.begin()->GetStartPos(), _buff.rbegin()->GetEndPos());
			_results->AddProtocolFrame(frame);
			_buff.clear();
			_txframe.reset();
		}
	}
	else
	{
		ProtocolFrame::ptr frame = ByteFrame::factory(_chlBytes, _buff.back().GetValue(), _buff.back().GetStartPos(), _buff.back().GetEndPos());
		_results->AddProtocolFrame(frame);
	}
}

void Iso7816Session::OnUnknown()
{
	{
		ProtocolFrame::ptr frame = ByteFrame::factory(_chlBytes, _buff.back().GetValue(), _buff.back().GetStartPos(), _buff.back().GetEndPos());
		_results->AddProtocolFrame(frame);
	}
}
