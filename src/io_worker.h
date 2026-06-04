/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#include <zephyr/kernel.h>

/// Owns a dedicated Zephyr workqueue and its thread, intended for slow
/// synchronous I/O (I2C sensor reads, SPI display refreshes) that must not
/// block the CHIP/Matter task or the system workqueue. Substrate only: holds
/// no measurement- or display-specific state.
class IoWorker {
public:
	static IoWorker &Instance()
	{
		static IoWorker sInstance;
		return sInstance;
	}

	/// Starts the worker thread. Safe to call once; do not re-invoke.
	/// Returns 0 on success or a negative errno on failure.
	int Init();

	/// Returns the queue handle so callers can submit work via
	/// k_work_submit_to_queue().
	k_work_q &Queue() { return mQueue; }

private:
	IoWorker() = default;

	k_work_q mQueue{};
	K_KERNEL_STACK_MEMBER(mStack, CONFIG_APP_IO_WORKQUEUE_STACK_SIZE);
};
