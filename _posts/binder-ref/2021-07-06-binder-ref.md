---
title: Binder Ref Analysis
date: 2021-07-06 13:14:00 +09:00
categories: [binder, linux kernel]
tags: [binder_ref, binder]
description: Analysis of binder_ref
---

# binder ref

### binder_ref 구조체

```c
/**
 * struct binder_ref - struct to track references on nodes
 * @data:        binder_ref_data containing id, handle, and current refcounts
 * @rb_node_desc: node for lookup by @data.desc in proc's rb_tree
 * @rb_node_node: node for lookup by @node in proc's rb_tree
 * @node_entry:  list entry for node->refs list in target node
 *               (protected by @node->lock)
 * @proc:        binder_proc containing ref
 * @node:        binder_node of target node. When cleaning up a
 *               ref for deletion in binder_cleanup_ref, a non-NULL
 *               @node indicates the node must be freed
 * @death:       pointer to death notification (ref_death) if requested
 *               (protected by @node->lock)
 *
 * Structure to track references from procA to target node (on procB). This
 * structure is unsafe to access without holding @proc->outer_lock.
 */
struct binder_ref {
	/* Lookups needed: */
	/*   node + proc => ref (transaction) */
	/*   desc + proc => ref (transaction, inc/dec ref) */
	/*   node => refs + procs (proc exit) */
	struct binder_ref_data data;
	struct rb_node rb_node_desc;
	struct rb_node rb_node_node;
	struct hlist_node node_entry;
	struct binder_proc *proc;
	struct binder_node *node;
	struct binder_ref_death *death;
};
```
### binder_ref가 쓰이는 함수들

tmpref를 제외하고 (tmpref는 나중에 보도록 하자) binder_ref에 관련된 함수들은 다음과 같다.

```c
binder_cleanup_ref_olocked (ref : struct binder_ref*) : void
binder_dec_ref_for_handle (proc : struct binder_proc*,desc : uint32_t,strong : bool,rdata : struct binder_ref_data*) : int
binder_dec_ref_olocked (ref : struct binder_ref*,strong : int) : bool
binder_free_ref (ref : struct binder_ref*) : void
binder_get_node_from_ref (proc : struct binder_proc*,desc : u32,need_strong_ref : bool,rdata : struct binder_ref_data*) : struct binder_node*
binder_get_node_refs_for_txn (node : struct binder_node*,procp : struct binder_proc**,error : uint32_t*) : struct binder_node*
binder_get_ref_for_node_olocked (proc : struct binder_proc*,node : struct binder_node*,new_ref : struct binder_ref*) : struct binder_ref*
binder_get_ref_olocked (proc : struct binder_proc*,desc : u32,need_strong_ref : bool) : struct binder_ref*
binder_inc_ref_for_node (proc : struct binder_proc*,node : struct binder_node*,strong : bool,target_list : struct list_head*,rdata : struct binder_ref_data*) : int
binder_inc_ref_olocked (ref : struct binder_ref*,strong : int,target_list : struct list_head*) : int
binder_ioctl_get_node_info_for_ref (proc : struct binder_proc*,info : struct binder_node_info_for_ref*) : int
binder_node_release (node : struct binder_node*,refs : int) : int
binder_update_ref_for_handle (proc : struct binder_proc*,desc : uint32_t,increment : bool,strong : bool,rdata : struct binder_ref_data*) : int
print_binder_ref_olocked (m : struct seq_file*,ref : struct binder_ref*) : void
print_binder_stats (m : struct seq_file*,prefix : char*,stats : struct binder_stats*) : void
print_binder_transaction_ilocked (m : struct seq_file*,proc : struct binder_proc*,prefix : char*,t : struct binder_transaction*) : void
print_binder_work_ilocked (m : struct seq_file*,proc : struct binder_proc*,prefix : char*,transaction_prefix : char*,w : struct binder_work*) : void
```


### binder_ref의 생성

binder_ref가 생성되는 코드는 `binder_inc_ref_for_node` 함수 뿐이다. **kzaloc** 을 통하여 생성된 new_ref는 `binder_get_ref_for_node_olocked`함수를 통하여 적절한 node, proc, 그리고 rbtree와 node의 linked list안으로 연결된다.

```c
static int binder_inc_ref_for_node(struct binder_proc *proc,
			struct binder_node *node,
			bool strong,
			struct list_head *target_list,
			struct binder_ref_data *rdata)
{
	struct binder_ref *ref;
	struct binder_ref *new_ref = NULL;
	int ret = 0;

	binder_proc_lock(proc);
	ref = binder_get_ref_for_node_olocked(proc, node, NULL);
	if (!ref) {
		binder_proc_unlock(proc);
		new_ref = kzalloc(sizeof(*ref), GFP_KERNEL);
		if (!new_ref)
			return -ENOMEM;
		binder_proc_lock(proc);
		ref = binder_get_ref_for_node_olocked(proc, node, new_ref);
	}
	ret = binder_inc_ref_olocked(ref, strong, target_list);
	*rdata = ref->data;
	binder_proc_unlock(proc);
	if (new_ref && ref != new_ref)
		/*
		 * Another thread created the ref first so
		 * free the one we allocated
		 */
		kfree(new_ref);
	return ret;
}
```
kzalloc한 후의 `binder_get_ref_for_node_olocked(proc, node, new_ref)` 함수를 살펴보면 다음과 같다. binder_ref의 data.desc는 현재 모든 ref의 desc중 가장 큰 값 + 1 값으로 세팅된다.

```c
static struct binder_ref *binder_get_ref_for_node_olocked(
					struct binder_proc *proc,
					struct binder_node *node,
					struct binder_ref *new_ref)
{
	struct binder_context *context = proc->context;
	struct rb_node **p = &proc->refs_by_node.rb_node;
	struct rb_node *parent = NULL;
	struct binder_ref *ref;
	struct rb_node *n;

	/* if ref is already here, return ref */
	/* but rb_node_node tree have node value with address value */
	/* is it wierd ? */
	while (*p) {
		parent = *p;
		ref = rb_entry(parent, struct binder_ref, rb_node_node);

		if (node < ref->node)
			p = &(*p)->rb_left;
		else if (node > ref->node)
			p = &(*p)->rb_right;
		else
			return ref;
	}

	if (!new_ref)
		return NULL;

	binder_stats_created(BINDER_STAT_REF);
	new_ref->data.debug_id = atomic_inc_return(&binder_last_id);

	/* set proc of new_ref */
	new_ref->proc = proc;
	/* set node of new_ref */
	new_ref->node = node;
	/* set new_ref->rb_node_node in proc rb tree */
	rb_link_node(&new_ref->rb_node_node, parent, p);
	rb_insert_color(&new_ref->rb_node_node, &proc->refs_by_node);


    /* set ref->data.desc */
    /* we set new_ref's desc will be the biggest desc + 1 */
	new_ref->data.desc = (node == context->binder_context_mgr_node) ? 0 : 1;
	for (n = rb_first(&proc->refs_by_desc); n != NULL; n = rb_next(n)) {
		ref = rb_entry(n, struct binder_ref, rb_node_desc);
		if (ref->data.desc > new_ref->data.desc)
			break;
		new_ref->data.desc = ref->data.desc + 1;
	}

	/* we link new_ref with approciate place with rb-tree */ 
	p = &proc->refs_by_desc.rb_node;
	while (*p) {
		parent = *p;
		ref = rb_entry(parent, struct binder_ref, rb_node_desc);

		if (new_ref->data.desc < ref->data.desc)
			p = &(*p)->rb_left;
		else if (new_ref->data.desc > ref->data.desc)
			p = &(*p)->rb_right;
		else
			BUG();
	}
	rb_link_node(&new_ref->rb_node_desc, parent, p);
	rb_insert_color(&new_ref->rb_node_desc, &proc->refs_by_desc);

	/* we add new_ref in the node list */
	binder_node_lock(node);
	hlist_add_head(&new_ref->node_entry, &node->refs);

	binder_debug(BINDER_DEBUG_INTERNAL_REFS,
		     "%d new ref %d desc %d for node %d\n",
		      proc->pid, new_ref->data.debug_id, new_ref->data.desc,
		      node->debug_id);
	binder_node_unlock(node);
	return new_ref;
}
```

이후에 `static int binder_inc_ref_for_node()` 함수가 불리게 되는데 해당 함수의 strong value는 weak handle일 경우가 거의 없는 것 같다. (TodoList 추가됨). 
