#include "big_brother.h"

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

/* Searches for a cluster that could correspond to the bb directory and returns
 * its index. If the cluster is not found, returns 0.
 */
u32 search_bb_orphan_dir_cluster(fat_volume vol, int offset) {
  u32 bb_dir_start_cluster = offset; /* First two clusters are reserved */
  fat_error("LOL EN QUE MOMENTO LLAME LA FUNCION?");
  while (
      le32_to_cpu(((const le32 *)vol->table->fat_map)[bb_dir_start_cluster]) !=
      FAT_CLUSTER_BAD_SECTOR) {
    bb_dir_start_cluster++;
  }
  if (!fat_table_is_valid_cluster_number(vol->table, bb_dir_start_cluster)) {
    fat_error("There was a problem fetching for a free cluster");
    bb_dir_start_cluster = FAT_CLUSTER_END_OF_CHAIN_MAX;
  }
  DEBUG("next bad cluster = %u", bb_dir_start_cluster);
  return bb_dir_start_cluster;
}

/* Creates the /bb directory as an orphan and adds it to the file tree as
 * child of root dir.
 */
static int bb_create_new_orphan_dir(fat_volume vol) {
  errno = 0;
  // ****MOST IMPORTANT PART, DO NOT SAVE DIR ENTRY TO PARENT ****

  char *fs_path = "/bb";
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
      fs_exists = true;
    }
  }

  root_file->start_cluster = FAT_CLUSTER_BAD_SECTOR;
}

int bb_init_log_dir(u32 start_cluster) {
  errno = 0;
  fat_volume vol = NULL;
  fat_tree_node root_node = NULL;
  vol = get_fat_volume();

  // Create a new file from scratch, instead of using a direntry like normally
  // done.
  fat_file loaded_bb_dir =
      fat_file_init_orphan_dir(BB_DIRNAME, vol->table, start_cluster);
  // Add directory to file tree. It's entries will be like any other dir.
  root_node = fat_tree_node_search(vol->file_tree, "/");
  vol->file_tree = fat_tree_insert(vol->file_tree, root_node, loaded_bb_dir);

  return -errno;
}
