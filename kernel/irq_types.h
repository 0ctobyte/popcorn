#ifndef _IRQ_TYPES_H_
#define _IRQ_TYPES_H_

typedef uint8_t irq_priority_t;
typedef uint16_t irq_id_t;

typedef enum {
    IRQ_TYPE_LEVEL_SENSITIVE,
    IRQ_TYPE_EDGE_TRIGGERED,
} irq_type_t;

#endif // _IRQ_TYPES_H_
