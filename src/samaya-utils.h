#pragma once

#include <glib.h>

// Returns 0 if subtraction would underflow, otherwise returns a - b
static inline guint64 G_GNUC_UNUSED guint64_sat_sub(guint64 a, guint64 b)
{
    return (b > a) ? 0 : (a - b);
}
