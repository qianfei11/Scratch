/*
 * Minimal TZC-400 QEMU device (register-only, no enforcement)
 *
 * - Exposes a 4KB MMIO region (typical TZC small footprint)
 * - Implements a few control registers and region storage
 * - Exposes compatible = "arm,tzc-400" so the kernel driver can bind
 *
 * This is intentionally minimal: it stores region information and
 * allows userspace/firmware to program regions. Next step is to hook
 * it up to actually filter memory accesses.
 */

#include "hw/arm/tzc400.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "qemu/module.h"
#include "sysemu/sysemu.h"
#include <inttypes.h>

/* Offsets (typical TZC-400 layout; simplified) */
#define TZC_CTRL_OFFSET        0x000
#define TZC_ACTION_OFFSET      0x004
#define TZC_INTSTATUS_OFFSET   0x008
#define TZC_RESERVED_OFFSET    0x00c
#define TZC_REGION_BASE        0x100
#define TZC_REGION_STRIDE      0x20
#define TZC_REGION_ATTR_OFFSET 0x10

/* Region attribute bits we model (simple) */
#define TZC_REGION_ENABLE_MASK 0x1
#define TZC_REGION_NS_RD_MASK  0x2
#define TZC_REGION_NS_WR_MASK  0x4

static uint64_t tzc_read(void *opaque, hwaddr offset, unsigned size)
{
    TZC400State *s = opaque;
    uint32_t val32 = 0;

    switch (offset) {
    case TZC_CTRL_OFFSET:
        val32 = s->ctrl;
        break;
    case TZC_ACTION_OFFSET:
        val32 = s->action;
        break;
    case TZC_INTSTATUS_OFFSET:
        val32 = s->intstatus;
        break;
    default:
        if (offset >= TZC_REGION_BASE) {
            unsigned idx = (offset - TZC_REGION_BASE) / TZC_REGION_STRIDE;
            unsigned off_in_reg = (offset - TZC_REGION_BASE) % TZC_REGION_STRIDE;
            if (idx < s->num_regions) {
                TZCRegion *r = &s->regions[idx];
                if (off_in_reg == 0x0) {
                    val32 = (uint32_t)(r->base & 0xffffffff);
                } else if (off_in_reg == 0x4) {
                    val32 = (uint32_t)((r->base >> 32) & 0xffffffff);
                } else if (off_in_reg == 0x8) {
                    val32 = (uint32_t)(r->top & 0xffffffff);
                } else if (off_in_reg == 0xC) {
                    val32 = (uint32_t)((r->top >> 32) & 0xffffffff);
                } else if (off_in_reg == TZC_REGION_ATTR_OFFSET) {
                    val32 = r->attr;
                } else {
                    val32 = 0;
                }
            } else {
                val32 = 0;
            }
        } else {
            val32 = 0;
        }
    }

    return val32;
}

static void raise_irq_if_needed(TZC400State *s)
{
    /* If action requests an interrupt and intstatus is non-zero, assert irq */
    if ((s->action & 0x1) && s->intstatus) {
        qemu_set_irq(s->irq, 1);
    } else {
        qemu_set_irq(s->irq, 0);
    }
}

static void tzc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    TZC400State *s = opaque;

    switch (offset) {
    case TZC_CTRL_OFFSET:
        /* Only least significant bit as enable in our minimal model */
        s->ctrl = (uint32_t)value;
        break;
    case TZC_ACTION_OFFSET:
        s->action = (uint32_t)value;
        raise_irq_if_needed(s);
        break;
    case TZC_INTSTATUS_OFFSET:
        /* writing clears bits: guest may clear interrupt status */
        s->intstatus &= ~((uint32_t)value);
        raise_irq_if_needed(s);
        break;
    default:
        if (offset >= TZC_REGION_BASE) {
            unsigned idx = (offset - TZC_REGION_BASE) / TZC_REGION_STRIDE;
            unsigned off_in_reg = (offset - TZC_REGION_BASE) % TZC_REGION_STRIDE;
            if (idx < s->num_regions) {
                TZCRegion *r = &s->regions[idx];
                if (off_in_reg == 0x0) {
                    r->base = (r->base & ~0xffffffffULL) | (uint32_t)value;
                } else if (off_in_reg == 0x4) {
                    r->base = (r->base & 0xffffffffULL) | ((uint64_t)(uint32_t)value << 32);
                } else if (off_in_reg == 0x8) {
                    r->top = (r->top & ~0xffffffffULL) | (uint32_t)value;
                } else if (off_in_reg == 0xC) {
                    r->top = (r->top & 0xffffffffULL) | ((uint64_t)(uint32_t)value << 32);
                } else if (off_in_reg == TZC_REGION_ATTR_OFFSET) {
                    r->attr = (uint32_t)value;
                    /*
                     * If region enable and an attribute would cause a violation,
                     * in a full model we'd set intstatus and maybe trigger abort.
                     * Here we simply set intstatus if the region is enabled and
                     * NS bits are masked in a way that could generate an event.
                     */
                    if (r->attr & TZC_REGION_ENABLE_MASK) {
                        /* set a dummy intstatus bit per region for demo/testing */
                        s->intstatus |= (1U << (idx & 0x1F));
                        raise_irq_if_needed(s);
                    }
                }
            } else {
                /* ignore */
            }
        }
    }
}

static const MemoryRegionOps tzc_mmio_ops = {
    .read = tzc_read,
    .write = tzc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void tzc_reset(DeviceState *dev)
{
    TZC400State *s = TZC400(dev);
    unsigned i;
    s->ctrl = 0;
    s->action = 0;
    s->intstatus = 0;
    for (i = 0; i < s->num_regions; i++) {
        s->regions[i].base = 0;
        s->regions[i].top = 0;
        s->regions[i].attr = 0;
    }
    qemu_set_irq(s->irq, 0);
}

static void tzc_realize(DeviceState *dev, Error **errp)
{
    TZC400State *s = TZC400(dev);
    /* Allocate a small register area */
    memory_region_init_io(&s->iomem, OBJECT(s), &tzc_mmio_ops, s, "tzc400-mmio", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq);

    /* Allocate region storage: typical TZC-400 supports up to 8/16 regions
     * We pick 8 for this minimal model; you can make it configurable later.
     */
    s->num_regions = 8;
    s->regions = g_new0(TZCRegion, s->num_regions);
    tzc_reset(dev);
}

static Property tzc_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void tzc_instance_init(Object *obj)
{
    /* Nothing much to do here as of now */
}

static void tzc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->user_creatable = true;
    dc->hotpluggable = false;
    dc->legacy_reset = tzc_reset;
    dc->realize = tzc_realize;
    device_class_set_props(dc, tzc_properties);
    dc->vmsd = NULL;
    /* set short description */
    dc->desc = "ARM TZC-400 (minimal register model)";
}

static const TypeInfo tzc_info = {
    .name = TYPE_TZC400,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(TZC400State),
    .instance_init = tzc_instance_init,
    .class_init = tzc_class_init,
};

static void tzc_register_types(void)
{
    type_register(&tzc_info);
}

type_init(tzc_register_types);