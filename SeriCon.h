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

#ifndef SERICON_MESSAGE_ID_SUPPORT
#define SERICON_MESSAGE_ID_SUPPORT 0
#endif

#ifndef SERICON_MESSAGE_RESEND_SUPPORT
#define SERICON_MESSAGE_RESEND_SUPPORT 0
#endif

#ifndef SERICON_DEFAULT_RX_TIMEOUT
#define SERICON_DEFAULT_RX_TIMEOUT 10UL
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef struct S_SeriCon SeriCon;
    typedef struct S_SeriConHAL SeriConHAL;
    typedef int(SeriConRx)(void);
    typedef int(SeriConTx)(uint8_t data);
    typedef uint16_t(SeriConTimer)(void);
    struct S_SeriConHAL
    {
        SeriConTimer *Timer;
        SeriConRx *Rx;
        SeriConTx *Tx;
    };

    typedef int(SeriConRxHandler)(size_t rx_sz, const uint8_t *rx_msg, uint8_t* tx_msg);

#define SERICON_RX_STATE_BIT_RESEBD 0x001
#define SERICON_RX_STATE_BIT_JOB 0x100
#define SERICON_RX_STATE_BIT_DONE 0x200
#define SERICON_RX_STATE_BIT_ERROR 0x400

    typedef enum E_SeriConRxState
    {
        SeriConRxState_Done = SERICON_RX_STATE_BIT_DONE,
        SeriConRxState_Normal = SERICON_RX_STATE_BIT_JOB,
        SeriConRxState_Resend1 = SERICON_RX_STATE_BIT_JOB | SERICON_RX_STATE_BIT_RESEBD,
        SeriConRxState_Resend2 = SERICON_RX_STATE_BIT_JOB | SERICON_RX_STATE_BIT_RESEBD | (SERICON_RX_STATE_BIT_RESEBD << 1),
        SeriConRxState_Error = SERICON_RX_STATE_BIT_ERROR
    } SeriConRxState;

    struct S_SeriCon
    {
        SeriConRxState RxState;
        int RxIndex;
        int TxIndex;
        int RxSize;
        int TxSize;
        int RxWorkSize;
        int TxWorkSize;
#if SERICON_MESSAGE_RESEND_SUPPORT
        int TxLastSize;
#endif
        uint8_t *RxWorkBuffer;
        uint8_t *TxWorkBuffer;
        SeriConRxHandler *Handler;
        const SeriConHAL *HAL;
        uint16_t RxCRC;
        uint16_t RxTime;
        uint16_t RxTimeout;
        uint16_t RX_TIMEOUT;
        uint16_t RxErrorCount;
#if SERICON_MESSAGE_ID_SUPPORT
        uint8_t TxID;
        uint8_t RxID;
#endif
    };

    extern void SeriCon_init(SeriCon *sc,
                             size_t rxWorkSize, uint8_t *rxWorkBuffer,
                             size_t txWorkSize, uint8_t *txWorkBuffer,
                             SeriConRxHandler *handler, const SeriConHAL *hal);
    extern int SeriCon_send(SeriCon *sc, size_t txSize, const uint8_t *tx_msg);
    uint8_t *SeriCon_getTxBuffer(SeriCon *sc);
    uint16_t SeriCon_getRxErrorCount(SeriCon *sc);
    uint16_t SeriCon_getRxTimeout(SeriCon *sc);
    void SeriCon_setRxTimeout(SeriCon *sc, uint16_t time);
    void SeriCon_task(SeriCon *sc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SERICON_H_ */
