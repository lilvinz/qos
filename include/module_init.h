/*
 * module_init.h
 *
 *  Created on: 15.12.2013
 *      Author: vke
 */

#ifndef MODULE_INIT_H_
#define MODULE_INIT_H_

typedef void (*initcall_t)(void);
typedef struct
{
    initcall_t fn_minit;
    initcall_t fn_mstart;
    initcall_t fn_mterm;
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

#define __define_module_initcall(level, ifn, sfn, tfn) \
    static const initmodule_t __initcall_##fn __attribute__((__used__)) \
    __attribute__((__section__(".initcall." #level ".init"))) = { .fn_minit = ifn, .fn_mstart = sfn, .fn_mterm = tfn };

#define MODULE_INITCALL(level, ifn, sfn, tfn) __define_module_initcall(level, ifn, sfn, tfn)

#define MODULE_INITIALISE_ALL() \
    { \
        for (initmodule_t *fn = __module_initcall_start; fn < __module_initcall_end; fn++) \
        { \
            if (fn->fn_minit) \
                (fn->fn_minit)(); \
        } \
    }

#define MODULE_START_ALL() \
    { \
        for (initmodule_t *fn = __module_initcall_start; fn < __module_initcall_end; fn++) \
        { \
            if (fn->fn_mstart) \
                (fn->fn_mstart)(); \
        } \
    }

#define MODULE_TERMINATE_ALL() \
    { \
        for (initmodule_t *fn = __module_initcall_start; fn < __module_initcall_end; fn++) \
        { \
            if (fn->fn_mterm) \
                (fn->fn_mterm)(); \
        } \
    }

#endif /* MODULE_INIT_H_ */
