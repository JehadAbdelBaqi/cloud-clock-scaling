#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * Board <-> gateway UART protocol. Must match docs/protocol.md.
 *
 *   Board -> gateway   S,<seq>,<peak>\n          sound reading
 *                      A,<seq>,<mhz>\n           ack: clock switched
 *   Gateway -> board   C,<seq>,<level>,<hold_s>\n
 */

#include <stdint.h>

#define PROTO_BAUD              115200u

#define PROTO_HEARTBEAT_MS      5000u   /* send a reading at least this often */
#define PROTO_LINE_MAX          32u     /* longest line incl. '\n' and '\0'   */

/* Clock levels — index into clock_mhz[] */
typedef enum {
    CLOCK_LOW  = 0,   /* 16 MHz, HSI  */
    CLOCK_MED  = 1,   /* 50 MHz, PLL  */
    CLOCK_HIGH = 2    /* 100 MHz, PLL */
} clock_level_t;

static const uint8_t clock_mhz[] = { 16u, 50u, 100u };

/* Parsed C,<seq>,<level>,<hold_s> command */
typedef struct {
    uint32_t      seq;
    clock_level_t level;
    uint16_t      hold_s;
} proto_command_t;

#endif /* PROTOCOL_H */
