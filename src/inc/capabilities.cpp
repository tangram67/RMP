/*
 * capabilities.cpp
 *
 *  Created on: 06.11.2020
 *      Author: dirk
 */

#include "capabilities.h"
#include "stringutils.h"
#include "fileutils.h"

namespace app {


TCapabilityMap fillCapabilityMap()
{
	TCapabilityMap capabilities;

	/* In a system with the [_POSIX_CHOWN_RESTRICTED] option defined, this
	   overrides the restriction of changing file ownership and group
	   ownership. */

	capabilities.insert(TCapabilityItem("CAP_CHOWN", CAP_CHOWN)); // 0

	/* Override all DAC access, including ACL execute access if
	   [_POSIX_ACL] is defined. Excluding DAC access covered by
	   CAP_LINUX_IMMUTABLE. */

	capabilities.insert(TCapabilityItem("CAP_DAC_OVERRIDE", CAP_DAC_OVERRIDE)); // 1

	/* Overrides all DAC restrictions regarding read and search on files
	   and directories, including ACL restrictions if [_POSIX_ACL] is
	   defined. Excluding DAC access covered by CAP_LINUX_IMMUTABLE. */

	capabilities.insert(TCapabilityItem("CAP_DAC_READ_SEARCH", CAP_DAC_READ_SEARCH)); // 2

	/* Overrides all restrictions about allowed operations on files, where
	   file owner ID must be equal to the user ID, except where CAP_FSETID
	   is applicable. It doesn't override MAC and DAC restrictions. */

	capabilities.insert(TCapabilityItem("CAP_FOWNER", CAP_FOWNER)); // 3

	/* Overrides the following restrictions that the effective user ID
	   shall match the file owner ID when setting the S_ISUID and S_ISGID
	   bits on that file; that the effective group ID (or one of the
	   supplementary group IDs) shall match the file owner ID when setting
	   the S_ISGID bit on that file; that the S_ISUID and S_ISGID bits are
	   cleared on successful return from chown(2) (not implemented). */

	capabilities.insert(TCapabilityItem("CAP_FSETID", CAP_FSETID)); // 4

	/* Overrides the restriction that the real or effective user ID of a
	   process sending a signal must match the real or effective user ID
	   of the process receiving the signal. */

	capabilities.insert(TCapabilityItem("CAP_KILL", CAP_KILL)); // 5

	/* Allows setgid(2) manipulation */
	/* Allows setgroups(2) */
	/* Allows forged gids on socket credentials passing. */

	capabilities.insert(TCapabilityItem("CAP_SETGID", CAP_SETGID)); // 6

	/* Allows set*uid(2) manipulation (including fsuid). */
	/* Allows forged pids on socket credentials passing. */

	capabilities.insert(TCapabilityItem("CAP_SETUID", CAP_SETUID)); // 7


	/**
	 ** Linux-specific capabilities
	 **/

	/* Without VFS support for capabilities:
	 *   Transfer any capability in your permitted set to any pid,
	 *   remove any capability in your permitted set from any pid
	 * With VFS support for capabilities (neither of above, but)
	 *   Add any capability from current's capability bounding set
	 *       to the current process' inheritable set
	 *   Allow taking bits out of capability bounding set
	 *   Allow modification of the securebits for a process
	 */

	capabilities.insert(TCapabilityItem("CAP_SETPCAP", CAP_SETPCAP)); // 8

	/* Allow modification of S_IMMUTABLE and S_APPEND file attributes */

	capabilities.insert(TCapabilityItem("CAP_LINUX_IMMUTABLE", CAP_LINUX_IMMUTABLE)); // 9

	/* Allows binding to TCP/UDP sockets below 1024 */
	/* Allows binding to ATM VCIs below 32 */

	capabilities.insert(TCapabilityItem("CAP_NET_BIND_SERVICE", CAP_NET_BIND_SERVICE)); // 10

	/* Allow broadcasting, listen to multicast */

	capabilities.insert(TCapabilityItem("CAP_NET_BROADCAST", CAP_NET_BROADCAST)); // 11

	/* Allow interface configuration */
	/* Allow administration of IP firewall, masquerading and accounting */
	/* Allow setting debug option on sockets */
	/* Allow modification of routing tables */
	/* Allow setting arbitrary process / process group ownership on
	   sockets */
	/* Allow binding to any address for transparent proxying (also via NET_RAW) */
	/* Allow setting TOS (type of service) */
	/* Allow setting promiscuous mode */
	/* Allow clearing driver statistics */
	/* Allow multicasting */
	/* Allow read/write of device-specific registers */
	/* Allow activation of ATM control sockets */

	capabilities.insert(TCapabilityItem("CAP_NET_ADMIN", CAP_NET_ADMIN)); // 12

	/* Allow use of RAW sockets */
	/* Allow use of PACKET sockets */
	/* Allow binding to any address for transparent proxying (also via NET_ADMIN) */

	capabilities.insert(TCapabilityItem("CAP_NET_RAW", CAP_NET_RAW)); // 13

	/* Allow locking of shared memory segments */
	/* Allow mlock and mlockall (which doesn't really have anything to do
	   with IPC) */

	capabilities.insert(TCapabilityItem("CAP_IPC_LOCK", CAP_IPC_LOCK)); // 14

	/* Override IPC ownership checks */

	capabilities.insert(TCapabilityItem("CAP_IPC_OWNER", CAP_IPC_OWNER)); // 15

	/* Insert and remove kernel modules - modify kernel without limit */
	capabilities.insert(TCapabilityItem("CAP_SYS_MODULE", CAP_SYS_MODULE)); // 16

	/* Allow ioperm/iopl access */
	/* Allow sending USB messages to any device via /dev/bus/usb */

	capabilities.insert(TCapabilityItem("CAP_SYS_RAWIO", CAP_SYS_RAWIO)); // 17

	/* Allow use of chroot() */

	capabilities.insert(TCapabilityItem("CAP_SYS_CHROOT", CAP_SYS_CHROOT)); // 18

	/* Allow ptrace() of any process */

	capabilities.insert(TCapabilityItem("CAP_SYS_PTRACE", CAP_SYS_PTRACE)); // 19

	/* Allow configuration of process accounting */

	capabilities.insert(TCapabilityItem("CAP_SYS_PACCT", CAP_SYS_PACCT)); // 20

	/* Allow configuration of the secure attention key */
	/* Allow administration of the random device */
	/* Allow examination and configuration of disk quotas */
	/* Allow setting the domainname */
	/* Allow setting the hostname */
	/* Allow calling bdflush() */
	/* Allow mount() and umount(), setting up new smb connection */
	/* Allow some autofs root ioctls */
	/* Allow nfsservctl */
	/* Allow VM86_REQUEST_IRQ */
	/* Allow to read/write pci config on alpha */
	/* Allow irix_prctl on mips (setstacksize) */
	/* Allow flushing all cache on m68k (sys_cacheflush) */
	/* Allow removing semaphores */
	/* Used instead of CAP_CHOWN to "chown" IPC message queues, semaphores
	   and shared memory */
	/* Allow locking/unlocking of shared memory segment */
	/* Allow turning swap on/off */
	/* Allow forged pids on socket credentials passing */
	/* Allow setting readahead and flushing buffers on block devices */
	/* Allow setting geometry in floppy driver */
	/* Allow turning DMA on/off in xd driver */
	/* Allow administration of md devices (mostly the above, but some
	   extra ioctls) */
	/* Allow tuning the ide driver */
	/* Allow access to the nvram device */
	/* Allow administration of apm_bios, serial and bttv (TV) device */
	/* Allow manufacturer commands in isdn CAPI support driver */
	/* Allow reading non-standardized portions of pci configuration space */
	/* Allow DDI debug ioctl on sbpcd driver */
	/* Allow setting up serial ports */
	/* Allow sending raw qic-117 commands */
	/* Allow enabling/disabling tagged queuing on SCSI controllers and sending
	   arbitrary SCSI commands */
	/* Allow setting encryption key on loopback filesystem */
	/* Allow setting zone reclaim policy */

	capabilities.insert(TCapabilityItem("CAP_SYS_ADMIN", CAP_SYS_ADMIN)); // 21

	/* Allow use of reboot() */

	capabilities.insert(TCapabilityItem("CAP_SYS_BOOT", CAP_SYS_BOOT)); // 22

	/* Allow raising priority and setting priority on other (different
	   UID) processes */
	/* Allow use of FIFO and round-robin (realtime) scheduling on own
	   processes and setting the scheduling algorithm used by another
	   process. */
	/* Allow setting cpu affinity on other processes */

	capabilities.insert(TCapabilityItem("CAP_SYS_NICE", CAP_SYS_NICE)); // 23

	/* Override resource limits. Set resource limits. */
	/* Override quota limits. */
	/* Override reserved space on ext2 filesystem */
	/* Modify data journaling mode on ext3 filesystem (uses journaling
	   resources) */
	/* NOTE: ext2 honors fsuid when checking for resource overrides, so
	   you can override using fsuid too */
	/* Override size restrictions on IPC message queues */
	/* Allow more than 64hz interrupts from the real-time clock */
	/* Override max number of consoles on console allocation */
	/* Override max number of keymaps */

	capabilities.insert(TCapabilityItem("CAP_SYS_RESOURCE", CAP_SYS_RESOURCE)); // 24

	/* Allow manipulation of system clock */
	/* Allow irix_stime on mips */
	/* Allow setting the real-time clock */

	capabilities.insert(TCapabilityItem("CAP_SYS_TIME", CAP_SYS_TIME)); // 25

	/* Allow configuration of tty devices */
	/* Allow vhangup() of tty */

	capabilities.insert(TCapabilityItem("CAP_SYS_TTY_CONFIG", CAP_SYS_TTY_CONFIG)); // 26

	/* Allow the privileged aspects of mknod() */

	capabilities.insert(TCapabilityItem("CAP_MKNOD", CAP_MKNOD)); // 27

	/* Allow taking of leases on files */

	capabilities.insert(TCapabilityItem("CAP_LEASE", CAP_LEASE)); // 28

	/* Allow writing the audit log via unicast netlink socket */

	capabilities.insert(TCapabilityItem("CAP_AUDIT_WRITE", CAP_AUDIT_WRITE)); // 29

	/* Allow configuration of audit via unicast netlink socket */

	capabilities.insert(TCapabilityItem("CAP_AUDIT_CONTROL", CAP_AUDIT_CONTROL)); // 30

	capabilities.insert(TCapabilityItem("CAP_SETFCAP", CAP_SETFCAP)); // 31

	/* Override MAC access.
	   The base kernel enforces no MAC policy.
	   An LSM may enforce a MAC policy, and if it does and it chooses
	   to implement capability based overrides of that policy, this is
	   the capability it should use to do so. */

	capabilities.insert(TCapabilityItem("CAP_MAC_OVERRIDE", CAP_MAC_OVERRIDE)); // 32

	/* Allow MAC configuration or state changes.
	   The base kernel requires no MAC configuration.
	   An LSM may enforce a MAC policy, and if it does and it chooses
	   to implement capability based checks on modifications to that
	   policy or the data required to maintain it, this is the
	   capability it should use to do so. */

	capabilities.insert(TCapabilityItem("CAP_MAC_ADMIN", CAP_MAC_ADMIN)); // 33

	/* Allow configuring the kernel's syslog (printk behaviour) */

	capabilities.insert(TCapabilityItem("CAP_SYSLOG", CAP_SYSLOG)); // 34

	/* Allow triggering something that will wake the system */

	capabilities.insert(TCapabilityItem("CAP_WAKE_ALARM", CAP_WAKE_ALARM)); // 35

	/* Allow preventing system suspends */

	capabilities.insert(TCapabilityItem("CAP_BLOCK_SUSPEND", CAP_BLOCK_SUSPEND)); // 36

	/* Allow reading the audit log via multicast netlink socket */

	capabilities.insert(TCapabilityItem("CAP_AUDIT_READ", CAP_AUDIT_READ)); // 37

	return capabilities;
}

static TCapabilityMap localCapabilitiesMap = fillCapabilityMap();


TCapabilities::TCapabilities() {
}

TCapabilities::~TCapabilities() {
}

cap_value_t TCapabilities::getCapabilityByName(const std::string& name) {
	if (!name.empty()) {
		std::string desc = util::toupper(name);
		if (!desc.empty()) {
			TCapabilityMap::const_iterator it = localCapabilitiesMap.find(desc);
			if (it != localCapabilitiesMap.end())
				return it->second;
		}
	}
	return INVALID_CAP_VALUE;
}

bool TCapabilities::saveCapabilitiesToFile(const std::string& fileName) {
	util::TStdioFile file;
	file.open(fileName, "w");
	bool r = false;
	if (file.isOpen()) {
		cap_t caps = cap_get_proc();
		if (util::assigned(caps)) {
			ssize_t size = 0;
			char *p = cap_to_text(caps, &size);
			if (util::assigned(p) && size > 0) {
				file.write(util::fileBaseName(fileName) + " ");
				file.write(p, size);
				file.write("\n");
				cap_free(p);
				r = true;
			}
			cap_free(caps);
		}
	}
	return r;
}


} /* namespace app */
