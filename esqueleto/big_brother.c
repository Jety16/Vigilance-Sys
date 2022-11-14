#include "big_brother.h"

#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "fat_table.h"
#include "fat_util.h"
#include "fat_volume.h"
#include "unistd.h"
// GList *fat_file_read_children(fat_file dir) {
//  u32 bytes_per_cluster = 0, cur_cluster = 0;
//  off_t cur_offset = 0;
//  u8 *buf = NULL;
//  GList *entry_list = NULL;
//
//  DEBUG("Reading children of \"%s\"", dir->filepath);
//  bytes_per_cluster = fat_table_bytes_per_cluster(dir->table);
//  cur_cluster = dir->start_cluster;
//  if (!fat_table_is_valid_cluster_number(dir->table, cur_cluster)) {
//    fat_error("Cluster number %u is invalid", cur_cluster);
//    errno = EIO;
//    return NULL;
//  }
//  cur_offset = fat_table_cluster_offset(dir->table, cur_cluster);
//
//  buf = alloca(bytes_per_cluster);
//  while (fat_table_cluster_is_bad_sector(cluster)) {
//    fat_dir_entry end_ptr;
//    end_ptr = (fat_dir_entry)(buf + bytes_per_cluster) - 1;
//    if (full_pread(dir->table->fd, buf, bytes_per_cluster, cur_offset) !=
//        bytes_per_cluster) {
//      errno = EIO;
//      return NULL;
//    }
//    read_cluster_dir_entries(buf, end_ptr, dir, &entry_list);
//    cur_cluster = fat_table_get_next_cluster(dir->table, cur_cluster);
//    cur_offset = fat_table_cluster_offset(dir->table, cur_cluster);
//  }
//  dir->children_read = 1;
//  return entry_list;
//}

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
  while (
      le32_to_cpu(((const le32 *)vol->table->fat_map)[bb_dir_start_cluster]) !=
      FAT_CLUSTER_BAD_SECTOR) {
    bb_dir_start_cluster++;
  }
  if (!fat_table_is_valid_cluster_number(vol->table, bb_dir_start_cluster)) {
    fat_error("There was a problem fetching for a free cluster");
    // Forzamos un EOC para catchearlo en la otra func
    bb_dir_start_cluster = FAT_CLUSTER_END_OF_CHAIN_MAX;
  }
  DEBUG("next bad cluster = %u", bb_dir_start_cluster);
  return bb_dir_start_cluster;
}

/* Creates the /bb directory as an orphan and adds it to the file tree as
 * child of root dir.
 */
int bb_create_new_orphan_dir(fat_volume vol) {
  errno = 0;

  // char *bb_path = "/bb";
  fat_file root_file, bb_file /* , child_file */;
  fat_tree_node root_node;
  bool bb_exists = false, find_cluster = false;
  u32 bb_cluster = search_bb_orphan_dir_cluster(vol, 2);
  off_t offset;
  u8 *buf = NULL;
  fat_dir_entry bb_dentry;
  /*  Busco el archivo */
  while (!find_cluster && fat_table_cluster_is_EOC(bb_cluster)) {
    DEBUG("next bad cluster = %u", bb_cluster);
    offset = fat_table_cluster_offset(vol->table, bb_cluster);
    // bb_dentry =
    //    (fat_dir_entry)fat_table_seek_cluster(vol->table, bb_cluster, offset);
    fat_error("HOLAAAAA");
    bb_dentry = (fat_dir_entry)pread(vol->table->fd, buf, 32, offset);
    find_cluster = bb_is_log_file_dentry(bb_dentry);
    bb_exists = true;
  }

  /* Inserto al arbol */
  root_node = fat_tree_node_search(vol->file_tree, dirname(strdup("/")));
  root_file = fat_tree_get_file(root_node);
  GList *children_list = fat_file_read_children(root_file);

  for (GList *l = children_list; l != NULL; l = l->next) {
    bb_file = (fat_file)l->data;
    fat_error("bb_file = %s \n", (char *)bb_file->name);
    vol->file_tree = fat_tree_insert(vol->file_tree, root_node, bb_file);
  }
  /* Creo el bb */
  if (!bb_exists) {
    int algo = bb_init_log_dir(bb_cluster, vol);
    if (algo < -1000) {  // ver esta guarda
      return 1;
    }
    // fat_error("HOLAAAA");
    // fat_table_set_next_cluster(vol->table, bb_cluster,
    // FAT_CLUSTER_BAD_SECTOR); fat_error("HOLAAAA");
    vol->file_tree = fat_tree_insert(vol->file_tree, root_node, bb_file);
    bb_exists = true;
  }
  return 0;
}

int bb_init_log_dir(u32 start_cluster, fat_volume vol) {
  errno = 0;

  fat_tree_node root_node = NULL;

  // Create a new file from scratch, instead of using a direntry like normally
  // done.

  fat_file loaded_bb_dir =
      fat_file_init_orphan_dir(BB_DIRNAME, vol->table, start_cluster);
  fat_error("AFTER FILE INIT ORPHAN DIR\n\n");

  // Add directory to file tree. It's entries will be like any other dir.
  root_node = fat_tree_node_search(vol->file_tree, "/");
  vol->file_tree = fat_tree_insert(vol->file_tree, root_node, loaded_bb_dir);

  return -errno;
}
