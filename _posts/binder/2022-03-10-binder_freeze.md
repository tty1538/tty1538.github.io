---
title: Binder Freeze Ioctl
date: 2022-03-10 11:40:00 +09:00
categories: [binder, linux kernel]
tags: [binder_freeze]
description: Analysis of binder_freeze
---


# binder freeze

BINDER_FREEZE ioctl에 대한 분석진행

```c
struct binder_freeze_info {
	__u32            pid;
	__u32            enable;
	__u32            timeout_ms;
};
```

1. ioctl을 보낼 때 binder_freeze_info 구조체를 이용하여 보내면 됨. 
2. 커널에서는 binder device를 open한 pid들을 저장한것을 쭉 돌며 ioctl 에서 보냈던 pid와 같은 pid를 가진 binder proc을 검색
```		hlist_for_each_entry(target_proc, &binder_procs, proc_node) {
			if (target_proc->pid != info.pid)
				continue;

			binder_inner_proc_lock(target_proc);
			target_proc->tmp_ref++;
			binder_inner_proc_unlock(target_proc);

			target_procs[i++] = target_proc;
		}
```
3. 해당 pid를 가진 모든 pid에 대해서 wait_event_interruptible_timeout을 호출. wiat_event_interruptible_timeout에서 timeout되고 나서 condition이 false일 경우 0, condition이 true일 경우 1, 만약 event를 받아서 tiemout전에 함수가 종료된었을 때 condition이 true라면 남은 jiffies시간이, condition이 false라면 -ERESTARSYS가 리턴됨
```
0 if the condition evaluated to false after the timeout elapsed, 1 if the condition evaluated to true after the timeout elapsed, the remaining jiffies (at least 1) if the condition evaluated to true before the timeout elapsed, or -ERESTARTSYS if it was interrupted by a signal.
```
4. 해당 프로세스에 이미 pending된 전송들이 있는 경우 sync_recv같은 변수들의 세팅을 해 준다. (하지만 sync_recv변수는 쓰이는 곳이 없음)
5. freeze된 프로세스의 경우 oneway가 아닌 경우 FROZEN_REPLY를 전달한다. oneway일 경우는 그냥 정상적으로 전달을 해버리는 것
```
static int binder_proc_transaction(struct binder_transaction *t,
				    struct binder_proc *proc,
				    struct binder_thread *thread)
{
...
	if ((proc->is_frozen && !oneway) || proc->is_dead ||
			(thread && thread->is_dead)) {
		binder_inner_proc_unlock(proc);
		binder_node_unlock(node);
		return proc->is_frozen ? BR_FROZEN_REPLY : BR_DEAD_REPLY;
	}
...
```

## 실험

만약에, service manager를 정지시켜버리면 어떤일이 일어날지? 궁금해서 테스트코드 작성,
pid를 인자로 받고 1000초 멈추게끔 코드를 작성해서 실험해 보았다.

아무일도 일어나지않음
확인해보니 binder_freeze ioctl이 막혀있음. 그렇다면 대체 얼마나 많은 ioctl이 막혀있는건지 확인이 필요함
확인해보니 ioctl별로 따로 막혀있는것 같았고, binder관련 다른 ioctl이 막혀있지는 않고, binder_freeze등만 막혀있는것으로 확인

```
int main(int argc, char *argv[]) {
  int fd, ret, pid;
  char pid_tmp[5];
  struct binder_freeze_info info;

  if (argc < 2)
    exit(EXIT_FAILURE);

  memcpy(pid_tmp, argv[1], 5);
  pid = atoi(pid_tmp);

  fd = open("/dev/binder", O_RDWR | O_CLOEXEC);
  if(fd < 0) {
      printf("err, binder open");
      exit(EXIT_FAILURE);
  }

  info.pid = pid;
  info.enable = 0;
  info.timeout_ms = 1000;
  
  ioctl(fd, BINDER_FREEZE, &info);
  exit(EXIT_SUCCESS);
}
```
```sh
emulator64_x86_64_arm64:/data/local/tmp $ ps -ef | grep servicemanager
system         191     1 0 14:23:19 ?     00:00:16 servicemanager
system         193     1 0 14:23:19 ?     00:00:11 hwservicemanager
shell        29324 29314 6 15:34:00 pts/0 00:00:00 grep servicemanager
emulator64_x86_64_arm64:/data/local/tmp $ ./poc 191

[263450.403369] type=1400 audit(1646894043.913:176): avc: denied { ioctl } for comm="poc" path="/dev/binderfs/binder" dev="binder" ino=4 ioctlcmd=0x620e scontext=u:r:shell:s0 tcontext=u:object_r:binder_device:s0 tclass=chr_file permissive=0
```


```
#/obj/ETC/product_sepolicy.cil_intermediates/product_policy.conf.dontaudit


# Only system server can access BINDER_FREEZE and BINDER_GET_FROZEN_INFO
allowxperm system_server binder_device:chr_file ioctl { 0x400c620e 0xc00c620f };

# BINDER_FREEZE is used to block ipc transactions to frozen processes, so it
# can be accessed by system_server only (b/143717177)
# BINDER_GET_FROZEN_INFO is used by system_server to determine the state of a frozen binder
# interface
neverallowxperm { domain -system_server } binder_device:chr_file ioctl { 0x400c620e 0xc00c620f };
#line 1 "system/sepolicy/private/system_server_startup.te"
type system_server_startup, domain, coredomain;
type system_server_startup_tmpfs, file_type;

```
