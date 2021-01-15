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

#ifndef _SERICON_H_
#define _SERICON_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct S_SeriCon SeriCon;
    typedef struct S_SeriConHAL SeriConHAL;
    typedef int(SeriConRx) (void);
    typedef int(SeriConTx) (uint8_t data);
    typedef uint16_t (SeriConTimer) (void);
    struct S_SeriConHAL {
        SeriConTimer *Timer;
        SeriConRx *Rx;
        SeriConTx *Tx;
    };
    
    typedef int (SeriConHandler) (SeriCon *sc, size_t sz, const uint8_t *rx_msg);

    struct S_SeriCon {
        int RxIndex;
        int TxIndex;
        int RxState;
        int RxSize;
        int TxSize;
        int RxWorkSize;
        int TxWorkSize;
        uint8_t *RxWorkBuffer;
        uint8_t *TxWorkBuffer;
        uint16_t RxCRC;
        uint16_t RxTime;
        uint16_t RxTimeout;
        uint16_t RX_TIMEOUT;
        uint16_t RxErrorCount;
        SeriConHandler *Handler;
        const SeriConHAL *HAL;
        uint8_t TxID;
        uint8_t RxID;
    };

    extern void SeriCon_init(SeriCon *sc,
            size_t rxWorkSize, uint8_t *rxWorkBuffer,
            size_t txWorkSize, uint8_t *txWorkBuffer,
            SeriConHandler *handler, const SeriConHAL *hal);
    extern int SeriCon_send(SeriCon *sc, size_t txSize, const uint8_t *txBuffer);
    uint8_t *SeriCon_getTxBuffer(SeriCon *sc);
    uint16_t SeriCon_getRxErrorCount(SeriCon *sc);
    uint16_t SeriCon_getRxTimeout(SeriCon *sc);
    void SeriCon_setRxTimeout(SeriCon *sc, uint16_t time);
    void SeriCon_task(SeriCon *sc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERICON_H_ */
