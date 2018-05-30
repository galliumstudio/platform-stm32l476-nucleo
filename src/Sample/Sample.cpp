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
#include "SampleRegInterface.h"
#include "SampleInterface.h"
#include "Sample.h"

FW_DEFINE_THIS_FILE("Sample.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
    "FAILED",
};

static char const * const interfaceEvtName[] = {
    "SAMPLE_START_REQ",
    "SAMPLE_START_CFM",
    "SAMPLE_STOP_REQ",
    "SAMPLE_STOP_CFM",
};

Sample::Sample() :
    Active((QStateHandler)&Sample::InitialPseudoState, SAMPLE, "SAMPLE",
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {}

QState Sample::InitialPseudoState(Sample * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Sample::Root);
}

QState Sample::Root(Sample * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_sampleReg); i++) {
                me->m_sampleReg[i].Init(me);
            }
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&Sample::Stopped);
            break;
        }
        case SAMPLE_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new SampleStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case SAMPLE_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->GetHsm().SaveInSeq(req);
            status = Q_TRAN(&Sample::Stopping);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState Sample::Stopped(Sample * const me, QEvt const * const e) {
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
        case SAMPLE_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new SampleStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case SAMPLE_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->GetHsm().SaveInSeq(req);
            status = Q_TRAN(&Sample::Starting);
            break;
        }
        default: {
            status = Q_SUPER(&Sample::Root);
            break;
        }
    }
    return status;
}

QState Sample::Starting(Sample * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SampleStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > SampleRegStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_sampleReg); i++) {
                Evt *evt = new SampleRegStartReq(SAMPLE_REG + i, GET_HSMN(), GEN_SEQ());
                me->GetHsm().SaveOutSeq(*evt);
                Fw::Post(evt);
            }
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
        case SAMPLE_REG_START_CFM: {
            EVENT(e);
            me->HandleCfmRsp(ERROR_EVT_CAST(*e));
            status = Q_HANDLED();
            break;
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            Evt *evt;
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                evt = new SampleStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(),
                                         failed.GetError(), failed.GetOrigin(), failed.GetReason());

            } else {
                evt = new SampleStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_TIMEOUT, GET_HSMN());
            }
            Fw::Post(evt);
            status = Q_TRAN(&Sample::Stopping);
            break;
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new SampleStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Sample::Started);
            break;
        }
        default: {
            status = Q_SUPER(&Sample::Root);
            break;
        }
    }
    return status;
}

QState Sample::Stopping(Sample * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SampleStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > SampleRegStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_sampleReg); i++) {
                Evt *evt = new SampleRegStopReq(SAMPLE_REG + i, GET_HSMN(), GEN_SEQ());
                me->GetHsm().SaveOutSeq(*evt);
                Fw::Post(evt);
            }
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
        case SAMPLE_STOP_REQ: {
            EVENT(e);
            me->GetHsm().Defer(e);
            status = Q_HANDLED();
            break;
        }
        case SAMPLE_REG_STOP_CFM: {
            EVENT(e);
            me->HandleCfmRsp(ERROR_EVT_CAST(*e));
            status = Q_HANDLED();
            break;
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            status = Q_HANDLED();
            break;
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new SampleStopCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Sample::Stopped);
            break;
        }
        default: {
            status = Q_SUPER(&Sample::Root);
            break;
        }
    }
    return status;
}

QState Sample::Started(Sample * const me, QEvt const * const e) {
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
            status = Q_SUPER(&Sample::Root);
            break;
        }
    }
    return status;
}

void Sample::HandleCfmRsp(ErrorEvt const &e) {
    Hsmn hsmn = GetHsm().GetHsmn();
    if (GetHsm().MatchOutSeq(e)) {
        if (e.GetError() == ERROR_SUCCESS) {
            if(GetHsm().IsOutSeqAllCleared()) {
                Evt *evt = new Evt(DONE, hsmn);
                PostSync(evt);
            }
        } else {
            Evt *evt = new Failed(hsmn, e.GetError(), e.GetOrigin(), e.GetReason());
            PostSync(evt);
        }
    }
}

/*
QState Sample::MyState(Sample * const me, QEvt const * const e) {
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
            status = Q_TRAN(&Sample::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&Sample::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
