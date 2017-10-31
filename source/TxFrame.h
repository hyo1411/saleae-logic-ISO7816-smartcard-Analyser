// Copyright © 2017 Adam Augustyn <adam@augustyn.net>, all rights reserved.
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

#ifndef TX_FRAME_HPP
#define TX_FRAME_HPP

class TxFrame
{
public:
	typedef std::shared_ptr<TxFrame> ptr;

public:

	virtual ~TxFrame()
	{
	}

	virtual void PushData(unsigned char data) = 0;
	virtual bool Valid() = 0;
	virtual bool Completed() = 0;
	virtual std::string GetLastElementName() = 0;
	virtual std::string ToString() = 0;

protected:
	TxFrame()
	{
	}
};

#endif //TX_FRAME_HPP
