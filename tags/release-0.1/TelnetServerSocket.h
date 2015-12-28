#ifndef __TELNETSERVERSOCKET_H
#define __TELNETSERVERSOCKET_H

#include <string>
using namespace std;

#include "libsocket++/ServerSocket.h"

class TelnetServerSocket : public ServerSocket {
	public:
		TelnetServerSocket( int port = 23 );
		virtual ~TelnetServerSocket();
		
		unsigned char getChar();
		string getLine( bool hidden=false );
		string cmdAsStr( unsigned char cmd );
		
		TelnetServerSocket* accept();
		
		static enum {
			ECHO=1,
			SE=240, 
			NOP=241,
			DM=242,
			BRK=243,
			IP=244,
			AO=245,
			AYT=246,
			EC=247,
			EL=248,
			GA=249,
			SB=250,
			WILL=251,
			WONT=252,
			DO=253,
			DONT=254,
			IAC=255,
		} TelnetCmdCode;
		
		static enum {
			BELL=7,
			BS=8,
			HT=9,
			LF=10,
			VT=11,
			FF=12,
			CR=13
		} TelnetCtrlCode;
		
		void setPeerEcho( bool val );
		bool getPeerEcho();
	protected:
		bool peerEcho;
};

#endif
