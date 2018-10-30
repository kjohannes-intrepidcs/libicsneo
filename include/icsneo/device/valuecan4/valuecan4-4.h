#ifndef __VALUECAN4_4_H_
#define __VALUECAN4_4_H_

#include "icsneo/device/valuecan4/valuecan4.h"
#include "icsneo/device/valuecan4/settings/valuecan4-4settings.h"
#include <string>

namespace icsneo {

class ValueCAN4_4 : public ValueCAN4 {
public:
	// Serial numbers start with V4 for 4-4
	static constexpr DeviceType::Enum DEVICE_TYPE = DeviceType::VCAN4_4;
	static std::vector<std::shared_ptr<Device>> Find() {
		std::vector<std::shared_ptr<Device>> found;

		for(auto neodevice : STM32::FindByProduct(PRODUCT_ID)) {
			if(std::string(neodevice.serial).substr(0, 2) == "V4")
				found.emplace_back(new ValueCAN4_4(neodevice));
		}

		return found;
	}

private:
	ValueCAN4_4(neodevice_t neodevice) : ValueCAN4(neodevice) {
		initialize<STM32, ValueCAN4_4Settings>();
		getWritableNeoDevice().type = DEVICE_TYPE;
	}
};

}

#endif