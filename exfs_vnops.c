/*	$OpenBSD: exfs_vnops.c,v 1.74 2016/01/19 19:11:21 stefan Exp $	*/
/*	$NetBSD: exfs_vnops.c,v 1.42 1997/10/16 23:56:57 christos Exp $	*/

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)exfs_vnops.c	8.15 (Berkeley) 12/5/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/dirent.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/poll.h>
#include <sys/specdev.h>
#include <sys/unistd.h>

#include <miscfs/fifofs/fifo.h>

#include <isofs/exfs/iso.h>
#include <isofs/exfs/exfs_extern.h>
#include <isofs/exfs/exfs_node.h>
#include <isofs/exfs/iso_rrip.h>

int exfs_kqfilter(void *v);


/*
 * Structure for reading directories
 */
struct isoreaddir {
	struct dirent saveent;
	struct dirent assocent;
	struct dirent current;
	off_t saveoff;
	off_t assocoff;
	off_t curroff;
	struct uio *uio;
	off_t uio_off;
	int eofflag;
};

int	iso_uiodir(struct isoreaddir *, struct dirent *, off_t);
int	iso_shipdir(struct isoreaddir *);

/*
 * Setattr call. Only allowed for block and character special devices.
 */
int
exfs_setattr(void *v)
{
    printf("zawel v exfs_setattr\n");
	struct vop_setattr_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;

	if (vap->va_flags != VNOVAL || vap->va_uid != (uid_t)VNOVAL ||
	    vap->va_gid != (gid_t)VNOVAL || vap->va_atime.tv_nsec != VNOVAL ||
	    vap->va_mtime.tv_nsec != VNOVAL || vap->va_mode != (mode_t)VNOVAL ||
	    (vap->va_vaflags & VA_UTIMES_CHANGE))
		return (EROFS);
	if (vap->va_size != VNOVAL) {
		switch (vp->v_type) {
		case VDIR:
			return (EISDIR);
		case VLNK:
		case VREG:
			return (EROFS);
		case VCHR:
		case VBLK:
		case VSOCK:
		case VFIFO:
			return (0);
		default:
			return (EINVAL);
		}
	}

	return (EINVAL);
}

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
int
exfs_open(void *v)
{
    printf("zawel v exfs_open \n");
	return (0);
}

/*
 * Close called
 *
 * Update the times on the inode on writeable file systems.
 */
/* ARGSUSED */
int
exfs_close(void *v)
{
    printf("zawel v exfs_close \n");
	return (0);
}

/*
 * Check mode permission on inode pointer. Mode is READ, WRITE or EXEC.
 * The mode is shifted to select the owner/group/other fields. The
 * super user is granted all permissions.
 */
int
exfs_access(void *v)
{
//	struct vop_access_args *ap = v;
//	struct iso_node *ip = VTOI(ap->a_vp);

    printf("zawel v exfs_access\n");
	return 0;
//	(vaccess(ap->a_vp->v_type, ip->inode.iso_mode & ALLPERMS,
//	    ip->inode.iso_uid, ip->inode.iso_gid, ap->a_mode, ap->a_cred));
}

int
exfs_getattr(void *v)
{
    printf("zawli v exfsgetattr\n");
	struct vop_getattr_args *ap = v;
	struct vnode *vp = ap->a_vp;
	register struct vattr *vap = ap->a_vap;
	register struct iso_node *ip = VTOI(vp);

	vap->va_fileid	= ip->i_number;

	vap->va_mode	= 0777;
	vap->va_nlink	= 1;	/* no inode sharing */
	vap->va_uid	= 0;	/* root */
	vap->va_gid	= 0;	/* main root group (wheel) */
//	vap->va_atime	= ip->inode.iso_atime;
//	vap->va_mtime	= ip->inode.iso_mtime;
//	vap->va_ctime	= ip->inode.iso_ctime;
//	vap->va_rdev	= ip->inode.iso_rdev;

//	vap->va_size	= (u_quad_t) ip->i_size;
	vap->va_flags	= 0;
	vap->va_gen	= 1;
	vap->va_blocksize = 256;
	vap->va_bytes	= 256;
	vap->va_type	= vp->v_type;
	return (0);
}

/*
 * Vnode op for reading.
 */
int
exfs_read(void *v)
{

	struct vop_read_args *ap = v;
	struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
	register struct iso_node *ip = VTOI(vp);
//	register struct iso_mnt *imp;
//	struct buf *bp;
//	daddr_t lbn, rablock;
//	off_t diff;
//	int error = 0;
//	long size, on;
//	size_t n;
    int bytestart; /* номер байта, с которого начнется загрузка в файл */
    long int a1; /* T(n-2) или T(1) в начале */
    long int a2; /* T(n-1) или T(2) в начале */
    long int a3; /* T(n) */
//    int nextval; /*Генерация T(n)  */
    int cur_byte=1; /*Текущий байт посл-ти*/
    int flag_type=1; /*Флаг записи в файл: 0 - можно записывать; 1 - еще р*/
//    int cnt;
    int count_of_num;
 int strlen_elem; /* кол-во символов T(n) */
    const int MAX_DIGIT = 15; /* максимальное кол-во разрядов для эл-та */
//    int radix = 10; /* основание системы счисления */
    char tempstr[15]; /* строка, которая будет получена преобразов*/
    int i;
    int flag_end = 0;
    int cur_elem = 1;
//    int len_digit;
    int val_type;
    char buf[256];
    int buf_fill=0;
    int amnt;

    printf("zawli v exfs_read\n");
	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
    printf("%d poly4en uio->uio_resid\n", (int) uio->uio_resid);
    printf("%d poly4en uio->uio_offset\n", (int) uio->uio_offset);
    printf("dlina - %d", (int) uio->uio_iov->iov_len);
    printf("i_number = %d", (int) ip->i_number);
    bytestart = (int) uio->uio_offset;
    count_of_num = (int) uio->uio_resid;


  if ((int) ip->i_number == 2)
    {
    val_type = 1;
    a1 = 1;
    a2 = 1;
//    printf("123\n");
    }
  else if ((int) ip->i_number == 3)
    {
    val_type = 2;
    a1 = 2;
    a2 = 1;
    }
  else if ((int) ip->i_number == 4)
    {
    val_type = 3;
    a1 = 0;
    a2 = 1;
    }
  else if ((int) ip->i_number == 5)
    {
    val_type = 4;
    a1 = 1;
    a2 = 2;
    }
  else if ((int) ip->i_number == 6)
    {
    val_type = 5;
    a1 = 0;
    a2 = 1;
    }
  else if ((int) ip->i_number == 1)
    {
        uio->uio_resid = 100;
//        uio->uio_iov->iov_len = 100;
//        printf("uio_len %d", (int) uio->uio_iov->iov_len);
        uiomove("fib.txt\npell.txt\nluke.txt\ncentpol.txt\nsquaretriangle.txt\n", 100, uio);
               return 0;
    }
  else
    {
    val_type = -1;
    a1 = 2;
    a2 = -2;
    printf("234\n");
    return EINVAL;
    }
   ;



    if (count_of_num <= 0)
        {
            printf("Error;\n");
        }

    while (flag_end == 0)
    {
        if (cur_elem == 1)
        {
            strlen_elem = snprintf(tempstr, MAX_DIGIT, "%ld", a1);
        }
        else if (cur_elem == 2)
        {
            strlen_elem = snprintf(tempstr, MAX_DIGIT, "%ld", a2);
        }
        else if (cur_elem > 2)
        {
            strlen_elem = snprintf(tempstr, MAX_DIGIT, "%ld", a3);
        }


        for (i=0; i<strlen_elem; i++)
        {
            if((flag_type == 0) || (cur_byte >= bytestart))
            {
                if(flag_type != 0)
                    flag_type = 0;
            }
            if (cur_byte > bytestart + count_of_num - 1)
            {
                flag_end = 1;
                break;
            }


            cur_byte++;

            if (flag_type == 0)
            {
                printf("%c", tempstr[i]);
                buf[buf_fill]=tempstr[i];
                buf_fill++;
            }



        }

        if (cur_elem >= 2)
        {

          if (cur_elem > 2)
          {
            a1 = a2;
            a2 = a3;
          }

          if (val_type == 1 || val_type == 2)
          {
            a3 = a1 + a2;
          }
          else if (val_type == 3)
          {
            a3 = 2 * a2 + a1;
          }
          else if (val_type == 4)
          {
            a3 = a2 + cur_elem ;
          }
          else if (val_type == 5)
          {
            a3 = 34*a2 - a1 + 2;
          }
        }
   if ((flag_end == 0) && (flag_type == 0))
   {
        printf("\n");
        buf[buf_fill]='\n';
        buf_fill++;
//        cur_elřem++;
        cur_byte++;

   }
   cur_elem++;
   }
   amnt = min((int) uio->uio_resid, buf_fill);
   uiomove(buf, amnt, uio);
	return (0);
}

/* ARGSUSED */
int
exfs_ioctl(void *v)
{
    printf("zawel v exfs_ioctl\n");
	return (ENOTTY);
}

/* ARGSUSED */
int
exfs_poll(void *v)
{
	struct vop_poll_args *ap = v;

	/*
	 * We should really check to see if I/O is possible.
	 */
    printf("zawel v exfs_pool\n");
	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

/*
 * Mmap a file
 *
 * NB Currently unsupported.
 */
/* ARGSUSED */
int
exfs_mmap(void *v)
{
    printf("zawel v exfs_mmap\n");
	return (EINVAL);
}

/*
 * Seek on a file
 *
 * Nothing to do, so just return.
 */
/* ARGSUSED */
int
exfs_seek(void *v)
{
    printf("zawel v exfs_seek\n");
	return (0);
}

/*
 * Vnode op for readdir
 */
int
exfs_readdir(void *v)
{
	struct vop_readdir_args *ap = v;
    register struct uio *uio = ap->a_uio;
	struct isoreaddir *idp;
	struct vnode *vdp = ap->a_vp;
	struct iso_node *dp;
//	struct iso_mnt *imp;
//	struct buf *bp = NULL;
//	struct iso_directory_record *ep;
//	int entryoffsetinblock;
//	doff_t endsearch;
//	u_long bmask;
    int error = 0;
//	int reclen;
//	u_short namelen;
//	cdino_t ino;
//    char dir_names[100];
//    static char buffer[256] = ".\n..\nfib.txt\npell.txt\nluke.txt\ncentpol.txt\nsquaretriangle.txt\n";

//    strcpy(dir_names, "fib.txt\npell.txt\nluke.txt\ncentpol.txt\nsquaretriangle.txt\n");

	printf("zawli v readdir");
    dp = VTOI(vdp);
    idp = malloc(sizeof *idp, M_ISOFSMNT, M_WAITOK);
//	/*
//	 * XXX
//	 * Is it worth trying to figure out the type?
//	 */
    idp->saveent.d_type = idp->assocent.d_type = idp->current.d_type =
        DT_UNKNOWN;
    idp->uio = uio;
    idp->eofflag = 1;
    idp->curroff = uio->uio_offset;
    idp->uio_off = uio->uio_offset;
    printf("uio_len %d", (int) idp->uio->uio_iov->iov_len);

    if (dp->i_number == 1)
    {
//        printf("zawli v 4tenie directorii");
//        *ap->a_eofflag = 1;
//          idp->uio->uio_resid = 100;
        printf("1");
          idp->uio->uio_iov->iov_len = 100;
          printf("2");
//        printf("uio_len %d", (int) uio->uio_iov->iov_len);
     //   uiomove(".\n..\nfib.txt\npell.txt\nluke.txt\ncentpol.txt\nsquaretriangle.txt\n", 100, uio);
        printf("3");
         uiomove(".\n..\nfib.txt\npell.txt\nluke.txt\ncentpol.txt\nsquaretriangle.txt\n", 100, idp->uio);

        *ap->a_eofflag = 1;
              printf("zawli v 4tenie directorii");
        return 0;
//        uio->uio_iov->iov_len = 10;
//        strncpy(uio->uio_iov->iov_base[0], "fib\npell\n", 10);
    }




//	imp = dp->i_mnt;
//	bmask = imp->im_bmask;
//
//
//	/*
//	 * These are passed to copyout(), so make sure there's no garbage
//	 * being leaked in padding or after short names.
//	 */

    //MALLOC(idp, struct isoreaddir *, sizeof(*idp), M_TEMP, M_WAITOK);
    idp = malloc(sizeof *idp, M_ISOFSMNT, M_WAITOK);
//	/*
//	 * XXX
//	 * Is it worth trying to figure out the type?
//	 */


//	if ((entryoffsetinblock = idp->curroff & bmask) &&
//	    (error = exfs_bufatoff(dp, (off_t)idp->curroff, NULL, &bp))) {
//		free(idp, M_TEMP, 0);
//		return (error);
//	}
//	endsearch = dp->i_size;
//
//	while (idp->curroff < endsearch) {
//		/*
//		 * If offset is on a block boundary,
//		 * read the next directory block.
//		 * Release previous if it exists.
//		 */
//		if ((idp->curroff & bmask) == 0) {
//			if (bp != NULL)
//				brelse(bp);
//			error = exfs_bufatoff(dp, (off_t)idp->curroff,
//					     NULL, &bp);
//			if (error)
//				break;
//			entryoffsetinblock = 0;
//		}
//		/*
//		 * Get pointer to next entry.
//		 */
//		ep = (struct iso_directory_record *)
//			((char *)bp->b_data + entryoffsetinblock);
//
//		reclen = isonum_711(ep->length);
//		if (reclen == 0) {
//			/* skip to next block, if any */
//			idp->curroff =
//			    (idp->curroff & ~bmask) + imp->logical_block_size;
//			continue;
//		}
//
//		if (reclen < ISO_DIRECTORY_RECORD_SIZE) {
//			error = EINVAL;
//			/* illegal entry, stop */
//			break;
//		}
//
//		if (entryoffsetinblock + reclen > imp->logical_block_size) {
//			error = EINVAL;
//			/* illegal directory, so stop looking */
//			break;
//		}
//
//		idp->current.d_namlen = isonum_711(ep->name_len);
//
//		if (reclen < ISO_DIRECTORY_RECORD_SIZE + idp->current.d_namlen) {
//			error = EINVAL;
//			/* illegal entry, stop */
//			break;
//		}
//
//		if (isonum_711(ep->flags)&2)
//			ino = isodirino(ep, imp);
//		else
//			ino = dbtob(bp->b_blkno) + entryoffsetinblock;
//
//		idp->curroff += reclen;
//
//		switch (imp->iso_ftype) {
//		case ISO_FTYPE_RRIP:
//			exfs_rrip_getname(ep,idp->current.d_name, &namelen,
//					   &ino, imp);
//			idp->current.d_fileno = ino;
//			idp->current.d_namlen = (u_char)namelen;
//			if (idp->current.d_namlen)
//				error = iso_uiodir(idp,&idp->current,idp->curroff);
//			break;
//		default:	/* ISO_FTYPE_DEFAULT || ISO_FTYPE_9660 */
//			idp->current.d_fileno = ino;
//			strlcpy(idp->current.d_name,"..",
//			    sizeof idp->current.d_name);
//			if (idp->current.d_namlen == 1 && ep->name[0] == 0) {
//				idp->current.d_namlen = 1;
//				error = iso_uiodir(idp,&idp->current,idp->curroff);
//			} else if (idp->current.d_namlen == 1 &&
//			    ep->name[0] == 1) {
//				idp->current.d_namlen = 2;
//				error = iso_uiodir(idp,&idp->current,idp->curroff);
//			} else {
//				isofntrans(ep->name,idp->current.d_namlen,
//					   idp->current.d_name, &namelen,
//					   imp->iso_ftype == ISO_FTYPE_9660,
//					   isonum_711(ep->flags) & 4,
//					   imp->joliet_level);
//				idp->current.d_namlen = (u_char)namelen;
//				if (imp->iso_ftype == ISO_FTYPE_DEFAULT)
//					error = iso_shipdir(idp);
//				else
//					error = iso_uiodir(idp,&idp->current,idp->curroff);
//			}
//		}
//		if (error)
//			break;
//
//		entryoffsetinblock += reclen;
//	}
//
//	if (!error && imp->iso_ftype == ISO_FTYPE_DEFAULT) {
//		idp->current.d_namlen = 0;
//		error = iso_shipdir(idp);
//	}
//	if (error < 0)
		error = 0;
//
//	if (bp)
//		brelse (bp);
//
	uio->uio_offset = idp->uio_off;
	*ap->a_eofflag = idp->eofflag;
//
    printf("dowli do free\n");
	free(idp, M_TEMP, 0);
//
	return (error);
}

/*
 * Return target name of a symbolic link
 * Shouldn't we get the parent vnode and read the data from there?
 * This could eventually result in deadlocks in exfs_lookup.
 * But otherwise the block read here is in the block buffer two times.
 */
typedef struct iso_directory_record ISODIR;
typedef struct iso_node             ISONODE;
typedef struct iso_mnt              ISOMNT;
int
exfs_readlink(void *v)
{
	struct vop_readlink_args *ap = v;
	ISONODE	*ip;
	ISODIR	*dirp;
	ISOMNT	*imp;
	struct	buf *bp;
	struct	uio *uio;
	u_short	symlen;
	int	error;
	char	*symname;

	ip  = VTOI(ap->a_vp);
	imp = ip->i_mnt;
	uio = ap->a_uio;

    printf("zawel v exfs_readlink\n");
	if (imp->iso_ftype != ISO_FTYPE_RRIP)
		return (EINVAL);

	/*
	 * Get parents directory record block that this inode included.
	 */
	error = bread(imp->im_devvp,
		      (ip->i_number >> imp->im_bshift) <<
		      (imp->im_bshift - DEV_BSHIFT),
		      imp->logical_block_size, &bp);
	if (error) {
		brelse(bp);
		return (EINVAL);
	}

	/*
	 * Setup the directory pointer for this inode
	 */
	dirp = (ISODIR *)(bp->b_data + (ip->i_number & imp->im_bmask));

	/*
	 * Just make sure, we have a right one....
	 *   1: Check not cross boundary on block
	 */
	if ((ip->i_number & imp->im_bmask) + isonum_711(dirp->length)
	    > imp->logical_block_size) {
		brelse(bp);
		return (EINVAL);
	}

	/*
	 * Now get a buffer
	 * Abuse a namei buffer for now.
	 */
	if (uio->uio_segflg == UIO_SYSSPACE &&
	    uio->uio_iov->iov_len >= MAXPATHLEN)
		symname = uio->uio_iov->iov_base;
	else
		symname = pool_get(&namei_pool, PR_WAITOK);

	/*
	 * Ok, we just gathering a symbolic name in SL record.
	 */
	if (exfs_rrip_getsymname(dirp, symname, &symlen, imp) == 0) {
		if (uio->uio_segflg != UIO_SYSSPACE ||
		    uio->uio_iov->iov_len < MAXPATHLEN)
			pool_put(&namei_pool, symname);
		brelse(bp);
		return (EINVAL);
	}
	/*
	 * Don't forget before you leave from home ;-)
	 */
	brelse(bp);

	/*
	 * return with the symbolic name to caller's.
	 */
	if (uio->uio_segflg != UIO_SYSSPACE ||
	    uio->uio_iov->iov_len < MAXPATHLEN) {
		error = uiomove(symname, symlen, uio);
		pool_put(&namei_pool, symname);
		return (error);
	}
	uio->uio_resid -= symlen;
	uio->uio_iov->iov_base = (char *)uio->uio_iov->iov_base + symlen;
	uio->uio_iov->iov_len -= symlen;
	return (0);
}

int
exfs_link(void *v)
{
	struct vop_link_args *ap = v;

    printf("zawel v exfs_link \n");
	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

int
exfs_symlink(void *v)
{
	struct vop_symlink_args *ap = v;

	printf("zawel v exfs_symlink\n");
	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

/*
 * Lock an inode.
 */
int
exfs_lock(void *v)
{
//	struct vop_lock_args *ap = v;
//	struct vnode *vp = ap->a_vp;
//    int temp_val;

//    printf("v1");
//    temp_val = 	lockmgr(&VTOI(vp)->i_lock, ap->a_flags, NULL);
//    printf("v2");
    return (0);
}

/*
 * Unlock an inode.
 */
int
exfs_unlock(void *v)
{
//	struct vop_unlock_args *ap = v;
//	struct vnode *vp = ap->a_vp;
//    int temp_val;

//    printf("u1");
//    temp_val = lockmgr(&VTOI(vp)->i_lock, LK_RELEASE, NULL);
//    printf("u2");
	return (0);
}

/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
 */
int
exfs_strategy(void *v)
{
	struct vop_strategy_args *ap = v;
	struct buf *bp = ap->a_bp;
	struct vnode *vp = bp->b_vp;
	struct iso_node *ip;
	int error;
	int s;

	printf("vowel v exfs_strategy\n");
	ip = VTOI(vp);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("exfs_strategy: spec");
	if (bp->b_blkno == bp->b_lblkno) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			s = splbio();
			biodone(bp);
			splx(s);
			return (error);
		}
		if ((long)bp->b_blkno == -1)
			clrbuf(bp);
	}
	if ((long)bp->b_blkno == -1) {
		s = splbio();
		biodone(bp);
		splx(s);
		return (0);
	}
	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	(vp->v_op->vop_strategy)(ap);
	return (0);
}

/*
 * Print out the contents of an inode.
 */
/*ARGSUSED*/
int
exfs_print(void *v)
{
	printf("tag VT_ISOFS, isofs vnode\n");
	return (0);
}

/*
 * Check for a locked inode.
 */
int
exfs_islocked(void *v)
{
	struct vop_islocked_args *ap = v;

	printf("zawel v exfs_islocked\n");
	return (lockstatus(&VTOI(ap->a_vp)->i_lock));
}

/*
 * Return POSIX pathconf information applicable to ex filesystems.
 */
int
exfs_pathconf(void *v)
{
	struct vop_pathconf_args *ap = v;
	int error = 0;

	printf("zawel v exfs_pathconf\n");
	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = 1;
		break;
	case _PC_NAME_MAX:
		if (VTOI(ap->a_vp)->i_mnt->iso_ftype == ISO_FTYPE_RRIP)
			*ap->a_retval = NAME_MAX;
		else
			*ap->a_retval = 37;
		break;
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		break;
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		break;
	case _PC_TIMESTAMP_RESOLUTION:
		*ap->a_retval = 1000000000;	/* one billion nanoseconds */
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/*
 * Global vfs data structures for isofs
 */
#define	exfs_create	eopnotsupp
#define	exfs_mknod	eopnotsupp
#define	exfs_write	eopnotsupp
#define	exfs_fsync	nullop
#define	exfs_remove	eopnotsupp
#define	exfs_rename	eopnotsupp
#define	exfs_mkdir	eopnotsupp
#define	exfs_rmdir	eopnotsupp
#define	exfs_advlock	eopnotsupp
#define	exfs_valloc	eopnotsupp
#define	exfs_vfree	eopnotsupp
#define	exfs_truncate	eopnotsupp
#define	exfs_update	eopnotsupp
#define	exfs_bwrite	eopnotsupp
#define exfs_revoke   vop_generic_revoke

/* Global vfs data structures for ex. */
struct vops exfs_vops = {
	.vop_lookup	= exfs_lookup,
	.vop_create	= exfs_create,
	.vop_mknod	= exfs_mknod,
	.vop_open	= exfs_open,
	.vop_close	= exfs_close,
	.vop_access	= exfs_access,
	.vop_getattr	= exfs_getattr,
	.vop_setattr	= exfs_setattr,
	.vop_read	= exfs_read,
	.vop_write	= exfs_write,
	.vop_ioctl	= exfs_ioctl,
	.vop_poll	= exfs_poll,
	.vop_kqfilter	= exfs_kqfilter,
	.vop_revoke	= exfs_revoke,
	.vop_fsync	= exfs_fsync,
	.vop_remove	= exfs_remove,
	.vop_link	= exfs_link,
	.vop_rename	= exfs_rename,
	.vop_mkdir	= exfs_mkdir,
	.vop_rmdir	= exfs_rmdir,
	.vop_symlink	= exfs_symlink,
	.vop_readdir	= exfs_readdir,
	.vop_readlink	= NULL,
	.vop_abortop	= vop_generic_abortop,
	.vop_inactive	= exfs_inactive,
	.vop_reclaim	= exfs_reclaim,
	.vop_lock	= exfs_lock,
	.vop_unlock	= exfs_unlock,
	.vop_bmap	= exfs_bmap,
	.vop_strategy	= exfs_strategy,
	.vop_print	= exfs_print,
	.vop_islocked	= exfs_islocked,
	.vop_pathconf	= exfs_pathconf,
	.vop_advlock	= exfs_advlock,
	.vop_bwrite	= vop_generic_bwrite
};

/* Special device vnode ops */
struct vops exfs_specvops = {
	.vop_access	= exfs_access,
	.vop_getattr	= exfs_getattr,
	.vop_setattr	= exfs_setattr,
	.vop_inactive	= exfs_inactive,
	.vop_reclaim	= exfs_reclaim,
	.vop_lock	= exfs_lock,
	.vop_unlock	= exfs_unlock,
	.vop_print	= exfs_print,
	.vop_islocked	= exfs_islocked,

	/* XXX: Keep in sync with spec_vops. */
	.vop_lookup	= vop_generic_lookup,
	.vop_create	= spec_badop,
	.vop_mknod	= spec_badop,
	.vop_open	= spec_open,
	.vop_close	= spec_close,
	.vop_read	= spec_read,
	.vop_write	= spec_write,
	.vop_ioctl	= spec_ioctl,
	.vop_poll	= spec_poll,
	.vop_kqfilter	= spec_kqfilter,
	.vop_revoke	= vop_generic_revoke,
	.vop_fsync	= spec_fsync,
	.vop_remove	= spec_badop,
	.vop_link	= spec_badop,
	.vop_rename	= spec_badop,
	.vop_mkdir	= spec_badop,
	.vop_rmdir	= spec_badop,
	.vop_symlink	= spec_badop,
	.vop_readdir	= spec_badop,
	.vop_readlink	= spec_badop,
	.vop_abortop	= spec_badop,
	.vop_bmap	= vop_generic_bmap,
	.vop_strategy	= spec_strategy,
	.vop_pathconf	= spec_pathconf,
	.vop_advlock	= spec_advlock,
	.vop_bwrite	= vop_generic_bwrite,
};

#ifdef FIFO
struct vops exfs_fifovops = {
	.vop_access	= exfs_access,
	.vop_getattr	= exfs_getattr,
	.vop_setattr	= exfs_setattr,
	.vop_inactive	= exfs_inactive,
	.vop_reclaim	= exfs_reclaim,
	.vop_lock	= exfs_lock,
	.vop_unlock	= exfs_unlock,
	.vop_print	= exfs_print,
	.vop_islocked	= exfs_islocked,
	.vop_bwrite	= vop_generic_bwrite,

	/* XXX: Keep in sync with fifo_vops. */
	.vop_lookup	= vop_generic_lookup,
	.vop_create	= fifo_badop,
	.vop_mknod	= fifo_badop,
	.vop_open	= fifo_open,
	.vop_close	= fifo_close,
	.vop_read	= fifo_read,
	.vop_write	= fifo_write,
	.vop_ioctl	= fifo_ioctl,
	.vop_poll	= fifo_poll,
	.vop_kqfilter	= fifo_kqfilter,
	.vop_revoke	= vop_generic_revoke,
	.vop_fsync	= nullop,
	.vop_remove	= fifo_badop,
	.vop_link	= fifo_badop,
	.vop_rename	= fifo_badop,
	.vop_mkdir	= fifo_badop,
	.vop_rmdir	= fifo_badop,
	.vop_symlink	= fifo_badop,
	.vop_readdir	= fifo_badop,
	.vop_readlink	= fifo_badop,
	.vop_abortop	= fifo_badop,
	.vop_bmap	= vop_generic_bmap,
	.vop_strategy	= fifo_badop,
	.vop_pathconf	= fifo_pathconf,
	.vop_advlock	= fifo_advlock,
};
#endif /* FIFO */

void filt_exdetach(struct knote *kn);
int filt_exread(struct knote *kn, long hint);
int filt_exwrite(struct knote *kn, long hint);
int filt_exvnode(struct knote *kn, long hint);

struct filterops exread_filtops =
	{ 1, NULL, filt_exdetach, filt_exread };
struct filterops exwrite_filtops =
	{ 1, NULL, filt_exdetach, filt_exwrite };
struct filterops exvnode_filtops =
	{ 1, NULL, filt_exdetach, filt_exvnode };

int
exfs_kqfilter(void *v)
{
	struct vop_kqfilter_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct knote *kn = ap->a_kn;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &exread_filtops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &exwrite_filtops;
		break;
	case EVFILT_VNODE:
		kn->kn_fop = &exvnode_filtops;
		break;
	default:
		return (EINVAL);
	}

	kn->kn_hook = (caddr_t)vp;

	SLIST_INSERT_HEAD(&vp->v_selectinfo.si_note, kn, kn_selnext);

	return (0);
}

void
filt_exdetach(struct knote *kn)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	SLIST_REMOVE(&vp->v_selectinfo.si_note, kn, knote, kn_selnext);
}

int
filt_exread(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	struct iso_node *node = VTOI(vp);

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		return (1);
	}

	kn->kn_data = node->i_size - kn->kn_fp->f_offset;
	if (kn->kn_data == 0 && kn->kn_sfflags & NOTE_EOF) {
		kn->kn_fflags |= NOTE_EOF;
		return (1);
	}

	return (kn->kn_data != 0);
}

int
filt_exwrite(struct knote *kn, long hint)
{
	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		return (1);
	}

	kn->kn_data = 0;
	return (1);
}

int
filt_exvnode(struct knote *kn, long hint)
{
	if (kn->kn_sfflags & hint)
		kn->kn_fflags |= hint;
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= EV_EOF;
		return (1);
	}
	return (kn->kn_fflags != 0);
}
