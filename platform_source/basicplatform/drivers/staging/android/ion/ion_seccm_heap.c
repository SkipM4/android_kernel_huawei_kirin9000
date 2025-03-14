/*
 * drivers/staging/android/ion/ion_seccm_heap.c
 *
 * Copyright(C) 2004-2019 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "seccm: " fmt

#include <linux/cma.h>
#include <linux/delay.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <platform_include/display/linux/dpu_drmdriver.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/scatterlist.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/version.h>
#ifdef CONFIG_MM_CMA_DEBUG
#include <linux/platform_drivers/mm_cma_debug.h>
#endif

#include <asm/tlbflush.h>

#include "mm_ion_priv.h"
#include "ion.h"

#define PER_REGION_HAVE_BITS	16
#define DEVICE_MEMORY	PROT_DEVICE_nGnRE

#ifdef CONFIG_HISI_ION_SEC_HEAP_DEBUG
#define ion_sec_dbg(format, arg...)    \
		pr_err(format, ##arg)
#else
#define ion_sec_dbg(format, arg...)
#endif

struct ion_seccm_heap {
	struct ion_heap	heap;
	size_t	heap_size;
	size_t	alloc_size;
	struct device_node	*nd;
	struct device	*dev;
	struct cma		*cma;
	struct gen_pool	*pool;
	/* heap mutex */
	struct mutex	mutex;
	u64	bitmap;
	u64	align_saddr;
	u64	water_mark;
	u64	per_alloc_sz;
	u64	per_bit_sz;
	u64	region_nr;
	u64	attr;
	u64	flag;
	u64	protect_id;
};

struct cma *mm_sec_cma = NULL;

static int mm_sec_cma_set_up(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
	phys_addr_t mask = align - 1;
	unsigned long node = rmem->fdt_node;
	struct cma *cma = NULL;
	int err;

	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL))
		return -EINVAL;

	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of CMA region\n");
		return -EINVAL;
	}

	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, rmem->name, &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}
#ifdef CONFIG_MM_CMA_DEBUG
	cma_set_flag(cma, node);
#endif
	mm_sec_cma = cma;
	ion_register_dma_camera_cma((void *)cma);

	return 0;
}

RESERVEDMEM_OF_DECLARE(mm_sec_cma, "mm-cma-pool", mm_sec_cma_set_up);

static unsigned int sz2order(size_t size)
{
	unsigned int ret = (unsigned int)(ilog2(size) - PAGE_SHIFT);
	return ret;
}

static void list_for_drm_create_mem(struct ion_seccm_heap *seccm_heap)
{
	u64 nr;
	u64 water_mark = seccm_heap->water_mark;
	u64 per_alloc_sz = seccm_heap->per_alloc_sz;
	size_t alloc_count = 1UL << sz2order(per_alloc_sz);
	u64 per_bit_sz = seccm_heap->per_bit_sz;
	u64 shift, start;
	struct page *pg = NULL;
	unsigned long virt;
	drm_sec_cfg sec_cfg;
	u64 saddr = seccm_heap->align_saddr;
#ifdef CONFIG_DFX_KERNELDUMP
	int k;
	struct page *tmp_page = NULL;
#endif

	nr = water_mark / per_alloc_sz;
	while (nr) {
		pg = cma_alloc(seccm_heap->cma, alloc_count,
				   sz2order(per_bit_sz),
				   GFP_KERNEL);
		if (!pg)
			break;

#ifdef CONFIG_DFX_KERNELDUMP
		tmp_page = pg;
		for (k = 0;
			k < (int)(1L << sz2order(per_alloc_sz));
			k++) {
			SetPageMemDump(tmp_page);
			tmp_page++;
		}
#endif

		start = (page_to_phys(pg) - saddr) / per_bit_sz;
		for (shift = start;
		     shift < start + per_alloc_sz / per_bit_sz;
		     shift++)
			seccm_heap->bitmap |= (1ULL << shift);

		gen_pool_free(seccm_heap->pool, page_to_phys(pg),
			      per_alloc_sz);

		if (seccm_heap->flag & ION_FLAG_SECURE_BUFFER) {
#ifdef CONFIG_MM_ION_FLUSH_CACHE_ALL
			ion_flush_all_cpus_caches_raw();
#else
			ion_flush_all_cpus_caches();
#endif

			virt = (unsigned long)__va(page_to_phys(pg));
			change_secpage_range(page_to_phys(pg),
					    virt,
					    per_alloc_sz,
					    __pgprot(DEVICE_MEMORY));

			flush_tlb_all();
			sec_cfg.sub_rgn_size = 0;
			sec_cfg.start_addr = seccm_heap->align_saddr;
			sec_cfg.sub_rgn_size = per_bit_sz;
			sec_cfg.bit_map = seccm_heap->bitmap;
			sec_cfg.sec_port = seccm_heap->attr;
			dfx_sec_ddr_set(&sec_cfg, (int)seccm_heap->protect_id);
		} else {
			memset(page_address(pg), 0x0, per_alloc_sz); /* unsafe_function_ignore: memset  */
		}
		nr--;
	}
	ion_sec_dbg("out %s %llu MB memory bitmap 0x%llx\n", __func__,
		    (water_mark - nr * per_alloc_sz) / SZ_1M,
		    seccm_heap->bitmap);
}

static int seccm_create_pool(
	struct ion_seccm_heap *seccm_heap,
	unsigned long flag)
{
	u64 cma_base;
	u64 cma_size;

	ion_sec_dbg("into %s\n", __func__);

	if (flag & ION_FLAG_SECURE_BUFFER)
		seccm_heap->flag = ION_FLAG_SECURE_BUFFER;
	else
		seccm_heap->flag = 0;

	cma_base = cma_get_base(seccm_heap->cma);
	cma_size = cma_get_size(seccm_heap->cma);
	seccm_heap->pool = gen_pool_create(12, -1);
	if (!seccm_heap->pool) {
		pr_err("in seccm_create_pool gen_pool_create failed\n");
		return -ENOMEM;
	}

	if (gen_pool_add(seccm_heap->pool, cma_base, cma_size, -1)) {
		pr_err("cma_base 0x%llx cma_size 0x%llx\n", cma_base, cma_size);
		return -ENOMEM;
	}

	if (!gen_pool_alloc(seccm_heap->pool, cma_size)) {
		pr_err("in seccm_create_pool gen_pool_alloc failed\n");
		return -ENOMEM;
	}

	/*
	 * For the drm memory is also used by Camera,
	 * So when drm create memory pool, need clean
	 * the camera drm heap.
	 */
	ion_clean_dma_camera_cma();
	list_for_drm_create_mem(seccm_heap);

	return 0;
}

static void seccm_add_pool(struct ion_seccm_heap *seccm_heap)
{
	u64 nr;
	u64 shift, start;
	struct page *pg = NULL;
	drm_sec_cfg sec_cfg;
	u64 water_mark = seccm_heap->water_mark;
	u64 per_alloc_sz = seccm_heap->per_alloc_sz;
	u64 per_bit_sz = seccm_heap->per_bit_sz;
	u64 saddr = seccm_heap->align_saddr;
	unsigned long virt;
#ifdef CONFIG_DFX_KERNELDUMP
	int k;
	struct page *tmp_page = NULL;
#endif

	ion_sec_dbg("into %s bitmap 0x%llx\n", __func__, seccm_heap->bitmap);

	nr = water_mark / per_alloc_sz;
	while (nr) {
		pg = cma_alloc(seccm_heap->cma,
			       (size_t)(1UL << sz2order(per_alloc_sz)),
			       sz2order(per_bit_sz), GFP_KERNEL);
		if (!pg)
			break;

#ifdef CONFIG_DFX_KERNELDUMP
		tmp_page = pg;
		for (k = 0; k < (int)(1 << sz2order(per_alloc_sz)); k++) {
			SetPageMemDump(tmp_page);
			tmp_page++;
		}
#endif

		start = (page_to_phys(pg) - saddr) / per_bit_sz;
		for (shift = start;
		     shift < start + per_alloc_sz / per_bit_sz;
		     shift++)
			seccm_heap->bitmap |= (1ULL << shift);

		gen_pool_free(seccm_heap->pool, page_to_phys(pg), per_alloc_sz);

		if (seccm_heap->flag & ION_FLAG_SECURE_BUFFER) {
#ifdef CONFIG_MM_ION_FLUSH_CACHE_ALL
			ion_flush_all_cpus_caches_raw();
#else
			ion_flush_all_cpus_caches();
#endif

			virt = (unsigned long)__va(page_to_phys(pg));
			change_secpage_range(page_to_phys(pg),
					    virt,
					    per_alloc_sz,
					    __pgprot(DEVICE_MEMORY));

			flush_tlb_all();
			sec_cfg.start_addr = seccm_heap->align_saddr;
			sec_cfg.sub_rgn_size = per_bit_sz;
			sec_cfg.bit_map = seccm_heap->bitmap;
			sec_cfg.sec_port = seccm_heap->attr;
			dfx_sec_ddr_set(&sec_cfg, (int)seccm_heap->protect_id);
		} else {
			memset(page_address(pg), 0x0, per_alloc_sz); /* unsafe_function_ignore: memset  */
		}
		nr--;
	}

	ion_sec_dbg("out %s %llu MB memory bitmap 0x%llx\n", __func__,
		    (water_mark - nr * per_alloc_sz) / SZ_1M,
		    seccm_heap->bitmap);
}

static void seccm_destroy_pool(struct ion_seccm_heap *seccm_heap)
{
	u64 shift;
	struct page *pg = NULL;
	u64 cma_base;
	u64 cma_size;
	u64 per_bit_sz = seccm_heap->per_bit_sz;
	u64 saddr = seccm_heap->align_saddr;
	unsigned long virt;

	ion_sec_dbg("into %s  bitmap 0x%llx\n", __func__, seccm_heap->bitmap);

	cma_base = cma_get_base(seccm_heap->cma);
	cma_size = cma_get_size(seccm_heap->cma);

	if (seccm_heap->flag & ION_FLAG_SECURE_BUFFER)
		dfx_sec_ddr_clr((int)seccm_heap->protect_id);

	for (shift = (cma_base - saddr) / per_bit_sz;
	     shift < ((cma_base + cma_size - saddr) / per_bit_sz);
	     shift++) {
		pg = phys_to_page(seccm_heap->align_saddr +
				  per_bit_sz * shift);

		if (seccm_heap->bitmap & (1ULL << shift)) {
			if (seccm_heap->flag & ION_FLAG_SECURE_BUFFER) {
				virt = (unsigned long)__va(page_to_phys(pg));
				change_secpage_range(page_to_phys(pg),
						    virt,
						    per_bit_sz,
						    PAGE_KERNEL);
				flush_tlb_all();
			}

			seccm_heap->bitmap &= ~(1ULL << shift);
			cma_release(seccm_heap->cma, pg,
				    (1UL << sz2order(per_bit_sz)));

			ion_sec_dbg("in %s bitmap 0x%llx fpn 0x%lx, shift %lld\n",
				    __func__,
				    seccm_heap->bitmap,
				    page_to_pfn(pg),
				    shift);
		} else {
			gen_pool_free(seccm_heap->pool,
				      page_to_phys(pg),
				      per_bit_sz);
		}
	}

	flush_tlb_all();
	gen_pool_destroy(seccm_heap->pool);
	seccm_heap->bitmap = 0;
	seccm_heap->pool = NULL;

	ion_sec_dbg("out %s\n", __func__);
}

static phys_addr_t seccm_alloc(struct ion_heap *heap,
				   unsigned long size,
				   unsigned long flag)
{
	unsigned long offset;
	struct ion_seccm_heap *seccm_heap =
		container_of(heap, struct ion_seccm_heap, heap);

	ion_sec_dbg("into %s %d %lx\n", __func__, heap->id, size);

	mutex_lock(&seccm_heap->mutex);

	if (seccm_heap->alloc_size + size > seccm_heap->heap_size) {
		pr_err("alloc size 0x%lx heap size 0x%lx\n",
		       seccm_heap->alloc_size, seccm_heap->heap_size);
		pr_err("heap cma alloc bitmap 0x%llx\n", seccm_heap->bitmap);
	}

	if (!seccm_heap->pool &&
	    seccm_create_pool(seccm_heap, flag)) {
		pr_err("seccm_create_pool is failed\n");
		offset = 0;
		goto err;
	}

	if (seccm_heap->flag != (flag & ION_FLAG_SECURE_BUFFER)) {
		pr_err("seccm_heap->flag is not same\n");
		offset = 0;
		goto err;
	}

	offset = gen_pool_alloc(seccm_heap->pool, size);
	if (!offset) {
		seccm_add_pool(seccm_heap);
		offset = gen_pool_alloc(seccm_heap->pool, size);
	}

	if (likely(!!offset))
		seccm_heap->alloc_size += size;
	else
		if (!seccm_heap->alloc_size)
			seccm_destroy_pool(seccm_heap);

err:
	mutex_unlock(&seccm_heap->mutex);

	ion_sec_dbg("out %s %lx\n", __func__, offset);

	return offset;
}

static void seccm_free(struct ion_heap *heap, phys_addr_t addr,
		       unsigned long size)
{
	struct ion_seccm_heap *seccm_heap =
		container_of(heap, struct ion_seccm_heap, heap);

	ion_sec_dbg("into %s  %d %lx\n", __func__, heap->id, addr);

	mutex_lock(&seccm_heap->mutex);

	gen_pool_free(seccm_heap->pool, addr, size);
	seccm_heap->alloc_size -= size;
	if (!seccm_heap->alloc_size)
		seccm_destroy_pool(seccm_heap);

	mutex_unlock(&seccm_heap->mutex);

	ion_sec_dbg("out %s\n", __func__);
}

static int ion_seccm_heap_allocate(struct ion_heap *heap,
				   struct ion_buffer *buffer,
				   unsigned long size,
				   unsigned long flags)
{
	struct sg_table *table = NULL;
	phys_addr_t paddr;
	int ret;

	ion_sec_dbg("into %s sz 0x%lx hp id %u\n", __func__, size, heap->id);

	if ((heap->id == ION_MISC_HEAP_ID) &&
	    (flags & ION_FLAG_SECURE_BUFFER)) {
		pr_err("%s:unsec heap could not alloc sec mem!\n", __func__);
		return -EINVAL;
	}

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = seccm_alloc(heap, size, flags);
	if (!paddr) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), size, 0);
	buffer->sg_table = table;
	ion_sec_dbg("out %s paddr 0x%lx size 0x%lx heap id %u\n",
		    __func__, paddr, size, heap->id);

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}

static void ion_seccm_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	ion_sec_dbg("into %s size 0x%lx heap id %u\n", __func__,
		    buffer->size, heap->id);

	if (!(buffer->flags & ION_FLAG_SECURE_BUFFER))
		(void)ion_heap_buffer_zero(buffer);

	seccm_free(heap, paddr, buffer->size);
	sg_free_table(table);
	kfree(table);

	ion_sec_dbg("out %s size 0x%lx heap id %u\n", __func__,
		    buffer->size, heap->id);
}

int ion_seccm_heap_phys(struct ion_heap *heap,
		struct ion_buffer *buffer,
		phys_addr_t *addr, size_t *len)
{
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
	if (heap->type != ION_HEAP_TYPE_SECCM) {
		pr_err("%s: not seccm mem!\n", __func__);
		return -EINVAL;
	}

	*addr = paddr;
	*len = buffer->size;

	return 0;
}

static int ion_seccm_heap_map_user(struct ion_heap *heap,
		struct ion_buffer *buffer,
		      struct vm_area_struct *vma)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return -EINVAL;
	return ion_heap_map_user(heap, buffer, vma);
}

static void *ion_seccm_heap_map_kernel(struct ion_heap *heap,
			  struct ion_buffer *buffer)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return NULL;
	return ion_heap_map_kernel(heap, buffer);
}

static void ion_seccm_heap_unmap_kernel(struct ion_heap *heap,
			   struct ion_buffer *buffer)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return;
	ion_heap_unmap_kernel(heap, buffer);
}

static struct ion_heap_ops seccm_heap_ops = {
	.allocate = ion_seccm_heap_allocate,
	.free = ion_seccm_heap_free,
	.map_user = ion_seccm_heap_map_user,
	.map_kernel = ion_seccm_heap_map_kernel,
	.unmap_kernel = ion_seccm_heap_unmap_kernel,
};

static void seccm_heap_init(struct ion_seccm_heap *seccm_heap,
				struct ion_platform_heap *heap_data)
{
	seccm_heap->bitmap = 0;
	seccm_heap->pool = NULL;
	seccm_heap->heap.ops = &seccm_heap_ops;
	seccm_heap->heap.type = ION_HEAP_TYPE_SECCM;
	seccm_heap->heap_size = heap_data->size;
	dev = heap_data->priv;
	seccm_heap->dev = dev;
}

static int set_seccm_heap_data(struct ion_seccm_heap *seccm_heap,
					struct ion_platform_heap *heap_data, u64 *per_bit_sz,
					u64 *region_nr, struct cma *cma)
{
	u64 per_alloc_sz;
	u64 protect_id;
	u64 attr;
	u64 water_mark;
	struct device_node *nd = NULL;

	seccm_heap_init(seccm_heap, heap_data);

	nd = of_get_child_by_name(dev->of_node, heap_data->name);
	if (!nd) {
		pr_err("can't of_get_child_by_name %s\n", heap_data->name);
		return -1;
	}

	ret = of_property_read_u64(nd, "per-alloc-size", &per_alloc_sz);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "per-alloc-size");
		return -1;
	}
	seccm_heap->per_alloc_sz = per_alloc_sz;

	ret = of_property_read_u64(nd, "per-bit-size", &per_bit_sz);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "per-bit-size");
		return -1;
	}
	seccm_heap->per_bit_sz = per_bit_sz;

	ret = of_property_read_u64(nd, "region-nr", &region_nr);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "region-nr");
		return -1;
	}
	seccm_heap->region_nr = region_nr;

	ret = of_property_read_u64(nd, "protect-id", &protect_id);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "prot-id");
		return -1;
	}
	seccm_heap->protect_id = protect_id;

	ret = of_property_read_u64(nd, "access-attr", &attr);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "access-attr");
		return -1;
	}
	seccm_heap->attr = attr;

	ret = of_property_read_u64(nd, "water-mark", &water_mark);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "water-mark");
		return -1;
	}
	seccm_heap->water_mark = water_mark;

	if (!mm_sec_cma)
		return -1;
	cma = mm_sec_cma;
	seccm_heap->cma = cma;
	return 0;
}

struct ion_heap *ion_seccm_heap_create(struct ion_platform_heap *heap_data)
{
	int ret = -1;
	u64 region_nr;
	u64 per_bit_sz;
	u64 align_saddr;
	u64 align_eaddr;
	struct device *dev = NULL;
	struct cma *cma = NULL;
	struct device_node *nd = NULL;
	struct ion_seccm_heap *seccm_heap = NULL;

	ion_sec_dbg("into %s\n", __func__);

	seccm_heap = kzalloc(sizeof(*seccm_heap), GFP_KERNEL);
	if (!seccm_heap)
		return ERR_PTR(-ENOMEM);

	mutex_init(&seccm_heap->mutex);

	ret = set_seccm_heap_data(seccm_heap, heap_data,
						&per_bit_sz, &region_nr, cma);
	if (ret)
		goto mutex_err;

	align_saddr = cma_get_base(cma);
	align_saddr = round_down(align_saddr,
				 per_bit_sz * PER_REGION_HAVE_BITS);

	align_eaddr = (align_saddr + cma_get_size(cma));
	align_eaddr = round_up(align_eaddr,
			       per_bit_sz * PER_REGION_HAVE_BITS);

	if (((align_eaddr - align_saddr) / per_bit_sz) >
		sizeof(seccm_heap->bitmap) * BITS_PER_BYTE) {
		pr_err("bitmap not enough saddr 0x%llx eaddr 0x%llx\n",
		       align_saddr, align_eaddr);
		goto mutex_err;
	}
	if (((align_eaddr - align_saddr) /
	    (per_bit_sz * PER_REGION_HAVE_BITS)) >
	     region_nr) {
		pr_err("region not enough, saddr 0x%llx eaddr 0x%llx\n",
		       align_saddr, align_eaddr);
		goto mutex_err;
	}
	seccm_heap->align_saddr = align_saddr;

	pr_err("Seccm heap info %s:\n"
		  "\t\t\t\t heap size : %lu MB\n"
		  "\t\t\t\t water_mark : %llu MB\n"
		  "\t\t\t\t per alloc size :  %llu MB\n"
		  "\t\t\t\t per bit size : %llu MB\n"
		  "\t\t\t\t region numb : %llu\n"
		  "\t\t\t\t protect id : %llu\n"
		  "\t\t\t\t attr  : 0x%llx\n"
		  "\t\t\t\t heap align saddr : 0x%llx\n"
		  "\t\t\t\t heap align eaddr : 0x%llx\n"
		  "\t\t\t\t cma base : 0x%llx\n"
		  "\t\t\t\t cma end : 0x%llx\n",
		  heap_data->name,
		  seccm_heap->heap_size / SZ_1M,
		  seccm_heap->water_mark / SZ_1M,
		  seccm_heap->per_alloc_sz / SZ_1M,
		  seccm_heap->per_bit_sz / SZ_1M,
		  seccm_heap->region_nr,
		  seccm_heap->protect_id,
		  seccm_heap->attr,
		  seccm_heap->align_saddr,
		  align_eaddr,
		  cma_get_base(cma),
		  cma_get_size(cma) + cma_get_base(cma));

	ion_sec_dbg("out %s\n", __func__);

	return &seccm_heap->heap;

mutex_err:
	kfree(seccm_heap);
	return ERR_PTR(-ENOMEM);
}
