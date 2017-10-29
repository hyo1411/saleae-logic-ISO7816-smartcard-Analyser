//
// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#ifndef IISO7816_SESSION_LOGGER_H
#define IISO7816_SESSION_LOGGER_H

#include "ByteBuffer.hpp"
#include "ISO7816Atr.hpp"
#include "ISO7816Pps.hpp"

class IIso7816SessionLogger
{
public:
	typedef unsigned long long int u64;
	enum Mode
	{
		INVERSE = 0xC0,
		DIRECT = 0xDC
	};

	enum SessionState
	{
		Start,
		Atr,
		Pps,
		Transmission,
		Unknown
	};

public:
	typedef std::shared_ptr<Iso7816Session> ptr;
	static Iso7816Session::ptr factory(u64 initialEtu);

	virtual void PushByte(unsigned char val, unsigned long long startPos, unsigned long long endPos);
	u64 GetEtu()
	{
		return _etu;
	}

protected:
	Iso7816Session(u64 initialEtu);

	unsigned char Transform(unsigned char val);

	void OnAtr();
	void OnPps();
	void OnTransmission();
	void OnUnknown();

protected:
	u64 _etu = 0;
	ByteBuffer _buff;
	Mode _mode;
	SessionState _state = SessionState::Start;
	ISO7816Atr::ptr _atr;
	ISO7816Pps::ptr _pps;
};

#endif //IISO7816_SESSION_LOGGER_H
