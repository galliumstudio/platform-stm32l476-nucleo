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
#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_timer.cpp")

using namespace QP;

// Include the following inline functions defined in ../qpcpp/source/qf_pkg.h.
// They are required to access private members of QEvt.
// Alternatively we can include ../qpcpp/source/qf_pkg.h. But that header file
// was intended for internal use within qpcpp.

//! helper macro to cast const away from an event pointer @p e_
#define QF_EVT_CONST_CAST_(e_) const_cast<QEvt *>(e_)
namespace QP {
//! access to the poolId_ of an event @p e
inline uint8_t QF_EVT_POOL_ID_(QEvt const * const e) { return e->poolId_; }
//! access to the refCtr_ of an event @p e
inline uint8_t QF_EVT_REF_CTR_(QEvt const * const e) { return e->refCtr_; }
//! decrement the refCtr_ of an event @p e
inline void QF_EVT_REF_CTR_DEC_(QEvt const * const e) {
    --(QF_EVT_CONST_CAST_(e))->refCtr_;
}
} // namespace QP

namespace FW {

// The timer signal does not matter here, since the hsmn is undefined.
// The event will be discarded by Active::dispatch().
// The signal is set to Q_USER_SIG in case the QTimeEvt constructor asserts (signal >= Q_USER_SIG).
Timer const CANCELED_TIMER(HSM_UNDEF, Q_USER_SIG);

// Allow hsmn == HSM_UNDEF. In that case GetContainer() returns NULL.
Timer::Timer(Hsmn hsmn, QP::QSignal signal) :
    QTimeEvt(signal),
    m_hsmn(hsmn) {
}

void Timer::Start(uint32_t timeoutMs, Type type) {
    QTimeEvtCtr timeoutTick = ROUND_UP_DIV(timeoutMs, BSP_MSEC_PER_TICK);
    Active *act = Fw::GetContainer(m_hsmn);
    FW_ASSERT(act && (type < INVALID));
    if (type == ONCE) {
        QTimeEvt::postIn(act, timeoutTick);
    } else {
        QTimeEvt::postEvery(act, timeoutTick);
    }
}

void Timer::Stop() {
    // Doesn't care what disarm returns. For a periodic timer, even when it is still armed
    // a previous timeout event might still be in event queue and must be removed.
    // In any cases we need to purge residue timer events in event queue.
    QTimeEvt::disarm();
    Active *act = Fw::GetContainer(m_hsmn);
    FW_ASSERT(act);
    QEQueue *eQueue = &act->m_eQueue;
    FW_ASSERT(eQueue);
    uint16_t queueCount = 0;
    QEvt const **queueStor = act->GetEvtQueueStor(&queueCount);
    FW_ASSERT(queueStor && queueCount);
    // Critical section must support nesting.
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    // Remove timer event in front of queue.
    QEvt const * frontEvt = eQueue->get();
    if (frontEvt) {
        if (frontEvt->sig == sig) {
            // Timer event is static.
            FW_ASSERT(QF_EVT_POOL_ID_(frontEvt) == 0);
            frontEvt = &CANCELED_TIMER;
        }
        // Put back front event.
        eQueue->postLIFO(frontEvt);
        // Since QEQueue::get() does not decrement reference counter, we need to
        // decrement it explicitly after postLIFO() increments it again.
        FW_ASSERT(QF_EVT_REF_CTR_(frontEvt) > 1);
        QF_EVT_REF_CTR_DEC_(frontEvt);
        // Remove timer event in queue storage if it is used.
        if (eQueue->getNFree() < queueCount) {
            for (uint16_t i = 0; i < queueCount; i++) {
                if (queueStor[i]->sig == sig) {
                    // Timer event is static.
                    FW_ASSERT(QF_EVT_POOL_ID_(queueStor[i]) == 0);
                    // Stub the removed event with a dummy event.
                    queueStor[i] = &CANCELED_TIMER;
                }
            }
        }
    }
    // Nothing to do if queue empty (frontEvt == NULL).
    QF_CRIT_EXIT(crit);
}

void Timer::Restart(uint32_t timeoutMs, Type type) {
    Stop();
    Start(timeoutMs, type);
}

} // namespace FW
