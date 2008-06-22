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

$Id$

*/

#include "OpenAlStream.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

OpenAlStream::OpenAlStream()
{
  init();
}

OpenAlStream::~OpenAlStream()
{
  close();
}

void OpenAlStream::close()
{
  alSourceStop(_source);
  empty();
  alDeleteSources(1, &_source);
  check();
  alDeleteBuffers(2, _buffers);
  check();

  alcMakeContextCurrent(NULL);
  alcDestroyContext(_ctx);
  alcCloseDevice(_dev);
}

void OpenAlStream::check() const
{
	int error = alGetError();

	if(error != AL_NO_ERROR)
	{
	  stringstream ss;
	  ss<<"OpenAL Error: "<<error;
	  throw runtime_error(ss.str());
	}
}

void OpenAlStream::empty()
{
    int queued;
    
    alGetSourcei(_source, AL_BUFFERS_QUEUED, &queued);
    
    while(queued--)
    {
        ALuint buffer;
    
        alSourceUnqueueBuffers(_source, 1, &buffer);
        check();
    }
}

void OpenAlStream::setBufferSize(int size)
{
  _bufferSize = size;
  _myBuffer.resize(0);
  _myBuffer.reserve(size);
}

void OpenAlStream::init()
{
  _dev = alcOpenDevice(NULL);
  _ctx = alcCreateContext(_dev, NULL);
  alcMakeContextCurrent(_ctx);

  setSampleRate(44100);
  setBufferSize(4096);
  setFormat(AL_FORMAT_STEREO16);

  
  alGenBuffers(2, _buffers);
  check();
  alGenSources(1, &_source);
  check();

  alSource3f(_source, AL_POSITION,        0.0, 0.0, 0.0);
  alSource3f(_source, AL_VELOCITY,        0.0, 0.0, 0.0);
  alSource3f(_source, AL_DIRECTION,       0.0, 0.0, 0.0);
  alSourcef (_source, AL_ROLLOFF_FACTOR,  0.0          );
  alSourcei (_source, AL_SOURCE_RELATIVE, AL_TRUE      );

}

void OpenAlStream::write(void* bufferToQueue, int size)
{

  char* cBufferToQueue = (char *) bufferToQueue;

  int currentSize = _myBuffer.size();

  for(int i = 0; i < size; ++i)
  {
	if(currentSize < _bufferSize)
	{
	  _myBuffer.push_back(cBufferToQueue[i]);
	  currentSize++;
	}
	else
	{
	  blockAndQueueBackBuffer();
	  _myBuffer.push_back(cBufferToQueue[i]);
	  currentSize = 1;
	}
  }
}

void OpenAlStream::flush()
{
  // push pending data into the queue
  
  if(_myBuffer.size() > 0)
	blockAndQueueBackBuffer();

  // wait for all buffers to be played.
  
  int numProcessed;
  int numQueued;

  alGetSourcei(_source, AL_BUFFERS_PROCESSED, &numProcessed);
  alGetSourcei(_source, AL_BUFFERS_QUEUED, &numQueued);

  // Note: busy wait here

  while(numProcessed < numQueued)
  {
	alGetSourcei(_source, AL_BUFFERS_PROCESSED, &numProcessed);
  }

}

void OpenAlStream::blockAndQueueBackBuffer()
{
  int numQueued;

  alGetSourcei(_source, AL_BUFFERS_QUEUED, &numQueued);

  switch(numQueued)
  {
		  case 2:
				  int processed;

				  // block until a buffer is free

				  alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);
				  while(processed == 0)  // Note: busy wait here
				  {
					alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);
				  }

				  ALuint buffer;

				  alSourceUnqueueBuffers(_source, 1, &buffer);
				  check();

				  alBufferData(buffer, _format, &_myBuffer[0], _myBuffer.size(), getSampleRate());

				  alSourceQueueBuffers(_source, 1, &buffer);
				  check();
				  break;

		  case 1:
				  // second buffer not queued yet so push onto buffer 2

				  alBufferData(_buffers[1], _format, &_myBuffer[0], _myBuffer.size(), getSampleRate());
				  alSourceQueueBuffers(_source, 1, &_buffers[1]); 
				  break;
		  case 0:
				  // first buffer not queued yet so push onto buffer 1
				  
				  alBufferData(_buffers[0], _format, &_myBuffer[0], _myBuffer.size(), getSampleRate());
				  alSourceQueueBuffers(_source, 1, &_buffers[0]); 

				  // now start playing.

				  alSourcePlay(_source);

				  break;
  }

  _myBuffer.resize(0);
}
