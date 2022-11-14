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
    char *bb_path = "/bb/fs.log";
    char *fs_path = "/fs.log";
    fat_file root_file, fs_file, child_file;
    fat_tree_node root_node;
    bool fs_exists = false;

    root_node = fat_tree_node_search(vol->file_tree, dirname(strdup(bb_path)));
    root_file = fat_tree_get_file(root_node);
    fat_error("CASI\n\n");
    GList *children_list = fat_file_read_children(root_file);
    for (GList *l = children_list; l != NULL; l = l->next) {
        child_file = (fat_file)l->data;
        vol->file_tree = fat_tree_insert(vol->file_tree, root_node, child_file);
        fat_error("fs exist: %d. child_file: %s", fs_exists, child_file->name);

        if (fs_exists == false && !strcmp(child_file->name, fs_path)) {
            fs_exists = true;
        }
    }
    if (!fs_exists) {
        fs_file = fat_file_init(vol->table, false, strdup(fs_path));
        // insert to directory tree representation
        vol->file_tree = fat_tree_insert(vol->file_tree, root_node, fs_file);
        // Write dentry in parent cluster
        fat_file_dentry_add_child(root_file, fs_file);
    }
}
/* Searches for a cluster that could correspond to the bb directory and returns
 * its index. If the cluster is not found, returns 0.
 */
u32 search_bb_orphan_dir_cluster(fat_volume vol, int offset) {
    u32 bb_dir_start_cluster = offset; /* First two clusters are reserved */
    fat_error("LOL EN QUE MOMENTO LLAME LA FUNCION?");
    while (le32_to_cpu(
               ((const le32 *)vol->table->fat_map)[bb_dir_start_cluster]) !=
           FAT_CLUSTER_BAD_SECTOR) {
        if ((((const le32 *)vol->table->fat_map)[bb_dir_start_cluster]) ==
            FAT_CLUSTER_BAD_SECTOR) {
            u32 bytes_per_cluster = fat_table_bytes_per_cluster(vol->table);
            off_t offset =
                fat_table_cluster_offset(vol->table, bb_dir_start_cluster);
            u8 *buf = alloca(bytes_per_cluster);
            full_pread(vol->table->fd, buf, bytes_per_cluster, offset);
            if (bb_is_log_file_dentry((fat_dir_entry)buf)) {
                break;
            }
        }
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
    // ****MOST IMPORTANT PART, DO NOT SAVE DIR ENTRY TO PARENT ****

    u32 bb_cluster = search_bb_orphan_dir_cluster(vol, 2);

    fat_error("HOLAAAA\n\n");
    bb_init_log_dir(bb_cluster, vol);
    create_fs_file(vol);
    fat_error("TERMINEEEEE\n\n");
    //
    // vol->file_tree = fat_tree_insert(vol->file_tree, root_node, bb_file);
    //// Write dentry in parent cluster
    //// fat_file_dentry_add_child(root_file, bb_file);
    // bb_exists = true;

    return 0;
    // root_file->start_cluster = FAT_CLUSTER_BAD_SECTOR;
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
