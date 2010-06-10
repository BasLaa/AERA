//	preprocessor.cpp
//
//	Author: Thor List, Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Thor List, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Thor List or Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include	"preprocessor.h"
#include	"compiler.h"
#include	"utils.h"


namespace	r_comp{

UNORDERED_MAP<std::string,RepliMacro	*>	RepliStruct::RepliMacros;
UNORDERED_MAP<std::string,int32>			RepliStruct::Counters;
std::list<RepliCondition	*>				RepliStruct::Conditions;
uint32										RepliStruct::GlobalLine=1;

RepliStruct::RepliStruct(RepliStruct::Type type) {
	this->type = type;
	line = GlobalLine;
	parent = NULL;
}

RepliStruct::~RepliStruct() {
	parent = NULL;
}

uint32	RepliStruct::getIndent(std::istream	*stream){

	uint32	count=0;
	while (!stream->eof()) {
	switch(stream->get()) {
		case 10:
			GlobalLine++;
		case 13:
			stream->seekg(-1, std::ios_base::cur);
			return count/3;
		case ' ':
			count++;
			break;
		default:
			stream->seekg(-1, std::ios_base::cur);
			return count/3;
	}
	}
	return	count/3;
}

int32	RepliStruct::parse(std::istream *stream,uint32	&curIndent,uint32	&prevIndent,int32	paramExpect){

	char c = 0, lastc = 0, lastcc, tc;
	std::string str, label;
	RepliStruct* subStruct;

	int32	paramCount = 0;
	int32	returnIndent = 0;

	bool inComment = false;
	bool expectSet = false;

	while (!stream->eof()) {
		lastcc = lastc;
		lastc = c;
		c = stream->get();
	//	printf("%c", c);
		switch(c) {
			case '\t':
				if (inComment) continue; // allow tabs in comments, does not matter anyway
				line = GlobalLine;
				error += "Tabs chars are not permitted. ";
				return -1;
			case '!':
				if (inComment) continue;
				if (this->type == Root) {
					subStruct = new RepliStruct(Directive);
					subStruct->parent = this;
					args.push_back(subStruct);
					if (!subStruct->parseDirective(stream, curIndent, prevIndent))
						return -1;
				}
				else {
					error += "Directive not allowed inside a structure. ";
					return -1;
				}
				break;
			case 10:
				GlobalLine++;
			case 13:
				// remain inComment?
				if (inComment) {
					if ((lastc == ' ') || ( (lastc == '.') && (lastcc == '.') ) )
						continue; // continue comment on next line
					else
						inComment = false; // end comment
				}
				// skip all CR and LF
				while ((!stream->eof()) && (stream->peek() < 32))
					if (stream->get() == 10)
						GlobalLine++;
				// get indents
				prevIndent = curIndent;
				curIndent = getIndent(stream);
				// Are we in a parenthesis or set
				if (curIndent > prevIndent) {
					// Are we in a set
					if (expectSet) {
						// Add new sub structure
						subStruct = new RepliStruct(Set);
						subStruct->parent = this;
						subStruct->label = label;
						label = "";
						args.push_back(subStruct);
						returnIndent = subStruct->parse(stream, curIndent, prevIndent);
						expectSet = false;
						if (returnIndent < 0)
							return -1;
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
						if (returnIndent > 0)
							return (returnIndent - 1);
					}
					// or a parenthesis
					else {
						subStruct = new RepliStruct(Structure);
						args.push_back(subStruct);
						returnIndent = subStruct->parse(stream, curIndent, prevIndent);
						expectSet = false;
						if (returnIndent < 0)
							return -1;
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
						if (returnIndent > 0)
							return (returnIndent - 1);
					}
				}
				else if (curIndent < prevIndent) {
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
					// current structure or set is complete
					return prevIndent - curIndent - 1;
				}
				else {
					// act as if we met a space
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set) || (type == Root)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
				}
				break;
			case ';':
				inComment = true;
				break;
			case ' ':
				if (inComment) continue;
				// next string is ready
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)	||	(type==Development)) {	//	Modification from Eric to have the Development treat tpl vars as atoms instead of a Development.cmd
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				break;
			case '(':
				if (inComment) continue;
				// Check for scenario 'xxx('
				if (str.size() > 0) {
					if (lastc == ':') { // label:(xxx)
						label = str;
						str = "";
					}
					else if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				subStruct = new RepliStruct(Structure);
				subStruct->label = label;
				label = "";
				subStruct->parent = this;
				args.push_back(subStruct);
				returnIndent = subStruct->parse(stream, curIndent, prevIndent);
				if (returnIndent < 0)
					return -1;
				if ((paramExpect > 0) && (++paramCount == paramExpect))
					return 0;
				if (returnIndent > 0)
					return (returnIndent - 1);
				break;
			case ')':
				if (inComment) continue;
				// Check for directive use of xxx):xxx or xxx):
				if (stream->peek() == ':') {
					// expect ':' or ':xxx'
					while ( (!stream->eof()) && ((c = stream->get()) > 32) )
						tail += c;
				}
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			case '{':
				if (inComment) continue;
				// Check for scenario 'xxx{'
				if (str.size() > 0) {
					if (lastc == ':') { // label:{xxx}
						label = str;
						str = "";
					}
					else if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				subStruct = new RepliStruct(Development);
				subStruct->label = label;
				label = "";
				subStruct->parent = this;
				args.push_back(subStruct);
				returnIndent = subStruct->parse(stream, curIndent, prevIndent);
				if (returnIndent < 0)
					return -1;
				if ((paramExpect > 0) && (++paramCount == paramExpect))
					return 0;
				if (returnIndent > 0)
					return (returnIndent - 1);
				break;
			case '}':
				if (inComment) continue;
				// Check for directive use of xxx):xxx or xxx):
				if (stream->peek() == ':') {
					// expect ':' or ':xxx'
					while ( (!stream->eof()) && ((c = stream->get()) > 32) )
						tail += c;
				}
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)	||	(type==Development)) {	//	Modification from Eric to have the Development treat tpl vars as atoms instead of a Development.cmd
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			case '|':
				if (inComment) continue;
				if (stream->peek() == '[') {
					stream->get(); // read the [
					stream->get(); // read the ]
					str += "|[]";
				}
				else
					str += c;
				break;
			case '[': // set start
				if (inComment) continue;
				if (lastc == ':') { // label:[xxx]
					label = str;
					str = "";
				}
				if (stream->peek() == ']') {
					stream->get(); // read the ]
					// if we have a <CR> or <LF> next we may have a line indent set
					if ( ((tc=stream->peek()) < 32) || (tc == ';') )
						expectSet = true;
					else {
						// this could be a xxx:[] or xxx[]
						if ((lastc != ':') && (lastc > 32)) { // label[]
							// act as if [] is part of the string and continue
							str += "[]";
							continue;
						}
						// create empty set
						subStruct = new RepliStruct(Set);
						subStruct->parent = this;
						subStruct->label = label;
						args.push_back(subStruct);
					}
				}
				else {
					// Check for scenario 'xxx['
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
					subStruct = new RepliStruct(Set);
					subStruct->parent = this;
					subStruct->label = label;
					label = "";
					args.push_back(subStruct);
					returnIndent = subStruct->parse(stream, curIndent, prevIndent);
					if (returnIndent < 0)
						return -1;
					if ((paramExpect > 0) && (++paramCount == paramExpect))
						return 0;
					if (returnIndent > 0)
						return (returnIndent - 1);
				}
				break;
			case ']':
				if (inComment) continue;
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			default:
				if (inComment) continue;
				str += c;
				break;
		}
	}
	return 0;
}


bool	RepliStruct::parseDirective(std::istream	*stream,uint32	&curIndent,uint32	&prevIndent){

	std::string str = "!";
//	RepliStruct* subStruct;
	char c;

	// We know that parent read a '!', so first find out which directive
	while ( (!stream->eof()) && ((c = stream->peek()) > 32) )
		str += stream->get();
	if (stream->eof()) {
		error += "Error in directive formatting, end of file reached unexpectedly. ";
		return false;
	}

	unsigned int paramCount = 0;

	if (str.compare("!def") == 0) // () ()
		paramCount = 2;
	else if (str.compare("!counter") == 0) // xxx val
		paramCount = 2;
	else if (str.compare("!undef") == 0) // xxx
		paramCount = 1;
	else if (str.compare("!ifdef") == 0) { // name
		this->type = Condition;
		paramCount = 1;
	}
	else if (str.compare("!ifundef") == 0) { // name
		this->type = Condition;
		paramCount = 1;
	}
	else if (str.compare("!else") == 0) { //
		this->type = Condition;
		paramCount = 0;
	}
	else if (str.compare("!endif") == 0) { //
		this->type = Condition;
		paramCount = 0;
	}
	else if (str.compare("!class") == 0) // ()
		paramCount = 1;
	else if (str.compare("!op") == 0) // ():xxx
		paramCount = 1;
	else if (str.compare("!dfn") == 0) // ()
		paramCount = 1;
	else if (str.compare("!load") == 0) // xxx
		paramCount = 1;
	else {
		error += "Unknown directive: '" + str + "'. ";
		return false;
	}
	cmd = str;

	if (paramCount == 0) {
		// read until end of line, including any comments
		while ( (!stream->eof()) && (stream->peek() > 13) )
			stream->get();
		// read the end of line too
		while ( (!stream->eof()) && (stream->peek() < 32) )
			if (stream->get() == 10)
				GlobalLine++;
		return true;
	}

	if (parse(stream, curIndent, prevIndent, paramCount) != 0) {
		error += "Error parsing the arguments for directive '" + cmd + "'. ";
		return false;
	}
	else
		return true;
}


int32	RepliStruct::process(){

	int32	changes=0,count;
	RepliStruct *structure, *newStruct, *tempStruct;
	RepliMacro* macro;
	RepliCondition* cond;
	std::string loadError;

	// expand Counters in all structures
	if (Counters.find(cmd) != Counters.end()) {
		// expand the counter
		cmd = String::Int2String(Counters[cmd]++);
		changes++;
	}
	// expand Macros in all structures
	if (RepliMacros.find(cmd) != RepliMacros.end()) {
		// expand the macro
		macro = RepliMacros[cmd];
		newStruct = macro->expandMacro(this);
		if (newStruct != NULL) {
			*this = *newStruct;
			delete(newStruct);
			changes++;
		}
		else {
			error = macro->error;
			macro->error = "";
			return -1;
		}
	}

	if (args.size() == 0)
		return changes;

	for (std::list<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
		structure = (*iter);

		// printf("Processing %s with %d args...\n", structure->cmd.c_str(), structure->args.size());
		if (structure->type == Condition) {
			if (structure->cmd.compare("!ifdef") == 0) {
				cond = new RepliCondition(structure->args.front()->cmd, false);
				Conditions.push_back(cond);
			}
			else if (structure->cmd.compare("!ifundef") == 0) {
				cond = new RepliCondition(structure->args.front()->cmd, true);
				Conditions.push_back(cond);
			}
			else if (structure->cmd.compare("!else") == 0) {
				// reverse the current condition
				Conditions.back()->reverse();
			}
			else if (structure->cmd.compare("!endif") == 0) {
				Conditions.pop_back();
			}
			return 0;
		}

		// Check Conditions to see if we are active at the moment
		for (std::list<RepliCondition*>::const_iterator iCon(Conditions.begin()), iConEnd(Conditions.end()); iCon != iConEnd; ++iCon) {
			// if just one active condition is not active we will ignore the current line
			// until we get an !else or !endif
			if (!((*iCon)->isActive(RepliMacros, Counters)))
				return 0;
		}

		if (structure->type == Directive) {
			if (structure->cmd.compare("!counter") == 0) {
				// register the counter
				if (structure->args.size() > 1)
					Counters[structure->args.front()->cmd] = atoi(structure->args.back()->cmd.c_str());
				else
					Counters[structure->args.front()->cmd] = 0;
			}
			else if (structure->cmd.compare("!def") == 0) {
				// check second sub structure only containing macros
				while ( (count = structure->args.back()->process()) > 0 )
					changes += count;
				if (count < 0)
					return -1;
				// register the macro
				macro = new RepliMacro(structure->args.front()->cmd, structure->args.front(), structure->args.back());
				RepliMacros[macro->name] = macro;
			}
			else if (structure->cmd.compare("!undef") == 0) {
				// remove the counter or macro
				RepliMacros.erase(RepliMacros.find(structure->args.front()->cmd));
				Counters.erase(Counters.find(structure->args.front()->cmd));
			}
			else if (structure->cmd.compare("!load") == 0) {
				// Check for a load directive...
				newStruct = loadReplicodeFile(structure->args.front()->cmd);
				if (newStruct == NULL) {
					structure->error += "Load: File '" + structure->args.front()->cmd + "' cannot be read! ";
					return -1;
				}
				else if ( (loadError = newStruct->printError()).size() > 0 ) {
					structure->error = loadError;
					delete(newStruct);
					return -1;
				}
				// Insert new data into current args
				// save location
				tempStruct = (*iter); 
				// insert new structures 
				args.insert(++iter, newStruct->args.begin(), newStruct->args.end());
				// reinit iterator and find location again
				iter = args.begin();
				iterEnd = args.end();
				while ((*iter) != tempStruct) iter++;
				// we want to remove the !load line, so get the next line
				iter++;
				args.remove(tempStruct);
				// and because we changed the list, repeat
				tempStruct = (*iter); 
				iter = args.begin();
				iterEnd = args.end();
				while ((*iter) != tempStruct) iter++;
				// now we have replaced the !load line with the loaded lines
				changes++;
			}
		}
		else { // a Structure, Set, Atom or Development
			if (RepliMacros.find(structure->cmd) != RepliMacros.end()) {
				// expand the macro
				macro = RepliMacros[structure->cmd];
				newStruct = macro->expandMacro(structure);
				if (newStruct != NULL) {
					*structure = *newStruct;
					delete(newStruct);
					changes++;
				}
				else {
					structure->error = macro->error;
					macro->error = "";
					return -1;
				}
			}

			// check for sub structures containing macros
			for (std::list<RepliStruct*>::iterator iter2(structure->args.begin()), iter2End(structure->args.end()); iter2 != iter2End; ++iter2) {
				if ( (count = (*iter2)->process()) > 0 )
					changes += count;
				else if (count < 0)
					return -1;
			}
		}

		// expand Counters in all structures
		if (Counters.find(structure->cmd) != Counters.end()) {
			// expand the counter
			structure->cmd = String::Int2String(Counters[structure->cmd]++);
			changes++;
		}
	}
	
	return changes;
}

RepliStruct	*RepliStruct::loadReplicodeFile(const	std::string	&filename){

	RepliStruct* newRoot = new RepliStruct(Root);
	std::ifstream loadStream(filename.c_str());
	if (loadStream.bad() || loadStream.fail() || loadStream.eof()) {
		newRoot->error += "Load: File '" + filename + "' cannot be read! ";
		loadStream.close();
		return newRoot;
	}
	// create new Root structure
	uint32	a=0,b=0;
	if (newRoot->parse(&loadStream, a, b) < 0) {
		// error is already recorded in newRoot
	}
	if (!loadStream.eof())
		newRoot->error = "Code structure error: Unmatched ) or ].\n";
	loadStream.close();
	return newRoot;
}


std::string	RepliStruct::print()	const{

	std::string str;
	switch (type) {
		case Atom:
			return cmd;
		case Structure:
		case Development:
		case Directive:
		case Condition:
			str = cmd;
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
				str += " " + (*iter)->print();
			if (type == Structure)
				return label + "(" + str + ")" + tail;
			else if (type == Development)
				return label + "{" + str + "}" + tail;
			else
				return str;
		case Set:
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), last(args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
				str += (*iter)->print();
				if (iter != last)
					str += " ";
			}
			return label + "[" + str + "]" + tail;
		case Root:
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
				str += (*iter)->print() + "\n";
			return str;
		default:
			break;
	}
	return str;
}

std::ostream	&operator<<(std::ostream	&os,RepliStruct	*structure){

	return operator<<(os, *structure);
}

std::ostream	&operator<<(std::ostream	&os,const	RepliStruct	&structure){

	switch (structure.type) {
		case RepliStruct::Atom:
			return os << structure.cmd;
		case RepliStruct::Directive:
		case RepliStruct::Condition:
			return os;
		case RepliStruct::Structure:
		case RepliStruct::Development:
			if (structure.type == RepliStruct::Structure)
				os << structure.label << "(";
			else if (structure.type == RepliStruct::Development)
				os << structure.label << "{";
			os << structure.cmd;
			for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
				os << " " << (*iter);
			if (structure.type == RepliStruct::Structure)
				os << ")" << structure.tail;
			else if (structure.type == RepliStruct::Development)
				os << "}" << structure.tail;
			break;
		case RepliStruct::Set:
			os << structure.label << "[";
			if (structure.args.size() > 0) {
				for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), last(structure.args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
					os << (*iter);
					if (iter != last)
						os << " ";
				}
			}
			os << "]" << structure.tail;
			break;
		case RepliStruct::Root:
			for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
				os << (*iter) << std::endl;
			break;
		default:
			break;
	}
    return os;
}

RepliStruct	*RepliStruct::clone()	const{

	RepliStruct* newStruct = new RepliStruct(type);
	newStruct->cmd = cmd;
	newStruct->label = label;
	newStruct->tail = tail;
	newStruct->parent = parent;
	newStruct->error = error;
	newStruct->line = line;
	for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
		newStruct->args.push_back((*iter)->clone());
	return newStruct;
}

std::string	RepliStruct::printError()	const{

	std::stringstream strError;
	if (error.size() > 0) {
		std::string com = cmd;
		RepliStruct* structure = parent;
		while ( (cmd.size() == 0) && (structure != NULL) ) {
			com = structure->cmd;
			structure = structure->parent;
		}
		if (com.size() == 0)
			strError << "Error";
		else
			strError << "Error in structure '" << com << "'";
		if (line > 0)
			strError << " line " << line;
		strError << ": " << error << std::endl;
	}
	for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
		strError << (*iter)->printError();
	return strError.str();
}

RepliMacro::RepliMacro(const	std::string	&name,RepliStruct	*src,RepliStruct	*dest){

	this->name = name;
	this->src = src;
	this->dest = dest;
}

RepliMacro::~RepliMacro(){

	name = "";
	src = NULL;
	dest = NULL;
}

uint32	RepliMacro::argCount(){

	if (src == NULL)
		return 0;
	return src->args.size();
}

RepliStruct	*RepliMacro::expandMacro(RepliStruct	*oldStruct){

	if (src == NULL) {
		error += "Macro '" + name + "' source not defined. ";
		return NULL;
	}
	if (dest == NULL) {
		error += "Macro '" + name + "' destination not defined. ";
		return NULL;
	}
	if (oldStruct == NULL) {
		error += "Macro '" + name + "' cannot expand empty structure. ";
		return NULL;
	}
	if (oldStruct->cmd.compare(this->name) != 0) {
		error += "Macro '" + name + "' cannot expand structure with different name '" + oldStruct->cmd + "'. ";
		return NULL;
	}

	if ( (src->args.size() > 0) && (src->args.size() != oldStruct->args.size()) ) {
		error += "Macro '" + name + "' requires " + String::Int2String(src->args.size()) + " arguments, cannot expand structure with " + String::Int2String(oldStruct->args.size()) + " arguments. ";
		return NULL;
	}

	RepliStruct* newStruct;

	// Special case of macros without args, just copy in the args from oldStruct
	if ((src->args.size() == 0) && (oldStruct->args.size() > 0) ) {
		newStruct = oldStruct->clone();
		newStruct->cmd = dest->cmd;
		return newStruct;
	}
	else
		newStruct = dest->clone();

	RepliStruct *findStruct;

	std::list<RepliStruct*>::const_iterator iOld(oldStruct->args.begin());
	for (std::list<RepliStruct*>::const_iterator iSrc(src->args.begin()), iSrcEnd(src->args.end()); iSrc != iSrcEnd; ++iSrc, ++iOld) {
		// printf("looking for '%s'\n", (*iSrc)->cmd.c_str());
		// find the Atom inside newStruct with the name of iSrc->cmd
		findStruct = newStruct->findAtom((*iSrc)->cmd);
		if (findStruct != NULL) {
			// overwrite data in findStruct with the matching one from old
			*findStruct = *(*iOld);
		}
	}
	
	return newStruct;
}

RepliStruct	*RepliStruct::findAtom(const	std::string	&name){

	RepliStruct* structure;
	for (std::list<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
		switch ((*iter)->type) {
			case Atom:
				if ((*iter)->cmd.compare(name) == 0)
					return (*iter);
				break;
			case Structure:
			case Set:
			case Development:
				structure = (*iter)->findAtom(name);
				if (structure != NULL)
					return structure;
				break;
			default:
				break;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RepliCondition::RepliCondition(const	std::string	&name,bool	reversed){

	this->name = name;
	this->reversed = reversed;
}

RepliCondition::~RepliCondition(){
}

bool RepliCondition::reverse(){

	reversed = !reversed;
	return true;
}

bool	RepliCondition::isActive(UNORDERED_MAP<std::string,RepliMacro	*>	&RepliMacros,UNORDERED_MAP<std::string,int32>	&Counters){

	bool foundIt = (RepliMacros.find(name) != RepliMacros.end());

	if (!foundIt)
		foundIt = (Counters.find(name) != Counters.end());

	if (reversed)
		return (!foundIt);
	else
		return foundIt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Preprocessor::Preprocessor():root(NULL){
	}

	Preprocessor::~Preprocessor(){
		
		if(root)
			delete	root;
	}

	bool	Preprocessor::process(ClassImage			*class_image,
								  std::istream			*stream,
								  std::ostringstream	*outstream,
								  std::string			*error){

		root=new RepliStruct(RepliStruct::Root);
		uint32	a=0, b=0;
		if(root->parse(stream,a,b)<0){

			*error=root->printError();
			return	false;
		}
		if(!stream->eof()){

			*error="Code structure error: Unmatched ) or ].\n";
			return	false;
		}

		//	printf("Replicode:\n\n%s\n",root->print().c_str());

		int32	pass=0,total=0,count;
		while((count=root->process())>0){
			
			total+=count;
			pass++;
		//	printf("Pass %d, %d changes, %d total\n", pass, count, total);
		}
		if(count<0){

			*error=root->printError();
			return	false;
		}
		//	printf("Replicode:\n\n%s\n",root->print().c_str());

		*outstream<<root;

		this->class_image=class_image;
		initialize();

		*error=root->printError();
		return (error->size()==0);
	}

	bool	Preprocessor::isTemplateClass(RepliStruct	*s){

		for(std::list<RepliStruct	*>::iterator	j(s->args.begin());j!=s->args.end();++j){

			std::string	name;
			std::string	type;
			switch((*j)->type){
			case	RepliStruct::Atom:
				if((*j)->cmd==":~")
					return	true;
				break;
			case	RepliStruct::Structure:		//	template instantiation; args are the actual parameters.
			case	RepliStruct::Development:	//	actual template arg as a list of args.
			case	RepliStruct::Set:			//	sets can contain tpl args.
				if(isTemplateClass(*j))
					return	true;
				break;
			default:
				break;
			}
		}
		return	false;
	}

	bool	Preprocessor::isSet(std::string	class_name){

		for(std::list<RepliStruct	*>::iterator	i(root->args.begin());i!=root->args.end();++i){

			if((*i)->type!=RepliStruct::Directive	||	(*i)->cmd!="!class")
				continue;
			RepliStruct	*s=*(*i)->args.begin();
			if(s->cmd==class_name)	//	set classes are written class_name[].
				return	false;
			if(s->cmd==class_name+"[]")
				return	true;
		}
		return	false;
	}

	void	Preprocessor::instantiateClass(RepliStruct	*tpl_class,std::list<RepliStruct	*>	&tpl_args,std::string	&instantiated_class_name){

		static	uint32	LastClassID=0;
		//	remove the trailing [].
		std::string	sset="[]";
		instantiated_class_name=tpl_class->cmd;
		instantiated_class_name=instantiated_class_name.substr(0,instantiated_class_name.length()-sset.length());
		//	append an ID to the tpl class name.
		char	buffer[255];
		sprintf(buffer,"%d",LastClassID++);
		instantiated_class_name+=buffer;

		std::vector<StructureMember>	members;
		std::list<RepliStruct	*>		_tpl_args;
		for(std::list<RepliStruct	*>::reverse_iterator	i=tpl_args.rbegin();i!=tpl_args.rend();++i)
			_tpl_args.push_back(*i);
		getMembers(tpl_class,members,_tpl_args,true);

		class_image->class_names[class_opcode]=instantiated_class_name;
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[instantiated_class_name]=Class(Atom::SSet(class_opcode,members.size()),instantiated_class_name,members);
		++class_opcode;
	}

	void	Preprocessor::getMember(std::vector<StructureMember>	&members,RepliStruct	*m,std::list<RepliStruct	*>	&tpl_args,bool	instantiate){

		size_t	p;
		std::string	name;
		std::string	type;
		switch(m->type){
		case	RepliStruct::Set:
			name=m->label.substr(0,m->label.length()-1);
			if(name=="mks")			//	special case.
				members.push_back(StructureMember(&Compiler::read_mks,name));
			else	if(name=="vws")	//	special case.
				members.push_back(StructureMember(&Compiler::read_vws,name));
			else	if(m->args.size()==0)	//	anonymous set of anything.
				members.push_back(StructureMember(&Compiler::read_set,name));
			else{							//	structured set, arg[0].cmd is ::type.

				type=(*m->args.begin())->cmd.substr(2,m->cmd.length()-1);
				if(isSet(type))
					members.push_back(StructureMember(&Compiler::read_set,name,type,StructureMember::I_SET));
				else	if(type==Class::Type)
					members.push_back(StructureMember(&Compiler::read_set,name,type,StructureMember::I_DCLASS));
				else
					members.push_back(StructureMember(&Compiler::read_set,name,type,StructureMember::I_EXPRESSION));
			}
			break;
		case	RepliStruct::Atom:
			if(m->cmd=="nil")
				break;
			p=m->cmd.find(':');
			name=m->cmd.substr(0,p);
			type=m->cmd.substr(p+1,m->cmd.length());
			if(name=="vw")	//	special case.
				members.push_back(StructureMember(&Compiler::read_view,name,type));
			else	if(type=="")
				members.push_back(StructureMember(&Compiler::read_any,name));
			else	if(type=="nb")
				members.push_back(StructureMember(&Compiler::read_number,name));
			else	if(type=="us")
				members.push_back(StructureMember(&Compiler::read_timestamp,name));
			else	if(type=="bl")
				members.push_back(StructureMember(&Compiler::read_boolean,name));
			else	if(type=="st")
				members.push_back(StructureMember(&Compiler::read_string,name));
			else	if(type=="did")
				members.push_back(StructureMember(&Compiler::read_device,name));
			else	if(type=="fid")
				members.push_back(StructureMember(&Compiler::read_function,name));
			else	if(type=="nid")
				members.push_back(StructureMember(&Compiler::read_node,name));
			else	if(type==Class::Expression)
				members.push_back(StructureMember(&Compiler::read_expression,name));
			else	if(type=="~"){

				RepliStruct	*_m=tpl_args.back();
				tpl_args.pop_back();
				switch(_m->type){
				case	RepliStruct::Structure:{	//	the tpl arg is an instantiated tpl set class.
					std::string	instantiated_class_name;
					instantiateClass(template_classes.find(_m->cmd)->second,_m->args,instantiated_class_name);
					members.push_back(StructureMember(&Compiler::read_set,_m->label.substr(0,_m->label.length()-1),instantiated_class_name,StructureMember::I_CLASS));
					break;
				}default:
					getMember(members,_m,tpl_args,true);
					break;
				}
			}else	if(type==Class::Type)
				members.push_back(StructureMember(&Compiler::read_class,name));
			else	//	type is a class name.
				members.push_back(StructureMember(&Compiler::read_expression,name,type));
			break;
		case	RepliStruct::Structure:{	//	template instantiation; (*m)->cmd is the template class, (*m)->args are the actual parameters.
			RepliStruct	*template_class=template_classes.find(m->cmd)->second;
			if(instantiate){

				std::string	instantiated_class_name;
				instantiateClass(template_class,m->args,instantiated_class_name);
				members.push_back(StructureMember(&Compiler::read_set,m->label.substr(0,m->label.length()-1),instantiated_class_name,StructureMember::I_CLASS));
			}else{

				for(std::list<RepliStruct	*>::reverse_iterator	i=m->args.rbegin();i!=m->args.rend();++i)	//	append the passed args to the ones held by m.
					tpl_args.push_back(*i);
				getMembers(template_class,members,tpl_args,false);
			}
			break;
		}case	RepliStruct::Development:
		   getMembers(m,members,tpl_args,instantiate);
		   break;
		default:
			break;
		}
	}

	void	Preprocessor::getMembers(RepliStruct	*s,std::vector<StructureMember>	&members,std::list<RepliStruct	*>	&tpl_args,bool	instantiate){

		for(std::list<RepliStruct	*>::iterator	j(s->args.begin());j!=s->args.end();++j)
			getMember(members,*j,tpl_args,instantiate);
	}

	ReturnType	Preprocessor::getReturnType(RepliStruct	*s){
			
		if(s->tail==":nb")
			return	NUMBER;
		else	if(s->tail==":us")
			return	TIMESTAMP;
		else	if(s->tail==":bl")
			return	BOOLEAN;
		else	if(s->tail==":st")
			return	STRING;
		else	if(s->tail==":nid")
			return	NODE_ID;
		else	if(s->tail==":did")
			return	DEVICE_ID;
		else	if(s->tail==":fid")
			return	FUNCTION_ID;
		else	if(s->tail==":[]")
			return	SET;
		return	ANY;
	}

	void	Preprocessor::initialize(){

		class_opcode=0;
		uint16	function_opcode=0;
		uint16	operator_opcode=0;

		std::vector<StructureMember>	r_xpr;
		class_image->classes[std::string(Class::Expression)]=Class(Atom::Object(class_opcode,0),Class::Expression,r_xpr);	//	to read unspecified expressions in classes and sets.
		++class_opcode;
		std::vector<StructureMember>	r_type;
		class_image->classes[std::string(Class::Type)]=Class(Atom::Object(class_opcode,0),Class::Type,r_type);	//	to read object types in expressions and sets.
		++class_opcode;

		for(std::list<RepliStruct	*>::iterator	i(root->args.begin());i!=root->args.end();++i){

			if((*i)->type!=RepliStruct::Directive)
				continue;

			RepliStruct	*s=*(*i)->args.begin();
			std::vector<StructureMember>	members;
			if((*i)->cmd=="!class"){

				std::string	sset="[]";
				std::string	class_name=s->cmd;
				size_t		p=class_name.find(sset);
				ClassType	class_type=(p==std::string::npos?T_CLASS:T_SET);
				if(class_type==T_SET)	//	remove the trailing [] since the RepliStructs for instantiated classes do so.
					class_name=class_name.substr(0,class_name.length()-sset.length());

				if(isTemplateClass(s)){

					template_classes[class_name]=s;
					continue;
				}

				std::list<RepliStruct	*>	tpl_args;
				getMembers(s,members,tpl_args,false);

				Atom	atom=Atom::Object(class_opcode,members.size());
				if(class_type==T_CLASS){	//	find out if the class is a sys class, i.e. has a view

					for(uint32	i=0;i<members.size();++i)
						if(members[i].read()==&Compiler::read_view){

							class_type=T_SYS_CLASS;
							if(class_name.find("mk.")!=std::string::npos)
								atom=Atom::Marker(class_opcode,members.size());
							break;
						}
				}

				class_image->class_names[class_opcode]=class_name;
				switch(class_type){
				case	T_SYS_CLASS:
						class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[class_name]=Class(atom,class_name,members);
				case	T_CLASS:
						class_image->classes_by_opcodes[class_opcode]=class_image->classes[class_name]=Class(atom,class_name,members);
					break;
				case	T_SET:
					class_image->classes_by_opcodes[class_opcode]=class_image->classes[class_name]=Class(Atom::SSet(class_opcode,members.size()),class_name,members);
					break;
				default:
					break;
				}
				++class_opcode;
			}else	if((*i)->cmd=="!op"){

				std::list<RepliStruct	*>	tpl_args;
				getMembers(s,members,tpl_args,false);
				ReturnType	return_type=getReturnType(s);

				std::string	operator_name=s->cmd;
				class_image->operator_names.push_back(operator_name);
				class_image->classes[operator_name]=Class(Atom::Operator(operator_opcode,s->args.size()),operator_name,members,return_type);
				++operator_opcode;
			}else	if((*i)->cmd=="!dfn"){	//	don't bother to read the members, it's always a set.

				std::vector<StructureMember>	r_set;
				r_set.push_back(StructureMember(&Compiler::read_set,""));

				std::string	function_name=s->cmd;
				class_image->function_names.push_back(function_name);
				class_image->classes[function_name]=Class(Atom::DeviceFunction(function_opcode),function_name,r_set);
				++function_opcode;
			}
		}
	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	HardCodedPreprocessor::HardCodedPreprocessor(){
	}

	HardCodedPreprocessor::~HardCodedPreprocessor(){
	}

	bool	HardCodedPreprocessor::process(ClassImage			*class_image,
											std::istream		*stream,
											std::ostringstream	*outstream,
											std::string			*error){

		initialize(class_image);
		*outstream<<stream->rdbuf();
		return	true;
	}

	void	HardCodedPreprocessor::initialize(ClassImage	*class_image){

		uint16	class_opcode=0;	//	shared with sys_classes
		uint16	function_opcode=0;
		uint16	operator_opcode=0;

		std::vector<StructureMember>	r_xpr;
		class_image->classes[std::string(Class::Expression)]=Class(Atom::Object(class_opcode,0),Class::Expression,r_xpr);	//	to read expression in sets, special case: see read_expression()
		++class_opcode;

		//	everything below shall be filled by the preprocessor
		//	implemented classes:
		//		view
		//		react_view
		//		ptn
		//		pgm
		//		|pgm
		//		ipgm
		//		cmd
		//		val_pair
		//		vec3: for testing only
		//		ent
		//		grp
		//		mk.rdx
		//		mk.|rdx
		//		mk.low_sln
		//		mk.high_sln
		//		mk.low_act
		//		mk.high_act
		//		mk.low_res
		//		mk.high_res
		//		mk.sln_chg
		//		mk.act_chg
		//		mk.new
		//		mk.position: for testing only
		//		mk.last_known: for testing only
		//	implemented operators:
		//		_now
		//		add
		//		sub
		//		mul
		//		div
		//		gtr
		//		lse
		//		lsr
		//		gte
		//		equ
		//		neq
		//		syn
		//		red
		//		ins
		//		at
		//	implemented functions:
		//		_inj
		//		_eje
		//		_mod
		//		_set
		//		_start
		
		//	non-sys classes /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//	view
		std::vector<StructureMember>	r_view;
		r_view.push_back(StructureMember(&Compiler::read_timestamp,"ijt"));
		r_view.push_back(StructureMember(&Compiler::read_number,"sln"));
		r_view.push_back(StructureMember(&Compiler::read_timestamp,"res"));
		r_view.push_back(StructureMember(&Compiler::read_expression,"grp"));
		r_view.push_back(StructureMember(&Compiler::read_any,"org"));

		class_image->class_names[class_opcode]="view";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("view")]=Class(Atom::SSet(class_opcode,5),"view",r_view);
		++class_opcode;

		//	react_view
		std::vector<StructureMember>	r_react_view;
		r_react_view.push_back(StructureMember(&Compiler::read_timestamp,"ijt"));
		r_react_view.push_back(StructureMember(&Compiler::read_number,"sln"));
		r_react_view.push_back(StructureMember(&Compiler::read_timestamp,"res"));
		r_react_view.push_back(StructureMember(&Compiler::read_expression,"grp"));
		r_react_view.push_back(StructureMember(&Compiler::read_any,"org"));
		r_react_view.push_back(StructureMember(&Compiler::read_number,"act"));

		class_image->class_names[class_opcode]="react_view";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("react_view")]=Class(Atom::SSet(class_opcode,6),"react_view",r_react_view);
		++class_opcode;
		
		//	ptn
		std::vector<StructureMember>	r_ptn;
		r_ptn.push_back(StructureMember(&Compiler::read_expression,"skel"));
		r_ptn.push_back(StructureMember(&Compiler::read_set,"guards",Class::Expression,StructureMember::I_EXPRESSION));	//	reads a set of expressions

		class_image->class_names[class_opcode]="ptn";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("ptn")]=Class(Atom::Object(class_opcode,2),"ptn",r_ptn);
		++class_opcode;

		//	_in_sec
		std::vector<StructureMember>	r__ins_sec;
		r__ins_sec.push_back(StructureMember(&Compiler::read_set,"inputs","ptn",StructureMember::I_EXPRESSION));	//	reads a set of patterns
		r__ins_sec.push_back(StructureMember(&Compiler::read_set,"timings",Class::Expression,StructureMember::I_EXPRESSION));	//	reads a set of expressions
		r__ins_sec.push_back(StructureMember(&Compiler::read_set,"guards",Class::Expression,StructureMember::I_EXPRESSION));	//	reads a set of expressions

		class_image->class_names[class_opcode]="_in_sec";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("_in_sec")]=Class(Atom::SSet(class_opcode,3),"_in_sec",r__ins_sec);
		++class_opcode;

		//	cmd
		std::vector<StructureMember>	r_cmd;
		r_cmd.push_back(StructureMember(&Compiler::read_function,"function"));
		r_cmd.push_back(StructureMember(&Compiler::read_device,"device"));
		r_cmd.push_back(StructureMember(&Compiler::read_set,"args"));

		class_image->class_names[class_opcode]="cmd";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("cmd")]=Class(Atom::Object(class_opcode,3),"cmd",r_cmd);
		++class_opcode;

		//	val_pair
		std::vector<StructureMember>	r_val_pair;
		r_val_pair.push_back(StructureMember(&Compiler::read_any,"value"));
		r_val_pair.push_back(StructureMember(&Compiler::read_number,"index"));

		class_image->class_names[class_opcode]="val_pair";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("val_pair")]=Class(Atom::SSet(class_opcode,2),"val_pair",r_val_pair);
		++class_opcode;

		//	vec3: non-standard (i.e. does not belong to std.replicode), only for testing
		std::vector<StructureMember>	r_vec3;
		r_vec3.push_back(StructureMember(&Compiler::read_number,"x"));
		r_vec3.push_back(StructureMember(&Compiler::read_number,"y"));
		r_vec3.push_back(StructureMember(&Compiler::read_number,"z"));

		class_image->class_names[class_opcode]="vec3";
		class_image->classes_by_opcodes[class_opcode]=class_image->classes[std::string("vec3")]=Class(Atom::Object(class_opcode,3),"vec3",r_vec3);
		++class_opcode;
		
		//	sys-classes /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//	pgm
		std::vector<StructureMember>	r_pgm;
		r_pgm.push_back(StructureMember(&Compiler::read_set,"tpl","ptn",StructureMember::I_EXPRESSION));	//	reads a set of patterns
		r_pgm.push_back(StructureMember(&Compiler::read_set,"input","_in_sec",StructureMember::I_CLASS));	//	reads a structured set of class _in_sec 
		r_pgm.push_back(StructureMember(&Compiler::read_set,"prods","cmd",StructureMember::I_EXPRESSION));	//	reads a set of commands
		r_pgm.push_back(StructureMember(&Compiler::read_timestamp,"tsc"));
		r_pgm.push_back(StructureMember(&Compiler::read_number,"csm"));
		r_pgm.push_back(StructureMember(&Compiler::read_number,"sig"));
		r_pgm.push_back(StructureMember(&Compiler::read_number,"nfr"));
		r_pgm.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_pgm.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_pgm.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_pgm.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="pgm";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("pgm")]=Class(Atom::Object(class_opcode,11),"pgm",r_pgm);
		++class_opcode;

		//	|pgm
		class_image->class_names[class_opcode]="|pgm";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("|pgm")]=Class(Atom::Object(class_opcode,11),"|pgm",r_pgm);
		++class_opcode;

		//	ipgm
		std::vector<StructureMember>	r_ipgm;
		r_ipgm.push_back(StructureMember(&Compiler::read_any,"code"));
		r_ipgm.push_back(StructureMember(&Compiler::read_set,"args","val_pair",StructureMember::I_SET));	//	reads a set of value pairs
		r_ipgm.push_back(StructureMember(&Compiler::read_view,"vw","react_view"));
		r_ipgm.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_ipgm.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_ipgm.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="ipgm";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("ipgm")]=Class(Atom::Object(class_opcode,6),"ipgm",r_ipgm);
		++class_opcode;

		//	ent
		std::vector<StructureMember>	r_ent;
		r_ent.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_ent.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_ent.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_ent.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="ent";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("ent")]=Class(Atom::Object(class_opcode,4),"ent",r_ent);
		++class_opcode;

		//	grp
		std::vector<StructureMember>	r_grp;
		r_grp.push_back(StructureMember(&Compiler::read_number,"upr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"spr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"sln_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"act_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"vis_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"c_sln"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"c_sln_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"c_act"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"c_act_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"dcy_per"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"dcy_tgt"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"dcy_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"sln_chg_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"sln_chg_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"act_chg_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"act_chg_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"avg_sln"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"high_sln"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"low_sln"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"avg_act"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"high_act"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"low_act"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"high_sln_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"low_sln_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"sln_ntf_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"high_act_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"low_act_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"act_ntf_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"high_res_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"low_res_thr"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"res_ntf_prd"));
		r_grp.push_back(StructureMember(&Compiler::read_expression,"ntf_grp"));
		r_grp.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_grp.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_grp.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_grp.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="grp";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("grp")]=Class(Atom::Object(class_opcode,36),"grp",r_grp);
		++class_opcode;

		//	mk.rdx
		std::vector<StructureMember>	r_rdx;
		r_rdx.push_back(StructureMember(&Compiler::read_any,"code"));
		r_rdx.push_back(StructureMember(&Compiler::read_any,"inputs"));
		r_rdx.push_back(StructureMember(&Compiler::read_set,"prods","val_pair",StructureMember::I_SET));	//	reads a set of value pairs
		r_rdx.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_rdx.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_rdx.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_rdx.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="mk.rdx";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.rdx")]=Class(Atom::Object(class_opcode,7),"mk.rdx",r_rdx);
		++class_opcode;

		//	mk.|rdx
		std::vector<StructureMember>	r_ardx;
		r_ardx.push_back(StructureMember(&Compiler::read_any,"code"));
		r_ardx.push_back(StructureMember(&Compiler::read_set,"prods","val_pair",StructureMember::I_SET));	//	reads a set of value pairs
		r_ardx.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_ardx.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_ardx.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_ardx.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="mk.|rdx";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.|rdx")]=Class(Atom::Object(class_opcode,6),"mk.|rdx",r_ardx);
		++class_opcode;

		//	utilities for notification markers
		std::vector<StructureMember>	r_ntf;
		r_ntf.push_back(StructureMember(&Compiler::read_any,"obj"));
		r_ntf.push_back(StructureMember(&Compiler::read_set,"prods","val_pair",StructureMember::I_SET));	//	reads a set of value pairs
		r_ntf.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_ntf.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_ntf.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_ntf.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		std::vector<StructureMember>	r_ntf_chg;
		r_ntf_chg.push_back(StructureMember(&Compiler::read_any,"obj"));
		r_ntf_chg.push_back(StructureMember(&Compiler::read_number,"chg"));
		r_ntf_chg.push_back(StructureMember(&Compiler::read_set,"prods","val_pair",StructureMember::I_SET));	//	reads a set of value pairs
		r_ntf_chg.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_ntf_chg.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_ntf_chg.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_ntf_chg.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		//	mk.low_sln
		class_image->class_names[class_opcode]="mk.low_sln";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.low_sln")]=Class(Atom::Object(class_opcode,6),"mk.low_sln",r_ntf);
		++class_opcode;

		//	mk.high_sln
		class_image->class_names[class_opcode]="mk.high_sln";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.high_sln")]=Class(Atom::Object(class_opcode,6),"mk.high_sln",r_ntf);
		++class_opcode;

		//	mk.low_act
		class_image->class_names[class_opcode]="mk.low_act";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.low_act")]=Class(Atom::Object(class_opcode,6),"mk.low_act",r_ntf);
		++class_opcode;

		//	mk.high_act
		class_image->class_names[class_opcode]="mk.high_act";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.high_act")]=Class(Atom::Object(class_opcode,6),"mk.high_act",r_ntf);
		++class_opcode;

		//	mk.low_res
		class_image->class_names[class_opcode]="mk.low_res";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.low_res")]=Class(Atom::Object(class_opcode,6),"mk.low_res",r_ntf);
		++class_opcode;

		//	mk.high_res
		class_image->class_names[class_opcode]="mk.high_res";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.high_res")]=Class(Atom::Object(class_opcode,6),"mk.high_res",r_ntf);
		++class_opcode;

		//	mk.sln_chg
		class_image->class_names[class_opcode]="mk.high_act";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.high_act")]=Class(Atom::Object(class_opcode,6),"mk.high_act",r_ntf_chg);
		++class_opcode;

		//	mk.act_chg
		class_image->class_names[class_opcode]="mk.high_act";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.high_act")]=Class(Atom::Object(class_opcode,6),"mk.high_act",r_ntf_chg);
		++class_opcode;

		//	mk.new
		class_image->class_names[class_opcode]="mk.new";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.new")]=Class(Atom::Object(class_opcode,6),"mk.new",r_ntf);
		++class_opcode;

		//	mk.position: non-standard
		std::vector<StructureMember>	r_mk_position;
		r_mk_position.push_back(StructureMember(&Compiler::read_any,"object"));
		r_mk_position.push_back(StructureMember(&Compiler::read_expression,"position","vec3"));
		r_mk_position.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_mk_position.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_mk_position.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_mk_position.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="mk.position";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.position")]=Class(Atom::Object(class_opcode,6),"mk.position",r_mk_position);
		++class_opcode;

		//	mk.last_known: non-standard
		std::vector<StructureMember>	r_mk_last_known;
		r_mk_last_known.push_back(StructureMember(&Compiler::read_any,"event"));
		r_mk_last_known.push_back(StructureMember(&Compiler::read_view,"vw","view"));
		r_mk_last_known.push_back(StructureMember(&Compiler::read_mks,"mks"));
		r_mk_last_known.push_back(StructureMember(&Compiler::read_vws,"vws"));
		r_mk_last_known.push_back(StructureMember(&Compiler::read_number,"psln_thr"));

		class_image->class_names[class_opcode]="mk.last_known";
		class_image->classes_by_opcodes[class_opcode]=class_image->sys_classes[std::string("mk.last_known")]=Class(Atom::Object(class_opcode,5),"mk.last_known",r_mk_last_known);
		++class_opcode;

		//	utilities /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		std::vector<StructureMember>	r_null;
		
		std::vector<StructureMember>	r_any;
		r_any.push_back(StructureMember(&Compiler::read_any,""));

		std::vector<StructureMember>	r_set;
		r_set.push_back(StructureMember(&Compiler::read_set,""));

		std::vector<StructureMember>	r_any_any;
		r_any_any.push_back(StructureMember(&Compiler::read_any,""));
		r_any_any.push_back(StructureMember(&Compiler::read_any,""));

		std::vector<StructureMember>	r_any_set;
		r_any_set.push_back(StructureMember(&Compiler::read_any,""));
		r_any_set.push_back(StructureMember(&Compiler::read_set,""));

		std::vector<StructureMember>	r_nb_nb;
		r_nb_nb.push_back(StructureMember(&Compiler::read_number,""));
		r_nb_nb.push_back(StructureMember(&Compiler::read_number,""));

		std::vector<StructureMember>	r_set_set_set;
		r_set_set_set.push_back(StructureMember(&Compiler::read_set,""));
		r_set_set_set.push_back(StructureMember(&Compiler::read_set,""));
		r_set_set_set.push_back(StructureMember(&Compiler::read_set,""));

		std::vector<StructureMember>	r_set_nb;
		r_set_nb.push_back(StructureMember(&Compiler::read_set,""));
		r_set_nb.push_back(StructureMember(&Compiler::read_number,""));

		std::vector<StructureMember>	r_ms_ms;
		r_ms_ms.push_back(StructureMember(&Compiler::read_timestamp,""));
		r_ms_ms.push_back(StructureMember(&Compiler::read_timestamp,""));

		//	operators /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class_image->classes[std::string("_now")]=Class(Atom::Operator(operator_opcode++,0),"_now",r_null,TIMESTAMP);
		class_image->operator_names.push_back("_now");
		class_image->classes[std::string("add")]=Class(Atom::Operator(operator_opcode++,2),"add",r_any_any,ANY);
		class_image->operator_names.push_back("add");
		class_image->classes[std::string("sub")]=Class(Atom::Operator(operator_opcode++,2),"sub",r_any_any,ANY);
		class_image->operator_names.push_back("sub");
		class_image->classes[std::string("mul")]=Class(Atom::Operator(operator_opcode++,2),"mul",r_nb_nb,NUMBER);
		class_image->operator_names.push_back("mul");
		class_image->classes[std::string("div")]=Class(Atom::Operator(operator_opcode++,2),"div",r_nb_nb,NUMBER);
		class_image->operator_names.push_back("div");
		class_image->classes[std::string("gtr")]=Class(Atom::Operator(operator_opcode++,2),"gtr",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("gtr");
		class_image->classes[std::string("lse")]=Class(Atom::Operator(operator_opcode++,2),"lse",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("lse");
		class_image->classes[std::string("lsr")]=Class(Atom::Operator(operator_opcode++,2),"lsr",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("lsr");
		class_image->classes[std::string("gte")]=Class(Atom::Operator(operator_opcode++,2),"gte",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("gte");
		class_image->classes[std::string("equ")]=Class(Atom::Operator(operator_opcode++,2),"equ",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("equ");
		class_image->classes[std::string("neq")]=Class(Atom::Operator(operator_opcode++,2),"neq",r_any_any,BOOLEAN);
		class_image->operator_names.push_back("neq");
		class_image->classes[std::string("syn")]=Class(Atom::Operator(operator_opcode++,1),"syn",r_any,ANY);
		class_image->operator_names.push_back("syn");
		class_image->classes[std::string("red")]=Class(Atom::Operator(operator_opcode++,3),"red",r_set_set_set,SET);
		class_image->operator_names.push_back("red");
		class_image->classes[std::string("ins")]=Class(Atom::Operator(operator_opcode++,2),"ins",r_any_set,ANY);
		class_image->operator_names.push_back("ins");
		class_image->classes[std::string("at")]=Class(Atom::Operator(operator_opcode++,2),"at",r_set_nb,ANY);
		class_image->operator_names.push_back("at");

		//	functions /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class_image->classes[std::string("_inj")]=Class(Atom::DeviceFunction(function_opcode++),"_inj",r_set);
		class_image->function_names.push_back("_inj");
		class_image->classes[std::string("_eje")]=Class(Atom::DeviceFunction(function_opcode++),"_eje",r_set);
		class_image->function_names.push_back("_eje");
		class_image->classes[std::string("_mod")]=Class(Atom::DeviceFunction(function_opcode++),"_mod",r_set);
		class_image->function_names.push_back("_mod");
		class_image->classes[std::string("_set")]=Class(Atom::DeviceFunction(function_opcode++),"_set",r_set);
		class_image->function_names.push_back("_set");
		class_image->classes[std::string("_start")]=Class(Atom::DeviceFunction(function_opcode++),"_start",r_set);
		class_image->function_names.push_back("_start");
	}
}