#ifndef _PERF_SYS_H
#define _PERF_SYS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <linux/perf_event.h>
#include <asm/barrier.h>

#if defined(__i386__)
#define cpu_relax()	asm volatile("rep; nop" ::: "memory");
#define CPUINFO_PROC	{"model name"}
#ifndef __NR_perf_event_open
# define __NR_perf_event_open 336
#endif
#ifndef __NR_futex
# define __NR_futex 240
#endif
#ifndef __NR_gettid
# define __NR_gettid 224
#endif
#endif

#if defined(__x86_64__)
#define cpu_relax()	asm volatile("rep; nop" ::: "memory");
#define CPUINFO_PROC	{"model name"}
#ifndef __NR_perf_event_open
# define __NR_perf_event_open 298
#endif
#ifndef __NR_futex
# define __NR_futex 202
#endif
#ifndef __NR_gettid
# define __NR_gettid 186
#endif
#endif

#ifdef __powerpc__
#include "../../arch/powerpc/include/uapi/asm/unistd.h"
#define CPUINFO_PROC	{"cpu"}
#endif

#ifdef __s390__
#define CPUINFO_PROC	{"vendor_id"}
#endif

#ifdef __sh__
#define CPUINFO_PROC	{"cpu type"}
#endif

#ifdef __hppa__
#define CPUINFO_PROC	{"cpu"}
#endif

#ifdef __sparc__
#define CPUINFO_PROC	{"cpu"}
#endif

#ifdef __alpha__
#define CPUINFO_PROC	{"cpu model"}
#endif

#ifdef __ia64__
#define cpu_relax()	asm volatile ("hint @pause" ::: "memory")
#define CPUINFO_PROC	{"model name"}
#endif

#ifdef __arm__
#define CPUINFO_PROC	{"model name", "Processor"}
#endif

#ifdef __aarch64__
#define cpu_relax()	asm volatile("yield" ::: "memory")
#endif

#ifdef __mips__
#define CPUINFO_PROC	{"cpu model"}
#endif

#ifdef __arc__
#define CPUINFO_PROC	{"Processor"}
#endif

#ifdef __metag__
#define CPUINFO_PROC	{"CPU"}
#endif

#ifdef __xtensa__
#define CPUINFO_PROC	{"core ID"}
#endif

#ifdef __tile__
#define cpu_relax()	asm volatile ("mfspr zero, PASS" ::: "memory")
#define CPUINFO_PROC    {"model name"}
#endif

#ifndef cpu_relax
#define cpu_relax() barrier()
#endif

#include <stdio.h>
#include <sys/prctl.h>
#include <errno.h>

char perf_err[BUFSIZ];

#ifndef PR_GET_ERR_DESC
#define PR_GET_ERR_DESC 47
#endif

static inline int
sys_perf_event_open(struct perf_event_attr *attr,
		      pid_t pid, int cpu, int group_fd,
		      unsigned long flags)
{
	int fd;

	perf_err[0] = 0;

	fd = syscall(__NR_perf_event_open, attr, pid, cpu,
		     group_fd, flags);

	if (fd < 0) {
		int q = prctl(PR_GET_ERR_DESC, &perf_err, sizeof(perf_err), 0, 0);
		fprintf(stderr, "kernel says (%d/%d): %s\n", q, errno, perf_err);
	}

#ifdef HAVE_ATTR_TEST
	if (unlikely(test_attr__enabled))
		test_attr__open(attr, pid, cpu, fd, group_fd, flags);
#endif
	return fd;
}

#endif /* _PERF_SYS_H */
