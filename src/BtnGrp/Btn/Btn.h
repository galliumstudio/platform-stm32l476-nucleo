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

#ifndef BTN_H
#define BTN_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class Btn : public Region {
public:
    Btn(Hsmn hsmn, char const *name, GPIO_TypeDef *port, uint16_t pin);

    typedef KeyValue<Hsmn, uint16_t> HsmnPin;
    typedef Map<Hsmn, uint16_t> HsmnPinMap;
    static HsmnPinMap &GetHsmnPinMap();
    static void SavePin(Hsmn hsmn, uint16_t pin);
    static Hsmn GetHsmn(uint16_t pin);
    static void GpioIntCallback(uint16_t pin);

protected:
    static QState InitialPseudoState(Btn * const me, QEvt const * const e);
    static QState Root(Btn * const me, QEvt const * const e);
        static QState Stopped(Btn * const me, QEvt const * const e);
        static QState Started(Btn * const me, QEvt const * const e);
            static QState Up(Btn * const me, QEvt const * const e);
            static QState Down(Btn * const me, QEvt const * const e);
        
    static void ConfigGpioInt(GPIO_TypeDef *port, uint16_t pin);
    static void EnableGpioInt(uint16_t pin);
    static void DisableGpioInt(uint16_t pin);
    
    Hsmn m_client;
    GPIO_TypeDef *m_port;
    uint16_t m_pin;
    Timer m_stateTimer;

    enum {
        STATE_TIMER = TIMER_EVT_START(BTN),
    };

    enum {
        BTN_TRIG = INTERNAL_EVT_START(BTN),
        BTN_UP,
        BTN_DOWN,
    };
};

} // namespace APP

#endif // BTN_H
