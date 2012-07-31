#ifndef __ASM_MACH_AR7240_KMALLOC_H
#define __ASM_MACH_AR7240_KMALLOC_H


#ifndef CONFIG_DMA_COHERENT
/*
 * Total overkill for most systems but need as a safe default.
 */
#ifdef CONFIG_KMALLOC_MINALIGN_64BYTE
#define ARCH_KMALLOC_MINALIGN	64
#endif
#endif

#endif /* __ASM_MACH_AR7240_KMALLOC_H */
