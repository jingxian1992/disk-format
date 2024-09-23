#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * Defines for what uname() should return 
 */
#define UTS_SYSNAME "Linux"
#define UTS_NODENAME "Dumpling 1992"	/* set by sethostname() */
#define UTS_RELEASE "char exp03"		/* patchlevel */
#define UTS_VERSION "0.12"
#define UTS_MACHINE "Intel i386 CPU"	/* hardware type */

/* Don't touch these, unless you really know what your doing. */
#define DEF_INITSEG	0x9000
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	0x9020
#define DEF_SYSSIZE	0x3000

/***************************************
	由于在QEMU和virtualbox里面，硬盘first盘，它在BIOS读取参数时，总是出错，
  所以我将first盘设置为硬编码格式。在本文件所在的目录里面，我单独编写一个名为
  【硬盘编码.txt】的文本文件，用以记录几种尺寸的硬盘的编码方式。
********************************************/
#ifndef HD_PARAM_TABLE
#define HD_PARAM_TABLE 0x90080

#define HD0_CYL				40
#define HD0_HEAD			16
#define HD0_SECTOR			63
#define HD0_WPCOM			65535
#define HD0_CTL				0xC8
#define HD0_LZONE			40

#define HD1_CYL				40
#define HD1_HEAD			16
#define HD1_SECTOR			63
#define HD1_WPCOM			65535
#define HD1_CTL				0xC8
#define HD1_LZONE			40

#define FIRST_PART_M	2
#define FIRST_PART_BLOCKNO (FIRST_PART_M * 1024)
#define FIRST_PART_SECTNO (FIRST_PART_M * 2048)

#define DISK_BUFFER		0x93000

#endif

/*
 * The root-device is no longer hard-coded. You can change the default
 * root-device by changing the line ROOT_DEV = XXX in boot/bootsect.s
 */

/*
 * The keyboard is now defined in kernel/chr_dev/keyboard.S
 */

/*
 * Normally, Linux can get the drive parameters from the BIOS at
 * startup, but if this for some unfathomable reason fails, you'd
 * be left stranded. For this case, you can define HD_TYPE, which
 * contains all necessary info on your harddisk.
 *
 * The HD_TYPE macro should look like this:
 *
 * #define HD_TYPE { head, sect, cyl, wpcom, lzone, ctl}
 *
 * In case of two harddisks, the info should be sepatated by
 * commas:
 *
 * #define HD_TYPE { h,s,c,wpcom,lz,ctl },{ h,s,c,wpcom,lz,ctl }
 */
/*
 This is an example, two drives, first is type 2, second is type 3:

#define HD_TYPE { 4,17,615,300,615,8 }, { 6,17,615,300,615,0 }

 NOTE: ctl is 0 for all drives with heads<=8, and ctl=8 for drives
 with more than 8 heads.

 If you want the BIOS to tell what kind of drive you have, just
 leave HD_TYPE undefined. This is the normal thing to do.
*/

#endif
