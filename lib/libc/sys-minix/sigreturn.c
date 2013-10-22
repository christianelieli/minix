#include "namespace.h"

#include <sys/cdefs.h>
#include <lib.h>

#include <signal.h>
#include <string.h>
#include <sys/signal.h>
#include <machine/signal.h>

#ifdef __weak_alias
__weak_alias(sigreturn, _sigreturn)
#endif

int sigreturn(struct sigcontext *scp)
{
  sigset_t set;

  /* The message can't be on the stack, because the stack will vanish out
   * from under us.  The send part of sendrec will succeed, but when
   * a message is sent to restart the current process, who knows what will
   * be in the place formerly occupied by the message?
   */
  static message m;

  /* Protect against race conditions by blocking all interrupts. */
  sigfillset(&set);		/* splhi */
  sigprocmask(SIG_SETMASK, &set, (sigset_t *) NULL);

  m.SIG_MAP = scp->sc_mask;
  m.SIG_FLAGS = scp->sc_flags;
  m.SIG_CTXT_PTR = (char *) scp;
  return(_syscall(PM_PROC_NR, SIGRETURN, &m));	/* normally this doesn't return */
}
