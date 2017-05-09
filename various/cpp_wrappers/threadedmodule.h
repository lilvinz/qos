/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.hpp"
#include "module.h"

#ifndef THREADED_MODULE_H_
#define THREADED_MODULE_H_

namespace qos {

    template <int N>
    class ThreadedModule : public Module
    {
    public:

        virtual void Start()
        {
            m_moduleThread.SetModule(this);
            m_moduleThread.start(GetThreadPrio());
        }

        virtual void Shutdown()
        {
            m_moduleThread.requestTerminate();
        }

    protected:
        virtual tprio_t GetThreadPrio() const = 0;

        virtual void ThreadMain() = 0;

        class ModuleThread : public chibios_rt::BaseStaticThread<N>
        {
        public:
            ModuleThread() :
                module(NULL)
            {

            }

            void SetModule(ThreadedModule* mod)
            {
                module = mod;
            }

        protected:
            virtual void main()
            {
                module->ThreadMain();
            }

        private:
            ThreadedModule* module;
        };

        ModuleThread m_moduleThread;
    };
}

#endif /* THREADED_MODULE_H_ */
