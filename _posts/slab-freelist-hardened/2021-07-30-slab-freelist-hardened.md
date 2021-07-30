---
title: slub poisioning mitigation with CONFIG_SLAB_FREELIST_HARDENED 
date: 2021-07-30 11:00:00 +09:00
categories: [kernel mitigation]
tags: [kernel mitigation, kernel config]
description: kernel mitigation for slub poisioning
---

# debugging CVE-2020-0423
특정 폰에서 CVE-2020-0423이 정상적으로 실행되지 않는 문제점이 있음. 

## kernel oops
문제가 일어나는 경우가 굉장히 다양해서 그 중에 하나만 먼저 분석해보자.   
해당 문제가 일어날 때의 커널 ram-oops는 다음을 참고하자.

```
[   52.559072] Unable to handle kernel paging request at virtual address 0033774e98805987
[   52.559193] pstate: 60400005 (nZCv daif +PAN -UAO)
[   52.559202] pc : kmem_cache_alloc_trace+0x1b0/0x2c0
[   52.559205] lr : kmem_cache_alloc_trace+0x40/0x2c0
[   52.559207] sp : ffffff8021db3800
[   52.559209] x29: ffffff8021db3860 x28: ffffff9f18184540 
[   52.559212] x27: ffffff9f18183c88 x26: ffffffdb1d1cf800 
[   52.559215] x25: ffffff9f17ef3000 x24: ffffffdb0681fc80 
[   52.559217] x23: e933774e98805987 x22: ffffffdb0681fc80 
[   52.559220] x21: 0000000000000018 x20: 00000000006080c0 
[   52.559223] x19: ffffff9f16033844 x18: 0000000000000000 
[   52.559225] x17: 0000000000000000 x16: 0000000000000000 
[   52.559228] x15: 00000075e36feb50 x14: 00000000073173a5 
[   52.559230] x13: 0000000000000000 x12: ffffffdc79948b00 
[   52.559233] x11: e933770a24f69777 x10: e933774e98805987 
[   52.559236] x9 : 000000000731739d x8 : ffffffdbb401c000 
[   52.559238] x7 : 0000000000000000 x6 : 000000000000003f 
[   52.559241] x5 : 0000000000000040 x4 : 0000000000000000 
[   52.559244] x3 : 0000000000000004 x2 : 0000000000000018 
[   52.559246] x1 : 00000000006080c0 x0 : 0000000000000000 
[   52.559249] Call trace:
[   52.559252]  kmem_cache_alloc_trace+0x1b0/0x2c0
[   52.559259]  binder_transaction+0xc44/0x3ca0
[   52.559262]  binder_ioctl_write_read+0x73c/0x4398
[   52.559265]  binder_ioctl+0x2d0/0x918
[   52.559269]  do_vfs_ioctl+0x62c/0x9c0
[   52.559272]  __arm64_sys_ioctl+0x70/0x98
[   52.559276]  el0_svc_common+0xa0/0x158
[   52.559278]  el0_svc_handler+0x6c/0x88
[   52.559281]  el0_svc+0x8/0xc
[   52.559284] Code: b940230a 9100212e f9409f0b 8b0a02ea (f940014c) 
```

해당 부분을 gdb로 확인하면 freelist_ptr 함수에서 에러가 일어난것을 알 수 있다.
```c
Reading symbols from ./vmlinux...
(gdb) l *kmem_cache_alloc_trace+0x1b0
0xffffff8008280938 is in kmem_cache_alloc_trace (../mm/slub.c:262).
257	
258	/* Returns the freelist pointer recorded at location ptr_addr. */
259	static inline void *freelist_dereference(const struct kmem_cache *s,
260						 void *ptr_addr)
261	{
262		return freelist_ptr(s, (void *)*(unsigned long *)(ptr_addr),
263				    (unsigned long)ptr_addr);
264	}
265	
266	static inline void *get_freepointer(struct kmem_cache *s, void *object)
```


##  CONFIG_SLAB_FREELIST_HARDENED

freelist_ptr안에서는 CONFIG_SLAB_FREELIST_HARDENED일 때는 그냥 단순한 ptr이 아니라 kmem_cache안에 있는 random value와 xor으로 저장되는 것을 알 수 있다.
>/mm/slub.c
```c
/********************************************************************
 * 			Core slab cache functions
 *******************************************************************/

/*
 * Returns freelist pointer (ptr). With hardening, this is obfuscated
 * with an XOR of the address where the pointer is held and a per-cache
 * random number.
 */
static inline void *freelist_ptr(const struct kmem_cache *s, void *ptr,
				 unsigned long ptr_addr)
{
#ifdef CONFIG_SLAB_FREELIST_HARDENED
	return (void *)((unsigned long)ptr ^ s->random ^ ptr_addr);
#else
	return ptr;
#endif
}
```

pixel4에서는 enable되어 있지 않은 config였는데, Xiaomi10T 에서는 해당 value가 set되어있다.

arch/arm64/configs/apollo_user_defconfig
```
CONFIG_EMBEDDED=y
# CONFIG_SLUB_DEBUG is not set
# CONFIG_COMPAT_BRK is not set
CONFIG_SLAB_FREELIST_RANDOM=y
CONFIG_SLAB_FREELIST_HARDENED=y
CONFIG_PROFILING=y
# CONFIG_ZONE_DMA32 is not set
CONFIG_ARCH_QCOM=y
```

해당 기능은 slub의 freeslit pointer를 xor를 통하여 random하게 만드므로 이 경우 CVE-2020-0423에서 사용했던 freelist poisoining이 정상적으로 동작할 수 없음.   
[Weaknesses in Linux Kernel Heap Hardening](https://blog.infosectcbr.com.au/2020/03/weaknesses-in-linux-kernel-heap.html) 다음 링크 등을 참고하여 해당 mitigation을 우회할 수 있는지에 대해 확인했다. 

1. free list pointer가 ptr ^ ptr_addr ^ s->random 으로 xor될 때 ptr과 ptr_addr은 똑같은 high bit를 공유하므로, ptr ^ ptr_addr는 0이 된다. 
2. 따라서 만약 난독화된 포인터 값을 알고 있을 경우 우리는 상위 52-bit의 s->random을 leak할 수 있다.
3. 또한 slub allocator는 slab에 그 기반을 두고 있으므로 allocation은 slab size로 round up 된다. 따라서 만약 우리가 16바이트 메모리를 할당한다고 가정했을때, 하위4개 비트는 0으로 처리된다. 만약 우리가 256byte를 allocate한다면 하위 8비트가 0이된다. 만약, 4096(0x1000)이라면 하위 12비트가 0이된다. 
4. 하지만 안드로이드에서는 최소 메모리 할당이 128바이트이므로 이 때 하위7개 비트는 0이 될 것이다.
5. 따라서 해당 포인터를 알 수 있다면 상위 52개비트 + 하위 7개비트 즉 59개의 비트를 알고 있는 상태로 4개 비트의 엔트로피 1/2^4 확률로 해당 주소값을 맞출 수 있으며 만약 페이지 주소정보만을 알아야 한다면 해당 값만으로도 충분히 알아낼 수 있다. 


## slub posioning 없이 exploit 진행이 가능할지

