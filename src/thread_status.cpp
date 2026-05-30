/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "thread_status.h"

#include <platform/CHIPDeviceLayer.h>
#include <openthread/link.h>
#include <openthread/thread.h>

using namespace ::chip::DeviceLayer;

std::tuple<bool, uint8_t> GetThreadConnectivity()
{
	bool connected = false;
	uint8_t lqi = 0;
	ThreadStackMgr().LockThreadStack();
	otInstance *ot = ThreadStackMgrImpl().OTInstance();
	const otDeviceRole role = otThreadGetDeviceRole(ot);
	connected = (role != OT_DEVICE_ROLE_DISABLED && role != OT_DEVICE_ROLE_DETACHED);
	if (role == OT_DEVICE_ROLE_CHILD) {
		int8_t rssi = 0;
		if (otThreadGetParentAverageRssi(ot, &rssi) == OT_ERROR_NONE) {
			lqi = otLinkConvertRssToLinkQuality(ot, rssi);
		}
	} else if (IS_ENABLED(CONFIG_OPENTHREAD_FTD) &&
		   (role == OT_DEVICE_ROLE_ROUTER || role == OT_DEVICE_ROLE_LEADER)) {
		otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
		otNeighborInfo info;
		while (otThreadGetNextNeighborInfo(ot, &iter, &info) == OT_ERROR_NONE) {
			if (info.mLinkQualityIn > lqi) {
				lqi = info.mLinkQualityIn;
			}
		}
	}
	ThreadStackMgr().UnlockThreadStack();
	return {connected, lqi};
}
