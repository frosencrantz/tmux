/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Tiago Cunha <me@tiagocunha.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>

#include "tmux.h"

/*
 * Sources a configuration file.
 */

static enum cmd_retval	cmd_source_file_exec(struct cmd *, struct cmd_q *);

static void		cmd_source_file_done(struct cmd_q *);

const struct cmd_entry cmd_source_file_entry = {
	.name = "source-file",
	.alias = "source",

	.args = { "q", 1, 1 },
	.usage = "[-q] path",

	.flags = 0,
	.exec = cmd_source_file_exec
};

static enum cmd_retval
cmd_source_file_exec(struct cmd *self, struct cmd_q *cmdq)
{
	struct args	*args = self->args;
	struct cmd_q	*cmdq1;
	int		 quiet;

	cmdq1 = cmdq_new(cmdq->client);
	cmdq1->flags |= cmdq->flags & CMD_Q_NOHOOKS;
	cmdq1->emptyfn = cmd_source_file_done;
	cmdq1->data = cmdq;

	quiet = args_has(args, 'q');
	switch (load_cfg(args->argv[0], cmdq1, quiet)) {
	case -1:
		cmdq_free(cmdq1);
		if (cfg_references == 0) {
			cfg_print_causes(cmdq);
			return (CMD_RETURN_ERROR);
		}
		return (CMD_RETURN_NORMAL);
	case 0:
		cmdq_free(cmdq1);
		if (cfg_references == 0)
			cfg_print_causes(cmdq);
		return (CMD_RETURN_NORMAL);
	}

	cmdq->references++;
	cfg_references++;

	cmdq_continue(cmdq1);
	return (CMD_RETURN_WAIT);
}

static void
cmd_source_file_done(struct cmd_q *cmdq1)
{
	struct cmd_q	*cmdq = cmdq1->data;

	if (cmdq1->client_exit >= 0)
		cmdq->client_exit = cmdq1->client_exit;

	cmdq_free(cmdq1);

	cfg_references--;

	if (cmdq_free(cmdq))
		return;

	if (cfg_references == 0)
		cfg_print_causes(cmdq);
	cmdq_continue(cmdq);
}
