/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "io_worker.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(io_worker, CONFIG_IO_WORKER_LOG_LEVEL);

int IoWorker::Init()
{
	k_work_queue_init(&mQueue);

	const k_work_queue_config config = {
		.name             = "io_worker",
		.no_yield         = false,
		.essential        = false,
		.work_timeout_ms  = 0,
	};

	k_work_queue_start(&mQueue, mStack,
	                   K_KERNEL_STACK_SIZEOF(mStack),
	                   CONFIG_APP_IO_WORKQUEUE_PRIORITY, &config);

	LOG_INF("IoWorker initialised (prio=%d, stack=%u)",
	        CONFIG_APP_IO_WORKQUEUE_PRIORITY,
	        (unsigned)CONFIG_APP_IO_WORKQUEUE_STACK_SIZE);
	return 0;
}
