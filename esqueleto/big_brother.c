#include "big_brother.h"

#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "fat_table.h"
#include "fat_util.h"
#include "fat_volume.h"

int bb_is_log_file_dentry(fat_dir_entry dir_entry) {
  return strncmp(LOG_FILE_BASENAME, (char *)(dir_entry->base_name), 3) == 0 &&
         strncmp(LOG_FILE_EXTENSION, (char *)(dir_entry->extension), 3) == 0;
}

int bb_is_log_filepath(char *filepath) {
  return strncmp(BB_LOG_FILE, filepath, 8) == 0;
}

int bb_is_log_dirpath(char *filepath) {
  return strncmp(BB_DIRNAME, filepath, 15) == 0;
}

static void create_fs_file(fat_volume vol) {
  char *fs_path = "/bb/fs.log";
  fat_file bb_file, fs_file, child_file;
  fat_tree_node bb_node;
  bool fs_exists = false;

  bb_node = fat_tree_node_search(vol->file_tree, dirname(strdup(fs_path)));

  bb_file = fat_tree_get_file(bb_node);

  GList *children_list = fat_file_read_children(bb_file);
  for (GList *l = children_list; l != NULL; l = l->next) {
    child_file = (fat_file)l->data;
    vol->file_tree = fat_tree_insert(vol->file_tree, bb_node, child_file);
    DEBUG("child_file: %s", child_file->name);

    if (fs_exists == false && !strcmp(child_file->name, "fs.log")) {
      fs_exists = true;
    }
  }

  if (!fs_exists) {
    DEBUG("fs_file doesn't exist, creating");
    fs_file = fat_file_init(vol->table, false, strdup(fs_path));
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, bb_node, fs_file);
    // Write dentry in parent cluster
    fat_file_dentry_add_child(bb_file, fs_file);
  }
}

u32 search_bb_orphan_dir_cluster(fat_volume vol) {
  /* Searches for a cluster that could correspond to the bb directory and returns
   * its index. If the cluster is not found, returns 0.
   */
  u32 bb_dir_start_cluster = 2;
  bool find_it = false;

  while (!find_it && bb_dir_start_cluster < min(10000, vol->table->num_data_clusters)){
    if ((((const le32 *)vol->table->fat_map)[bb_dir_start_cluster]) ==
        FAT_CLUSTER_BAD_SECTOR) {
      u32 bytes_per_cluster = fat_table_bytes_per_cluster(vol->table);
      off_t offset = fat_table_cluster_offset(vol->table, bb_dir_start_cluster);
      u8 *buf = alloca(bytes_per_cluster);

      // Check if this cluster is the bb dir cluster
      full_pread(vol->table->fd, buf, bytes_per_cluster, offset);
      if (bb_is_log_file_dentry((fat_dir_entry)buf)) {
        find_it = true;
      }
    }
    if (find_it) {
      DEBUG("bad cluster for bb dir = %u", bb_dir_start_cluster);
      return bb_dir_start_cluster;
    } 
    bb_dir_start_cluster++;
  }

  if (!find_it){
      DEBUG("Can't find a cluster for the bb, setting next_free cluster as a BAD_SECTOR");
      bb_dir_start_cluster = fat_table_get_next_free_cluster(vol->table);
      fat_table_set_next_cluster(vol->table, bb_dir_start_cluster, FAT_CLUSTER_BAD_SECTOR);
      DEBUG("bad cluster for bb dir = %u", bb_dir_start_cluster);
  }
  return bb_dir_start_cluster;
}

int bb_init_log_dir(u32 start_cluster, fat_volume vol) {
  // Create a new file from scratch, instead of using a direntry like normally
  // done.
  errno = 0;
  fat_tree_node root_node = NULL;

  fat_file loaded_bb_dir =
      fat_file_init_orphan_dir(BB_DIRNAME, vol->table, start_cluster);

  // Add directory to file tree. It's entries will be like any other dir.
  root_node = fat_tree_node_search(vol->file_tree, "/");
  vol->file_tree = fat_tree_insert(vol->file_tree, root_node, loaded_bb_dir);

  return -errno;
}


/* Creates the /bb directory as an orphan and adds it to the file tree as
 * child of root dir.
 */
int bb_create_new_orphan_dir(fat_volume vol) {
  errno = 0;
  u32 bb_cluster = search_bb_orphan_dir_cluster(vol);

  bb_init_log_dir(bb_cluster, vol);

  create_fs_file(vol);

  return 0;
}