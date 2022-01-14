---
title: Binder Device Analysis
date: 2022-01-14 07:47:00 +09:00
categories: [binder, linux kernel]
tags: [binder_device, binder]
description: Analysis of binder_device
---


# binder_device

binder device는 [binderfs](/_posts/binder/2021-11-15-binderfs.md)가 들어오면서 새롭게 추가 된 feature이다. binder_device는 filesystem에서 해당 디바이스가 갖고있는 노드 넘버, binder context 정보, binderfs로 mount된 super block의 inode 정보 등을 가지고 있다.

```
/**
 * struct binder_device - information about a binder device node
 * @hlist:          list of binder devices (only used for devices requested via
 *                  CONFIG_ANDROID_BINDER_DEVICES)
 * @miscdev:        information about a binder character device node
 * @context:        binder context information
 * @binderfs_inode: This is the inode of the root dentry of the super block
 *                  belonging to a binderfs mount.
 */
struct binder_device {
	struct hlist_node hlist;
	struct miscdevice miscdev;
	struct binder_context context;
	struct inode *binderfs_inode;
	refcount_t ref;
};
```

## binderfs의 binder_device

binderfs에서 binder_device를 만드는 함수는 딱 두 곳이다.

```c
static int binderfs_binder_device_create(struct inode *ref_inode,
					 struct binderfs_device __user *userp,
					 struct binderfs_device *req)

static int binderfs_binder_ctl_create(struct super_block *sb)
```

하나는 binder_device자체를 만드는 곳이고, 하나는 binder_control을 만드는 곳이다. binder-control은 binderfs에 있는 ioctl file이며 이를 통하여 binder_device를 생성할 수 있는 파일이다. binder-control은 각 mount마다 하나씩 밖에 존재하지 않는다. 

binderfs_binder_device_create는 두 가지 조건에서 쓰인다.
1. binderfs_fill_super 함수에서 불리는데 fill super는 superblock을 채우는 함수이며, mount할 때 처음 불린다. 이 때 binderfs/binder, binderfs/hwbinder, binderfs/vndbinder의 세 개 파일을 만든다.

2. binder_ctl_ioctl에서 불리며, 이는 BINDER_CTL_ADD ioctl을 통해 추가가 가능하다. 하지만 binderfs문서에서 봤다 싶이, binder-control node는 root권한으로만 rw가 가능하므로 실제로 우리가 쓸 수 있는 부분은 없다.

## binder.c에서의 binder_device

binder.c 에서 binder_device 객체는 init_binder_device함수에서만 생성됨

```c
static int __init init_binder_device(const char *name)
{
	int ret;
	struct binder_device *binder_device;

	binder_device = kzalloc(sizeof(*binder_device), GFP_KERNEL);
	if (!binder_device)
		return -ENOMEM;

	binder_device->miscdev.fops = &binder_fops;
	binder_device->miscdev.minor = MISC_DYNAMIC_MINOR;
	binder_device->miscdev.name = name;

	refcount_set(&binder_device->ref, 1);
	binder_device->context.binder_context_mgr_uid = INVALID_UID;
	binder_device->context.name = name;
	mutex_init(&binder_device->context.context_mgr_node_lock);

	ret = misc_register(&binder_device->miscdev);
	if (ret < 0) {
		kfree(binder_device);
		return ret;
	}

	hlist_add_head(&binder_device->hlist, &binder_devices);

	return ret;
}
```

해당 함수는 binder_init의 함수에서만 불리는데 현재는 BINDERFS가 enable되어 있는 상태라 불리지 않음.
```c
	if (!IS_ENABLED(CONFIG_ANDROID_BINDERFS) &&
	    strcmp(binder_devices_param, "") != 0) {
		/*
		* Copy the module_parameter string, because we don't want to
		* tokenize it in-place.
		 */
		device_names = kstrdup(binder_devices_param, GFP_KERNEL);
		if (!device_names) {
			ret = -ENOMEM;
			goto err_alloc_device_names_failed;
		}

		device_tmp = device_names;
		while ((device_name = strsep(&device_tmp, ","))) {
			ret = init_binder_device(device_name);
			if (ret)
				goto err_init_binder_device_failed;
		}
	}
```

결론적으로 binder.c에서도 binder_device 객체를 생성할 수 있는 방법은 없음. 그런데 'binderfs의 static void binderfs_evict_inode(struct inode *inode)' 에 breakpoint를 걸고 에뮬레이터 사용 시 계속해서 break가 걸리는 것을 확인. backtrace는 다음과 같다.

```gdb
bt
 ► f 0 0xffffffff820b845c binderfs_evict_inode+12
   f 1 0xffffffff815b3dfa evict+282
   f 2 0xffffffff815b3c45 iput+837
   f 3 0xffffffff815aca86 dentry_unlink_inode+310
   f 4 0xffffffff815b0cf4 d_delete+100
   f 5 0xffffffff820b70bd binderfs_remove_file+189
   f 6 0xffffffff820c23e1 binder_release+129
   f 7 0xffffffff81586466 __fput+262

```

내가 알아본 바에 따르면 binder_device는 생성될 수가 없는데 어떤 방식으로 inode가 생겼는지 궁금해서 검색 시작 해당 부분에서 binderfs_remove_file에서 dentry값을 확인 해 보았다.

```gdb
pwngdb> p *(struct dentry *)dentry
$22 = {
...
  d_name = {
    {
      {
        hash = 3998031908,
        len = 4
      },
      hash_len = 21177901092
    },
    name = 0xffff888061117d38 "5453"
  },
...
```
해당 부모 노드의 dentry값은 다음과 같다.
```gdb
pwngdb> p *(struct dentry *)0xffff88800483cc30
$14 = {
...
  d_name = {
    {
      {
        hash = 3649675987,
        len = 4
      },
      hash_len = 20829545171
    },
    name = 0xffff88800483cc68 "proc"
  },
...
```

조금 더 찾아보니 해당 값은 binder_logs에 있는 proc값인데
```sh
emulator64_x86_64_arm64:/dev/binderfs/binder_logs/proc # ls
1031  1318  201  336  345  350   354  359  366  375  383  460  465  473   5895  6599  704   7476  842
1070  1342  207  337  346  3500  355  360  369  376  385  461  466  474   5925  6631  7285  7512  857
1245  1392  208  342  347  351   356  363  370  379  386  462  467  4952  5975  6874  7310  7579  887
1281  191   209  343  348  352   357  364  371  380  434  463  468  542   6303  699   7355  767
1297  193   210  344  349  353   358  365  372  382  458  464  469  5857  634   703   7474  792
```

각각의 pid값이 들어가고 해당 pid안에 들어가면 어떤 binder_thread, node, ref등이 저장되어있는지(?) 혹은 어떤 전송을 하는지 나와있는것으로 보인다. **TODO 추가조사 필요**

```log
emulator64_x86_64_arm64:/dev/binderfs/binder_logs/proc # cat 792
binder proc state:
proc 792
context binder
  thread 792: l 00 need_return 0 tr 0
  thread 802: l 00 need_return 0 tr 0
  thread 805: l 12 need_return 0 tr 0
  thread 808: l 11 need_return 0 tr 0
  thread 863: l 11 need_return 0 tr 0
  thread 927: l 00 need_return 0 tr 0
  thread 978: l 00 need_return 0 tr 0
  thread 992: l 11 need_return 0 tr 0
  thread 1246: l 00 need_return 0 tr 0
  thread 1964: l 00 need_return 0 tr 0
  thread 2019: l 11 need_return 0 tr 0
  thread 2393: l 00 need_return 0 tr 0
  thread 2490: l 11 need_return 0 tr 0
  node 10662: u00007dacbe27bc20 c00007dacee255530 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 44677: u00007dacbe285880 c00007dacee258dd0 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 44041: u00007dacbe286450 c00007dacee258fb0 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 13702: u00007dacbe286630 c00007dacee256190 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 13696: u00007dacbe286660 c00007dacee255590 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 14251: u00007dacbe287410 c00007dacee2569d0 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 14393: u00007dacbe2875c0 c00007dacee257270 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 337
  node 14261: u00007dacbe287710 c00007dacee256eb0 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 tr 1 proc 542
  node 14397: u00007dacbe2877a0 c00007dacee2565b0 pri 0:139 hs 1 hw 1 ls 0 lw 0 is 2 iw 2 tr 1 proc 191 542
...
  ref 10632: desc 0 node 1 s 1 w 1 d 0000000000000000
  ref 10651: desc 1 node 5376 s 1 w 1 d 0000000000000000
  ref 13471: desc 2 node 4555 s 1 w 1 d 0000000000000000

```
이 proc dir은  binderfs.c에서 만드는데 파일생성할때 pid기준으로 생성하므로 snprintf같은 함수가 있나 확인을 해봤다.
```c
/*binder.c*/

	if (binder_binderfs_dir_entry_proc && !existing_pid) {
		char strbuf[11];
		struct dentry *binderfs_entry;

		snprintf(strbuf, sizeof(strbuf), "%u", proc->pid);
		/*
		 * Similar to debugfs, the process specific log file is shared
		 * between contexts. Only create for the first PID.
		 * This is ok since same as debugfs, the log file will contain
		 * information on all contexts of a given PID.
		 */
		binderfs_entry = binderfs_create_file(binder_binderfs_dir_entry_proc,
			strbuf, &proc_fops, (void *)(unsigned long)proc->pid);
		if (!IS_ERR(binderfs_entry)) {
			proc->binderfs_entry = binderfs_entry;
		} else {
			int error;

			error = PTR_ERR(binderfs_entry);
			pr_warn("Unable to create file %s in binderfs (error %d)\n",
				strbuf, error);
		}
	}
```
해당 함수에서 snprintf로 proc->pid를 strbuf로 이동하는데 strbuf는 char[11]이다. 혹시 int의 최대값을 찾아봤는데 2,147,483,647로 10자리로 이상없이 넣는게 가능하다. (아쉽)

어쨌든 다음과 같은 proc값이 생성되고 삭제될 때 해당 부분을 거치므로 이에 대한 조사를 계속해서 진행중. 해당 파일은 binderfs_create_file로 생성되고 binderfs_remove_file로 삭제되는데, 이 때 그러면 binder_device가 어떻게 이루어져 있는지 확인이 필요했다.

```gdb
pwndbg> p device
$17 = (struct binder_device *) 0x1575 <cpu_debug_store+1397>
pwndbg> p inode
$18 = (struct inode *) 0xffff888061118000
```

그런데 이상하게도 binder_device의 주소가 이상하다. 해당 값은 i_private값으로 들어가 있는데, 0x1575값으로 들어가 있다.

```gdb
 ► f 0 0xffffffff820b715f binderfs_create_file+31
   f 1 0xffffffff820c20af binder_open+879
   f 2 0xffffffff8157fba4 do_dentry_open+740
   f 3 0xffffffff815a1edf path_openat+3599
   f 4 0xffffffff815a102c do_filp_open+156
   f 5 0xffffffff8157ce70 do_sys_openat2+176
   f 6 0xffffffff8157d149 __x64_sys_openat+121
   f 7 0xffffffff8157d149 __x64_sys_openat+121
```

다음과 같은 콜스택으로 다시 binder_open함수를 보면

```c
static int binder_open(struct inode *nodp, struct file *filp)
{
	struct binder_proc *proc, *itr;
...
	if (binder_binderfs_dir_entry_proc && !existing_pid) {
		char strbuf[11];
		struct dentry *binderfs_entry;

		snprintf(strbuf, sizeof(strbuf), "%u", proc->pid);
		/*
		 * Similar to debugfs, the process specific log file is shared
		 * between contexts. Only create for the first PID.
		 * This is ok since same as debugfs, the log file will contain
		 * information on all contexts of a given PID.
		 */
		binderfs_entry = binderfs_create_file(binder_binderfs_dir_entry_proc,
			strbuf, &proc_fops, (void *)(unsigned long)proc->pid);
		if (!IS_ERR(binderfs_entry)) {
			proc->binderfs_entry = binderfs_entry;
		} else {
			int error;

			error = PTR_ERR(binderfs_entry);
			pr_warn("Unable to create file %s in binderfs (error %d)\n",
				strbuf, error);
		}
	}
	return 0;
}	

struct dentry *binderfs_create_file(struct dentry *parent, const char *name,
				    const struct file_operations *fops,
				    void *data)
{
...
	new_inode = binderfs_make_inode(sb, S_IFREG | 0444);
...
	new_inode->i_fop = fops;
	new_inode->i_private = data;
...
}
```

다음고 같이 이루어져 있는데 맨 마지막 값인 proc->pid를 inode의 private값으로 넣어버린다. 이러면 나중에 evict_inode값에서 에러가 일어나야 할 것 같지만 실제로는

```c
static void binderfs_evict_inode(struct inode *inode)
{
	struct binder_device *device = inode->i_private;
	struct binderfs_info *info = BINDERFS_SB(inode->i_sb);

	clear_inode(inode);

	if (!S_ISCHR(inode->i_mode) || !device)
		return;

	mutex_lock(&binderfs_minors_mutex);
	--info->device_count;
	ida_free(&binderfs_minors, device->miscdev.minor);
	mutex_unlock(&binderfs_minors_mutex);

	if (refcount_dec_and_test(&device->ref)) {
		kfree(device->context.name);
		kfree(device);
	}
}
```

저 앞의 S_ISCHR에 걸려서 그냥 return 되어버림. 그 이유는 binder-control/ binder-control ioctl을 통하여 만드는 binder_device객체들은 모두 S_IFCHR로 special node를 만들기 때문이다. 의도한것으로 보이긴 하지만 뭔가 아주 엉성한 코드다. 결론적으로는 모두 잘 작동하네.

















