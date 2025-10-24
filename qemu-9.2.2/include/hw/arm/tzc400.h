/*
 * Minimal TZC-400 QEMU device (register-only, no enforcement)
 * Header
 */
#ifndef HW_ARM_TZC400_H
#define HW_ARM_TZC400_H

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "exec/memory.h"

#define TYPE_TZC400 "tzc400"
#define TZC400(obj) OBJECT_CHECK(TZC400State, (obj), TYPE_TZC400)

typedef struct TZCRegion {
    uint64_t base; /* region base */
    uint64_t top;  /* region top */
    uint32_t attr; /* attributes (enable, ns/rw bits, etc) */
} TZCRegion;

typedef struct TZC400State {
    /* parent */
    SysBusDevice parent_obj;

    /* mmio region for registers */
    MemoryRegion iomem;

    /* IRQ to GIC */
    qemu_irq irq;

    /* number of regions supported */
    unsigned int num_regions;

    /* settings */
    uint32_t ctrl;
    uint32_t action;
    uint32_t intstatus;

    /* region storage */
    TZCRegion *regions;
} TZC400State;

#endif /* HW_ARM_TZC400_H */