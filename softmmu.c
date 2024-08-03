#include <stdio.h>
#include <stdint.h>

#define MASK 0x1FF   // 9位掩码
#define SHIFT 12     // 页内偏移量的位数

typedef struct {
    uint64_t entries[512];
} PageTable;

uint64_t translate(PageTable *pgd, uint64_t va) {
    uint64_t pud_index, pmd_index, pte_index, offset;

    pud_index = (va >> 39) & MASK;
    pmd_index = (va >> 30) & MASK;
    pte_index = (va >> 21) & MASK;
    offset = va & ((1ULL << SHIFT) - 1);

    if (pgd->entries[pud_index] == 0) {
        // PUD项不存在
        return 0;
    }

    PageTable *pud = (PageTable *)pgd->entries[pud_index];

    if (pud->entries[pmd_index] == 0) {
        // PMD项不存在
        return 0;
    }

    PageTable *pmd = (PageTable *)pud->entries[pmd_index];

    if (pmd->entries[pte_index] == 0) {
        // PTE项不存在
        return 0;
    }

    uint64_t pa = pmd->entries[pte_index] + offset;

    return pa;
}

int main() {
    return 0;
}

