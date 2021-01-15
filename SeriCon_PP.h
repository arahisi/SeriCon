/*
Copyright 2021, Shoji Yamamoto
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
*/

#ifndef _SERICON_PP_H_
#define _SERICON_PP_H_

#include <stdint.h>
#include <stdio.h>
#include "SeriCon_dep.h"

#define SERICON 0

class SeriConPP
{
	typedef void(RxHandle)(SeriConPP *sc, size_t sz, const uint8_t *rx);

public:
	SeriConPP(void);
	SeriConPP(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer);
	void begin(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer, Stream &stream, RxHandle &handle);
	void begin(Stream &stream, RxHandle &handle);
	int send(size_t txSize, const uint8_t *txBuffer);
	uint8_t *getTxBuffer(void);
	uint32_t getRxErrorCount(void);
	uint32_t getRxTimeout(void);
	void setRxTimeout(uint32_t us);
	void task(void);

private:
	Stream *stream;
	RxHandle *handle;
	int rxIndex;
	int txIndex;
	int rxError;
	int reqResend;
	int rxSize;
	int txSize;
	int txLastSize;
	int rxWorkSize;
	int txWorkSize;
	uint8_t *rxWorkBuffer;
	uint8_t *txWorkBuffer;
	uint16_t rxCRC;
	uint32_t rxTime;
	uint32_t rxTimeout;
	uint32_t RX_TIMEOUT;
	uint32_t rxErrorCount;
};

#endif /* _SERICON_PP_H_ */
