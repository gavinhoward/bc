#ifndef BC_H
#define BC_H

#include <stdint.h>

#define BC_FLAG_WARN (1<<0)
#define BC_FLAG_VERSION (1<<1)
#define BC_FLAG_STANDARD (1<<2)
#define BC_FLAG_QUIET (1<<3)
#define BC_FLAG_MATHLIB (1<<4)
#define BC_FLAG_INTERACTIVE (1<<5)
#define BC_FLAG_HELP (1<<6)

typedef void (*BcFreeFunc)(void*);
typedef int (*BcCmpFunc)(void*, void*);

#define BC_BASE_MAX_DEF (99)
#define BC_DIM_MAX_DEF (2048)
#define BC_SCALE_MAX_DEF (99)
#define BC_STRING_MAX_DEF (1024)

typedef enum BcStatus {

  BC_STATUS_SUCCESS,

  BC_STATUS_INVALID_OPTION,
  BC_STATUS_MALLOC_FAIL,
  BC_STATUS_INVALID_PARAM,

  BC_STATUS_NO_LIMIT,
  BC_STATUS_INVALID_LIMIT,

  BC_STATUS_LEX_INVALID_TOKEN,
  BC_STATUS_LEX_NO_STRING_END,
  BC_STATUS_LEX_NO_COMMENT_END,
  BC_STATUS_LEX_EOF,

  BC_STATUS_PARSE_INVALID_TOKEN,
  BC_STATUS_PARSE_INVALID_EXPR,
  BC_STATUS_PARSE_INVALID_PRINT,
  BC_STATUS_PARSE_INVALID_FUNC,
  BC_STATUS_PARSE_INVALID_ASSIGN,
  BC_STATUS_PARSE_NO_AUTO,
  BC_STATUS_PARSE_LIMITS,
  BC_STATUS_PARSE_QUIT,
  BC_STATUS_PARSE_EOF,
  BC_STATUS_PARSE_BUG,

  BC_STATUS_VM_FILE_ERR,
  BC_STATUS_VM_FILE_READ_ERR,
  BC_STATUS_VM_DIVIDE_BY_ZERO,
  BC_STATUS_VM_NEG_SQRT,
  BC_STATUS_VM_MISMATCHED_PARAMS,
  BC_STATUS_VM_UNDEFINED_FUNC,
  BC_STATUS_VM_FILE_NOT_EXECUTABLE,
  BC_STATUS_VM_SIGACTION_FAIL,
  BC_STATUS_VM_INVALID_SCALE,
  BC_STATUS_VM_INVALID_IBASE,
  BC_STATUS_VM_INVALID_OBASE,
  BC_STATUS_VM_INVALID_STMT,
  BC_STATUS_VM_INVALID_EXPR,
  BC_STATUS_VM_INVALID_STRING,
  BC_STATUS_VM_STRING_LEN,
  BC_STATUS_VM_INVALID_NAME,
  BC_STATUS_VM_ARRAY_LENGTH,
  BC_STATUS_VM_INVALID_TEMP,
  BC_STATUS_VM_INVALID_READ_EXPR,
  BC_STATUS_VM_PRINT_ERR,
  BC_STATUS_VM_BREAK,
  BC_STATUS_VM_CONTINUE,
  BC_STATUS_VM_HALT,

  BC_STATUS_POSIX_NAME_LEN,
  BC_STATUS_POSIX_SCRIPT_COMMENT,

} BcStatus;

#define BC_POSIX_ERR_EXIT_OR_WARN(status, file, line, msg)  \
  do {                                                   \
    if (bc_std || bc_warn) {                             \
      bc_posix((status), (file), (line), (msg));         \
      if (bc_std) return (status);                       \
    }                                                    \
  } while (0)

BcStatus bc_exec(unsigned int flags, unsigned int filec, const char *filev[]);

void bc_error(BcStatus status);
void bc_error_file(BcStatus status, const char* file, uint32_t line);

void bc_posix(BcStatus status, const char* f, uint32_t line, const char* msg);

extern long bc_interactive;
extern long bc_std;
extern long bc_warn;

extern long bc_had_sigint;

extern const char* const bc_mathlib;
extern const char* const bc_version;

#endif // BC_H
