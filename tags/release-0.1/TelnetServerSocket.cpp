#include "TelnetServerSocket.h"

#include <map>
#include <sstream>
#include <string>
#include <deque>
using namespace std;

#include "logger.h"

TelnetServerSocket::TelnetServerSocket( int port ) : ServerSocket( port ) {
	peerEcho = true; //By default, assume the client is going to echo characters
}

TelnetServerSocket::~TelnetServerSocket() {
}

string TelnetServerSocket::cmdAsStr( unsigned char c ) {
	//Setup some proto string mappings
	map<unsigned char,string> protoStr;
	protoStr[1] = "ECHO";
	protoStr[251] = "WILL";
	protoStr[252] = "WONT";
	protoStr[253] = "DO";
	protoStr[254] = "DONT";
	protoStr[255] = "IAC";
	
	//Return the corresponding string if it is defined, otherwise return the character code as a string
	if( protoStr.find(c) == protoStr.end() ) {
		stringstream ss;
		ss << (int)c;
		return ss.str();
	} else {
		return protoStr[c];
	}
}

unsigned char TelnetServerSocket::getChar() {
	unsigned char c; //Return value
	deque<unsigned char> history; //We will store a log of characters received here
	
	while( true ) {
		//Read a character from the stream
		(*this) >> c;
		
		//Handle telnet IAC's
		if( c == IAC ) {
			//Build a debug string for this telnet command
			stringstream debugStr;
			debugStr << "received control code: IAC ";
			
			//Read the command byte
			unsigned char cmd;
			(*this) >> cmd;
			debugStr << cmdAsStr(cmd);
			
			//These commands will send a 3rd byte, the argument
			unsigned char arg = 0;
			if( cmd == DO || cmd == DONT || cmd == WILL || cmd == WONT ) {
				//Read the arg
				(*this) >> arg;
				debugStr << " " << cmdAsStr(arg);
			}
			
			//Handle echo negociation
			if( arg == ECHO ) {
				switch( cmd ) {
					case WILL:
						peerEcho = true;
						break;
					
					case WONT:
						peerEcho = false;
						break;
				}
			}
			
			//Send the debug message to the log
			Logger::debug() << debugStr.str() << endl;
		} else {
			//Put this character into the history log
			history.push_front( c );
			
			//Handle escape sequences
			if( c == 27 ) {
				continue;
			}
			
			if( history[1] == 27 ) {
				continue;
			}
			
			if( history[1] == '[' && history[2] == 27 ) {
				continue;
			}
			
			if( history[1] == '1' && history[2] == '[' && history[3] == 27 ) {
				continue;
			}
			
			if( history[2] == '1' && history[3] == '[' && history[4] == 27 ) {
				continue;
			}
			
			if( history[1] == '3' && history[2] == '[' && history[3] == 27 ) {
				continue;
			}
			
			if( history[1] == 'O' && history[2] == 27 ) {
				continue;
			}
			//End handling escape sequences
			
			//Return the character
			return c;
		}
	}
}

string TelnetServerSocket::getLine( bool hidden ) {
	bool prevPeerEcho = getPeerEcho();
	if( hidden && getPeerEcho() ) {
		setPeerEcho( false );
	}

	string line; //The return value
	while( true ) {
		//Get a formatted character
		unsigned char c = getChar();
		
		//Remember whether or not we erased a character from the line
		bool erasedChar = false;
		
		//Handle line endings
		if( c == '\r' ) {
			//Read another character after we receive \r, because telnet always does either \r\n or \r\0
			(*this) >> c;
			
			//If we are supposed to be doing the echo'ing, send end-of-line to the client
			if( !getPeerEcho() ) {
				(*this) << "\r\n";
			}
			
			//If we altered the peer echo status, retrun it to what it was before
			if( hidden ) {
				setPeerEcho( prevPeerEcho );
			}
			
			//Since we received end-of-line, return the string as it is
			return line;
		} else {
			//Handle backspace and ^H
			if( c == 127 || c == 8 ) {
				//Replace the last character in the line with nothing
				if( !line.empty() ) {
					line.replace( line.size()-1, 1, "" );
					erasedChar = true;
				}
			} else {
				//Add the character to the end of line
				line += c;
			}
		}
		
		if( !hidden && !getPeerEcho() ) {
			//Handle character erasing (backspace) by moving left, printing a space, and moving left again
			// This is either the way it's supposed to be done or a hack, I couldn't find any info on
			// "the right way" to do backspaces so I have no clue.
			if( erasedChar ) {
				(*this) << '\x08' << ' ' << '\x08';
			} else if( c != 127 && c != 8 ) {
				//If this has nothing to do with backspaces than echo the character
				(*this) << c;
			}
		}
	}
}

TelnetServerSocket* TelnetServerSocket::accept() {
	//Create an unbinded TelnetServerSocket
	TelnetServerSocket* sock = new TelnetServerSocket( -1 );
	
	//Accept a connection, using the socket we created
	ServerSocket::accept( sock );
	
	//Return the socket
	return sock;
}

void TelnetServerSocket::setPeerEcho( bool val ) {
	if( val == true ) {
		//Tell the client we wont echo characters, this should make it want to echo them instead
		(*this) << IAC << WONT << ECHO;
		
	} else {
		//Tell the client that we will do the echo'ing of characters as we receive them, that way the client doesnt
		(*this) << IAC << WILL << ECHO;
	}
	
	peerEcho = val;
}

bool TelnetServerSocket::getPeerEcho() {
	return peerEcho;
}
