/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
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

#include <stdarg.h>
#include <stdio.h>
#include "bsp.h"
#include "qpcpp.h"
#include "fw_hsm.h"
#include "fw_pipe.h"
#include "fw_log.h"
#include "fw.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_log.cpp")

using namespace QP;

namespace FW {

uint8_t Log::m_verbosity = DEFAULT_VERBOSITY;
uint32_t Log::m_onStor[ROUND_UP_DIV(MAX_HSM_COUNT, 32)];
Bitset Log::m_on(m_onStor, ARRAY_COUNT(m_onStor), MAX_HSM_COUNT);
Log::HsmnInf Log::m_hsmnInfStor[MAX_HSM_COUNT];
Log::HsmnInfMap Log::m_hsmnInfMap(m_hsmnInfStor, ARRAY_COUNT(m_hsmnInfStor), HsmnInf(HSM_UNDEF, Inf(NULL, 0)));

char const * const Log::m_typeName[NUM_TYPE] = {
    "<ERROR>",
    "<WARNING>",
    "<CRITICAL>",
    "",
    ""
};
char const Log::m_truncatedError[] = "<##TRUN##>";

void Log::AddInterface(Hsmn hsmn, Fifo *fifo, QSignal sig) {
    FW_LOG_ASSERT((hsmn != HSM_UNDEF) && fifo && sig);
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_hsmnInfMap.Save(HsmnInf(hsmn, Inf(fifo, sig)));
    QF_CRIT_EXIT(crit);
}

void Log::RemoveInterface(Hsmn hsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_hsmnInfMap.ClearByKey(hsmn);
    QF_CRIT_EXIT(crit);
}

void Log::Write(char const *buf, uint32_t len) {
    uint32_t writeCount = 0;
    uint32_t index = m_hsmnInfMap.GetTotalCount();
    while (index--) {
        HsmnInf *hsmnInf = m_hsmnInfMap.GetByIndex(index);
        // Maintain critical section within loop to reduce interrupt latency.
        // Okay to miss a new entry after passing its index.
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        Hsmn hsm = hsmnInf->GetKey();
        if (hsm != HSM_UNDEF) {
            Fifo *fifo = hsmnInf->GetValue().GetFifo();
            FW_ASSERT(fifo);
            bool status1 = false;
            bool status2 = false;
            if (fifo->IsTruncated()) {
                fifo->WriteNoCrit(reinterpret_cast<uint8_t const *>(m_truncatedError), CONST_STRING_LEN(m_truncatedError), &status1);
            }
            if (!fifo->IsTruncated()) {
                fifo->WriteNoCrit(reinterpret_cast<uint8_t const *>(buf), len, &status2);
            }
            QF_CRIT_EXIT(crit);
            // Post MUST be outside critical section.
            if (status1 || status2) {
                QSignal sig = hsmnInf->GetValue().GetSig();
                FW_ASSERT(sig);
                Evt *evt = new Evt(sig, hsm);
                Fw::Post(evt);
            }
            writeCount++;
        } else {
            QF_CRIT_EXIT(crit);
        }
    }
    if (writeCount == 0) {
        // Gallium - If no FIFO has been setup, write to BSP usart directly.
        BspWrite(buf, len);
    }
}

uint32_t Log::Print(char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[BUF_LEN];
    uint32_t len = vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    len = LESS(len, sizeof(buf) - 1);
    Write(buf, len);
    return len;
}

void Log::PrintItem(uint32_t &index, uint32_t minWidth, uint32_t itemPerLine, char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    uint32_t len = vsnprintf(buf, MAX_LEN, format, arg);
    va_end(arg);
    len = LESS(len, (MAX_LEN - 1));
    if (len < (MAX_LEN - 1)) {
        uint32_t paddedLen = ROUND_UP_DIV(len, minWidth) * minWidth;
        paddedLen = LESS(paddedLen, MAX_LEN - 1);
        FW_ASSERT((paddedLen >= len) && (paddedLen <= (MAX_LEN - 1)));
        memset(&buf[len], ' ', paddedLen - len);
        len = paddedLen;
        buf[len] = 0;
    }
    if (((index + 1) % itemPerLine) == 0) {
        FW_ASSERT(len <= (sizeof(buf) - 3));
        buf[len++] = '\n';
        buf[len++] = '\r';
        buf[len] = 0;
    }
    Write(buf, len);
}

void Log::Event(Type type, Hsmn hsmn, QP::QEvt const *e, char const *func) {
    FW_ASSERT(e && func);
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    if (e->sig == QHsm::Q_ENTRY_SIG) {
        hsm->SetState(func);
    }
    if (!IsOutput(type, hsmn)) {
        return;
    }
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    uint32_t len = snprintf(buf, MAX_LEN, "%lu %s(%u): %s %s ", GetSystemMs(), hsm->GetName(), hsmn, func, GetEvtName(e->sig));
    len = LESS(len, (MAX_LEN - 1));
    if (len < (MAX_LEN - 1)) {
        // Add details of event if available.
        if (IS_EVT_HSMN_VALID(e->sig) && !IS_TIMER_EVT(e->sig)) {
            Evt const *evt = static_cast<Evt const *>(e);
            Hsmn from = evt->GetFrom();
            len += snprintf(&buf[len], MAX_LEN - len, "from %s(%d) seq=%d", GetHsmName(from), from, evt->GetSeq());
            len = LESS(len, MAX_LEN - 1);
        }
    }
    FW_ASSERT(len <= (sizeof(buf) - 3));
    buf[len++] = '\n';
    buf[len++] = '\r';
    buf[len] = 0;
    Write(buf, len);
}

void Log::Debug(Type type, Hsmn hsmn, char const *format, ...) {
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    if (!IsOutput(type, hsmn)) {
        return;
    }
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    // Note there is no space after type name.
    uint32_t len = snprintf(buf, MAX_LEN, "%lu %s(%u): %s", GetSystemMs(), hsm->GetName(), hsmn, GetTypeName(type));
    len = LESS(len, (MAX_LEN - 1));
    if (len < (MAX_LEN - 1)) {
        va_list arg;
        va_start(arg, format);
        len += vsnprintf(&buf[len], MAX_LEN - len, format, arg);
        va_end(arg);
        len = LESS(len, MAX_LEN - 1);
    }
    FW_ASSERT(len <= (sizeof(buf) - 3));
    buf[len++] = '\n';
    buf[len++] = '\r';
    buf[len] = 0;
    Write(buf, len);
}

void Log::BufLinePrint(uint8_t const *lineBuf, uint32_t lineLen, uint8_t unit, uint32_t lineLabel) {
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    // Print label.
    uint32_t len = snprintf(buf, MAX_LEN, "[0x%.8lx] ", lineLabel);
    len = LESS(len, MAX_LEN - 1);
    // Print hex data.
    uint8_t i = 0;
    for (i = 0; i < lineLen; i += unit) {
        if (len < (MAX_LEN - 1)) {
            if (unit == 1) {
                len += snprintf(&buf[len], MAX_LEN - len, "%.2x ", lineBuf[i]);
            } else if (unit == 2) {
                len += snprintf(&buf[len], MAX_LEN - len, "%.4x ", *((uint16_t *)&lineBuf[i]));
            } else {
                len += snprintf(&buf[len], MAX_LEN - len, "%.8lx ", *((uint32_t *)&lineBuf[i]));
            }
            len = LESS(len, MAX_LEN - 1);
        }
    }
    // Print padding.
    if (len < (MAX_LEN - 1)) {
        // Each unit has 2 * unit byte size plus a space.
        uint32_t padding = ((BYTE_PER_LINE - lineLen) / unit) * (unit * 2 + 1);
        padding =  LESS(padding, MAX_LEN - 1 - len);
        FW_ASSERT((len + padding) <= (MAX_LEN - 1));
        memset(&buf[len], ' ', padding);
        len += padding;
        buf[len] = 0;
    }
    // Print ASCII.
    for (i = 0; i < lineLen; i++) {
        if (len < (MAX_LEN - 1)) {
            char ch = lineBuf[i];
            buf[len++] = ((ch >= 0x20) && (ch <= 0x7E)) ? ch : '_';
        }
    }
    buf[len] = 0;
    // Print newline.
    FW_ASSERT(len <= (sizeof(buf) - 3));
    buf[len++] = '\n';
    buf[len++] = '\r';
    buf[len] = 0;
    Write(buf, len);
}

void Log::BufPrint(uint8_t const *dataBuf, uint32_t dataLen, uint8_t unit, uint32_t label) {
    FW_ASSERT((unit == 1) || (unit == 2) || (unit == 4));
    FW_ASSERT(dataBuf && (((uint32_t)dataBuf % unit) == 0) && ((dataLen % unit) == 0) && ((BYTE_PER_LINE % unit) == 0));
    Print("Buffer 0x%.8x len %lu:\n\r", dataBuf, dataLen);
    uint32_t dataIndex = 0;
    while (dataIndex < dataLen) {
        // Print at most 16 bytes of data in each line.
        uint8_t count = BYTE_PER_LINE;
        if (dataIndex + count > dataLen) {
            count = dataLen - dataIndex;
        }
        Log::BufLinePrint(&dataBuf[dataIndex], count, unit, label + dataIndex);
        dataIndex += count;
    }
}

void Log::BufDebug(Type type, Hsmn hsmn, uint8_t const *dataBuf, uint32_t dataLen, uint8_t align, uint32_t label) {
    if (IsOutput(type, hsmn)) {
        BufPrint(dataBuf, dataLen, align, label);
    }
}

void Log::On(Hsmn hsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.Set(hsmn);
    QF_CRIT_EXIT(crit);
}

void Log::Off(Hsmn hsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.Clear(hsmn);
    QF_CRIT_EXIT(crit);
}

void Log::OnAll() {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.SetAll();
    QF_CRIT_EXIT(crit);
}

void Log::OffAll() {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.ClearAll();
    QF_CRIT_EXIT(crit);
}

bool Log::IsOutput(Type type, Hsmn hsmn) {
    return (type < m_verbosity) && m_on.IsSet(hsmn);
}

char const *Log::GetEvtName(QP::QSignal sig) {
    if (sig < Q_USER_SIG) {
        return Hsm::GetBuiltinEvtName(sig);
    };
    Hsmn hsmn = GET_EVT_HSMN(sig);
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    return hsm->GetEvtName(sig);
}

// Must allow HSM_UNDEF since the "m_from" hsmn of an event is optional
// (e.g. an internal event or event sent from main).
char const *Log::GetHsmName(Hsmn hsmn) {
    Hsm *hsm = Fw::GetHsm(hsmn);
    if (!hsm) {
        return Hsm::GetUndefName();
    }
    return hsm->GetName();
}

char const *Log::GetTypeName(Type type) {
    FW_ASSERT(type < NUM_TYPE);
    return m_typeName[type];
}

} // namespace FW
