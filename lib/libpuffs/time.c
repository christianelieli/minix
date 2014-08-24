/* Created (MFS based):
 *   February 2010 (Evgeniy Ivanov)
 */

#include "fs.h"
#include "puffs.h"
#include "puffs_priv.h"


/*===========================================================================*
 *				fs_utime				     *
 *===========================================================================*/
int fs_utime(ino_t ino_nr, struct timespec *atime, struct timespec *mtime)
{
  struct puffs_node *pn;
  struct vattr va;
  PUFFS_MAKECRED(pcr, &global_kcred);

  if (global_pu->pu_ops.puffs_node_setattr == NULL)
	return(EINVAL);

  if( (pn = puffs_pn_nodewalk(global_pu, 0, &ino_nr)) == NULL)
        return(EINVAL);

  /* FIXME: shouldn't this check the special UTIME_ values? */
  puffs_vattr_null(&va);
  va.va_atime = *atime;
  va.va_mtime = *mtime;
  va.va_ctime = clock_timespec();

  if (global_pu->pu_ops.puffs_node_setattr(global_pu, pn, &va, pcr) != 0)
	return(EINVAL);

  return(OK);
}
