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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SeriCon_PP.h"
#include "CRC16_CCITT.h"

SeriConPP::SeriConPP(void)
{
	DEBUG_LOGGER_FI(SERICON);
	this->rxCRC = CRC16_CCITT.Initial;
	this->handle = NULL;
	this->stream = NULL;
	this->rxSize = 0;
	this->txIndex = 0;
	this->rxIndex = 0;
	this->rxTime = 0;
	this->rxTimeout = 0;
	this->rxWorkSize = 0;
	this->txWorkSize = 0;
	this->rxWorkBuffer = NULL;
	this->txWorkBuffer = NULL;
	this->txSize = 0;
	this->txLastSize = 0;
	this->rxError = 0;
	this->rxErrorCount = 0;
	this->reqResend = 0;
	this->RX_TIMEOUT = 10000UL; // default 10ms
	DEBUG_LOGGER_FO(SERICON);
}

SeriConPP::SeriConPP(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer) : SeriConPP()
{
	DEBUG_LOGGER_FI(SERICON);
	this->rxWorkSize = (int)rxWorkSize;
	this->txWorkSize = (int)txWorkSize;
	this->rxWorkBuffer = rxWorkBuffer;
	this->txWorkBuffer = txWorkBuffer;
	DEBUG_LOGGER_FO(SERICON);
}

void SeriConPP::begin(Stream &stream, RxHandle &handle)
{
	DEBUG_LOGGER_FI(SERICON);
	this->rxIndex = 0;
	this->stream = &stream;
	this->handle = &handle;
	this->rxCRC = 0xFFFF;
	DEBUG_LOGGER_FO(SERICON);
}

void SeriConPP::begin(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer, Stream &stream, RxHandle &handle)
{
	DEBUG_LOGGER_FI(SERICON);
	this->begin(stream, handle);
	this->rxWorkSize = (int)rxWorkSize;
	this->txWorkSize = (int)txWorkSize;
	this->rxWorkBuffer = rxWorkBuffer;
	this->txWorkBuffer = txWorkBuffer;
	DEBUG_LOGGER_FO(SERICON);
}

uint8_t *SeriConPP::getTxBuffer(void)
{
	DEBUG_LOGGER_FI(SERICON);
	task();
	uint8_t *ptr = (this->txSize) ? NULL : &this->txWorkBuffer[4];
	DEBUG_LOGGER_H32(SERICON, ptr);
	DEBUG_LOGGER_FO(SERICON);
	return ptr;
}

int SeriConPP::send(size_t txSize, const uint8_t *txBuffer)
{
	DEBUG_LOGGER_FI(SERICON);
	uint16_t txCRC;
	task();
	if (this->txSize || (txSize > 256) || ((int)(txSize + 5) > this->txWorkSize) || (0 == txSize))
	{
		return 0;
	}
	DEBUG_LOGGER_BUF(SERICON, txSize, txBuffer);
	this->txWorkBuffer[0] = 0xFF;
	this->txWorkBuffer[1] = 0x55;
	this->txWorkBuffer[2] = 0;
	this->txWorkBuffer[3] = (int)(txSize - 1);
	if (&this->txWorkBuffer[4] != txBuffer)
	{
		memcpy(&this->txWorkBuffer[4], txBuffer, txSize);
	}
	txCRC = uint16_t_GetCRC(&CRC16_CCITT, this->txWorkBuffer, txSize + 4);
	this->txWorkBuffer[txSize + 4] = (uint8_t)(txCRC);
	this->txWorkBuffer[txSize + 5] = (uint8_t)(txCRC >> 8);
	this->txIndex = 0;
	this->txSize = (int)(txSize + 6);
	this->txLastSize = this->txSize;
	task();
	DEBUG_LOGGER_FO(SERICON);
	return (int)txSize;
}

uint32_t SeriConPP::getRxErrorCount(void)
{
	return this->rxErrorCount;
}

uint32_t SeriConPP::getRxTimeout(void)
{
	return this->RX_TIMEOUT;
}

void SeriConPP::setRxTimeout(uint32_t us)
{
	this->RX_TIMEOUT = us;
}

void SeriConPP::task(void)
{
	DEBUG_LOGGER_FI(SERICON);
	if (this->txSize)
	{
		DEBUG_LOGGER_BUF(SERICON, this->txSize, &this->txWorkBuffer[this->txIndex]);
		size_t ws = this->stream->write(&this->txWorkBuffer[this->txIndex], this->txSize);
		DEBUG_LOGGER_U32(SERICON, ws);
		this->txSize -= (int)ws;
		this->txIndex += (int)ws;
	}
	while (1)
	{
		int stop_rx = 1;
		if (0 == this->rxIndex || this->rxTimeout)
		{
			int c = this->stream->read();
			uint32_t now = ::micros();
			if (c != EOF)
			{
				DEBUG_LOGGER_H8(SERICON, c);
				stop_rx = 0;
				this->rxTimeout = this->RX_TIMEOUT;
				if (this->rxIndex >= this->rxWorkSize)
				{
					this->rxError = 1;
				}
				else if (!this->rxError)
				{
					this->rxWorkBuffer[this->rxIndex] = (uint8_t)c;
					rxCRC = uint16_t_CRC(&CRC16_CCITT, this->rxWorkBuffer[this->rxIndex], rxCRC);
					DEBUG_LOGGER_I32(SERICON, this->rxIndex);
					if (0 == this->rxIndex)
					{ // head0
						this->reqResend = 0;
						if (0x00 == c)
						{
							this->reqResend = 1;
						}
						else if (0xFF != c)
						{
							DEBUG_LOGGER_MSG(SERICON, "HEAD0");
							this->rxError = 1;
						}
					}
					else if (1 == this->rxIndex)
					{ // head1
						if (this->reqResend)
						{
							if (0xAA == c)
							{
								this->reqResend = 2;
							}
							else
							{
								this->reqResend = 0;
								this->rxError = 1;
							}
						}
						else if (0x55 != c)
						{
							DEBUG_LOGGER_MSG(SERICON, "HEAD1");
							this->rxError = 1;
						}
					}
					else if (2 == this->rxIndex)
					{ // Don't care
						if (this->reqResend)
						{
							this->reqResend = 0;
							this->rxError = 1;
						}
					}
					else if (3 == this->rxIndex)
					{ // size
						this->rxSize = (size_t)c;
						this->rxSize += 7;
						if (this->rxSize > this->rxWorkSize)
						{
							DEBUG_LOGGER_MSG(SERICON, "SIZE");
							this->rxError = 1;
						}
					}
					else if (4 <= this->rxIndex && this->rxIndex <= (this->rxSize - 2))
					{ // data & CRC16(LOW)
					}
					else if (this->rxIndex == (this->rxSize - 1))
					{ // CRC16(HIGH)
						DEBUG_LOGGER_H32(SERICON, this->rxCRC);
						if (0 == this->rxCRC)
						{
							DEBUG_LOGGER_BUF(SERICON, this->rxSize, this->rxWorkBuffer);
							handle(this, this->rxWorkBuffer[3] + 1, &this->rxWorkBuffer[4]);
							this->rxIndex = 0;
							this->rxCRC = CRC16_CCITT.Initial;
							this->rxTimeout = 0;
							return;
						}
						else
						{
							this->rxError = 1;
						}
					}
				}
				this->rxIndex++;
			}
			else
			{
				uint32_t diff;
				if (now >= this->rxTime)
				{
					diff = now - this->rxTime;
				}
				else
				{
					diff = 0xFFFFFFFF - this->rxTime;
					diff += now + 1;
				}
				if (diff >= this->rxTimeout)
				{
					if (this->rxError)
					{
						this->rxError = 0;
						this->rxErrorCount++;
						if (this->rxIndex >= this->rxWorkSize)
						{
							// do nothing
						}
						else if (0 == this->txSize)
						{
							this->txWorkBuffer[0] = 0x00;
							this->txWorkBuffer[1] = 0xAA;
							this->txIndex = 0;
							this->txSize = 2;
							DEBUG_LOGGER_MSG(SERICON, "Rx Error & Send resend request");
						}
					}
					else if (2 == this->reqResend)
					{
						this->reqResend = 0;
						if (0 == this->txSize && this->txLastSize)
						{
							this->txWorkBuffer[0] = 0xFF;
							this->txWorkBuffer[1] = 0x55;
							this->txIndex = 0;
							this->txSize = this->txLastSize;
							DEBUG_LOGGER_MSG(SERICON, "Resend data");
						}
					}
					this->rxIndex = 0;
					this->rxCRC = CRC16_CCITT.Initial;
					this->rxTimeout = 0;
				}
				else
				{
					this->rxTimeout -= diff;
				}
			}
			this->rxTime = now;
		}
		if (stop_rx)
		{
			break;
		}
	}
	DEBUG_LOGGER_FO(SERICON);
}
