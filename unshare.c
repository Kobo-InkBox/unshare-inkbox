/* 
 * unshare: Implementation of the GNU `unshare' command with the clone() system call for the InkBox OS project
 *
 * Copyright (C) 2022 Nicolas Mailloux <nicolecrivain@gmail.com>
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static char child_stack[1048576];

int arguments_count;
int arguments_count_getopt;
int mount_proc = 0;
char ** arguments_list;
char * proc_mountpoint;

void print_help() {
	printf("%s: Run specified process in specified namespace(s).\nUsage: %s [arguments] -- [[<absolute path to executable>] [<executable arguments>]]\nArguments:\n\t-m:\t\t   mounts namespace\n\t-u:\t\t   UTS namespace\n\t-i:\t\t   IPC namespace\n\t-n:\t\t   network namespace\n\t-p:\t\t   PID namespace\n\t-U:\t\t   user namespace\n\t-P [<mountpoint>]: mount proc filesystem at mountpoint before executing process (chroot only)\n\t--help:\t\t   Displays this help\n", arguments_list[0], arguments_list[0]);
}

// https://stackoverflow.com/a/4553076
int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int child_fn() {
	if(mount_proc == 1) {
		if(0 != mount("proc", proc_mountpoint, "proc", 0, "")) {
			fprintf(stderr, "ERROR: Failed to mount proc filesystem at '%s' because of the following error: %s.\n", proc_mountpoint, strerror(errno));
		}
	}
	char ** arguments_list_clone = arguments_list + arguments_count_getopt;
	execv(arguments_list_clone[0], arguments_list_clone);
}

int main(int argc, char * argv[]) {
	arguments_count = argc;
	arguments_list = argv;

	if(arguments_count < 2) {
		fprintf(stderr, "ERROR: you must specify an executable's path. Aborting!\n\n");
		print_help();
		return 1;
	} else if(strcmp(arguments_list[1], "--help") == 0) {
		print_help();
		return 0;
	} else {
		int flags, opt;
		flags = 0;
		while((opt = getopt(argc, argv, "imnpuUP:")) != -1) {
			switch (opt) {
				case 'i': flags |= CLONE_NEWIPC;				break;
				case 'm': flags |= CLONE_NEWNS;					break;
				case 'n': flags |= CLONE_NEWNET;				break;
				case 'p': flags |= CLONE_NEWPID;				break;
				case 'u': flags |= CLONE_NEWUTS;				break;
				case 'U': flags |= CLONE_NEWUSER;				break;
				case 'P': proc_mountpoint = optarg; mount_proc = 1;		break;
				default:  break;
			}
		}
		flags |= SIGCHLD;
		arguments_count_getopt = optind;

		char ** arguments_list_clone = arguments_list + arguments_count_getopt;
		if(is_regular_file(arguments_list_clone[0]) == 0) {
			fprintf(stderr, "ERROR: cannot find the specified executable '%s'. Aborting!\n\n", arguments_list_clone[0]);
			print_help();
			return 1;
		} else {
			pid_t child_pid = clone(child_fn, child_stack+1048576, flags, NULL);
			if(child_pid < 0) {
				fprintf(stderr, "ERROR: %s failed to run '%s' successfully because of the following error: %s. Aborting!\n", arguments_list[0], arguments_list_clone[0], strerror(errno));
				return 1;
			} else {
				waitpid(child_pid, NULL, 0);
				if(mount_proc == 1) {
					if(0 != umount(proc_mountpoint)) {
						fprintf(stderr, "ERROR: Failed to unmount proc filesystem at '%s' because of the following error: %s.\n", proc_mountpoint, strerror(errno));
					}
				}
			}
		}
		return 0;
	}
}
