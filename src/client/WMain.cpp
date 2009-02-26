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

#include "WMain.hpp"
#include "ChatBox.hpp"
#include "WTable.hpp"
#include "SettingsDialog.hpp"
#include "AboutDialog.hpp"
#include "StringListModel.hpp"

#include "Config.h"
#include "Debug.h"
#include "SysAccess.h"
#include "ConfigParser.hpp"

#include "pclient.hpp"

#include <cstdio>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include <QTreeView>
#include <QStandardItemModel>
#include <QListView>
#include <QCloseEvent>


extern ConfigParser config;


WMain::WMain(QWidget *parent) : QMainWindow(parent, 0)
{
	this->setWindowTitle(tr("HoldingNuts Foyer"));
	this->setWindowIcon(QIcon(":/res/pclient.ico"));

	// header
	QDateTime datetime = QDateTime::currentDateTime();

	QLabel *lblWelcome = new QLabel(tr("Welcome"), this);
//	QLabel *lblDateTime = new QLabel(datetime.toString("dddd, yyyy-MM-dd, hh:mm"), this);
	QLabel *lblLogo = new QLabel(this);
	lblLogo->setFixedSize(QSize(75, 75));

	QHBoxLayout *lHeader = new QHBoxLayout();
	lHeader->addWidget(lblWelcome, Qt::AlignVCenter);
	lHeader->addWidget(lblLogo);

	// model game
	QStringList strlstHeaderLabels;
	
	strlstHeaderLabels << tr("Name") 
		<< tr("Gametype") 
		<< tr("Players") 
		<< tr("Max. Players")
		<< tr("State");
		
	modelGameList = new QStandardItemModel(0, strlstHeaderLabels.count(), this);
	modelGameList->setHorizontalHeaderLabels(strlstHeaderLabels);

	// view game
	viewGameList = new QTreeView(this);
	viewGameList->setRootIsDecorated(false);
	viewGameList->setAlternatingRowColors(true);
	viewGameList->setModel(modelGameList);
	viewGameList->setSelectionMode(QAbstractItemView::SingleSelection);
	
	connect(
		viewGameList->selectionModel(),
		SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this,
		SLOT(gameListSelectionChanged(const QItemSelection&, const QItemSelection&)));

	// model player
	modelPlayerList = new StringListModel(this);

	// view player
	viewPlayerList = new QListView(this);
	viewPlayerList->setAlternatingRowColors(true);
	viewPlayerList->setModel(modelPlayerList);
	viewPlayerList->setSelectionMode(QAbstractItemView::NoSelection);

	// game
	QPushButton *btnRegister = new QPushButton(tr("&Register"), this);
	btnRegister->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(btnRegister, SIGNAL(clicked()), this, SLOT(actionRegister()));
	
	QPushButton *btnUnregister = new QPushButton(tr("&Unregister"), this);
	btnUnregister->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(btnUnregister, SIGNAL(clicked()), this, SLOT(actionUnregister()));
	
	QHBoxLayout *lRegister = new QHBoxLayout();
	lRegister->addWidget(btnRegister);
	lRegister->addWidget(btnUnregister);

	// gameinfo
	QVBoxLayout *lGameInfo = new QVBoxLayout();
	lGameInfo->addWidget(new QLabel(tr("Game Name"), this));
	lGameInfo->addWidget(new QLabel(tr("Blinds"), this));
	lGameInfo->addWidget(new QLabel(tr("Starting Blinds"), this));
	lGameInfo->addWidget(new QLabel(tr("Stake"), this));
	lGameInfo->addWidget(new QLabel(tr("Stake"), this));
	lGameInfo->addWidget(viewPlayerList);
	lGameInfo->addLayout(lRegister);

	
	// connection
	editSrvAddr = new QLineEdit(QString::fromStdString(config.get("default_host")), this);
	connect(editSrvAddr, SIGNAL(textChanged(const QString&)), this, SLOT(slotSrvTextChanged()));
	connect(editSrvAddr, SIGNAL(returnPressed()), this, SLOT(actionConnect()));
	
	btnConnect = new QPushButton(tr("&Connect"), this);
	btnConnect->setEnabled(false);
	btnConnect->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(btnConnect, SIGNAL(clicked()), this, SLOT(actionConnect()));
	
	btnClose = new QPushButton(tr("Cl&ose"), this);
	btnClose->setEnabled(false);
	btnClose->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(btnClose, SIGNAL(clicked()), this, SLOT(actionClose()));
		
	QHBoxLayout *lConnect = new QHBoxLayout();
	lConnect->addWidget(new QLabel(tr("Host:"), this));
	lConnect->addWidget(editSrvAddr);
	lConnect->addWidget(btnConnect);
	lConnect->addWidget(btnClose);

//	QGroupBox *groupSrv = new QGroupBox(tr("Connection"), this);
//	groupSrv->setLayout(lConnect);

	
	// the foyer chat box
	m_pChat = new ChatBox(tr("Foyer Chat"), ChatBox::INPUTLINE_BOTTOM, 0, this);
	m_pChat->showChatBtn(true);
	connect(m_pChat, SIGNAL(dispatchedMessage(QString)), this, SLOT(actionChat(QString)));


	// new game
	btnCreateGame = new QPushButton(tr("Create own Game"), this);
	btnCreateGame->setEnabled(false);
	btnCreateGame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// final layout
	QGridLayout *layout = new QGridLayout;
	// row 0
	layout->addLayout(lHeader, 0, 0, 1, 2);
	// row 1
//	layout->addWidget(groupSrv, 1, 0, 1, 2);
	layout->addLayout(lConnect, 1, 0, 1, 2);
	// row 2
	layout->addWidget(viewGameList, 2, 0, 1, 1);
	layout->addLayout(lGameInfo, 2, 1, 1, 1);
	// row 3
	layout->addWidget(m_pChat, 3, 0, 1, 1);
	layout->addWidget(btnCreateGame, 3, 1, 1, 1, Qt::AlignBottom | Qt::AlignRight);
	
	layout->setColumnStretch(0, 2);
	layout->setColumnMinimumWidth(0, 300);
	layout->setColumnMinimumWidth(1, 150);
	
	// TODO: widgets decoration
//	this->setStyleSheet("background-color: black");

	// create a main widget containing the layout
	QWidget *central = new QWidget(this);
	central->setLayout(layout);
	this->setCentralWidget(central);
	
	
	// menus
	QAction *action;
	QMenu *game = new QMenu(this);

	action = new QAction(tr("&Settings"), this);
	action->setShortcut(tr("CTRL+S"));
	connect(action, SIGNAL(triggered()), this, SLOT(actionSettings()));
	game->addAction(action);
	
	game->addSeparator();
	
	action = new QAction(tr("&About"), this);
	action->setShortcut(tr("CTRL+A"));
	connect(action, SIGNAL(triggered()), this, SLOT(actionAbout()));
	game->addAction(action);
	
#ifdef DEBUG
	game->addSeparator();
	
	action = new QAction(tr("Test"), this);
	connect(action, SIGNAL(triggered()), this, SLOT(actionTest()));
	game->addAction(action);
#endif
	
	menuBar()->addMenu(game)->setText(tr("&Game"));
}

void WMain::addLog(const QString &line)
{
	m_pChat->addMessage(line, Qt::darkGreen);
}

void WMain::addChat(const QString &from, const QString &text)
{
	m_pChat->addMessage(text, from);
}

void WMain::addServerMessage(const QString &text)
{
	m_pChat->addMessage(text, Qt::blue);
}

void WMain::addServerErrorMessage(int code, const QString &text)
{
	QString qmsg = tr("Error") + ": " + text + " (Code: " + QString::number(code) + ")";
	m_pChat->addMessage(qmsg, Qt::red);
}

void WMain::addPlayer(const QString& name)
{
//	modelPlayerList->appendRow(new QStandardItem(name));
	modelPlayerList->add(name);
}

void WMain::updateGamelist(
	int gid,
	const QString& name,
	const QString& currentPlayers,
	const QString& maxPlayers)
{
	// QStandardItem * QStandardItemModel::itemFromIndex ( const QModelIndex & index )
	// TODO: check for memory garbage
	modelGameList->setItem(gid, 0, new QStandardItem(name));
	modelGameList->setItem(gid, 1, new QStandardItem("gametype"));
	modelGameList->setItem(gid, 2, new QStandardItem(currentPlayers));
	modelGameList->setItem(gid, 3, new QStandardItem(maxPlayers));
	modelGameList->setItem(gid, 4, new QStandardItem("gamestate"));
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
		
		clearGamelist();
	}
}

void WMain::clearGamelist()
{
	// TODO: check for memory holes
	modelGameList->removeRows(0, modelGameList->rowCount());
}

void WMain::clearPlayerlist()
{
	modelPlayerList->clear();
}

void WMain::slotSrvTextChanged()
{
	updateConnectionStatus();
}

QString WMain::getUsername() const
{
	return QString::fromStdString(config.get("player_name"));
}

void WMain::actionConnect()
{
	if (!editSrvAddr->text().length() || ((PClient*)qApp)->isConnected())
		return;
	
	if (!((PClient*)qApp)->doConnect(editSrvAddr->text(), config.getInt("default_port")))
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
#ifdef DEBUG
	WTable *table = new WTable(0, 0);
	table->slotShow();
#endif
}

void WMain::actionRegister()
{
	QItemSelectionModel *pSelect = viewGameList->selectionModel();
	
	if (pSelect->hasSelection())
	{
		const int gid = pSelect->selectedRows().at(0).row();
		((PClient*)qApp)->doRegister(gid, true);
	}
}

void WMain::actionUnregister()
{
	QItemSelectionModel *pSelect = viewGameList->selectionModel();
	
	if (pSelect->hasSelection())
	{
		const int gid = pSelect->selectedRows().at(0).row();
		((PClient*)qApp)->doRegister(gid, false);
	}
}

void WMain::actionSettings()
{
	char cfgfile[1024];
	snprintf(cfgfile, sizeof(cfgfile), "%s/client.cfg", sys_config_path());
	
	SettingsDialog dialogSettings(cfgfile, config);
	dialogSettings.exec();
}

void WMain::actionAbout()
{
	AboutDialog dialogAbout;
	dialogAbout.exec();
}

void WMain::actionChat(QString msg)
{
	((PClient*)qApp)->chatAll(msg);
}

void WMain::gameListSelectionChanged(
	const QItemSelection& selected,
	const QItemSelection& deselected)
{
	if (!selected.isEmpty())
	{
		clearPlayerlist();

		((PClient*)qApp)->requestPlayerlist((*selected.begin()).topLeft().row());
	}
}

void WMain::closeEvent(QCloseEvent *event)
{
	if (((PClient*)qApp)->isTableWindowRemaining())
	{
		event->ignore();
		showMinimized();
	}
	else
		event->accept();
}

