/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* See https://github.com/lulle2007200/emuMMC/blob/internal-emummc/source/ */

#include "../nx/smc.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>

#define IRAM_LOG_CTX_ADDR 0x4003C000
#define IRAM_LOG_MAGIC    0xaabbccdd

/* The secure monitor maps one page for the copy, so this must be page aligned. */
_Alignas(4096) static u8 working_buf[4096];

typedef struct _log_ctx_t {
    u32 magic;
    u32 sz;
    u32 start;
    u32 end;
    char buf[];
} log_ctx_t;

static bool init_done = false;
static Result smc_result = 0;

[[noreturn]] static void smcRebootToFatalError(void) {
    SecmonArgs args;
    args.X[0] = 0xC3000401;                /* smcSetConfig */
    args.X[1] = SplConfigItem_NeedsReboot; /* Exosphere reboot */
    args.X[3] = 3;                         /* UserRebootType_ToFatalError */
    svcCallSecureMonitor(&args);

    while (true)
        ;
}

void Log(const char *data, ...) {
    static const u32 max_log_sz = sizeof(working_buf) - sizeof(log_ctx_t);
    log_ctx_t *log_ctx = (log_ctx_t *)working_buf;

    if (smc_result != 0) {
        return;
    }

    smc_result = smcCopyFromIram(working_buf, IRAM_LOG_CTX_ADDR, sizeof(working_buf));
    if (smc_result != 0) {
        return;
    }

    if (!init_done) {
        init_done = true;
        log_ctx->magic  = IRAM_LOG_MAGIC;
        log_ctx->sz     = max_log_sz;
        log_ctx->start  = 0;
        log_ctx->end    = 0;
        log_ctx->buf[0] = '\0';
    }

    va_list args;
    va_start(args, data);
    s32 res = vsnprintf(log_ctx->buf + log_ctx->end, max_log_sz - log_ctx->end, data, args);
    va_end(args);

    if (res > 0 && res < (s32)(max_log_sz - log_ctx->end)) {
        log_ctx->end += res;
    }
    log_ctx->buf[log_ctx->end] = '\0';

    smc_result = smcCopyToIram(IRAM_LOG_CTX_ADDR, working_buf, sizeof(working_buf));
}

[[noreturn]] void ViewLog() {
    smcRebootToFatalError();
}
