// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#include <string>
#include <vector>
#include <memory>

#ifndef ISO7816ATR_HPP
#define ISO7816ATR_HPP

class ISO7816Atr
{
public:
	typedef std::shared_ptr<ISO7816Atr> ptr;
	static const unsigned char INVERSE = 0x3F;
	static const unsigned char DIRECT  = 0x3B;

	enum Tx
	{
		TA = 0x10,
		TB = 0x20,
		TC = 0x40,
		TD = 0x80
	};

	enum AtrElement
	{
		TS = 0,
		T0,
		TA1,
		TB1,
		TC1,
		TD1,
		TA2,
		TB2,
		TC2,
		TD2,
		Tx,
		TK,
		TCK
	};

public:
	static ISO7816Atr::ptr factory()
	{
		return ISO7816Atr::ptr(new ISO7816Atr());
	}
	virtual ~ISO7816Atr()
	{
	}



	bool Valid()
	{
	}

	std::string ToString()
	{
		return "TBD...";
	}

protected:

private:
	ISO7816Atr();
};

#endif //ISO7816ATR_HPP
