// $Id$

#ifndef MSXFDC_HH
#define MSXFDC_HH

#include "MSXDevice.hh"
#include <memory>

namespace openmsx {

class Rom;
class DiskDrive;

class MSXFDC : public MSXDevice
{
public:
	virtual void powerDown(const EmuTime& time);
	virtual byte readMem(word address, const EmuTime& time);
	virtual const byte* getReadCacheLine(word start) const;

protected:
	MSXFDC(const XMLElement& config, const EmuTime& time);
	virtual ~MSXFDC();

	std::auto_ptr<Rom> rom;
	std::auto_ptr<DiskDrive> drives[4];
};

} // namespace openmsx

#endif
