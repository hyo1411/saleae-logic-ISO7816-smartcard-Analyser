// Copyright © 2016 Adam Augustyn <adam@augustyn.net>, all rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
//

#include <string>

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

class DecoderException : public std::exception
{
public:
	DecoderException(unsigned long long int pos)
	{
		_pos = pos;
	}

	virtual unsigned long long int getPosition()
	{
		return _pos;
	}

protected:
	unsigned long long int _pos;
};

class OutOfSyncException : public DecoderException
{
public:
	OutOfSyncException(unsigned long long int pos) : DecoderException(pos)
	{
	}
};


class ParityException : public DecoderException
{
public:
	ParityException(unsigned long long int pos) : DecoderException(pos)
	{
	}
};

class ErrorSignalException : public DecoderException
{
public:
	ErrorSignalException(unsigned long long int pos) : DecoderException(pos)
	{
	}
};

class ResetException : public DecoderException
{
public:
	ResetException(unsigned long long int pos) : DecoderException(pos)
	{
	}
};

#endif //EXCEPTIONS_HPP
