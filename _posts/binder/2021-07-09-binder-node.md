---
title: Binder Node Analysis
date: 2021-07-06 13:14:00 +09:00
categories: [binder, linux kernel]
tags: [binder_node, binder]
description: Analysis of binder_node
---


# binder_node

binder_node 구조체의 생김새는 다음과 같다. 
```c
/**
 * struct binder_node - binder node bookkeeping
 * @debug_id:             unique ID for debugging
 *                        (invariant after initialized)
 * @lock:                 lock for node fields
 * @work:                 worklist element for node work
 *                        (protected by @proc->inner_lock)
 * @rb_node:              element for proc->nodes tree
 *                        (protected by @proc->inner_lock)
 * @dead_node:            element for binder_dead_nodes list
 *                        (protected by binder_dead_nodes_lock)
 * @proc:                 binder_proc that owns this node
 *                        (invariant after initialized)
 * @refs:                 list of references on this node
 *                        (protected by @lock)
 * @internal_strong_refs: used to take strong references when
 *                        initiating a transaction
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @local_weak_refs:      weak user refs from local process
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @local_strong_refs:    strong user refs from local process
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @tmp_refs:             temporary kernel refs
 *                        (protected by @proc->inner_lock while @proc
 *                        is valid, and by binder_dead_nodes_lock
 *                        if @proc is NULL. During inc/dec and node release
 *                        it is also protected by @lock to provide safety
 *                        as the node dies and @proc becomes NULL)
 * @ptr:                  userspace pointer for node
 *                        (invariant, no lock needed)
 * @cookie:               userspace cookie for node
 *                        (invariant, no lock needed)
 * @has_strong_ref:       userspace notified of strong ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @pending_strong_ref:   userspace has acked notification of strong ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @has_weak_ref:         userspace notified of weak ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @pending_weak_ref:     userspace has acked notification of weak ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @has_async_transaction: async transaction to node in progress
 *                        (protected by @lock)
 * @sched_policy:         minimum scheduling policy for node
 *                        (invariant after initialized)
 * @accept_fds:           file descriptor operations supported for node
 *                        (invariant after initialized)
 * @min_priority:         minimum scheduling priority
 *                        (invariant after initialized)
 * @inherit_rt:           inherit RT scheduling policy from caller
 * @txn_security_ctx:     require sender's security context
 *                        (invariant after initialized)
 * @async_todo:           list of async work items
 *                        (protected by @proc->inner_lock)
 *
 * Bookkeeping structure for binder nodes.
 */

struct binder_node {
	int debug_id;
	spinlock_t lock;
	struct binder_work work;
	union {
		struct rb_node rb_node;
		struct hlist_node dead_node;
	};
	struct binder_proc *proc;
	struct hlist_head refs;
	int internal_strong_refs;
	int local_weak_refs;
	int local_strong_refs;
	int tmp_refs;
	binder_uintptr_t ptr;
	binder_uintptr_t cookie;
	struct {
		/*
		 * bitfield elements protected by
		 * proc inner_lock
		 */
		u8 has_strong_ref:1;
		u8 pending_strong_ref:1;
		u8 has_weak_ref:1;
		u8 pending_weak_ref:1;
	};
	struct {
		/*
		 * invariant after initialization
		 */
		u8 sched_policy:2;
		u8 inherit_rt:1;
		u8 accept_fds:1;
		u8 txn_security_ctx:1;
		u8 min_priority;
	};
	bool has_async_transaction;
	struct list_head async_todo;
};
```


### binder_node의 생성

binder_node의 생성은 binder_new_node함수로 이루어진다. 특별히 분석할 만한 부분은 없다. kzalloc으로 새로은 binder_node를 할당하고 해당 proc과 fp->binder값을 통하여 원래 node가 있는지 검사한다. 만약 없다면 new_node를 쓰고 만약 있다면 해당 node를 쓰고 new_node를 해제한다.
```c
static struct binder_node *binder_new_node(struct binder_proc *proc,
					   struct flat_binder_object *fp)
{
	struct binder_node *node;
	struct binder_node *new_node = kzalloc(sizeof(*node), GFP_KERNEL);

	if (!new_node)
		return NULL;
	binder_inner_proc_lock(proc);
	node = binder_init_node_ilocked(proc, new_node, fp);
	binder_inner_proc_unlock(proc);
	if (node != new_node)
		/*
		 * The node was already added by another thread
		 */
		kfree(new_node);

	return node;
}
```

해당 binder_node는 `flat_binder_object *fp` 의 member중 ptr, cookie를 가져와서 설정하는데 
```c
struct flat_binder_object {
	struct binder_object_header	hdr;
	__u32				flags;

	/* 8 bytes of data. */
	union {
		binder_uintptr_t	binder;	/* local object */
		__u32			handle;	/* remote object */
	};

	/* extra data associated with local object */
	binder_uintptr_t	cookie;
};
```
[binder_ref](https://tty1538.github.io/binder-ref) 에서 설명했던 것과 같이 해당 binder값이 `node->ptr`값으로 들어가게 된다. (cookie에 대해서는 나중에 다시 알아봐야 할 것)이외에 다른것들을 설정해주고 끝나게 된다. 

`binder_new_node`를 부르는 코드는 2경우인데 하나는 context mgr로 servicemanager가 들어가 있기 때문이고, 두 번째는 binder_transalte_bider가 불릴 때이다. 