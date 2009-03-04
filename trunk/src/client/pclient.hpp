/*
 * Copyright 2008, 2009, Dominik Geyer
 *
 * This file is part of HoldingNuts.
 *
 * HoldingNuts is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HoldingNuts is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HoldingNuts.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Dominik Geyer <dominik.geyer@holdingnuts.net>
 */


#ifndef _PCLIENT_H
#define _PCLIENT_H

#include <string>
#include <vector>
#include <map>

#include <QApplication>
#include <QtNetwork>
#include <QTranslator>
#include <QLocale>
#include <QTimer>
#include <QDir>
#include <QRegExp>

#include "Tokenizer.hpp"

#include "Card.hpp"
#include "HoleCards.hpp"
#include "CommunityCards.hpp"
#include "GameLogic.hpp"
#include "Player.hpp"

#include "Protocol.h"

#include "WMain.hpp"
#include "WTable.hpp"


typedef struct {
	char msgbuf[1024*256];
	int buflen;
	int last_msgid;
	
	int cid;   // our client-id assigned by server
	
	bool introduced;   // PCLIENT->PSERVER sequence success
} servercon;

typedef struct {
	char name[255];
} playerinfo;
// TODO: why not replace with QStringList or something else

typedef struct {
	bool sitting;
	bool subscribed;
	table_snapshot snap;
	HoleCards holecards;
	WTable *window;
} tableinfo;

typedef std::map<int,tableinfo>		tables_type;

typedef struct {
	// TODO: ???
	bool			registered;
	
	//! \brief name of the game
	QString			name;
	//! \brief gametype see Protocol.h 
	gametype		type;
	//! \brief gamemode see Protocol.h 
	gamemode		mode;
	//! \brief timeout in seconds if no answer from player
	unsigned int	player_timeout;
	//! \brief current playerscount registered in game
//	unsigned int	players_count;		// TODO: remove -> players.clount()
	//! \brief maximum players allowed
	unsigned int	players_max;
	//! \brief registered Players
	QStringList		players;

	tables_type		tables;
} gameinfo;
// TODO: merge gameinfo with GameListTableModel { QMap<gid, gameinfo*> };

// map<gid, gameinfo>
typedef std::map<int,gameinfo>		games_type;
// map<cid, playerinfo>
typedef std::map<int,playerinfo>	players_type;



class PClient : public QApplication
{
Q_OBJECT

public:
	PClient(int &argc, char **argv);
	~PClient();
	
	int init();
	
	bool doConnect(QString strServer, unsigned int port);
	void doClose();
	
	bool isConnected() const { return connected || connecting; };
	
	void chatAll(const QString& text);
	void chat(const QString& text, int gid, int tid);
	
	bool doSetAction(int gid, Player::PlayerAction action, float amount=0.0f);
	
	void doRegister(int gid, bool bRegister=true);
	
	gameinfo* getGameInfo(int gid);
	tableinfo* getTableInfo(int gid, int tid);
	playerinfo* getPlayerInfo(int cid);
	int getMyCId();
	
	WMain* getMainWnd() const { return wMain; };
	bool isTableWindowRemaining();
	
	//! \brief query playerlist from Server
	void requestPlayerlist(int gid);
	
	static QString getGametype(gametype type);
	static QString getGamemode(gamemode mode);
	
private:
	WMain *wMain;
		
	bool connected;
	bool connecting;
	
	QTcpSocket	*tcpSocket;

	//! \brief timer updates the gamelist
	QTimer		*timerGamelistUpdate;
	
private:	
	int netSendMsg(const char *msg);
	
	int serverExecute(const char *cmd);
	int serverParsebuffer();
	
	void serverCmdPserver(Tokenizer &t);
	void serverCmdErr(Tokenizer &t);
	void serverCmdMsg(Tokenizer &t);
	void serverCmdSnap(Tokenizer &t);
	void serverCmdPlayerlist(Tokenizer &t);
	void serverCmdClientinfo(Tokenizer &t);
	void serverCmdGameinfo(Tokenizer &t);
	void serverCmdGamelist(Tokenizer &t);
	
	bool addTable(int gid, int tid);

private slots:
	void netRead();
	void netError(QAbstractSocket::SocketError socketError);
	void netConnected();
	void netDisconnected();

	//! \brief query gamelist from Server
	void requestGamelist();
	
#ifdef DEBUG
	void slotDbgRegister();
#endif
};

#endif /* _PCLIENT_H */
