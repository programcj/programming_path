#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <mntent.h>
#include <sys/mount.h>

int mount_print() {
#define bb_path_mtab_file "/proc/mounts"
	FILE *mountTable = setmntent(bb_path_mtab_file, "r");
	struct mntent mtpair, *mtcur = &mtpair;
	char getmntent_buf[300];
	if (!mountTable)
		return -1;

	while (getmntent_r(mountTable, &mtpair, getmntent_buf, 300)) {
		printf("%s on %s type %s (%s)\n", mtcur->mnt_fsname, mtcur->mnt_dir,
				mtcur->mnt_type, mtcur->mnt_opts);
	}
	endmntent(mountTable);
	return 0;
}

int main(int argc, char const *argv[])
{
    /* code */
    mount_print();
    mount("/dev/sdcard1", "/tmp/sdcard", "ntfs", MS_MGC_VAL | MS_RDONLY | MS_NOSUID,"");
    umount2("/tmp/sdcard", MNT_FORCE);
    return 0;
}
