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
#include "BtnInterface.h"
#include "Btn.h"
#include "bsp.h"

FW_DEFINE_THIS_FILE("Btn.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "BTN_TRIG",
    "BTN_UP",
    "BTN_DOWN",
};

static char const * const interfaceEvtName[] = {
    "BTN_START_REQ",
    "BTN_START_CFM",
    "BTN_STOP_REQ",
    "BTN_STOP_CFM",
    "BTN_UP_IND",
    "BTN_DOWN_IND",
};

Btn::HsmnPinMap &Btn::GetHsmnPinMap() {
    static HsmnPin hsmnPinStor[BTN_COUNT];
    static HsmnPinMap hsmnPinMap(hsmnPinStor, ARRAY_COUNT(hsmnPinStor), HsmnPin(HSM_UNDEF, 0));
    return hsmnPinMap;
}

// No need to have critical section since mapping is not changed after global/static object construction.
void Btn::SavePin(Hsmn hsmn, uint16_t pin) {
    GetHsmnPinMap().Save(HsmnPin(hsmn, pin));
}

// No need to have critical section since mapping is not changed after global/static object construction.
Hsmn Btn::GetHsmn(uint16_t pin) {
    HsmnPin *kv = GetHsmnPinMap().GetFirstByValue(pin);
    FW_ASSERT(kv);
    Hsmn hsmn = kv->GetKey();
    FW_ASSERT(hsmn != HSM_UNDEF);
    return hsmn;
}

void Btn::ConfigGpioInt(GPIO_TypeDef *port, uint16_t pin) {
    FW_ASSERT(port);
    GPIO_InitTypeDef   GPIO_InitStructure;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Pin = pin;
    HAL_GPIO_Init(port, &GPIO_InitStructure);

    IRQn_Type irq;
    uint32_t prio;
    switch(pin) {
        case GPIO_PIN_0: irq = EXTI0_IRQn; prio =  EXTI0_PRIO; break;
        case GPIO_PIN_1: irq = EXTI1_IRQn; prio =  EXTI1_PRIO; break;
        case GPIO_PIN_2: irq = EXTI2_IRQn; prio =  EXTI2_PRIO; break;
        case GPIO_PIN_3: irq = EXTI3_IRQn; prio =  EXTI3_PRIO; break;
        case GPIO_PIN_4: irq = EXTI4_IRQn; prio =  EXTI4_PRIO; break;
        case GPIO_PIN_5:
        case GPIO_PIN_6:
        case GPIO_PIN_7:
        case GPIO_PIN_8:
        case GPIO_PIN_9: irq = EXTI9_5_IRQn; prio = EXTI9_5_PRIO; break;
        case GPIO_PIN_10:
        case GPIO_PIN_11:
        case GPIO_PIN_12:
        case GPIO_PIN_13:
        case GPIO_PIN_14:
        case GPIO_PIN_15: irq = EXTI15_10_IRQn; prio =  EXTI15_10_PRIO; break;
        default: FW_ASSERT(0); break;
    }
    NVIC_SetPriority(irq, prio);
    NVIC_EnableIRQ(irq);
}

void Btn::EnableGpioInt(uint16_t pin) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    EXTI->IMR1 = BIT_SET(EXTI->IMR1, pin, 0);
    QF_CRIT_EXIT(crit);
}

void Btn::DisableGpioInt(uint16_t pin) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    EXTI->IMR1 = BIT_CLR(EXTI->IMR1, pin, 0);
    QF_CRIT_EXIT(crit);
}

void Btn::GpioIntCallback(uint16_t pin) {
    static Sequence counter = 0; 
    Hsmn hsmn = Btn::GetHsmn(pin);
    Evt *evt = new Evt(Btn::BTN_TRIG, hsmn, HSM_UNDEF, counter++);
    Fw::Post(evt);
    DisableGpioInt(pin);
}

Btn::Btn(Hsmn hsmn, char const *name, GPIO_TypeDef *port, uint16_t pin) :
    Region((QStateHandler)&Btn::InitialPseudoState, hsmn, name,
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_client(HSM_UNDEF), m_port(port), m_pin(pin),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {
    // Save hsmn to pin mapping.
    SavePin(hsmn, pin);
}

QState Btn::InitialPseudoState(Btn * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Btn::Root);
}

QState Btn::Root(Btn * const me, QEvt const * const e) {
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
            status = Q_TRAN(&Btn::Stopped);
            break;
        }
        case BTN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new BtnStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
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

QState Btn::Stopped(Btn * const me, QEvt const * const e) {
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
        case BTN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new BtnStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case BTN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_client = req.GetFrom();
            Evt *evt = new BtnStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Btn::Started);
            break;
        }
        default: {
            status = Q_SUPER(&Btn::Root);
            break;
        }
    }
    return status;
}

QState Btn::Started(Btn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            ConfigGpioInt(me->m_port, me->m_pin);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            DisableGpioInt(me->m_pin);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&Btn::Up);
            break;
        }
        case BTN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new BtnStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Btn::Stopped);
            break;
        }
        default: {
            status = Q_SUPER(&Btn::Root);
            break;
        }
    }
    return status;
}

QState Btn::Up(Btn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            Evt *evt = new Evt(BTN_UP_IND, me->m_client, GET_HSMN(), GEN_SEQ());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case BTN_TRIG: {
            EVENT(e);
            EnableGpioInt(me->m_pin);
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
                Evt *evt = new Evt(BTN_DOWN, GET_HSMN(), GET_HSMN(), GEN_SEQ());
                me->PostSync(evt);
            }
            status = Q_HANDLED();
            break;
        }
        case BTN_DOWN: {
            status = Q_TRAN(&Btn::Down);
            break;
        }
        default: {
            status = Q_SUPER(&Btn::Started);
            break;
        }
    }
    return status;
}

QState Btn::Down(Btn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            Evt *evt = new Evt(BTN_DOWN_IND, me->m_client, GET_HSMN(), GEN_SEQ());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case BTN_TRIG: {
            EVENT(e);
            EnableGpioInt(me->m_pin);
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_RESET) {
                Evt *evt = new Evt(BTN_UP, GET_HSMN(), GET_HSMN(), GEN_SEQ());
                me->PostSync(evt);
            }
            status = Q_HANDLED();
            break;
        }
        case BTN_UP: {
            status = Q_TRAN(&Btn::Up);
            break;
        }
        default: {
            status = Q_SUPER(&Btn::Started);
            break;
        }
    }
    return status;
}


/*
QState Btn::MyState(Btn * const me, QEvt const * const e) {
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
            status = Q_TRAN(&Btn::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&Btn::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
