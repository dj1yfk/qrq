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

#ifndef OPEN_AL_STREAM
#define OPEN_AL_STREAM

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <vector>

class OpenAlStream
{
		public: 
				OpenAlStream();
				OpenAlStream::~OpenAlStream();

				void setBufferSize(int size);
				void setFormat(ALenum format);
				void setSampleRate(int sampleRate);
				int getSampleRate() const;
				
				void write(void* bufferToQueue, int size);
				void flush();

		private:
				void check() const;
				void init();
				void empty();
				void close();
				void blockAndQueueBackBuffer();

				int _bufferSize;
				int _sampleRate;
				ALuint _buffers[2];
				ALuint _source;
				ALenum _format;
				std::vector<char> _myBuffer;
				ALCdevice *_dev;
				ALCcontext *_ctx;
};

inline int OpenAlStream::getSampleRate() const
{
  return _sampleRate;
}

inline void OpenAlStream::setFormat(ALenum format)
{
  _format = format;
}

inline void OpenAlStream::setSampleRate(int sampleRate)
{
  _sampleRate = sampleRate;
}


#endif

