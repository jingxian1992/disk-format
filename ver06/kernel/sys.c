#include <stdio.h>
#include <errno.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <string.h>
#include <linux/sched.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/sys.h>

/* 
 * The timezone where the local system is located.  Used as a default by some
 * programs who obtain this value by using gettimeofday.
 */
struct timezone sys_tz;
struct utsname thisname;
extern int session_of_pgrp(int pgrp);

int sys_ftime()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

int sys_ptrace()
{
	return -ENOSYS;
}

int sys_stty()
{
	return -ENOSYS;
}

int sys_gtty()
{
	return -ENOSYS;
}

int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

/*
 * This is done BSD-style, with no consideration of the saved gid, except
 * that if you set the effective gid, it sets the saved gid too.  This 
 * makes it possible for a setgid program to completely drop its privileges,
 * which is often a useful assertion to make when you are doing a security
 * audit over a program.
 *
 * The general idea is that a program which uses just setregid() will be
 * 100% compatible with BSD.  A program which uses just setgid() will be
 * 100% compatible with POSIX w/ Saved ID's. 
 */
int sys_setregid(int rgid, int egid)
{
	if (rgid>0) {
		if ((current->gid == rgid) || 
		    suser())
			current->gid = rgid;
		else
			return(-EPERM);
	}
	if (egid>0) {
		if ((current->gid == egid) ||
		    (current->egid == egid) ||
		    suser()) {
			current->egid = egid;
			current->sgid = egid;
		} else
			return(-EPERM);
	}
	return 0;
}

/*
 * setgid() is implemeneted like SysV w/ SAVED_IDS 
 */
int sys_setgid(int gid)
{
	if (suser())
		current->gid = current->egid = current->sgid = gid;
	else if ((gid == current->gid) || (gid == current->sgid))
		current->egid = gid;
	else
		return -EPERM;
	return 0;
}

int sys_acct()
{
	return -ENOSYS;
}

int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

int sys_time(int * tloc)
{
	int i;

	i = CURRENT_TIME();
	if (tloc) {
		verify_area(tloc,4);
		put_fs_long(i,(unsigned int *)tloc);
	}
	return i;
}

/*
 * Unprivileged users may change the real user id to the effective uid
 * or vice versa.  (BSD-style)
 *
 * When you set the effective uid, it sets the saved uid too.  This 
 * makes it possible for a setuid program to completely drop its privileges,
 * which is often a useful assertion to make when you are doing a security
 * audit over a program.
 *
 * The general idea is that a program which uses just setreuid() will be
 * 100% compatible with BSD.  A program which uses just setuid() will be
 * 100% compatible with POSIX w/ Saved ID's. 
 */
int sys_setreuid(int ruid, int euid)
{
	int old_ruid = current->uid;
	
	if (ruid>0) {
		if ((current->euid==ruid) ||
                    (old_ruid == ruid) ||
		    suser())
			current->uid = ruid;
		else
			return(-EPERM);
	}
	if (euid>0) {
		if ((old_ruid == euid) ||
                    (current->euid == euid) ||
		    suser()) {
			current->euid = euid;
			current->suid = euid;
		} else {
			current->uid = old_ruid;
			return(-EPERM);
		}
	}
	return 0;
}

/*
 * setuid() is implemeneted like SysV w/ SAVED_IDS 
 * 
 * Note that SAVED_ID's is deficient in that a setuid root program
 * like sendmail, for example, cannot set its uid to be a normal 
 * user and then switch back, because if you're root, setuid() sets
 * the saved uid too.  If you don't like this, blame the bright people
 * in the POSIX commmittee and/or USG.  Note that the BSD-style setreuid()
 * will allow a root program to temporarily drop privileges and be able to
 * regain them by swapping the real and effective uid.  
 */
int sys_setuid(int uid)
{
	if (suser())
		current->uid = current->euid = current->suid = uid;
	else if ((uid == current->uid) || (uid == current->suid))
		current->euid = uid;
	else
		return -EPERM;
	return(0);
}

int sys_stime(int * tptr)
{
	if (!suser())
		return -EPERM;
	startup_time = get_fs_long((unsigned int *)tptr) - jiffies/HZ;
	jiffies_offset = 0;
	return 0;
}

int sys_times(struct tms * tbuf)
{
	if (tbuf) {
		verify_area(tbuf,sizeof(struct tms));
		put_fs_long(current->utime,(unsigned int *)&tbuf->tms_utime);
		put_fs_long(current->stime,(unsigned int *)&tbuf->tms_stime);
		put_fs_long(current->cutime,(unsigned int *)&tbuf->tms_cutime);
		put_fs_long(current->cstime,(unsigned int *)&tbuf->tms_cstime);
	}
	return jiffies;
}

int sys_brk(unsigned int end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 *
 * OK, I think I have the protection semantics right.... this is really
 * only important on a multi-user system anyway, to make sure one user
 * can't send a signal to a process owned by another.  -TYT, 12/12/91
 */
int sys_setpgid(int pid, int pgid)
{
	int i; 

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = current->pid;
	if (pgid < 0)
		return -EINVAL;
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && (task[i]->pid == pid) &&
		    ((task[i]->p_pptr == current) || 
		     (task[i] == current))) {
			if (task[i]->leader)
				return -EPERM;
			if ((task[i]->session != current->session) ||
			    ((pgid != pid) && 
			     (session_of_pgrp(pgid) != current->session)))
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

int sys_getpgrp(void)
{
	return current->pgrp;
}

int sys_setsid(void)
{
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}

/*
 * Supplementary group ID's
 */
int sys_getgroups(int gidsetsize, unsigned short *grouplist)
{
	int	i;

	if (gidsetsize)
		verify_area(grouplist, sizeof(unsigned short) * gidsetsize);

	for (i = 0; (i < NGROUPS) && (current->groups[i] != NOGROUP);
	     i++, grouplist++) {
		if (gidsetsize) {
			if (i >= gidsetsize)
				return -EINVAL;
			put_fs_word(current->groups[i], (short *) grouplist);
		}
	}
	return(i);
}

int sys_setgroups(int gidsetsize, unsigned short *grouplist)
{
	int	i;

	if (!suser())
		return -EPERM;
	if (gidsetsize > NGROUPS)
		return -EINVAL;
	for (i = 0; i < gidsetsize; i++, grouplist++) {
		current->groups[i] = get_fs_word((unsigned short *) grouplist);
	}
	if (i < NGROUPS)
		current->groups[i] = NOGROUP;
	return 0;
}

int in_group_p(unsigned short grp)
{
	int	i;

	if (grp == current->egid)
		return 1;

	for (i = 0; i < NGROUPS; i++) {
		if (current->groups[i] == NOGROUP)
			break;
		if (current->groups[i] == grp)
			return 1;
	}
	return 0;
}

int sys_uname(struct utsname * name)
{
	int i;

	if (!name) return -ERROR;
	verify_area(name,sizeof *name);
	for(i=0;i<sizeof *name;i++)
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return 0;
}

/*
 * Only sethostname; gethostname can be implemented by calling uname()
 */
int sys_sethostname(char *name, int len)
{
	int	i;
	
	if (!suser())
		return -EPERM;
	if (len > (MAXHOSTNAMELEN - 1))
		return -EINVAL;
	for (i=0; i < len; i++) {
		if ((thisname.nodename[i] = get_fs_byte(name+i)) == 0)
			break;
	}
	if (thisname.nodename[i]) {
		thisname.nodename[i>=(MAXHOSTNAMELEN-1)? (MAXHOSTNAMELEN-1) : i] = 0;
	}
	return 0;
}

int sys_getrlimit(int resource, struct rlimit *rlim)
{
	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	verify_area(rlim,sizeof *rlim);
	put_fs_long(current->rlim[resource].rlim_cur, 
		    (unsigned int *) rlim);
	put_fs_long(current->rlim[resource].rlim_max, 
		    ((unsigned int *) rlim)+1);
	return 0;	
}

int sys_setrlimit(int resource, struct rlimit *rlim)
{
	struct rlimit new, *old;

	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	old = current->rlim + resource;
	new.rlim_cur = get_fs_long((unsigned int *) rlim);
	new.rlim_max = get_fs_long(((unsigned int *) rlim)+1);
	if (((new.rlim_cur > old->rlim_max) ||
	     (new.rlim_max > old->rlim_max)) &&
	    !suser())
		return -EPERM;
	*old = new;
	return 0;
}

/*
 * It would make sense to put struct rusuage in the task_struct,
 * except that would make the task_struct be *really big*.  After
 * task_struct gets moved into malloc'ed memory, it would
 * make sense to do this.  It will make moving the rest of the information
 * a lot simpler!  (Which we're not doing right now because we're not
 * measuring them yet).
 */
int sys_getrusage(int who, struct rusage *ru)
{
	struct rusage r;
	unsigned int	*lp, *lpend, *dest;

	if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)
		return -EINVAL;
	verify_area(ru, sizeof *ru);
	memset((char *) &r, 0, sizeof(r));
	if (who == RUSAGE_SELF) {
		r.ru_utime.tv_sec = CT_TO_SECS(current->utime);
		r.ru_utime.tv_usec = CT_TO_USECS(current->utime);
		r.ru_stime.tv_sec = CT_TO_SECS(current->stime);
		r.ru_stime.tv_usec = CT_TO_USECS(current->stime);
	} else {
		r.ru_utime.tv_sec = CT_TO_SECS(current->cutime);
		r.ru_utime.tv_usec = CT_TO_USECS(current->cutime);
		r.ru_stime.tv_sec = CT_TO_SECS(current->cstime);
		r.ru_stime.tv_usec = CT_TO_USECS(current->cstime);
	}
	lp = (unsigned int *) &r;
	lpend = (unsigned int *) (&r+1);
	dest = (unsigned int *) ru;
	for (; lp < lpend; lp++, dest++) 
		put_fs_long(*lp, dest);
	return(0);
}

int sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (tv) {
		verify_area(tv, sizeof *tv);
		put_fs_long(startup_time + CT_TO_SECS(jiffies+jiffies_offset),
			    (unsigned int *) tv);
		put_fs_long(CT_TO_USECS(jiffies+jiffies_offset), 
			    ((unsigned int *) tv)+1);
	}
	if (tz) {
		verify_area(tz, sizeof *tz);
		put_fs_long(sys_tz.tz_minuteswest, (unsigned int *) tz);
		put_fs_long(sys_tz.tz_dsttime, ((unsigned int *) tz)+1);
	}
	return 0;
}

/*
 * The first time we set the timezone, we will warp the clock so that
 * it is ticking GMT time instead of local time.  Presumably, 
 * if someone is setting the timezone then we are running in an
 * environment where the programs understand about timezones.
 * This should be done at boot time in the /etc/rc script, as
 * soon as possible, so that the clock can be set right.  Otherwise,
 * various programs will get confused when the clock gets warped.
 */
int sys_settimeofday(struct timeval *tv, struct timezone *tz)
{
	static int	firsttime = 1;
	void 		adjust_clock();

	if (!suser())
		return -EPERM;
	if (tz) {
		sys_tz.tz_minuteswest = get_fs_long((unsigned int *) tz);
		sys_tz.tz_dsttime = get_fs_long(((unsigned int *) tz)+1);
		if (firsttime) {
			firsttime = 0;
			if (!tv)
				adjust_clock();
		}
	}
	if (tv) {
		int sec, usec;

		sec = get_fs_long((unsigned int *)tv);
		usec = get_fs_long(((unsigned int *)tv)+1);
	
		startup_time = sec - jiffies/HZ;
		jiffies_offset = usec * HZ / 1000000 - jiffies%HZ;
	}
	return 0;
}

/*
 * Adjust the time obtained from the CMOS to be GMT time instead of
 * local time.
 * 
 * This is ugly, but preferable to the alternatives.  Otherwise we
 * would either need to write a program to do it in /etc/rc (and risk
 * confusion if the program gets run more than once; it would also be 
 * hard to make the program warp the clock precisely n hours)  or
 * compile in the timezone information into the kernel.  Bad, bad....
 *
 * XXX Currently does not adjust for daylight savings time.  May not
 * need to do anything, depending on how smart (dumb?) the BIOS
 * is.  Blast it all.... the best thing to do not depend on the CMOS
 * clock at all, but get the time via NTP or timed if you're on a 
 * network....				- TYT, 1/1/92
 */
void adjust_clock()
{
	startup_time += sys_tz.tz_minuteswest*60;
}

int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}

char sys_write_simple_buf[1024];
extern int sys_write_simple(const char * user_buf, int count)
{
	tty_write(0, user_buf, count);	
	return 100;
}

extern int sys_print_sys_time(void)
{
	int year, moon, day, hour, minute, second;
	second = CMOS_READ(0);
	minute = CMOS_READ(2);
	hour = CMOS_READ(4);
	day = CMOS_READ(7);
	moon = CMOS_READ(8);
	year = CMOS_READ(9);

	BCD_TO_BIN(&second);
	BCD_TO_BIN(&minute);
	BCD_TO_BIN(&hour);
	BCD_TO_BIN(&day);
	BCD_TO_BIN(&moon);
	BCD_TO_BIN(&year);
	
	year = year + 2000;
	
	printk("\tthe current time is: ");
	printk("%d-%d-%d   %d:%d:%d\n", year, moon, day, hour, minute, second);
	
	return 0;
	
}

extern int sys_print_sys_info(void)
{
	printk("\tcurrent system name: ");
	printk(thisname.sysname);
	printk("\n\tcurrent node name: ");
	printk(thisname.nodename);
	printk("\n\tcurrent release: ");
	printk(thisname.release);
	printk("\n\tcurrent version number: ");
	printk(thisname.version);
	printk("\n\tcurrent machine: ");
	printk(thisname.machine);
	printk("\n");
	
	return 0;
}

extern int	timerisset(struct timeval * tvp)
{
	if (tvp->tv_sec)
		return 1;
	else if (tvp->tv_usec)
		return 1;
	else
		return 0;
}
extern int timercmp(struct timeval * tvp, struct timeval * uvp)
{
	if (tvp->tv_sec != uvp->tv_sec)
	{
		if (tvp->tv_sec > uvp->tv_sec)
			return 1;
		else if (tvp->tv_sec == uvp->tv_sec)
			return 0;
		else
			return -1;		
	}
	else
	{
		if (tvp->tv_usec > uvp->tv_usec)
			return 1;
		else if (tvp->tv_usec == uvp->tv_usec)
			return 0;
		else
			return -1;			
	}
}

extern void timerclear(struct timeval * tvp)
{
	tvp->tv_sec = tvp->tv_usec = 0;
}
		
extern void system_info_init(void)
{
	int i;
	sys_tz.tz_minuteswest = ((360 - 120) << 2);
	sys_tz.tz_dsttime = 0;
	for (i = 0; i < 64; i++)
	{
		thisname.sysname[i] = 0;
	}
	strcpy(thisname.sysname, "Linux");
	
	for (i = 0; i < MAXHOSTNAMELEN; i++)
	{
		thisname.nodename[i] = 0;
	}
	strcpy(thisname.nodename, "OS Explorer");

	for (i = 0; i < 64; i++)
	{
		thisname.release[i] = 0;
	}
	strcpy(thisname.release, "open source and study.");
	
	for (i = 0; i < 64; i++)
	{
		thisname.version[i] = 0;
	}
	strcpy(thisname.version, "0.12 study version.");
	
	for (i = 0; i < 64; i++)
	{
		thisname.machine[i] = 0;
	}
	strcpy(thisname.machine, "intel 386 CPU");

}








