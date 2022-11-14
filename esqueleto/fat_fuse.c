/*
 * fat_fuse.c
 *
 * main() for a program to mount a FAT filesystem using FUSE
 */

#include <alloca.h>
#include <errno.h>
#include <getopt.h>
#include <gmodule.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "big_brother.h"
#include "fat_fuse_ops.h"
#include "fat_volume.h"

static void usage() {
  const char *usage_str = "Usage: fat-fuse [-f] [-d] [-r] VOLUME MOUNTPOINT\n";
  fputs(usage_str, stdout);
}

static void usage_short() {
  const char *usage_str = "Usage: fat-fuse [-f] [-d] [-r] VOLUME MOUNTPOINT\n";
  fputs(usage_str, stderr);
}

static const char *shortopts = "dfhr";
static const struct option longopts[] = {
    {"debug", no_argument, NULL, 'd'},
    {"foreground", no_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"readonly", no_argument, NULL, 'r'},
    {NULL, 0, NULL, 0},
};

/* Filesystem operations for FUSE.  Only some of the possible operations are
 * implemented (the rest stay as NULL pointers and are interpreted as not
 * implemented by FUSE). */
struct fuse_operations fat_fuse_operations = {
    .fgetattr = fat_fuse_fgetattr,
    .getattr = fat_fuse_getattr,
    .open = fat_fuse_open,
    .opendir = fat_fuse_opendir,
    .mkdir = fat_fuse_mkdir,
    .mknod = fat_fuse_mknod,
    .read = fat_fuse_read,
    .readdir = fat_fuse_readdir,
    .release = fat_fuse_release,
    .releasedir = fat_fuse_releasedir,
    .utime = fat_fuse_utime,
    .truncate = fat_fuse_truncate,
    .write = fat_fuse_write,
    .unlink = fat_fuse_unlink,
    .rmdir = fat_fuse_rmdir,
};

void create_fs_file(fat_volume vol) {
  char *fs_path = "/fs.log";
  fat_file root_file, fs_file, child_file;
  fat_tree_node root_node;
  bool fs_exists = false;

  root_node = fat_tree_node_search(vol->file_tree, dirname(strdup(fs_path)));
  root_file = fat_tree_get_file(root_node);
  GList *children_list = fat_file_read_children(root_file);

  for (GList *l = children_list; l != NULL; l = l->next) {
    child_file = (fat_file)l->data;
    vol->file_tree = fat_tree_insert(vol->file_tree, root_node, child_file);

    if (fs_exists == false && !strcmp(child_file->name, "fs.log")) {
      fs_exists = true;
    }
    if (!fs_exists && l->next == NULL) {
      fs_file = fat_file_init(vol->table, false, strdup(fs_path));
      // insert to directory tree representation
      vol->file_tree = fat_tree_insert(vol->file_tree, root_node, fs_file);
      // Write dentry in parent cluster
      fat_file_dentry_add_child(root_file, fs_file);
    }
  }
}
int main(int argc, char **argv) {
  char *volume;
  char *mountpoint;
  int c;
  fat_volume vol;
  char *fuse_argv[50];
  int fuse_argc;
  int fuse_status;
  int ret;
  int mount_flags = FAT_MOUNT_FLAG_READWRITE;
  int debug = 0, foreground = 0;

  while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (c) {
      case 'd':
        debug = 1;
        break;
      case 'f':
        foreground = 1;
        break;
      case 'h':
        usage();
        return 0;
      case 'r':
        mount_flags = FAT_MOUNT_FLAG_READONLY;
        break;
      default:
        usage_short();
        return 2;
    }
  }

  argc -= optind;
  argv += optind;
  if (argc != 2) {
    usage_short();
    return 2;
  }

  volume = argv[0];
  mountpoint = argv[1];
  fuse_argc = 0;
  fuse_argv[fuse_argc] = "fat-fuse";
  fuse_argc++;

  fuse_argv[fuse_argc] = "-s";  // Single-threaded
  fuse_argc++;

  if (mount_flags & FAT_MOUNT_FLAG_READONLY) {
    DEBUG("Read only mode");
    fuse_argv[fuse_argc] = "-o";
    fuse_argc++;
    fuse_argv[fuse_argc] = "ro";
    fuse_argc++;
  }
  if (foreground) {
    fuse_argv[fuse_argc] = "-f";  // Run in foreground
    fuse_argc++;
  }
  if (debug) {
    fuse_argv[fuse_argc] = "-d";  // Debug mode
    fuse_argc++;
  }

  fuse_argv[fuse_argc] = mountpoint;
  fuse_argc++;
  fuse_argv[fuse_argc] = NULL;

  // Mount the FAT volume with mount flags
  vol = fat_volume_mount(volume, mount_flags);
  if (!vol) {
    fat_error("Failed to mount FAT volume \"%s\": %m", volume);
    return 1;
  }
  // create_fs_file(vol);
  // We pass the value 2 bcs first and second cluster are reserved

  bb_create_new_orphan_dir(vol);
  fat_tree_node bb_node = fat_tree_node_search(vol->file_tree, "/bb");
  fat_file bb_file = fat_tree_get_file(bb_node);
  fat_file fs_file = fat_file_init(vol->table, false, strdup("/bb/fs.log"));
  // insert to directory tree representation
  vol->file_tree = fat_tree_insert(vol->file_tree, bb_node, fs_file);
  // Write dentry in parent cluster
  fat_file_dentry_add_child(bb_file, fs_file);

  // Call fuse_main() to pass control to FUSE.  This will daemonize the
  // process, causing it to detach from the terminal.  fat_volume_unmount()
  // will not be called until the filesystem is unmounted and fuse_main()
  // returns in the daemon process.
  fuse_status = fuse_main(fuse_argc, fuse_argv, &fat_fuse_operations, vol);
  ret = fat_volume_unmount(vol);
  if (ret)
    fat_error("failed to unmount FAT volume \"%s\": %m", volume);
  else
    ret = fuse_status;
  return ret;
}
