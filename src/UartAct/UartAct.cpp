/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "UartInInterface.h"
#include "UartOutInterface.h"
#include "UartActInterface.h"
#include "UartAct.h"

FW_DEFINE_THIS_FILE("UartAct.cpp")

namespace APP {

UartAct::HsmnHalMap &UartAct::GetHsmnHalMap() {
    static HsmnHal hsmnHalStor[UART_ACT_COUNT];
    static HsmnHalMap hsmnHalMap(hsmnHalStor, ARRAY_COUNT(hsmnHalStor), HsmnHal(HSM_UNDEF, NULL));
    return hsmnHalMap;
}

// No need to have critical section since mapping is not changed after global/static object construction.
void UartAct::SaveHal(Hsmn hsmn, UART_HandleTypeDef *hal) {
    GetHsmnHalMap().Save(HsmnHal(hsmn, hal));
}

// No need to have critical section since mapping is not changed after global/static object construction.
UART_HandleTypeDef *UartAct::GetHal(Hsmn hsmn) {
    FW_ASSERT(hsmn != HSM_UNDEF);
    HsmnHal *kv = GetHsmnHalMap().GetByKey(hsmn);
    FW_ASSERT(kv);
    UART_HandleTypeDef *hal = kv->GetValue();
    FW_ASSERT(hal);
    return hal;
}

// No need to have critical section since mapping is not changed after global/static object construction.
Hsmn UartAct::GetHsmn(UART_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    HsmnHal *kv = GetHsmnHalMap().GetFirstByValue(hal);
    FW_ASSERT(kv);
    Hsmn hsmn = kv->GetKey();
    FW_ASSERT(hsmn != HSM_UNDEF);
    return hsmn;
}

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "START",
    "DONE",
    "FAIL",
};

static char const * const interfaceEvtName[] = {
    "UART_ACT_START_REQ",
    "UART_ACT_START_CFM",
    "UART_ACT_STOP_REQ",
    "UART_ACT_STOP_CFM",
    "UART_ACT_FAIL_IND",
};

static uint16_t GetInst(Hsmn hsmn) {
    uint16_t inst = hsmn - UART_ACT;
    FW_ASSERT(inst < UART_ACT_COUNT);
    return inst;
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *hal) {
    Hsmn hsmn = UartAct::GetHsmn(hal);
    UartOut::DmaCompleteCallback(UART_OUT + GetInst(hsmn));
}

UartAct::UartAct(Hsmn hsmn, char const *name, char const *inName, char const *outName, USART_TypeDef *dev) :
    Active((QStateHandler)&UartAct::InitialPseudoState, hsmn, name,
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_uartInHsmn(UART_IN + GetInst(hsmn)),
    m_uartOutHsmn(UART_OUT + GetInst(hsmn)),
    m_uartIn(m_uartInHsmn, inName, m_hal),
    m_uartOut(m_uartOutHsmn, outName, m_hal),
    m_client(HSM_UNDEF), m_outFifo(NULL), m_inFifo(NULL),
    m_stateTimer(hsmn, STATE_TIMER) {
    FW_ASSERT((hsmn >= UART_ACT) && (hsmn <= UART_ACT_LAST));
    FW_ASSERT(dev);
    memset(&m_hal, 0, sizeof(m_hal));
    m_hal.Instance = dev;
    // Save hsmn to HAL mapping.
    SaveHal(hsmn, &m_hal);
}

QState UartAct::InitialPseudoState(UartAct * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&UartAct::Root);
}

QState UartAct::Root(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_uartIn.Init(me);
            me->m_uartOut.Init(me);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartAct::Stopped);
            break;
        }
        case UART_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new UartActStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->GetHsm().SaveInSeq(req);
            status = Q_TRAN(&UartAct::Stopping);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState UartAct::Stopped(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new UartActStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_START_REQ: {
            EVENT(e);
            UartActStartReq const &req = static_cast<UartActStartReq const &>(*e);
            // For now serial configuration is fixed. In the future, it can be passed in with START_REQ.
            me->m_hal.Init.BaudRate = 115200;
            me->m_hal.Init.WordLength = UART_WORDLENGTH_8B;
            me->m_hal.Init.StopBits = UART_STOPBITS_1;
            me->m_hal.Init.Parity = UART_PARITY_NONE;
            me->m_hal.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
            me->m_hal.Init.Mode = UART_MODE_TX_RX;
            HAL_UART_DeInit(&me->m_hal);
            HAL_StatusTypeDef halStatus = HAL_UART_Init(&me->m_hal);
            if(halStatus == HAL_OK) {
                me->m_client = req.GetFrom();
                me->m_outFifo = req.GetOutFifo();
                me->m_inFifo = req.GetInFifo();
                me->GetHsm().SaveInSeq(req);
                Evt *evt = new Evt(START, GET_HSMN());
                me->PostSync(evt);
            } else {
                ERROR("HAL_UART_Init failed(%d", halStatus);
                Evt *evt = new UartActStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_HAL, GET_HSMN());
                Fw::Post(evt);
            }
            status = Q_HANDLED();
            break;
        }
        case START: {
            EVENT(e);
            status = Q_TRAN(&UartAct::Starting);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Starting(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = UartActStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartInStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > UartOutStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();

            FW_ASSERT(me->m_outFifo && me->m_inFifo);
            Evt *evt = new UartOutStartReq(me->m_uartOutHsmn, GET_HSMN(), GEN_SEQ(), me->m_outFifo, me->m_client);
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new UartInStartReq(me->m_uartInHsmn, GET_HSMN(), GEN_SEQ(), me->m_inFifo);
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->GetHsm().ClearInSeq();
            status = Q_HANDLED();
            break;
        }
        case UART_OUT_START_CFM:
        case UART_IN_START_CFM: {
            EVENT(e);
            me->HandleCfmRsp(ERROR_EVT_CAST(*e));
            status = Q_HANDLED();
            break;
        }
        case FAIL:
        case STATE_TIMER: {
            EVENT(e);
            Evt *evt;
            if (e->sig == FAIL) {
                ErrorEvt const &fail = ERROR_EVT_CAST(*e);
                evt = new UartActStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(),
                                         fail.GetError(), fail.GetOrigin(), fail.GetReason());

            } else {
                evt = new UartActStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_TIMEOUT, GET_HSMN());
            }
            Fw::Post(evt);
            status = Q_TRAN(&UartAct::Stopping);
            break;
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new UartActStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&UartAct::Started);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Stopping(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = UartActStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartInStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > UartOutStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();

            Evt *evt = new UartInStopReq(me->m_uartInHsmn, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new UartOutStopReq(me->m_uartOutHsmn, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->GetHsm().ClearInSeq();
            status = Q_HANDLED();
            me->GetHsm().Recall();
            break;
        }
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            me->GetHsm().Defer(e);
            status = Q_HANDLED();
            break;
        }
        case UART_IN_STOP_CFM:
        case UART_OUT_STOP_CFM: {
            EVENT(e);
            me->HandleCfmRsp(ERROR_EVT_CAST(*e));
            status = Q_HANDLED();
            break;
        }
        case FAIL:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            status = Q_HANDLED();
            break;
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new UartActStopCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&UartAct::Stopped);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Started(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("Instance = %d", GetInst(GET_HSMN()));
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartAct::Normal);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Normal(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case UART_IN_FAIL_IND:
        case UART_OUT_FAIL_IND: {
            EVENT(e);
            ErrorEvt const &ind = ERROR_EVT_CAST(*e);
            Evt *evt = new UartActFailInd(me->m_client, GET_HSMN(), GEN_SEQ(),
                                         ind.GetError(), ind.GetOrigin(), ind.GetReason());
            Fw::Post(evt);
            status = Q_TRAN(&UartAct::Failed);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Started);
            break;
        }
    }
    return status;
}

QState UartAct::Failed(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Started);
            break;
        }
    }
    return status;
}

void UartAct::HandleCfmRsp(ErrorEvt const &e) {
    Hsmn hsmn = GetHsm().GetHsmn();
    if (GetHsm().MatchOutSeq(e)) {
        if (e.GetError() == ERROR_SUCCESS) {
            if(GetHsm().IsOutSeqAllCleared()) {
                Evt *evt = new Evt(DONE, hsmn);
                PostSync(evt);
            }
        } else {
            Evt *evt = new Fail(hsmn, e.GetError(), e.GetOrigin(), e.GetReason());
            PostSync(evt);
        }
    }
}

/*
QState UartAct::MyState(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartAct::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
