#include <kern/debug.h>
#include <mach/kern_return.h>
#include <riscv64/riscv64/mach_riscv64.server.h>

/* routine riscv64_get_hwcaps(
 *               host            : host_t;
 *       out     hwcaps          : hwcaps_t, CountInOut;
 *       out     midr_el1        : uint64_t;
 *       out     revidr_el1      : uint64_t);
 */

kern_return_t
riscv64_get_hwcaps(
	host_t host,
	hwcaps_t hwcaps,
	mach_msg_type_number_t *hwcapsCnt,
	uint64_t* midr_el1,
	uint64_t* revidr_el1)
{
	panic("TODO: not implemented");
}
