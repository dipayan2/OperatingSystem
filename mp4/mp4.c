#define pr_fmt(fmt) "cs423_mp4: " fmt

#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/binfmts.h>
//add
#include <linux/xattr.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/printk.h>
#include "mp4_given.h"

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
	/*
	 * Add your code here
	 * ...
	 */
	struct dentry *dentry;
	int size;
	int ret;
	char *cred_ctx;
	int sid;

	//error handling for inode
	if (!inode || !inode->i_op || !inode->i_op->getxattr) {
		return MP4_NO_ACCESS;
	}

	//get dentry of inode
	dentry = d_find_alias(inode);

	//error handling dentry
	if (!dentry) {
		return MP4_NO_ACCESS;
	}

	size = 128;
	cred_ctx = kmalloc(size, GFP_KERNEL);
	if(!cred_ctx) {
		if(dentry)
			dput(dentry);
		return MP4_NO_ACCESS;
	}

	//first time get xattr and error handling
	ret = inode->i_op->getxattr(dentry, XATTR_MP4_SUFFIX, cred_ctx, size);
	size = ret;

	if(ret == -ERANGE) {
		//buffer overflows, should query the correct buffer size
		kfree(cred_ctx);
		ret = inode->i_op->getxattr(dentry, XATTR_MP4_SUFFIX, NULL, 0);
		//queried size even < 0, error, terminate.
		if(ret < 0) {
			if(dentry)
				dput(dentry);
			return MP4_NO_ACCESS;
		}

		//update the size by the newly queried correct size
		size = ret;
		cred_ctx = kmalloc(size, GFP_KERNEL);
		if(!cred_ctx) {
			if(dentry)
				dput(dentry);
			return -ENOMEM;
		}
		//second time get xattr and error handling
		ret = inode->i_op->getxattr(dentry, XATTR_MP4_SUFFIX, cred_ctx, size);
	}

	if(dentry)
		dput(dentry);

	if(ret < 0) {
		kfree(cred_ctx);
		return MP4_NO_ACCESS;
	} else {
		cred_ctx[size] = '\0';
		sid = __cred_ctx_to_sid(cred_ctx);
		kfree(cred_ctx);
	}

	if(printk_ratelimit()) {
		pr_info("mp4: get node sid helper passed!");
	}

	return sid;

}

/**
 * mp4_bprm_set_creds - Set the credentials for a new task
 *
 * @bprm: The linux binary preparation structure
 *
 * returns 0 on success.
 */

 //This hook is responsible for setting the credentials cred_ctx (and thus our subjective security blob) for each process that is launched from a given binary file.

static int mp4_bprm_set_creds(struct linux_binprm *bprm)
{
	int osid;
	struct inode * inode;
	struct mp4_security * curr_blob;

	//const char * fileName = bprm -> filename;
	if(!bprm || !bprm->file || !bprm->file->f_inode){
		return -ENOMEM;
	}
	inode = bprm->file->f_inode;

	//getting dentry: d_find_alias(bprm->file->f_inode)?

	//1.read the xattr value of the inode used to create the process
	//read the xattr value of the inode, get the label out of it
	osid = get_inode_sid(inode);

	//2.if that labels reads MP4 TARGET SID
	//you should set the created task’s blob to MP4 TARGET SID as well.
	if (osid == MP4_TARGET_SID) {
		if (!(bprm -> cred)) {
			if(printk_ratelimit()) {
				pr_info("bprm -> cred is NULL!");
			}
			return -ENOMEM;
		}
		if(!(bprm -> cred -> security)) {
			//should allocate a new one????
			if(printk_ratelimit()) {
				pr_info("bprm -> cred -> security is NULL!");
			}
			return -ENOMEM;
		}
		curr_blob = bprm -> cred -> security;
		curr_blob -> mp4_flags = osid;
	}



	return 0;
}

/**
 * mp4_cred_alloc_blank - Allocate a blank mp4 security label
 *
 * @cred: the new credentials
 * @gfp: the atomicity of the memory allocation
 *
 */

 //In Linux, all of a task’s credentials are held in (uid, gid) or through (groups, keys, LSM security) a refcounted structure of type ‘struct cred’. Each task points to its credentials by a pointer called ‘cred’ in its task_struct.

static int mp4_cred_alloc_blank(struct cred *cred, gfp_t gfp)
{
     //Add your code here
	 struct mp4_security * my_security_blob;

	 if(!cred){
		return -ENOMEM;
	}

	 my_security_blob = (struct mp4_security *)kmalloc(sizeof(struct mp4_security), gfp);
	 if(!my_security_blob) {
		 return -ENOMEM;
	 }
	 //initialized label should always be MP4_NO_ACCESS
	 my_security_blob -> mp4_flags = MP4_NO_ACCESS;
	 //hook the void pointer from cred to the new security blob we created
	 cred -> security = my_security_blob;


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
	/*
	 * Add your code here
	 * ...
	 */
	 //struct mp4_security * curr_blob;

	 if(!cred) {
		 return;
	 }

	 if(!cred->security) {
		 return;
	 }
	 /*
	  * cred->security == NULL if security_cred_alloc_blank() or
	  * security_prepare_creds() returned an error.
	  */
	//  BUG_ON(cred->security && (unsigned long) cred->security < PAGE_SIZE);

	 kfree(cred->security);
	 cred->security = NULL;

	 
}

/**
 * mp4_cred_prepare - Prepare new credentials for modification
 *
 * @new: the new credentials
 * @old: the old credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_prepare(struct cred *new, const struct cred *old, gfp_t gfp)
{
	struct mp4_security *old_blob;
	struct mp4_security * new_blob;

	if(!new) {
		return -ENOMEM;
	}

	if(!old || !old->security){
		new_blob = (struct mp4_security *)kmalloc(sizeof(struct mp4_security), gfp);
		if(!new_blob) {
			return -ENOMEM;
		}
		new_blob -> mp4_flags = MP4_NO_ACCESS;
		new->security = new_blob;
	} else {
		old_blob = old->security;

		// new_blob = kmemdup(old_blob, sizeof(struct mp4_security), gfp);
		new_blob = (struct mp4_security *)kmalloc(sizeof(struct mp4_security), gfp);
		if (!new_blob){
			return -ENOMEM;
		}
		new_blob -> mp4_flags = old_blob -> mp4_flags;
		new->security = new_blob;
	}


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
	/*
	 * Add your code here
	 * ...
	 */
	struct mp4_security * curr_cred;
	int task_sid;
	char *name_ptr;
	char *value_ptr;

	if(!current_cred() || !current_cred()->security){
		return -EOPNOTSUPP;
	}

	if(!inode || !dir) {
		return -EOPNOTSUPP;
	}

	curr_cred = current_cred() -> security;
	task_sid = curr_cred->mp4_flags; //how to get the current task's security blob: current_cred()?

	// put the attribute name
	// use kmalloc?
	name_ptr = kstrdup(XATTR_MP4_SUFFIX, GFP_KERNEL);
	if(!name_ptr) {
		return -ENOMEM;
	}
	*name = name_ptr;

	//intermediate check
	// if(printk_ratelimit()) {
	//    pr_info("5th HOOK: mp4, the current task sid is %d", task_sid);
	// }


	// put the value and length
	if(task_sid == MP4_TARGET_SID) {
		//if inode is a directory, set xattr to "dir-write", else set to "read-write"
		// if(S_ISDIR(inode->i_mode)) {
		// 	value_ptr = kstrdup("dir-write", GFP_KERNEL);
		// 	//error handling
		// 	if (!value_ptr) {
		// 		return -ENOMEM;
		// 	}
		// 	*value = value_ptr;

		// 	//put length
		// 	*len = 10;
		// } else {
			//put value
			value_ptr = kstrdup("read-write", GFP_KERNEL);
			//error handling
			if (!value_ptr) {
				return -ENOMEM;
			}
			*value = value_ptr;

			//put length
			*len = 11;
		//}

	} else {
		return -EOPNOTSUPP;
	}

	// if(printk_ratelimit()) {
	//    pr_info("5th HOOK: mp4_inode_init_security called!");
	// }

	return 0;
}


/**
 * mp4_has_permission - Check if subject has permission to an object
 *
 * @ssid: the subject's security id
 * @osid: the object's security id
 * @mask: the operation mask: Right to check for (%MAY_READ, %MAY_WRITE, %MAY_EXEC)
 *
 * returns 0 is access granter, -EACCES otherwise
 *
 */

static int mp4_has_permission(int ssid, int osid, int mask)
{

	//1.Cannot access the inode
	if(osid == MP4_NO_ACCESS) {
		return -EACCES;
	}

	//2.Can access the inode, check if access allowed
	if(osid == MP4_READ_OBJ) {
		if((mask & MAY_WRITE) > 0 || (mask & MAY_EXEC) > 0 ||    (mask & MAY_APPEND) > 0) {
			return -EACCES;
		}
	}
	else if(osid == MP4_READ_WRITE){
		if((mask & MAY_EXEC) > 0) {
			return -EACCES;
		}
	}
	else if(osid == MP4_WRITE_OBJ ){
		if((mask & MAY_READ) > 0 || (mask & MAY_EXEC) > 0) {
			return -EACCES;
		}
	}
	else if(osid == MP4_EXEC_OBJ ){
		if((mask & MAY_WRITE) > 0 ||(mask & MAY_APPEND) > 0) {
			return -EACCES;
		}
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

 //For those programs that are not labeled as target
 //our module will allow them full access to directories (regardless of the directories’ security labels), //and will allow them read-only access to files that have been assigned one of our custom labels.

static int mp4_inode_permission(struct inode *inode, int mask)
{

	 struct dentry *dentry;
	 struct mp4_security *current_cred;
	 char *dir;
	 char *buf;
	 int ret;
	 int authorized;
	 int ssid;
	 int osid;
	 int len = 256;

	 if (!mask) {
		 return 0;
	 }

	 
	 //get the dentry for getting dir name
	 if(!inode) {
		 return -EACCES;
	 }

	 dentry = d_find_alias(inode);
	 if(!dentry) {
		 return -EACCES;
	 }

	 //get the path for checking whether should skip them
	 buf = kmalloc(len, GFP_KERNEL);
	 if(!buf) {
		 if(dentry)
		 	dput(dentry);
		 return -EACCES;
	 }


	 if(!dentry) {
		 return -EACCES;
	 }

	 dir = dentry_path_raw(dentry, buf, len);

	 if(!dir) {
		 kfree(buf);
		 if(dentry)
		 	dput(dentry);
		 return -EACCES;
	 }

	 //should skip path
	 if (mp4_should_skip_path(dir)) {
		 kfree(buf);
		 if(dentry)
		 	dput(dentry);
	 	 return 0; //TODO: skip is granted or no access?
	 }

	 if (!current_cred() || !current_cred()->security) {  // ssid
		 kfree(buf);
		 if(dentry)
		 	dput(dentry);
		 return -EACCES;
	 }

	 current_cred = current_cred()->security;
	 ssid = current_cred -> mp4_flags;
	 osid = get_inode_sid(inode);

	 //Our Policy!
	 if(ssid && osid ){
		 if (ssid == MP4_TARGET_SID || (ssid != MP4_TARGET_SID && !S_ISDIR(inode->i_mode))) {
			//Enter MAC policy!
			ret = mp4_has_permission(ssid, osid, mask);
		 }
 	  	 else { //task is not target, and the inode is directory
			 ret = 0;
		 }
	 }
	 else {
		ret = 0;
	 }


	 authorized = ret == 0 ? 1 : 0;
	 /* Then, use this code to print relevant denials: for our processes or on our objects */
	 if(printk_ratelimit()) {
	 	pr_info("[inode Perm] handling task ssid: %d, inode osid: %d. Authorized ? : %d.\n", ssid, osid, authorized);
 	 }


 	 if(dentry){
	 	dput(dentry);
	}
	 kfree(buf);
	 return ret; /* permissive */
	 //return 0;
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