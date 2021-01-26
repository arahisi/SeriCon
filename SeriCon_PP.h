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

#include "SeriCon.h"

class SeriConPP
{
public:
	SeriConPP(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer, SeriConRxHandler *handler, const SeriConHAL *hal);
	int send(size_t txSize, const uint8_t *txBuffer);
	uint8_t *getTxBuffer(void);
	uint32_t getRxErrorCount(void);
	uint32_t getRxTimeout(void);
	void setRxTimeout(uint16_t time);
	void task(void);
private:
	SeriCon sc;
};

#endif /* _SERICON_PP_H_ */
