// $Id$

// This class implements a 16 bit signed DAC

#ifndef __DACSOUND16S_HH__
#define __DACSOUND16S_HH__

#include <list>
#include <string>
#include "openmsx.hh"
#include "SoundDevice.hh"
#include "EmuTime.hh"

using std::list;
using std::string;

namespace openmsx {

class DACSound16S : public SoundDevice
{
public:
	DACSound16S(const string& name, const string& desc,
		    short maxVolume, const EmuTime &time); 
	virtual ~DACSound16S();

	void reset(const EmuTime& time);
	void writeDAC(short value, const EmuTime& time);
	
	// SoundDevice
	virtual const string& getName() const;
	virtual const string& getDescription() const;
	virtual void setVolume(int newVolume);
	virtual void setSampleRate(int sampleRate);
	virtual int* updateBuffer(int length) throw();
	
private:
	inline int getSample(const EmuTime& time);
	
	struct Sample {
		Sample(int value_, const EmuTime& time_)
			: value(value_), time(time_) {}
		int value;
		EmuTime time;
	};
	list<Sample> samples;

	float oneSampDur;
	int lastValue;
	short lastWrittenValue;
	EmuTime lastTime;
	EmuTime nextTime;
	int volume;
	int* buffer;

	const string name;
	const string desc;

	class MSXCPU& cpu;
	class RealTime& realTime;
};

} // namespace openmsx

#endif
