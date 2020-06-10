#ifndef _TMPFS_H
#define _TMPFS_H

#define EOF (-1)

#define TMP_FILE_SIZE 512
struct tmpfs_node{
	char buffer[TMP_FILE_SIZE];
};

struct vnode_operations* tmpfs_v_ops;
struct file_operations* tmpfs_f_ops;

void set_tmpfs_vnode(struct vnode* vnode);
int setup_mount_tmpfs(struct filesystem* fs, struct mount* mount);
int lookup_tmpfs(struct dentry* dir, struct vnode** target,const char* component_name);
int create_tmpfs(struct dentry* dir, struct vnode** target,const char* component_name);
int write_tmpfs(struct file* file, const void* buf, size_t len);
int read_tmpfs(struct file* file, void* buf, size_t len);
#endif