#include <linux/module.h>
#include <linux/highmem.h>
#include <asm/pgtable.h>

MODULE_LICENSE("GPL");

void va2pa(unsigned long va) {
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep, pte;
    unsigned long pa, offset;
    
    pgd = pgd_offset(current->mm, va);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd))) {
        printk("Invalid PGD\n");
        return;
    }

    pud = pud_offset(pgd, va);
    if (pud_none(*pud) || unlikely(pud_bad(*pud))) {
        printk("Invalid PUD\n");
        return;
    }

    pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd))) {
        printk("Invalid PMD\n");
        return;
    }

    ptep = pte_offset_map(pmd, va);
    if (!ptep) {
        printk("Invalid PTEP\n");
        return;
    }
    
    pte = *ptep;
    pa = (pte_val(pte) & PAGE_MASK) | (~PAGE_MASK & va);
    offset = va & ~PAGE_MASK;
    
    printk("Virtual addr 0x%lx is mapped to physical addr 0x%lx, offset is 0x%lx\n", va, pa, offset);
    pte_unmap(ptep);
}

static int __init v2p_init(void) {
    unsigned long va = (unsigned long)__builtin_return_address(0);
    va2pa(va);
    return 0;
}

static void __exit v2p_exit(void) {
    printk("Module exit\n");
}

module_init(v2p_init);
module_exit(v2p_exit);

