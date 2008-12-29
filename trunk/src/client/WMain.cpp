/*
 * Copyright 2008, Dominik Geyer
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
 */


#include <cstdio>

#include "WMain.hpp"
#include "WTable.hpp"

#include "Config.h"
#include "Debug.h"

#include "pclient.hpp"

void WMain::actionConnect()
{
	if (!((PClient*)qApp)->doConnect(editSrvAddr->text(), DEFAULT_SERVER_PORT))
		addLog(tr("Error connecting."));
	else
		btnConnect->setEnabled(false);
}

void WMain::actionClose()
{
	((PClient*)qApp)->doClose();
}


void WMain::actionTest()
{
	WTable *table = new WTable();
	table->show();
}

void WMain::slotSrvTextChanged()
{
	if (!editSrvAddr->text().length() || ((PClient*)qApp)->isConnected())
		btnConnect->setEnabled(false);
	else
		btnConnect->setEnabled(true);
}

void WMain::actionChat()
{
	if (editChat->text().length())
	{
		((PClient*)qApp)->chatAll(editChat->text().simplified());
		editChat->clear();
		editChat->setFocus();
	}
}

void WMain::updateConnectionStatus()
{
	if (((PClient*)qApp)->isConnected())
	{
		btnConnect->setEnabled(false);
		btnClose->setEnabled(true);
	}
	else
	{
		if (editSrvAddr->text().length())
			btnConnect->setEnabled(true);
		else
			btnConnect->setEnabled(false);
		btnClose->setEnabled(false);
	}
}

void WMain::addLog(QString line)
{
	editLog->append(line);
}

void WMain::addChat(QString from, QString text)
{
	//QString chatline = "[" + from + "]: " + text;
	//editChatLog->append(chatline);
	
	editChatLog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
	editChatLog->setFontWeight(QFont::Bold);
	editChatLog->insertPlainText("[" + from + "]");
	editChatLog->setFontWeight(QFont::Normal);
	editChatLog->insertPlainText(": " + text + "\r\n");
}

WMain::WMain(QWidget *parent) : QWidget(parent)
{
	//setFixedSize(200, 120);
	setWindowTitle("HoldingNuts foyer");
	
	QGroupBox *groupSrv = new QGroupBox(tr("Connection"));
	
	btnConnect = new QPushButton(tr("Connect"), this);
	btnConnect->setEnabled(false);
	connect(btnConnect, SIGNAL(clicked()), this, SLOT(actionConnect()));
	
	btnClose = new QPushButton(tr("Close"), this);
	btnClose->setEnabled(false);
	connect(btnClose, SIGNAL(clicked()), this, SLOT(actionClose()));
	
	editSrvAddr = new QLineEdit(this);
	editSrvAddr->setText("localhost");
	connect(editSrvAddr, SIGNAL(textChanged(const QString&)), this, SLOT(slotSrvTextChanged()));
	
	QHBoxLayout *lsrvinp = new QHBoxLayout();
	lsrvinp->addWidget(editSrvAddr);
	lsrvinp->addWidget(btnConnect);
	lsrvinp->addWidget(btnClose);
	
	editLog = new QTextEdit(this);
	editLog->setReadOnly(true);
	
	QVBoxLayout *lsrv = new QVBoxLayout();
	lsrv->addLayout(lsrvinp);
	lsrv->addWidget(editLog);
	
	groupSrv->setLayout(lsrv);
	/////////////////////
	
	QGroupBox *groupChat = new QGroupBox(tr("Foyer chat"));
	
	editChat = new QLineEdit(this);
	connect(editChat, SIGNAL(returnPressed()), this, SLOT(actionChat()));
	
	QPushButton *btnChat = new QPushButton(tr("Chat"), this);
	connect(btnChat, SIGNAL(clicked()), this, SLOT(actionChat()));
	
	QHBoxLayout *lchatinp = new QHBoxLayout();
	lchatinp->addWidget(editChat);
	lchatinp->addWidget(btnChat);
	
	editChatLog = new QTextEdit(this);
	editChatLog->setReadOnly(true);
	
	QVBoxLayout *lchat = new QVBoxLayout();
	lchat->addLayout(lchatinp);
	lchat->addWidget(editChatLog);
	
	groupChat->setLayout(lchat);
	/////////////////////
	
	QPushButton *btnTest = new QPushButton(tr("Test"), this);
	connect(btnTest, SIGNAL(clicked()), this, SLOT(actionTest()));
	
	/////////////////////
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(groupSrv);
	layout->addWidget(groupChat);
	layout->addWidget(btnTest);
	setLayout(layout);
}
