#ifndef BC_INSTRUCTIONS_H
#define BC_INSTRUCTIONS_H

#include <stdint.h>

#define BC_INST_CALL ((uint8_t) 'C')
#define BC_INST_RETURN ((uint8_t) 'R')
#define BC_INST_RETURN_ZERO ((uint8_t) '$')

#define BC_INST_PUSH_LAST ((uint8_t) 'L')
#define BC_INST_PUSH_SCALE ((uint8_t) '.')
#define BC_INST_PUSH_IBASE ((uint8_t) 'I')
#define BC_INST_PUSH_OBASE ((uint8_t) 'O')

#define BC_INST_PUSH_NUM ((uint8_t) 'N')
#define BC_INST_PUSH_ZERO ((uint8_t) '0')
#define BC_INST_POP ((uint8_t) 'P')

#define BC_INST_HALT ((uint8_t) 'H')

#define BC_INST_PRINT ((uint8_t) 'p')
#define BC_INST_STR ((uint8_t) 'S')
#define BC_INST_PRINT_STR ((uint8_t) 's')

#endif // BC_INSTRUCTIONS_H
