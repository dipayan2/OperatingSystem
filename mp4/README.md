# CS423 MP4: Linux Security Modules

### Checking the MP4 Linux Security Module is intitalzed in Kernel
For checking if the security module is running, run `dmesg | grep mp4`

### MP Design

#### 1.mp4_cred_alloc_blank
For this hook I first allocated a new mp4_security as the new security blob, and set its sid to `MP4_NO_ACCESS` as default. The allocate the provided the cred to the new blob.

#### 2.mp4_cred_prepare
For this hook, a new security blob called `new_blob` is allocated which should be a deep copy of the old security blob referred by `old -> security`, so that the sid field can also be copied. if `old` cred is empty or `old->security` is empty then we use `mp4_cred_alloc_blank` to get the `new` cred.
#### 3.mp4_cred_free
For this hook I did error handling such as that `cred->security` may be null, and after that used `kfree` to deallocate the security blob.


#### 4.mp4_bprm_set_creds
For this hook, the goal is to set the credentials for each process that is launched from a given binary file. First, I get the inode of the bprm by `bprm->file->f_inode` after several error checks, and then use the helper function `get_inode_sid` to get the sid of that inode.

We check if the attribute value `inode` of the `bprnm` is `target`, then we allocate the flag of `bprm->cred->security`  to `MP4_TARGET_SID`



#### 5.mp4_inode_init_security
For this hook, the idea is that the `name` and `value` and `len` attribute are the place where we should set the these value. So the way to do it is that:

In this we allocate the `XATTR_MP4_SUFFIX` to the `name` variable and then the `value` to `read-write`, provided that the sid of `current` is `MP4_TARGET_SID`



#### 6.mp4_inode_permission
This hook is partially implemented with the function `mp4_inode_permission` completed. We use the function `mp4_has_permission` to implement the policy, while we also use the `inode` and get it's path name to skip over paths to speed up the boot.





### Details about the test cases that you used to verify that your security module is working

I created two tests scripts called `test.perm` and `test.perm.unload` at the home directory:
`test.perm`:

setfattr -n security.mp4 -v target /bin/cat
setfattr -n security.mp4 -v dir /home
setfattr -n security.mp4 -v dir /home/dipayan2/
setfattr -n security.mp4 -v dir /home/dipayan2/dipayan2/
setfattr -n security.mp4 -v read-only /home/dipayan2/dipayan2/file.txt

`test.perm.unload`:

setfattr -x security.mp4 /bin/cat
setfattr -x security.mp4 /home
setfattr -x security.mp4 ~
setfattr -x security.mp4 ~/file.txt


To run the tests:
Source the script by  `source test.perm
Then unloaded the script by `source test.perm.unload`

### pwd policy
In the file `pwd_perm.txt` I have the set of files accessed to change password and we would need to `setattr` to them, but due to lack of time, I just addedd the file
