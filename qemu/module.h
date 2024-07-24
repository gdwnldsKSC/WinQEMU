/*
 * QEMU Module Infrastructure
 *
 * Copyright IBM, Corp. 2009
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_MODULE_H
#define QEMU_MODULE_H

/* This should not be used directly.  Use block_init etc. instead.  */
#define module_init(function, type)                                         \
static void __attribute__((constructor)) do_qemu_init_ ## function(void) {  \
    register_module_init(function, type);                                   \
}

typedef enum {
    MODULE_INIT_BLOCK,
    MODULE_INIT_DEVICE,
    MODULE_INIT_MACHINE,
    MODULE_INIT_MAX
} module_init_type;

/* This should not be used directly.  Use block_init etc. instead.  */
#ifndef _MSC_VER
#define module_init(function, type)                                         \
static void __attribute__((constructor)) do_qemu_init_ ## function(void) {  \
   register_module_init(function, type);                                    \
}
#else
#pragma section(".CRT$XCU",read)  // Designate the .CRT$XCU section

// Helper for initializer registration
#define module_init(f, p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void) 

    // Conditional for Win32 vs. Win64
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f, "")
#else
#define INITIALIZER(f) INITIALIZER2_(f, "_")
#endif
#endif



#define block_init(function) module_init(function, MODULE_INIT_BLOCK)
#define device_init(function) module_init(function, MODULE_INIT_DEVICE)
#define machine_init(function) module_init(function, MODULE_INIT_MACHINE)

void register_module_init(void (*fn)(void), module_init_type type);

void module_call_init(module_init_type type);

#endif
