---
title: Binder Alloc Analysis
date: 2021-07-09 19:39:00 +09:00
categories: [binder, linux kernel]
tags: [binder_alloc, binder]
description: Analysis of binder_alloc
---

# binder alloc

binder_alloc 구조체는 binder_alloc.c 로 binder.c 와는 다르게 파일이 따로 나와있다.   
binder_alloc 은 binder deivce에서 각 process의 virtual memory와 physical memory 관리를 담당한다. 저번 분석에서 분석을 대략적으로만 해서 이번에 조금 더 자세히 볼 수 있도록 한다.   
![binder transaction 구성도](/assets/img/android_binder.png)

`binder_alloc 구조체`
```c
/**
 * struct binder_alloc - per-binder proc state for binder allocator
 * @vma:                vm_area_struct passed to mmap_handler
 *                      (invarient after mmap)
 * @tsk:                tid for task that called init for this proc
 *                      (invariant after init)
 * @vma_vm_mm:          copy of vma->vm_mm (invarient after mmap)
 * @buffer:             base of per-proc address space mapped via mmap
 * @buffers:            list of all buffers for this proc
 * @free_buffers:       rb tree of buffers available for allocation
 *                      sorted by size
 * @allocated_buffers:  rb tree of allocated buffers sorted by address
 * @free_async_space:   VA space available for async buffers. This is
 *                      initialized at mmap time to 1/2 the full VA space
 * @pages:              array of binder_lru_page
 * @buffer_size:        size of address space specified via mmap
 * @pid:                pid for associated binder_proc (invariant after init)
 * @pages_high:         high watermark of offset in @pages
 *
 * Bookkeeping structure for per-proc address space management for binder
 * buffers. It is normally initialized during binder_init() and binder_mmap()
 * calls. The address space is used for both user-visible buffers and for
 * struct binder_buffer objects used to track the user buffers
 */
struct binder_alloc {
	struct mutex mutex;
	struct vm_area_struct *vma;
	struct mm_struct *vma_vm_mm;
	void __user *buffer;
	struct list_head buffers;
	struct rb_root free_buffers;
	struct rb_root allocated_buffers;
	size_t free_async_space;
	struct binder_lru_page *pages;
	size_t buffer_size;
	uint32_t buffer_free;
	int pid;
	size_t pages_high;
};
```
### binder mmap
   
binder mmap은 `binder_alloc_mmap_handler` 함수로 이루어진다. 

binder_alloc 은 binder_buffer라는 구조체를 담을 수 있는 `buffers`라는 linked list로 가지고 있는데 그 내용은 다음과 같다. 
```c
struct binder_buffer {
	struct list_head entry; /* free and allocated entries by address */
	struct rb_node rb_node; /* free entry by size or allocated entry */
				/* by address */
	unsigned free:1;
	unsigned allow_user_free:1;
	unsigned async_transaction:1;
	unsigned debug_id:29;

	struct binder_transaction *transaction;

	struct binder_node *target_node;
	size_t data_size;
	size_t offsets_size;
	size_t extra_buffers_size;
	void __user *user_data;
	int    pid;
};
```

1. `alloc->buffer`는 mmap을 통해 들어온 `vma->vm_start` 주소로 설정
2. mmap으로 들어온 사이즈만큼 kcalloc을 통한 커널 메모리 kcalloc
3. `alloc->buffersize`는 vma->vm_end - vma->vm_start로 설정
4. `binder_buffer` 구조체 kzalloc
5. buffer->user_data = alloc->buffer 를 통해 시작 user address 설정
6. `list_add(&buffer->entry, &alloc->buffers)` 를 통하여 해당 buffer를 alloc의 linked list와 연결
7. `binder_insert_free_buffer(alloc, buffer)` 호출





---
## 사전조사

리눅스 커널에서의 mmap기능 정리    
출처 : [Linuix Kernel Memory Mapping](https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html)
   
    

### overview
   
리눅스 커널에서 kernel address space를 user address space로 mapping하는 것이 가능하다. 이것은 user space information을 kernel space로 복사하거나 그 반대의 오버헤드를 줄이게끔 해 준다.

해당 피처는 device driver의 **struct file_operation** 의 mmap implemantation과 userspace에서의 mmap system call로 가능해진다.

가상 메모리 관리의 기본 단위는 페이지이며 크기는 일반적으로 4K이지만 일부 플랫폼에서는 최대 64K일 수 있다. 가상 메모리로 작업할 때마다 가상 주소와 물리적 주소의 두 가지 유형의 주소로 작업한다. 모든 CPU 액세스(커널 공간 포함)는 페이지 테이블의 도움으로 MMU에 의해 물리적 주소로 변환되는 가상 주소를 사용한다.

메모리의 물리적 페이지는 PFN(페이지 프레임 번호)으로 식별된다. PFN은 물리적 주소를 페이지 크기로 나누어(또는 PAGE_SHIFT 비트가 있는 물리적 주소를 오른쪽으로 이동하여) 쉽게 계산할 수 있다.
![paging](/assets/img/paging.png)

효율성을 위해 가상 주소 공간은 사용자 공간과 커널 공간으로 나뉜다. 같은 이유로 커널 공간에는 가장 낮은 물리적 주소(보통 0)부터 시작하여 물리적 메모리에 연속적으로 매핑되는 lowmem이라는 메모리 매핑 영역이 포함된다. lowmem이 매핑되는 가상 주소는 PAGE_OFFSET에 의해 결정됨.

32비트 시스템에서 사용 가능한 모든 메모리를 lowmem에 매핑할 수 있는 것은 아니므로 커널 공간에 물리적 메모리를 임의로 매핑하는 데 사용할 수 있는 highmem이라는 별도의 영역이 있다.

kmalloc()에 의해 할당된 메모리는 lowmem에 상주하며 물리적으로 연속적임. vmalloc()에 의해 할당된 메모리는 인접하지 않고 lowmem에 상주하지 않음(highmem에 전용 영역이 있음).   
![kernel-virtmem-map](/assets/img/kernel-virtmem-map.png)


### Structures used for memory mapping
   
장치의 메모리 매핑 메커니즘을 논의하기 전에 Linux 커널의 메모리 관리 하위 시스템과 관련된 몇 가지 기본 구조에 대한 연구임.
기본 구조 중 일부는 `struct page, struct vm_area_struct, struct mm_struct`.

#### struct page
   
구조체 페이지는 시스템의 모든 물리적 페이지에 대한 정보를 포함하는 데 사용됨. 커널에는 시스템의 모든 페이지에 대한 구조 페이지 구조가 있음.

- virt_to_page()는 가상 주소와 관련된 페이지를 반환
- pfn_to_page()는 페이지 프레임 번호와 관련된 페이지를 반환
- page_to_pfn()은 구조체 페이지와 연결된 페이지 프레임 번호를 반환
- page_address()는 구조체 페이지의 가상 주소를 반환. 이 함수는 lowmem의 페이지에 대해서만 호출 가능
- kmap()은 커널에서 임의의 물리적 페이지(highmem에서 올 수 있음)에 대한 매핑을 생성하고 페이지를 직접 참조하는 데 사용할 수 있는 가상 주소를 반환

#### struct vm_area_struct
   
struct vm_area_struct는 인접한 가상 메모리 영역에 대한 정보를 보유. 프로세스의 메모리 영역은 cat /proc/[pid]/maps 를 통해 보는것 가능

```
root@qemux86:~# cat /proc/1/maps
#address          perms offset  device inode     pathname
08048000-08050000 r-xp 00000000 fe:00 761        /sbin/init.sysvinit
08050000-08051000 r--p 00007000 fe:00 761        /sbin/init.sysvinit
08051000-08052000 rw-p 00008000 fe:00 761        /sbin/init.sysvinit
092e1000-09302000 rw-p 00000000 00:00 0          [heap]
4480c000-4482e000 r-xp 00000000 fe:00 576        /lib/ld-2.25.so
4482e000-4482f000 r--p 00021000 fe:00 576        /lib/ld-2.25.so
4482f000-44830000 rw-p 00022000 fe:00 576        /lib/ld-2.25.so
44832000-449a9000 r-xp 00000000 fe:00 581        /lib/libc-2.25.so
449a9000-449ab000 r--p 00176000 fe:00 581        /lib/libc-2.25.so
449ab000-449ac000 rw-p 00178000 fe:00 581        /lib/libc-2.25.so
449ac000-449af000 rw-p 00000000 00:00 0
b7761000-b7763000 rw-p 00000000 00:00 0
b7763000-b7766000 r--p 00000000 00:00 0          [vvar]
b7766000-b7767000 r-xp 00000000 00:00 0          [vdso]
bfa15000-bfa36000 rw-p 00000000 00:00 0          [stack]
```

struct vm_area_struct는 사용자 공간에서 발행된 각 mmap() 호출에서 생성됨.   
mmap() 작업을 지원하는 드라이버는 연결된 struct vm_area_struct를 완료하고 초기화해야 하며 이 구조의 가장 중요한 필드는 아래와 같음

- vm_start, vm_end - 각각 메모리 영역의 시작과 끝(이 필드는 /proc/<pid>/maps에도 나타남);
- vm_file - 연관된 파일 구조에 대한 포인터(있는 경우).
- vm_pgoff - 파일 내 영역의 오프셋.
- vm_flags - 플래그 세트;
- vm_ops - 이 영역에 대한 작업 기능 세트
- vm_next, vm_prev - 동일한 프로세스의 영역이 목록 구조로 연결됨 

### Device driver memory mapping
   
device driver를 구현 할 때, `struct file_operations` 를 구현해 놓고 나면 userspace는 해당 file로 mmap systemcall을 호출할 수 있다. 

```c
void *mmap(caddr_t addr, size_t len, int prot,
           int flags, int fd, off_t offset);
```

device driver의 mmap operation은 다음과 같다. 

```c
int (*mmap)(struct file *filp, struct vm_area_struct *vma);
```

- filp 필드는 장치가 사용자 공간에서 열릴 때 생성되는 구조체 파일에 대한 포인터. 
- vma 필드는 메모리가 장치에 의해 매핑되어야 하는 가상 주소 공간을 나타내는 데 사용됨. 

드라이버는 kmalloc(), vmalloc(), alloc_pages()를 사용하여 메모리를 할당한 다음 remap_pfn_range()와 같은 도우미 함수를 사용하여 vma 매개변수에 표시된 대로 사용자 주소 공간에 매핑해주면 끝.

remap_pfn_range()는 인접한 물리적 주소 공간을 vm_area_struct가 나타내는 가상 공간으로 매핑함.

```c
int remap_pfn_range (structure vm_area_struct *vma, unsigned long addr,
                     unsigned long pfn, unsigned long size, pgprot_t prot);
```

- vma - 매핑이 이루어지는 가상 메모리 공간.
- addr - 다시 매핑이 시작되는 가상 주소 공간. addr과 addr + size 사이의 가상 주소 공간에 대한 페이지 테이블은 필요에 따라 형성.
- pfn - 가상 주소가 매핑되어야 하는 페이지 프레임 번호
- size - 매핑되는 메모리의 크기(바이트)
- prot - 이 매핑에 대한 보호 플래그

물리적 메모리의 페이지 프레임 번호를 얻으려면 메모리 할당이 수행된 방법을 고려해야 함.   
각 kmalloc(), vmalloc(), alloc_pages()에 대해 다른 접근 방식을 사용해야 하며 kmalloc ()의 경우 다음과 같이 사용할 수 있음.   

```c
static char *kmalloc_area;

unsigned long pfn = virt_to_phys((void *)kmalloc_area)>>PAGE_SHIFT;
```