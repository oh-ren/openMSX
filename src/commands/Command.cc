// $Id$

#include "Command.hh"
#include "CommandController.hh"
#include "TclObject.hh"

using std::vector;
using std::string;

namespace openmsx {

// class CommandCompleter

CommandCompleter::CommandCompleter(CommandController& commandController_,
                                   const string& name)
	: Completer(name)
	, commandController(commandController_)
{
	if (!getName().empty()) {
		getCommandController().registerCompleter(*this, getName());
	}
}

CommandCompleter::~CommandCompleter()
{
	if (!getName().empty()) {
		getCommandController().unregisterCompleter(*this, getName());
	}
}

CommandController& CommandCompleter::getCommandController() const
{
	return commandController;
}


// class Command

Command::Command(CommandController& commandController, const string& name)
	: CommandCompleter(commandController, name)
{
	init();
}

Command::Command(CommandController& commandController, const char* name)
	: CommandCompleter(commandController, name)
{
	init();
}

void Command::init()
{
	if (!getName().empty()) {
		getCommandController().registerCommand(*this, getName());
	}
}

Command::~Command()
{
	if (!getName().empty()) {
		getCommandController().unregisterCommand(*this, getName());
	}
}

void Command::tabCompletion(vector<string>& /*tokens*/) const
{
	// do nothing
}


// class SimpleCommand

SimpleCommand::SimpleCommand(CommandController& commandController,
                             const string& name)
	: Command(commandController, name)
{
}

SimpleCommand::SimpleCommand(CommandController& commandController,
                             const char* name)
	: Command(commandController, name)
{
}

SimpleCommand::~SimpleCommand()
{
}

void SimpleCommand::execute(const vector<TclObject*>& tokens,
                            TclObject& result)
{
	vector<string> strings;
	strings.reserve(tokens.size());
	for (vector<TclObject*>::const_iterator it = tokens.begin();
	     it != tokens.end(); ++it) {
		strings.push_back((*it)->getString());
	}
	result.setString(execute(strings));
}

} // namespace openmsx
