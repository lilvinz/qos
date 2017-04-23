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

#ifndef MODULE_INIT_H_
#define MODULE_INIT_H_

typedef void (*initcall_t)(void);

typedef struct
{
    initcall_t fn_minit;
    initcall_t fn_mstart;
    initcall_t fn_mstop;
} initmodule_t;

// variables from linker script
extern initmodule_t __module_initcall_start[], __module_initcall_end[];

/* initcalls are now grouped by functionality into separate
 * subsections. Ordering inside the subsections is determined
 * by link order.
 *
 * The `id' arg to __define_initcall() is needed so that multiple initcalls
 * can point at the same handler without causing duplicate-symbol build errors.
 */

#define __define_module_initcall(level, initfn, startfn, stopfn)               \
    static const initmodule_t __initcall_##fn __attribute__((__used__))        \
    __attribute__((__section__(".initcall." #level ".init")))                  \
    = { .fn_minit = initfn, .fn_mstart = startfn, .fn_mstop = stopfn };

#define MODULE_INITCALL(level, initfn, startfn, stopfn) \
    __define_module_initcall(level, initfn, startfn, stopfn)

#define MODULE_INITIALISE_ALL()                                                \
    {                                                                          \
        for (initmodule_t *fn = __module_initcall_start;                       \
            fn < __module_initcall_end;                                        \
            ++fn)                                                              \
        {                                                                      \
            if (fn->fn_minit)                                                  \
                (fn->fn_minit)();                                              \
        }                                                                      \
    }

#define MODULE_START_ALL()                                                     \
    {                                                                          \
        for (initmodule_t *fn = __module_initcall_start;                       \
            fn < __module_initcall_end;                                        \
            ++fn)                                                              \
        {                                                                      \
            if (fn->fn_mstart)                                                 \
                (fn->fn_mstart)();                                             \
        }                                                                      \
    }

#define MODULE_STOP_ALL()                                                      \
    {                                                                          \
        for (initmodule_t *fn = __module_initcall_end - 1;                     \
            __module_initcall_end > __module_initcall_start &&                 \
            __module_initcall_end - 1 >= __module_initcall_start &&            \
            fn >= __module_initcall_start;                                     \
            --fn)                                                              \
        {                                                                      \
            if (fn->fn_mstop)                                                  \
                (fn->fn_mstop)();                                              \
        }                                                                      \
    }

#endif /* MODULE_INIT_H_ */
