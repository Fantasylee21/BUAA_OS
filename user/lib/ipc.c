// User-level IPC library routines

#include <env.h>
#include <lib.h>
#include <mmu.h>

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void ipc_send(u_int whom, u_int val, const void *srcva, u_int perm) {
	int r;
	while ((r = syscall_ipc_try_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}
	user_assert(r == 0);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int ipc_recv(u_int *whom, void *dstva, u_int *perm) {
	int r = syscall_ipc_recv(dstva);
	if (r != 0) {
		user_panic("syscall_ipc_recv err: %d", r);
	}

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}

// u_int get_time(u_int *us) {
// 	u_int triger = 0x0000;
// 	u_int reads = 0x0010;
// 	u_int readus = 0x0020;
// 	u_int clock = 0x15000000;
// 	u_int s;
// 	u_int start = 1;
// 	syscall_write_dev(&start, clock + triger, 4);
// 	syscall_read_dev(&s, clock + reads, 4);
// 	syscall_read_dev(us, clock + readus, 4);
// 	return s;
// }

// void usleep(u_int us) {
// 	// 读取进程进入 usleep 函数的时间
// 	u_int u_inus;
// 	u_int u_intime = get_time(&u_inus);
// 	int inus = (int) u_inus;
// 	int intime = (int) u_intime;
// 	while (1) {
// 		// 读取当前时间
// 		u_int u_nowus;
// 		u_int u_nowtime = get_time(&u_nowus);
// 		int nowus = (int) u_nowus;
// 		int nowtime = (int) u_nowtime;
// 		if (((nowtime - intime) * 1000000 + (nowus - inus)) > us) {
// 			return;
// 		} else {
// 			// 进程切换
// 			syscall_yield();
// 		}
// 	}
// }