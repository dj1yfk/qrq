/*
Copyright (c) 2008 Marc Vaillant

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <iostream>
#include "OpenAlImp.h"
#include "OpenAlStream.h"

extern long samplerate;

using namespace std;

int write_audio(void * cookie, void* incomingBuffer, int incomingSize)
{
  OpenAlStream* pStream = (OpenAlStream*) cookie;

  try
  {
	pStream->write(incomingBuffer, incomingSize);
  }
  catch(std::exception& e)
  {
	cerr<<"Exception: "<<e.what()<<endl;
	return 0;
  }
  return incomingSize;
}

int close_audio(void * cookie)
{
  OpenAlStream* pStream = (OpenAlStream*) cookie;
  pStream->flush();
  delete pStream;

  return 0;
}

void* open_dsp (char * dummy)
{
  OpenAlStream* pStream;

  try
  {
	pStream = new OpenAlStream();
  }
  catch(exception& e)
  {
	cerr<<"Exception: "<<e.what()<<endl;
	return 0;
  }

  pStream->setSampleRate(samplerate);

  return pStream;;
}
