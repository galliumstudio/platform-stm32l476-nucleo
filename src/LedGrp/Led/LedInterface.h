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

#ifndef LED_INTERFACE_H
#define LED_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

enum {
    LED_START_REQ = INTERFACE_EVT_START(LED),
    LED_START_CFM,
    LED_STOP_REQ,
    LED_STOP_CFM,
    LED_ON_REQ,
    LED_ON_CFM,
    LED_OFF_REQ,
    LED_OFF_CFM,
};

enum {
    LED_REASON_UNSPEC = 0,
};

class LedStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    LedStartReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(LED_START_REQ, to, from, seq) {}
};

class LedStartCfm : public ErrorEvt {
public:
    LedStartCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_START_CFM, to, from, seq, error, origin, reason) {}
};

class LedStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    LedStopReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(LED_STOP_REQ, to, from, seq) {}
};

class LedStopCfm : public ErrorEvt {
public:
    LedStopCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_STOP_CFM, to, from, seq, error, origin, reason) {}
};

class LedOnReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    LedOnReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(LED_ON_REQ, to, from, seq) {}
};

class LedOnCfm : public ErrorEvt {
public:
    LedOnCfm(Hsmn to, Hsmn from, Sequence seq,
                 Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_ON_CFM, to, from, seq, error, origin, reason) {}
};

class LedOffReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    LedOffReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(LED_OFF_REQ, to, from, seq) {}
};

class LedOffCfm : public ErrorEvt {
public:
    LedOffCfm(Hsmn to, Hsmn from, Sequence seq,
                  Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_OFF_CFM, to, from, seq, error, origin, reason) {}
};

} // namespace APP

#endif // LED_INTERFACE_H
