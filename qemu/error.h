/*
 * QEMU Error Objects
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.  See
 * the COPYING.LIB file in the top-level directory.
 */
#ifndef ERROR_H
#define ERROR_H

/*
 * A class representing internal errors within QEMU.  An error has a ErrorClass
 * code and a human message.
 *
 * MSVC modification: this opaque forward typedef is declared before the
 * includes below.  qapi-types.h now pulls in qemu-common.h (-> cpu.h ->
 * cpu-qom.h) which references the bare "Error" type; if error.h is the entry
 * point of that include cycle, cpu-qom.h re-includes error.h (guard already
 * set, skipped) and would see Error undefined unless it is typedef'd first.
 */
typedef struct Error Error;

#include "compiler.h"
#include "qapi-types.h"
#include <stdbool.h>

/**
 * Set an indirect pointer to an error given a ErrorClass value and a
 * printf-style human message.  This function is not meant to be used outside
 * of QEMU.
 */
void error_set(Error **err, ErrorClass err_class, const char *fmt, ...) GCC_FMT_ATTR(3, 4);

/**
 * Returns true if an indirect pointer to an error is pointing to a valid
 * error object.
 */
bool error_is_set(Error **err);

/*
 * Get the error class of an error object.
 */
ErrorClass error_get_class(const Error *err);

/**
 * Returns an exact copy of the error passed as an argument.
 */
Error *error_copy(const Error *err);

/**
 * Get a human readable representation of an error object.
 */
const char *error_get_pretty(Error *err);

/**
 * Propagate an error to an indirect pointer to an error.  This function will
 * always transfer ownership of the error reference and handles the case where
 * dst_err is NULL correctly.  Errors after the first are discarded.
 */
void error_propagate(Error **dst_err, Error *local_err);

/**
 * Free an error object.
 */
void error_free(Error *err);

#endif
