#include "qemu/osdep.h"
#include "qemu/qemu-print.h"
#include "exec/log.h"
#include "migration/vmstate.h"
#include "hw/qdev-core.h"
#include "hw/qdev-properties.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#include "hw/gpu/mali-g71.h"

typedef struct MaliGPUState
{
    MemoryRegion mmio;
    uint8_t *vram;
    SysBusDevice parent_obj;
    qemu_irq irq_line;

    // MMIO寄存器状态
    struct
    {
        uint32_t status;
        uint32_t job_slot;
        // 添加其他必要寄存器...
    } regs;
} MaliGPUState;

#define TYPE_PCI_MALI_GPU_DEVICE "mali-g71"
DECLARE_INSTANCE_CHECKER(MaliGPUState, MALI_GPU,
                         TYPE_PCI_MALI_GPU_DEVICE)

static uint64_t mali_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    MaliGPUState *s = opaque;
    switch (addr)
    {
    case 0x0000: // 状态寄存器
        return s->regs.status;
    case 0x1000: // Job slot 0
        return s->regs.job_slot;
    // 添加其他寄存器处理...
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "Bad MMIO read @0x%" HWADDR_PRIx "\n", addr);
        return 0;
    }
}

static void mali_mmio_write(void *opaque, hwaddr addr,
                            uint64_t value, unsigned size)
{
    MaliGPUState *s = opaque;
    switch (addr)
    {
    case 0x0000:
        s->regs.status = value;
        if (value & 0x1)
        { // 示例：清除中断
            qemu_irq_lower(s->irq_line);
        }
        break;
    case 0x2000: // DMA控制寄存器
        if (value & 0x1)
        {
            // dma_memory_write(&s->dma, s->vram, 0x1000, value);
        }
        break;
        // 添加其他寄存器处理...
    }
}

static const MemoryRegionOps mali_mmio_ops = {
    .read = mali_mmio_read,
    .write = mali_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void mali_irq_handler(void *opaque, int n, int level)
{
    MaliGPUState *s = opaque;
    if (level) {
        // 触发中断
        qemu_irq_raise(s->irq_line);
    } else {
        // 清除中断
        qemu_irq_lower(s->irq_line);
    }
}

static void mali_gpu_realize(DeviceState *dev, Error **errp)
{
    MaliGPUState *s = MALI_GPU(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    // 初始化MMIO区域
    memory_region_init_io(&s->mmio, OBJECT(s), &mali_mmio_ops, s,
                          "mali-gpu-mmio", 0x1000000);
    sysbus_init_mmio(sbd, &s->mmio);

    // 注册中断
    s->irq_line = qemu_allocate_irq(mali_irq_handler, s, 0);
    sysbus_init_irq(sbd, &s->irq_line);

    // 分配虚拟内存
    s->vram = g_malloc0(0x100000); // 1MB虚拟显存
}

// 设备初始化
static void mali_gpu_init(Object *obj)
{
    MaliGPUState *s = g_malloc0(sizeof(*s));

    // 初始化MMIO区域
    memory_region_init_io(&s->mmio, OBJECT(s), &mali_mmio_ops, s,
                          "mali-gpu", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->mmio);

    // 分配虚拟显存
    s->vram = g_malloc(0x1000000);
}

static const VMStateDescription vmstate_mali_gpu = {
    .name = "mali-gpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(regs.status, MaliGPUState),
        VMSTATE_END_OF_LIST()
    }
};

static Property mali_gpu_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void mali_gpu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = mali_gpu_realize;
    dc->vmsd = &vmstate_mali_gpu;
    device_class_set_props(dc, mali_gpu_properties);
}

static const TypeInfo mali_gpu_info = {
    .name = TYPE_MALI_GPU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MaliGPUState),
    .class_init = mali_gpu_class_init,
    .instance_init = mali_gpu_init,
};

static void mali_gpu_register_types(void)
{
    type_register_static(&mali_gpu_info);
}

type_init(mali_gpu_register_types);
