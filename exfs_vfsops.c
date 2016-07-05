/*	$OpenBSD: exfs_vfsops.c,v 1.73 2016/03/07 18:43:59 naddy Exp $	*/
/*	$NetBSD: exfs_vfsops.c,v 1.26 1997/06/13 15:38:58 pk Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/specdev.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/stat.h>

#include <isofs/exfs/iso.h>
#include <isofs/exfs/exfs_extern.h>
#include <isofs/exfs/iso_rrip.h>
#include <isofs/exfs/exfs_node.h>

const struct vfsops exfs_vfsops = {
	exfs_mount,
	exfs_start,
	exfs_unmount,
	exfs_root,
	exfs_quotactl,
	exfs_statfs,
	exfs_sync,
	exfs_vget,
	exfs_fhtovp,
	exfs_vptofh,
	exfs_init,
	exfs_sysctl,
	exfs_check_export
};

/*
 * Called by vfs_mountroot when iso is going to be mounted as root.
 */

static	int iso_mountfs(struct mount *mp,
	    struct proc *p, struct iso_args *argp);

int
exfs_mountroot(void)
{
//    printf(" exfs_mountroot_start \n");
	struct mount *mp;
	extern struct vnode *rootvp;
	struct proc *p = curproc;	/* XXX */
	int error;
	struct iso_args args;

	/*
	 * Get vnodes for swapdev and rootdev.
	 */
//    printf(" exfs_mountroot_start \n");
	if ((error = bdevvp(swapdev, &swapdev_vp)) ||
	    (error = bdevvp(rootdev, &rootvp))) {
		printf("exfs_mountroot: can't setup bdevvp's");
                return (error);
        }

//    printf("1 exfs_mountroot \n");
	if ((error = vfs_rootmountalloc("ex", "root_device", &mp)) != 0)
		return (error);
	args.flags = ISOFSMNT_ROOT;
    printf("2 exfs_mountroot \n");
	if ((error = iso_mountfs(mp, p, &args)) != 0) {
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp);
                free(mp, M_MOUNT, 0);
                return (error);
        }

//    printf("3 exfs_mountroot \n");
        TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);
        (void)exfs_statfs(mp, &mp->mnt_stat, p);
//    printf("4 exfs_mountroot \n");
	vfs_unbusy(mp);
	inittodr(0);

//    printf(" exfs_mountroot_end \n");
        return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
exfs_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
//    printf(" exfs_mount_start \n");
	struct iso_mnt *imp = NULL;
	struct iso_args args;
	struct vnode *devvp;
	char fspec[MNAMELEN];
	int error;

	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		return (EROFS);

	error = copyin(data, &args, sizeof(struct iso_args));
	if (error)
		return (error);

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		imp = VFSTOISOFS(mp);
		if (args.fspec == NULL)
			return (vfs_export(mp, &imp->im_export,
			    &args.export_info));
	}

	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	error = copyinstr(args.fspec, fspec, sizeof(fspec), NULL);
//    printf("1 exfs_mount \n");
	if (error)
		return (error);
//    printf("2 exfs_mount \n");
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, fspec, p);
//    printf("3 exfs_mount \n");
	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = iso_mountfs(mp, p, &args);
//    printf("4 exfs_mount \n");
	if (error) {
		vrele(devvp);
		return (error);
	}
	imp = VFSTOISOFS(mp);

	bzero(mp->mnt_stat.f_mntonname, MNAMELEN);
	strlcpy(mp->mnt_stat.f_mntonname, path, MNAMELEN);
	bzero(mp->mnt_stat.f_mntfromname, MNAMELEN);
	strlcpy(mp->mnt_stat.f_mntfromname, fspec, MNAMELEN);
	bzero(mp->mnt_stat.f_mntfromspec, MNAMELEN);
	strlcpy(mp->mnt_stat.f_mntfromspec, fspec, MNAMELEN);
	bcopy(&args, &mp->mnt_stat.mount_info.iso_args, sizeof(args));

	exfs_statfs(mp, &mp->mnt_stat, p);

//    printf(" exfs_mount_end \n");
	return (0);
}

/*
 * Common code for mount and mountroot
 */
static int
iso_mountfs(mp, p, argp)
	struct mount *mp;
	struct proc *p;
	struct iso_args *argp;
{
//    printf(" iso_mountfs_start \n");
	register struct iso_mnt *isomp = (struct iso_mnt *)0;
	extern struct vnode *rootvp;
	int iso_bsize;
	int logical_block_size;
	int sess;
	iso_bsize = ISO_DEFAULT_BLOCK_SIZE;

//    printf("2 iso_mountfs \n");
	if (argp->flags & ISOFSMNT_SESS) {
		sess = argp->sess;
		if (sess < 0)
			sess = 0;
	} else {
		sess = 0;
	}

//    printf("8 iso_mountfs \n");
	logical_block_size = 512;

	isomp = malloc(sizeof *isomp, M_ISOFSMNT, M_WAITOK);
	bzero((caddr_t)isomp, sizeof *isomp);
	isomp->logical_block_size = logical_block_size;
	isomp->volume_space_size = 64;
	isomp->root_size = 512;
	isomp->joliet_level = 0;
	/*
	 * Since an ISO9660 multi-session CD can also access previous sessions,
	 * we have to include them into the space considerations.
	 */
//    printf("11 iso_mountfs \n");
	isomp->volume_space_size += sess;
	isomp->im_bmask = logical_block_size - 1;
	isomp->im_bshift = ffs(logical_block_size) - 1;

//    printf("12 iso_mountfs \n");
	mp->mnt_data = (qaddr_t)isomp;
	mp->mnt_stat.f_fsid.val[0] = 0;
	mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
	mp->mnt_flag |= MNT_LOCAL;
	isomp->im_mountp = mp;

//    printf("14 iso_mountfs \n");
	isomp->im_flags = argp->flags & (ISOFSMNT_NORRIP | ISOFSMNT_GENS |
	    ISOFSMNT_EXTATT | ISOFSMNT_NOJOLIET);
	switch (isomp->im_flags & (ISOFSMNT_NORRIP | ISOFSMNT_GENS)) {
	default:
	    isomp->iso_ftype = ISO_FTYPE_DEFAULT;
	    break;
	case ISOFSMNT_GENS|ISOFSMNT_NORRIP:
	    isomp->iso_ftype = ISO_FTYPE_9660;
	    break;
	case 0:
	    isomp->iso_ftype = ISO_FTYPE_RRIP;
	    break;
	}

//    printf(" iso_mountfs_end \n");

	return (0);
}

/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int
exfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
//    printf(" exfs_start \n");
	return (0);
}

/*
 * unmount system call
 */
int
exfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
//    printf(" exfs_unmount_start \n");
	register struct iso_mnt *isomp;
	int error, flags = 0;
//    printf("1 exfs_unmount \n");

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
#if 0
    printf("1a exfs_unmount \n");
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp))
		return (EBUSY);
#endif
//    printf("2 exfs_unmount \n");
	if ((error = vflush(mp, NULLVP, flags)) != 0)
		return (error);

//    printf("3 exfs_unmount \n");
	isomp = VFSTOISOFS(mp);

//    printf("4 exfs_unmount \n");
	free((caddr_t)isomp, M_ISOFSMNT, 0);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
//    printf(" exfs_unmount_end \n");
	return (error);
}

/*
 * Return root of a filesystem
 */
int
exfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
//    printf(" exfs_root_start \n");
	struct iso_mnt *imp = VFSTOISOFS(mp);
	struct iso_directory_record *dp =
	    (struct iso_directory_record *)imp->root;
	cdino_t ino = 1;

	/*
	 * With RRIP we must use the `.' entry of the root directory.
	 * Simply tell vget, that it's a relocated directory.
	 */
//    printf(" exfs_root_end \n");
	return (exfs_vget_internal(mp, ino, vpp,
	    imp->iso_ftype == ISO_FTYPE_RRIP, dp));
}

/*
 * Do operations associated with quotas, not supported
 */
/* ARGSUSED */
int
exfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
//    printf(" exfs_quotactl \n");

	return (EOPNOTSUPP);
}

/*
 * Get file system statistics.
 */
int
exfs_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
    printf(" exfs_statfs \n");
	register struct iso_mnt *isomp;

	isomp = VFSTOISOFS(mp);

    printf("1 exfs_statfs \n");
	sbp->f_bsize = isomp->logical_block_size;
	sbp->f_iosize = sbp->f_bsize;	/* XXX */
	sbp->f_blocks = isomp->volume_space_size;
	sbp->f_bfree = 0; /* total free blocks */
	sbp->f_bavail = 0; /* blocks free for non superuser */
	sbp->f_files =  0; /* total files */
	sbp->f_ffree = 0; /* free file nodes */
    printf(" exfs_statfs \n");
	if (sbp != &mp->mnt_stat) {
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname,
		    MNAMELEN);
		bcopy(&mp->mnt_stat.mount_info.iso_args,
		    &sbp->mount_info.iso_args, sizeof(struct iso_args));
	}
    printf(" exfs_statfs_end \n");
	return (0);
}

/* ARGSUSED */
int
exfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
    printf(" exfs_sync \n");
	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is in range
 * - call iget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 * - check that the generation number matches
 */

struct ifid {
	ushort	ifid_len;
	ushort	ifid_pad;
	int	ifid_ino;
	long	ifid_start;
};

/* ARGSUSED */
int
exfs_fhtovp(mp, fhp, vpp)
	register struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
    printf(" ifid_start \n");
	struct ifid *ifhp = (struct ifid *)fhp;
	register struct iso_node *ip;
	struct vnode *nvp;
	int error;
    printf("1 ifid \n");

#ifdef	ISOFS_DBG
    printf("1a ifid \n");
	printf("fhtovp: ino %d, start %ld\n", ifhp->ifid_ino,
	    ifhp->ifid_start);
#endif

    printf("2 ifid \n");
	if ((error = VFS_VGET(mp, ifhp->ifid_ino, &nvp)) != 0) {
		*vpp = NULLVP;
		return (error);
	}
    printf("3 ifid \n");
	ip = VTOI(nvp);
    printf("4 ifid \n");
	if (ip->inode.iso_mode == 0) {
		vput(nvp);
		*vpp = NULLVP;
		return (ESTALE);
	}
	*vpp = nvp;
    printf(" ifid_end \n");
	return (0);
}

int
exfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{

    printf(" exfs_vget_start \n");
	if (ino > (cdino_t)-1)
		panic("exfs_vget: alien ino_t %llu",
		    (unsigned long long)ino);

	/*
	 * XXXX
	 * It would be nice if we didn't always set the `relocated' flag
	 * and force the extra read, but I don't want to think about fixing
	 * that right now.
	 */
    printf(" exfs_vget_end \n");
	return (exfs_vget_internal(mp, ino, vpp,
#if 0
	    VFSTOISOFS(mp)->iso_ftype == ISO_FTYPE_RRIP,
#else
	    0,
#endif
	    NULL));
}

int
exfs_vget_internal(mp, ino, vpp, relocated, isodir)
	struct mount *mp;
	cdino_t ino;
	struct vnode **vpp;
	int relocated;
	struct iso_directory_record *isodir;
{
    printf(" exfs_vget_internal_start \n");
	register struct iso_mnt *imp;
	struct iso_node *ip;
	struct buf *bp;
	struct vnode *vp;
	dev_t dev;
	int error;
    printf("1 exfs_vget_internal \n");

retry:
    printf("2 exfs_vget_internal \n");
	imp = VFSTOISOFS(mp);
	dev = imp->im_dev;
    printf("3 exfs_vget_internal \n");
	if ((*vpp = exfs_ihashget(dev, ino)) != NULLVP)
		return (0);

    printf("4 exfs_vget_internal \n");
	/* Allocate a new vnode/iso_node. */
	if ((error = getnewvnode(VT_ISOFS, mp, &exfs_vops, &vp)) != 0) {
		*vpp = NULLVP;
		return (error);
	}
    printf("5 exfs_vget_internal \n");
	ip = malloc(sizeof(*ip), M_ISOFSNODE, M_WAITOK | M_ZERO);
	lockinit(&ip->i_lock, PINOD, "isoinode", 0, 0);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_dev = dev;
	ip->i_number = ino;

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	error = exfs_ihashins(ip);
    printf("6 exfs_vget_internal \n");

	if (error) {
		vrele(vp);

		if (error == EEXIST)
			goto retry;

		return (error);
	}
    printf("7 exfs_vget_internal \n");

	if (isodir == 0) {
		int lbn, off;

    printf("7a exfs_vget_internal \n");
		lbn = lblkno(imp, ino);
		if (lbn >= imp->volume_space_size) {
			vput(vp);
			printf("fhtovp: lbn exceed volume space %d\n", lbn);
			return (ESTALE);
		}
    printf("7b exfs_vget_internal \n");

		off = blkoff(imp, ino);
		if (off + ISO_DIRECTORY_RECORD_SIZE > imp->logical_block_size)
		    {
			vput(vp);
			printf("fhtovp: crosses block boundary %d\n",
			    off + ISO_DIRECTORY_RECORD_SIZE);
			return (ESTALE);
		}
    printf("7c exfs_vget_internal \n");

		error = bread(imp->im_devvp,
			      lbn << (imp->im_bshift - DEV_BSHIFT),
			      imp->logical_block_size, &bp);
    printf("7d exfs_vget_internal \n");
		if (error) {
			vput(vp);
			brelse(bp);
			printf("fhtovp: bread error %d\n",error);
			return (error);
		}
    printf("7e exfs_vget_internal \n");
		isodir = (struct iso_directory_record *)(bp->b_data + off);

    printf("7f exfs_vget_internal \n");
		if (off + isonum_711(isodir->length) >
		    imp->logical_block_size) {
			vput(vp);
			if (bp != 0)
				brelse(bp);
			printf("fhtovp: directory crosses block boundary %d[off=%d/len=%d]\n",
			    off +isonum_711(isodir->length), off,
			    isonum_711(isodir->length));
			return (ESTALE);
		}
    printf("8 exfs_vget_internal \n");

#if 0
    printf("8a exfs_vget_internal \n");
		if (isonum_733(isodir->extent) +
		    isonum_711(isodir->ext_attr_length) != ifhp->ifid_start) {
			if (bp != 0)
				brelse(bp);
			printf("fhtovp: file start miss %d vs %d\n",
			    isonum_733(isodir->extent) +
			    isonum_711(isodir->ext_attr_length),
			    ifhp->ifid_start);
			return (ESTALE);
		}
#endif
	} else
		bp = 0;

    printf("9 exfs_vget_internal \n");
	ip->i_mnt = imp;

    printf("11 exfs_vget_internal \n");
	ip->iso_extent = isonum_733(isodir->extent);
	ip->i_size = (u_int32_t) isonum_733(isodir->size);
	ip->iso_start = isonum_711(isodir->ext_attr_length) + ip->iso_extent;

	/*
	 * Setup time stamp, attribute
	 */
    printf("12 exfs_vget_internal \n");
	vp->v_type = VNON;
	switch (imp->iso_ftype) {
	default:	/* ISO_FTYPE_9660 */
	    {
		struct buf *bp2;
		int off;
		if ((imp->im_flags & ISOFSMNT_EXTATT) &&
		    (off = isonum_711(isodir->ext_attr_length)))
			exfs_bufatoff(ip, (off_t)-(off << imp->im_bshift),
			    NULL, &bp2);
		else
			bp2 = NULL;
		exfs_defattr(isodir, ip, bp2);
		exfs_deftstamp(isodir, ip, bp2);
		if (bp2)
			brelse(bp2);
		break;
	    }
	case ISO_FTYPE_RRIP:
		exfs_rrip_analyze(isodir, ip, imp);
		break;
	}

    printf("13 exfs_vget_internal \n");
	if (bp != 0)
		brelse(bp);

	/*
	 * Initialize the associated vnode
	 */
    printf("14 exfs_vget_internal \n");
	//НОВЫЙ ВАРИАНТ
	if (ino==1)
	{
		vp->v_type =VDIR;
	}
	else
	{
		vp->v_type =VREG;
	}

    printf("15 exfs_vget_internal \n");
	if (ip->iso_extent == imp->root_extent)
		vp->v_flag |= VROOT;

	/*
	 * XXX need generation number?
	 */

	*vpp = vp;
    printf(" exfs_vget_internal_end \n");
	return (0);
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
exfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
    printf(" exfs_vptofh_start \n");
	register struct iso_node *ip = VTOI(vp);
	register struct ifid *ifhp;

	ifhp = (struct ifid *)fhp;
	ifhp->ifid_len = sizeof(struct ifid);

	ifhp->ifid_ino = ip->i_number;
	ifhp->ifid_start = ip->iso_start;

#ifdef	ISOFS_DBG
	printf("vptofh: ino %d, start %ld\n",
	    ifhp->ifid_ino,ifhp->ifid_start);
#endif
    printf(" exfs_vptofh_end \n");
	return (0);
}

/*
 * Verify a remote client has export rights and return these rights via
 * exflagsp and credanonp.
 */
int
exfs_check_export(mp, nam, exflagsp, credanonp)
	register struct mount *mp;
	struct mbuf *nam;
	int *exflagsp;
	struct ucred **credanonp;
{
    printf(" exfs_check_export_start \n");
	register struct netcred *np;
	register struct iso_mnt *imp = VFSTOISOFS(mp);
	/*
	 * Get the export permission structure for this <mp, client> tuple.
	 */
	np = vfs_export_lookup(mp, &imp->im_export, nam);
    printf("1 exfs_check_export \n");
	if (np == NULL)
		return (EACCES);

	*exflagsp = np->netc_exflags;
	*credanonp = &np->netc_anon;
    printf(" exfs_check_export_end \n");
	return (0);
}
