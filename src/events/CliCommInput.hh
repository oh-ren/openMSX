// $Id$

#ifndef CLICOMMINPUT_HH
#define CLICOMMINPUT_HH

#include <deque>
#include <string>
#include <libxml/parser.h>
#include "Thread.hh"
#include "Schedulable.hh"
#include "Semaphore.hh"
#include "CommandLineParser.hh"

namespace openmsx {

class CliCommInput : private Runnable, private Schedulable
{
public:
	CliCommInput(CommandLineParser::ControlType type,
	             const std::string& arguments);
	virtual ~CliCommInput();

private:
	void execute(const std::string& command);
	virtual void executeUntil(const EmuTime& time, int userData);
	virtual const std::string& schedName() const;
	virtual void run();

	std::deque<std::string> cmds;
	Semaphore lock;
	Thread thread;
	CommandLineParser::ControlType ioType;
	std::string ioArguments;
	

	enum State {
		START, TAG_OPENMSX, TAG_COMMAND
	};
	struct ParseState {
		ParseState();
		State state;
		unsigned unknownLevel;
		std::string content;
		CliCommInput* object;
	};

	static ParseState user_data;

	static void cb_start_element(ParseState* user_data, const xmlChar* name,
	                             const xmlChar** attrs);
	static void cb_end_element(ParseState* user_data, const xmlChar* name);
	static void cb_text(ParseState* user_data, const xmlChar* chars, int len);
};

} // namespace openmsx

#endif
