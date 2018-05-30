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

#include "bsp.h"
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LedInterface.h"
#include "Led.h"

FW_DEFINE_THIS_FILE("Led.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
};

static char const * const interfaceEvtName[] = {
    "LED_START_REQ",
    "LED_START_CFM",
    "LED_STOP_REQ",
    "LED_STOP_CFM",
    "LED_ON_REQ",
    "LED_ON_CFM",
    "LED_OFF_REQ",
    "LED_OFF_CFM",
};

Led::Led(Hsmn hsmn, char const *name) :
    Region((QStateHandler)&Led::InitialPseudoState, hsmn, name,
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_client(HSM_UNDEF),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {}

QState Led::InitialPseudoState(Led * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Led::Root);
}

QState Led::Root(Led * const me, QEvt const * const e) {
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
            status = Q_TRAN(&Led::Stopped);
            break;
        }
        case LED_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState Led::Stopped(Led * const me, QEvt const * const e) {
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
        case LED_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_client = req.GetFrom();
            Evt *evt = new LedStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::Started);
            break;
        }
        default: {
            status = Q_SUPER(&Led::Root);
            break;
        }
    }
    return status;
}

QState Led::Started(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);       
            BSP_LED_Init(LED2);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            // Gallium - todo ST BSP does not provide BSP_LED_DeInit().
            //BSP_LED_DeInit(LED2);
            status = Q_HANDLED();
            break;
        }
        case LED_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::Stopped);
            break;
        }
        case LED_ON_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOnCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            BSP_LED_On(LED2);
            status = Q_HANDLED();
            break;
        }
        case LED_OFF_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOffCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);   
            BSP_LED_Off(LED2);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&Led::Root);
            break;
        }
    }
    return status;
}

/*
QState Led::MyState(Led * const me, QEvt const * const e) {
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
            status = Q_TRAN(&Led::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&Led::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
