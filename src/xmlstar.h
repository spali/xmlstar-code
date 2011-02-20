#ifndef XMLSTAR_H
#define XMLSTAR_H

#include <config.h>
#include <stdlib.h>

#if !HAVE_DECL_STRDUP
# include "strdup.h"
#endif

typedef enum { /* EXIT_SUCCESS = 0, EXIT_FAILURE = 1, */
               EXIT_BAD_ARGS = EXIT_FAILURE+1, EXIT_BAD_FILE } exit_status;

#endif  /* XMLSTAR_H */