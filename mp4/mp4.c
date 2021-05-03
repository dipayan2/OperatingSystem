#define pr_fmt(fmt) "cs423_mp4: " fmt

#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/binfmts.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/xattr.h>
#include <linux/capability.h>
#include <linux/unistd.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/ext2_fs.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/kd.h>
#include <linux/stat.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/quota.h>
#include <linux/parser.h>
#include <linux/nfs_mount.h>
#include <linux/hugetlb.h>
#include <linux/personality.h>
#include <linux/sysctl.h>
#include <linux/audit.h>
#include <linux/string.h>
#include "mp4_given.h"

#define TARGETLEN 50
/**
 * get_inode_sid - Get the inode mp4 security label id
 *
 * @inode: the input inode
 *
 * @return the inode's security id if found.
 *
 */
static int get_inode_sid(struct inode *inode)
{
	
	int rc = -1;
	struct dentry *dentry;
	char *attrVal = NULL;
	int len = 0;
	int sid = 0;

	// Check if inode exists
	if (!inode || !inode->i_op || !inode->i_op->getxattr) {
		pr_info("No inode security found\n");
		return 0;
	}
	dentry = d_find_alias(inode);
	if (!dentry){
		pr_err("Denrty no found\n");
		return 0;
	}
	len = TARGETLEN;
	attrVal = (char*)kmalloc(sizeof(char)*len, GFP_KERNEL);
	if (!attrVal){
		dput(dentry);
		return 0;
	}

	rc = inode->i_op->getxattr(dentry, "security.mp4", attrVal, len*sizeof(char));
	if(rc == -ERANGE){
		/* Needs a larger buffer*/
		/**
		 * Will handle later
		 * **/
		dput(dentry);
		kfree(attrVal);
		return 0;
	}
	dput(dentry);
	if(rc<0){
		kfree(attrVal);
		attrVal = NULL;
		return 0;
	}
	// Actual result
	sid = __cred_ctx_to_sid(attrVal);
	kfree(attrVal);
	return sid;
}


/**
 * mp4_cred_alloc_blank - Allocate a blank mp4 security label
 *
 * @cred: the new credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_alloc_blank(struct cred *cred, gfp_t gfp)
{
	

	struct mp4_security *tsec;

	tsec = (struct mp4_security *) kzalloc(sizeof(struct mp4_security), gfp);
	
	if (!tsec){
		pr_err("No memroy is cred_alloc_blank");
		return -ENOMEM;
	}
	tsec->mp4_flags = MP4_NO_ACCESS;
	cred->security = tsec;
	return 0;
}

/**
 * mp4_bprm_set_creds - Set the credentials for a new task
 *
 * @bprm: The linux binary preparation structure
 *
 * returns 0 on success.
 */
static int mp4_bprm_set_creds(struct linux_binprm *bprm)
{
	// Get inode of the creating process
	int rc = 0;	
	int tmp;
	int newSid;
	struct mp4_security *curr_sec;
	struct cred *curr_cred;
	struct inode *inode = file_inode(bprm->file);

	// Get the current cred
	curr_cred = current_cred();

	if (!curr_cred){
		pr_err("No cred found, exiting!!");
		return 1;
	}
	// Get the inode cred
	if (!inode){
		pr_err("No inode found\n");
		return 1;
	}

	newSid = get_inode_sid(inode); // this is what we need to do
	if (newSid == MP4_TARGET_SID){
		// Do things
		curr_sec = curr_cred->security;
		if (curr_sec == NULL){
			// Set the security
			tmp = mp4_cred_alloc_blank(curr_cred,GFP_KERNEL);
			
		}
		curr_sec = curr_cred->security;
		curr_sec->mp4_flags = MP4_TARGET_SID;
		
	}
	else{
		// Don't do anything
		return 0;
	}


	
	return 0;
}




/**
 * mp4_cred_free - Free a created security label
 *
 * @cred: the credentials struct
 *
 */
static void mp4_cred_free(struct cred *cred)
{	
	//  Check if the cred security is not null
	struct mp4_security *tsec;
	tsec = cred->security;
	if(tsec != NULL){
		//pr_info("Memory freed mp4_cred_free");
		kfree(tsec);
	}
	cred->security = NULL;
	return;
}

/**
 * mp4_cred_prepare - Prepare new credentials for modification
 *
 * @new: the new credentials
 * @old: the old credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_prepare(struct cred *new, const struct cred *old,
			    gfp_t gfp)
{
	struct mp4_security *tsec;
	const struct mp4_security* old_sec;
	if(!old){
		pr_err("Old cred is NULL\n");
		return mp4_cred_alloc_blank(new,gfp);
	}
	old_sec = old->security;
	if (!old_sec){
		pr_err("Old cred  security is NULL\n");
		return mp4_cred_alloc_blank(new,gfp);
	}
	tsec = kmemdup(old_sec, sizeof(struct mp4_security), gfp);
	if (!tsec){
		return -ENOMEM;
	}
	new->security = tsec;
	return 0;
}

/**
 * mp4_inode_init_security - Set the security attribute of a newly created inode
 *
 * @inode: the newly created inode
 * @dir: the containing directory
 * @qstr: unused
 * @name: where to put the attribute name
 * @value: where to put the attribute value
 * @len: where to put the length of the attribute
 *
 * returns 0 if all goes well, -ENOMEM if no memory, -EOPNOTSUPP to skip
 *
 */
static int mp4_inode_init_security(struct inode *inode, struct inode *dir,
				   const struct qstr *qstr,
				   const char **name, void **value, size_t *len)
{
	pr_info("Init  security\n");
	struct mp4_security *curr_sec;
	int proc_sid;
	struct cred *curr_cred;
	int new_sid;
	struct mp4_security *isec;// = inode->i_security;
	char *namep = NULL;
	char *context;

	if(!inode){
		pr_err("Empty Inode");
		return 0;
	}
	if (!dir){
		pr_err("Empty Dir");
		return 0;
	}


	// Get the sid of the process
	curr_cred = current_cred();
	if(!curr_cred){
		pr_info("No cred found");
		return 0;
	}
	curr_sec = curr_cred->security;
	if(!curr_sec){
		pr_info("No security module for current proc found");
		return 0;
	}
	if (curr_sec->mp4_flags == MP4_TARGET_SID){
		//Do things 
		// set I node attribute to read-write
		new_sid = MP4_READ_WRITE;
		//isec = inode->i_security;
		//isec->mp4_flags = MP4_READ_WRITE;
		if(name){
			namep = kstrdup(XATTR_MP4_SUFFIX, GFP_KERNEL);
			if(!namep){
				return -ENOMEM;
			}
			*name = namep;
		}
		if (value && len){
			context = kstrdup("read-write", GFP_KERNEL);
			if (!context){
				kfree(namep);
				return -ENOMEM;
			}
			*value = context;
			*len = strlen(context);
		}


	}
	return 0;
}

/**
 * mp4_has_permission - Check if subject has permission to an object
 *
 * @ssid: the subject's security id
 * @osid: the object's security id
 * @mask: the operation mask
 *
 * returns 0 is access granter, -EACCES otherwise
 *
 */
static int mp4_has_permission(int ssid, int osid, int mask)
{

	// check if the mask should allow the object
	//pr_info("Has  security\n");
	if(ssid== MP4_TARGET_SID){
		// Do things
		if(osid==MP4_NO_ACCESS){
			//pr_info("Access denied");
			return -EACCES;
		}
		if(osid==MP4_READ_OBJ && (mask&(MAY_READ))){
			return 0;
		}
		if(osid == MP4_WRITE_OBJ && (mask&(MAY_WRITE|MAY_APPEND))){
			return 0;
		}
		if(osid == MP4_READ_WRITE && (mask&(MAY_READ|MAY_WRITE|MAY_APPEND))){
			return 0;
		}
		if(osid == MP4_EXEC_OBJ && (mask&(MAY_EXEC))){
			return 0;
		}
		if(osid == MP4_RW_DIR && (mask&(MAY_READ|MAY_WRITE|MAY_APPEND))){
			return 0;
		}
		if(osid == MP4_READ_DIR && (mask&(MAY_READ|MAY_WRITE|MAY_APPEND|MAY_EXEC))){
			return 0;
		}
		if(osid == MP4_TARGET_SID && (mask&(MAY_READ|MAY_WRITE|MAY_APPEND|MAY_EXEC))){
			return 0;
		}
		else{
			//pr_info("Access Denied\n");
			return -EACCES;
		}
	}
	else{
		return 0;
	}
	return 0;
}

/**
 * mp4_inode_permission - Check permission for an inode being opened
 *
 * @inode: the inode in question
 * @mask: the access requested
 *
 * This is the important access check hook
 *
 * returns 0 if access is granted, -EACCES otherwise
 *
 */
static int mp4_inode_permission(struct inode *inode, int mask)
{
	// pr_info("Perm  security\n");
	const struct cred *curr_cred;
	struct mp4_security *tsec;
	struct mp4_security *curr_sec;
	struct dentry *dentry;
	int ssid, osid;
	char *buffer, *path;
	path = NULL;
	if(!inode){
		return -EACCES;
	}
	curr_cred = current_cred();
	if(!curr_cred){
		return -EACCES;
	}
	curr_sec= curr_cred->security;
	if (!curr_sec){
		return -EACCES;
	}
	
	// dentry = d_find_alias(inode);
	// if (!dentry){
	// 	//pr_err("Denrty no found\n");
		
	// }else{
	// 	buffer = (char *)__get_free_page(GFP_KERNEL);
	// 	if (!buffer){
	// 		// No buffer thind
	// 	}else{
	// 		path = dentry_path_raw(dentry, buffer, PAGE_SIZE);
	// 		free_page((unsigned long)buffer);
	// 	}

	// 	dput(dentry);
	// }
	// if(path != NULL ){
	// 	if(mp4_should_skip_path(path)){
	// 		return 0;
	// 	}
	// }
	ssid = curr_sec->mp4_flags;
	osid = get_inode_sid(inode);
	return mp4_has_permission(ssid,osid,mask);
}


/*
 * This is the list of hooks that we will using for our security module.
 */
static struct security_hook_list mp4_hooks[] = {
	/*
	 * inode function to assign a label and to check permission
	 */
	LSM_HOOK_INIT(inode_init_security, mp4_inode_init_security),
	LSM_HOOK_INIT(inode_permission, mp4_inode_permission),

	/*
	 * setting the credentials subjective security label when laucnhing a
	 * binary
	 */
	LSM_HOOK_INIT(bprm_set_creds, mp4_bprm_set_creds),

	/* credentials handling and preparation */
	LSM_HOOK_INIT(cred_alloc_blank, mp4_cred_alloc_blank),
	LSM_HOOK_INIT(cred_free, mp4_cred_free),
	LSM_HOOK_INIT(cred_prepare, mp4_cred_prepare)
};

static __init int mp4_init(void)
{
	/*
	 * check if mp4 lsm is enabled with boot parameters
	 */
	if (!security_module_enable("mp4"))
		return 0;

	pr_info("mp4 LSM initializing..");

	/*
	 * Register the mp4 hooks with lsm
	 */
	security_add_hooks(mp4_hooks, ARRAY_SIZE(mp4_hooks));

	return 0;
}

/*
 * early registration with the kernel
 */
security_initcall(mp4_init);
