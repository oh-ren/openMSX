// $Id$

#ifndef CARTRIDGESLOTMANAGER_HH
#define CARTRIDGESLOTMANAGER_HH

namespace openmsx {

class HardwareConfig;
class MSXCPUInterface;

class CartridgeSlotManager
{
public:
	static CartridgeSlotManager& instance();

	void reserveSlot(int slot);
	void readConfig();
	void getSlot(int slot, int& ps, int& ss);
	void getSlot(int& ps, int& ss);
	void getSlot(int& ps);

	static int getSlotNum(const std::string& slot);

private:
	CartridgeSlotManager();
	~CartridgeSlotManager();
	void createExternal(unsigned ps);
	void createExternal(unsigned ps, unsigned ss);

	int slots[16];
	int slotCounter;
	HardwareConfig& hardwareConfig;
	MSXCPUInterface& cpuInterface;
};

} // namespace openmsx

#endif
