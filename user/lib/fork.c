#include <env.h>
#include <lib.h>
#include <mmu.h>

/* Overview:
 *   Map the faulting page to a private writable copy.
 *
 * Pre-Condition:
 * 	'va' is the address which led to the TLB Mod exception.
 *
 * Post-Condition:
 *  - Launch a 'user_panic' if 'va' is not a copy-on-write page.
 *  - Otherwise, this handler should map a private writable copy of
 *    the faulting page at the same address.
 */
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
	u_int va = tf->cp0_badvaddr;
	u_int perm;

	/* Step 1: Find the 'perm' in which the faulting address 'va' is mapped. */
	/* Hint: Use 'vpt' and 'VPN' to find the page table entry. If the 'perm' doesn't have
	 * 'PTE_COW', launch a 'user_panic'. */
	/* Exercise 4.13: Your code here. (1/6) */
	perm = vpt[VPN(va)] & 0xfff;
	if (!(perm & PTE_COW)) {
		user_panic("cow_entry:PTE_COW not found\n");
	}
	/* Step 2: Remove 'PTE_COW' from the 'perm', and add 'PTE_D' to it. */
	/* Exercise 4.13: Your code here. (2/6) */
	perm = (perm & (~PTE_COW)) | PTE_D;
	/* Step 3: Allocate a new page at 'UCOW'. */
	/* Exercise 4.13: Your code here. (3/6) */
	syscall_mem_alloc(0, (void *)UCOW, perm);
	/* Step 4: Copy the content of the faulting page at 'va' to 'UCOW'. */
	/* Hint: 'va' may not be aligned to a page! */
	/* Exercise 4.13: Your code here. (4/6) */
	u_int align_va = ROUNDDOWN(va, PAGE_SIZE);
	memcpy((void *)UCOW, (void *)align_va, PAGE_SIZE);
	// Step 5: Map the page at 'UCOW' to 'va' with the new 'perm'.
	/* Exercise 4.13: Your code here. (5/6) */
	syscall_mem_map(0,(void *)UCOW, 0, (void *)va, perm);
	// Step 6: Unmap the page at 'UCOW'.
	/* Exercise 4.13: Your code here. (6/6) */
	syscall_mem_unmap(0, (void *)UCOW);
	// Step 7: Return to the faulting routine.
	int r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}

/* Overview:
 *   Grant our child 'envid' access to the virtual page 'vpn' (with address 'vpn' * 'PAGE_SIZE') in
 * our (current env's) address space. 'PTE_COW' should be used to isolate the modifications on
 * unshared memory from a parent and its children.
 *
 * Post-Condition:
 *   If the virtual page 'vpn' has 'PTE_D' and doesn't has 'PTE_LIBRARY', both our original virtual
 *   page and 'envid''s newly-mapped virtual page should be marked 'PTE_COW' and without 'PTE_D',
 *   while the other permission bits are kept.
 *
 *   If not, the newly-mapped virtual page in 'envid' should have the exact same permission as our
 *   original virtual page.
 *
 * Hint:
 *   - 'PTE_LIBRARY' indicates that the page should be shared among a parent and its children.
 *   - A page with 'PTE_LIBRARY' may have 'PTE_D' at the same time, you should handle it correctly.
 *   - You can pass '0' as an 'envid' in arguments of 'syscall_*' to indicate current env because
 *     kernel 'envid2env' converts '0' to 'curenv').
 *   - You should use 'syscall_mem_map', the user space wrapper around 'msyscall' to invoke
 *     'sys_mem_map' in kernel.
 */
static void duppage(u_int envid, u_int vpn) {
	int r;
	u_int addr;
	u_int perm;

	/* Step 1: Get the permission of the page. */
	/* Hint: Use 'vpt' to find the page table entry. */
	/* Exercise 4.10: Your code here. (1/2) */
	addr = vpn * PAGE_SIZE;
	perm = vpt[vpn] & 0xfff;
	/* Step 2: If the page is writable, and not shared with children, and not marked as COW yet,
	 * then map it as copy-on-write, both in the parent (0) and the child (envid). */
	/* Hint: The page should be first mapped to the child before remapped in the parent. (Why?)
	 */
	/* Exercise 4.10: Your code here. (2/2) */
	if ((perm & PTE_D) && !(perm & PTE_LIBRARY) && !(perm & PTE_COW)) {
		u_int new_perm = ((perm & (~PTE_D)) | PTE_COW);
		r= syscall_mem_map(0, (void *)addr, envid, (void *)addr, new_perm);
		if(r){
            user_panic("syscall_mem_map failed1: %d\n", r);
        }
		r= syscall_mem_map(0, (void *)addr, 0, (void *)addr, new_perm);
		if(r){
            user_panic("syscall_mem_map failed2: %d\n", r);
        }
	} else {
		r= syscall_mem_map(0, (void *)addr, envid, (void *)addr, perm);
		if(r){
            user_panic("syscall_mem_map failed3: %d\n", r);
        }
	}
}

/* Overview:
 *   User-level 'fork'. Create a child and then copy our address space.
 *   Set up ours and its TLB Mod user exception entry to 'cow_entry'.
 *
 * Post-Conditon:
 *   Child's 'env' is properly set.
 *
 * Hint:
 *   Use global symbols 'env', 'vpt' and 'vpd'.
 *   Use 'syscall_set_tlb_mod_entry', 'syscall_getenvid', 'syscall_exofork',  and 'duppage'.
 */
int fork(void) {
	u_int child;
	u_int i;

	/* Step 1: Set our TLB Mod user exception entry to 'cow_entry' if not done yet. */
	if (env->env_user_tlb_mod_entry != (u_int)cow_entry) {
		try(syscall_set_tlb_mod_entry(0, cow_entry));
	}

	/* Step 2: Create a child env that's not ready to be scheduled. */
	// Hint: 'env' should always point to the current env itself, so we should fix it to the
	// correct value.
	child = syscall_exofork();
	if (child == 0) {
		env = envs + ENVX(syscall_getenvid());
		return 0;
	}

	/* Step 3: Map all mapped pages below 'USTACKTOP' into the child's address space. */
	// Hint: You should use 'duppage'.
	/* Exercise 4.15: Your code here. (1/2) */
	for (int vpn = 0; vpn < USTACKTOP; vpn+=PAGE_SIZE) {
		if ((vpd[vpn>>PDSHIFT] & PTE_V) && (vpt[vpn>>PGSHIFT] & PTE_V)) {
			duppage(child, VPN(vpn));
		}
	}
	/* Step 4: Set up the child's tlb mod handler and set child's 'env_status' to
	 * 'ENV_RUNNABLE'. */
	/* Hint:
	 *   You may use 'syscall_set_tlb_mod_entry' and 'syscall_set_env_status'
	 *   Child's TLB Mod user exception entry should handle COW, so set it to 'cow_entry'
	 */
	/* Exercise 4.15: Your code here. (2/2) */
	try(syscall_set_tlb_mod_entry(child, cow_entry));
	try(syscall_set_env_status(child, ENV_RUNNABLE));
	return child;
}

// int make_shared(void *va) {
//     u_int perm = PTE_D | PTE_V;
//     if (!(vpd[va >> 22] & PTE_V) || !(vpt[va >> 12] & PTE_V)) { 
//         //当前进程的页表中不存在该虚拟页
//         if (syscall_mem_alloc(0, ROUNDDOWN(va, BY2PG), perm) != 0) { 
//             //将envid设为0，表示默认curenv
//             return -1;
//     	}
//     } 
//     perm = vpt[VPN(va)] & 0xfff; //获得va的perm
//     if (va >= (void *)UTOP || 
//         ((vpd[va >> 22] & PTE_V) && (vpt[va >> 12] & PTE_V) && !(perm & PTE_D))) {
//         return -1;
//     }
//     perm = perm | PTE_LIBRARY;
//     u_int addr = VPN(va) * BY2PG; 
//     if (syscall_mem_map(0, (void *)addr, 0, (void *)addr, perm) != 0) {
// 			return -1;
// 	} 
//     return ROUNDDOWN(vpt[VPN(va)] & (~0xfff), BY2PG);
// }





// int fork(void) (user/lib/fork.c)

// 【注意】： env = envs + ENVX(syscall_getenvid());

// syscall_getenvid()：获得当前进程的envid

// envs + ENVX(…) : 由envid获得env

// 【注意】

// vpd是页目录首地址，以vpd为基地址，加上页目录项偏移数即可指向va对应页目录项，即(*vpd) + (va >> 22) 或 vpd[va >> 22] ；

// 二级页表的物理地址：vpd[va >> 22] & (~0xfff)

// 提前判断有效位： (vpd[va >> 22] & PTE_V) 或 (vpd[VPN(va) >> 10] & PTE_V)

// vpt是页表首地址，以vpt为基地址，加上页表项偏移数即可指向va对应的页表项，即(*vpt) + (va >> 12) 或 vpt[va >> 12] 即 vpt[VPN(va)]；

// 物理页面地址：vpt[va >> 12] & (~0xfff)

// 提前判断有效位： (vpt[va >> 12] & PTE_V) 或 (vpt[VPN(va)] & PTE_V)

// vpn = VPN(va) = va >> 12（ 虚拟页号）

// static void ... cow_entry(...) (user/lib/fork.c)
