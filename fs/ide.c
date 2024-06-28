/*
 * operations on IDE disk.
 */

#include "serv.h"
#include <lib.h>
#include <malta.h>
#include <mmu.h>

/* Overview:
 *   Wait for the IDE device to complete previous requests and be ready
 *   to receive subsequent requests.
 */
static uint8_t wait_ide_ready() {
	uint8_t flag;
	while (1) {
		panic_on(syscall_read_dev(&flag, MALTA_IDE_STATUS, 1));
		if ((flag & MALTA_IDE_BUSY) == 0) {
			break;
		}
		syscall_yield();
	}
	return flag;
}

/* Overview:
 *  read data from IDE disk. First issue a read request through
 *  disk register and then copy data from disk buffer
 *  (512 bytes, a sector) to destination array.
 *
 * Parameters:
 *  diskno: disk number.
 *  secno: start sector number.
 *  dst: destination for data read from IDE disk.
 *  nsecs: the number of sectors to read.
 *
 * Post-Condition:
 *  Panic if any error occurs. (you may want to use 'panic_on')
 *
 * Hint: Use syscalls to access device registers and buffers.
 * Hint: Use the physical address and offsets defined in 'include/malta.h'.
 */
void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs) {
	uint8_t temp;
	u_int offset = 0, max = nsecs + secno;
	panic_on(diskno >= 2);

	// Read the sector in turn
	while (secno < max) {
		temp = wait_ide_ready();
		// Step 1: Write the number of operating sectors to NSECT register
		temp = 1;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_NSECT, 1));

		// Step 2: Write the 7:0 bits of sector number to LBAL register
		temp = secno & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAL, 1));

		// Step 3: Write the 15:8 bits of sector number to LBAM register
		/* Exercise 5.3: Your code here. (1/9) */
		temp = ((secno >> 8) & 0xff);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAM, 1));
		// Step 4: Write the 23:16 bits of sector number to LBAH register
		/* Exercise 5.3: Your code here. (2/9) */
		temp = ((secno >> 16) & 0xff);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAH, 1));
		// Step 5: Write the 27:24 bits of sector number, addressing mode
		// and diskno to DEVICE register
		temp = ((secno >> 24) & 0x0f) | MALTA_IDE_LBA | (diskno << 4);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_DEVICE, 1));

		// Step 6: Write the working mode to STATUS register
		temp = MALTA_IDE_CMD_PIO_READ;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_STATUS, 1));

		// Step 7: Wait until the IDE is ready
		temp = wait_ide_ready();

		// Step 8: Read the data from device
		for (int i = 0; i < SECT_SIZE / 4; i++) {
			panic_on(syscall_read_dev(dst + offset + i * 4, MALTA_IDE_DATA, 4));
		}

		// Step 9: Check IDE status
		panic_on(syscall_read_dev(&temp, MALTA_IDE_STATUS, 1));

		offset += SECT_SIZE;
		secno += 1;
	}
}

/* Overview:
 *  write data to IDE disk.
 *
 * Parameters:
 *  diskno: disk number.
 *  secno: start sector number.
 *  src: the source data to write into IDE disk.
 *  nsecs: the number of sectors to write.
 *
 * Post-Condition:
 *  Panic if any error occurs.
 *
 * Hint: Use syscalls to access device registers and buffers.
 * Hint: Use the physical address and offsets defined in 'include/malta.h'.
 */
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs) {
	uint8_t temp;
	u_int offset = 0, max = nsecs + secno;
	panic_on(diskno >= 2);

	// Write the sector in turn
	while (secno < max) {
		temp = wait_ide_ready();
		// Step 1: Write the number of operating sectors to NSECT register
		/* Exercise 5.3: Your code here. (3/9) */
		temp = 1;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_NSECT, 1));
		// Step 2: Write the 7:0 bits of sector number to LBAL register
		/* Exercise 5.3: Your code here. (4/9) */
		temp = secno & 0xff;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAL, 1));
		// Step 3: Write the 15:8 bits of sector number to LBAM register
		/* Exercise 5.3: Your code here. (5/9) */

		temp = ((secno >> 8) & 0xff);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAM, 1));
		// Step 4: Write the 23:16 bits of sector number to LBAH register
		/* Exercise 5.3: Your code here. (6/9) */

		temp = ((secno >> 16) & 0xff);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_LBAH, 1));
		// Step 5: Write the 27:24 bits of sector number, addressing mode
		// and diskno to DEVICE register
		/* Exercise 5.3: Your code here. (7/9) */
		temp = ((secno >> 24) & 0x0f) | MALTA_IDE_LBA | (diskno << 4);
		panic_on(syscall_write_dev(&temp, MALTA_IDE_DEVICE, 1));
		// Step 6: Write the working mode to STATUS register
		/* Exercise 5.3: Your code here. (8/9) */
		temp = MALTA_IDE_CMD_PIO_WRITE;
		panic_on(syscall_write_dev(&temp, MALTA_IDE_STATUS, 1));

		// Step 7: Wait until the IDE is ready
		temp = wait_ide_ready();

		// Step 8: Write the data to device
		for (int i = 0; i < SECT_SIZE / 4; i++) {
			/* Exercise 5.3: Your code here. (9/9) */
			panic_on(syscall_write_dev(src + offset + i * 4, MALTA_IDE_DATA, 4));
		}

		// Step 9: Check IDE status
		panic_on(syscall_read_dev(&temp, MALTA_IDE_STATUS, 1));

		offset += SECT_SIZE;
		secno += 1;
	}
}

//fs/ide.c
// int time_read() {
//     int time = 0;
//     if (syscall_read_dev((u_int) & time, 0x15000000, 4) < 0)
//         user_panic("time_read panic");
//     if (syscall_read_dev((u_int) & time, 0x15000010, 4) < 0)
//         user_panic("time_read panic");
//     return time;
// }

// void raid0_write(u_int secno, void *src, u_int nsecs) {
//     int i;
//     for (i = secno; i < secno + nsecs; i++) {
//         if (i % 2 == 0) {
//             ide_write(1, i / 2, src + (i - secno) * 0x200, 1);
//         } else {
//             ide_write(2, i / 2, src + (i - secno) * 0x200, 1);
//         }
//     }
// }

// void raid0_read(u_int secno, void *dst, u_int nsecs) {
//     int i;
//     for (i = secno; i < secno + nsecs; i++) {
//         if (i % 2 == 0) {
//             ide_read(1, i / 2, dst + (i - secno) * 0x200, 1);
//         } else {
//             ide_read(2, i / 2, dst + (i - secno) * 0x200, 1);
//         }
//     }
// }

//fs/ide.c
// int raid4_valid(u_int diskno) {
//     return !ide_read(diskno, 0, (void *) 0x13004000, 1);
// }

// #define MAXL (128)

// int raid4_write(u_int blockno, void *src) {
//     int i;
//     int invalid = 0;
//     int check[MAXL];
//     for (i = 0; i < 8; i++) {
//         if (raid4_valid(i % 4 + 1)) {
//             ide_write(i % 4 + 1, 2 * blockno + i / 4, src + i * 0x200, 1);
//         } else { invalid++; }
//     }
//     if (!raid4_valid(5)) {
//         return invalid / 2 + 1;
//     }
//     int j, k;
//     for (i = 0; i < 2; i++) {
//         for (j = 0; j < MAXL; j++) {
//             check[j] = 0;
//             for (k = 0; k < 4; k++) {
//                 check[j] ^= *(int *) (src + (4 * i + k) * 0x200 + j * 4);
//             }
//         }
//         ide_write(5, 2 * blockno + i, (void *) check, 1);
//     }
//     return invalid / 2;
// }

// int raid4_read(u_int blockno, void *dst) {
//     int i;
//     int invalid = 0;
//     int wrong = 0;
//     int check[2 * MAXL];
//     for (i = 1; i <= 5; i++) {
//         if (!raid4_valid(i)) {
//             invalid++;
//             wrong = i;
//         }
//     }
//     if (invalid > 1) {
//         return invalid;
//     }
//     for (i = 0; i < 8; i++) {
//         if (i % 4 + 1 != wrong) {
//             ide_read(i % 4 + 1, 2 * blockno + i / 4, dst + i * 0x200, 1);
//         }
//     }
//     if (wrong == 5) {
//         return 1;
//     }
//     int j, k;
//     ide_read(5, 2 * blockno, check, 2);
//     for (i = 0; i < 2; i++) {
//         for (j = 0; j < MAXL; j++) {
//             for (k = 0; k < 4; k++) {
//                 check[i * MAXL + j] ^= *(int *) (dst + (4 * i + k) * 0x200 + j * 4);
//             }
//         }
//     }
//     if (!wrong) {
//         for (j = 0; j < 2 * MAXL; j++) {
//             if (check[j] != 0) {
//                 return -1;
//             }
//         }
//         return 0;
//     }
//     wrong--;
//     user_bcopy(check, dst + wrong * 0x200, 0x200);
//     user_bcopy((void *) check + 0x200, dst + 0x800 + wrong * 0x200, 0x200);
//     return 1;
// }
