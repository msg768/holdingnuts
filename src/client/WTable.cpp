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
 *     Michael Miller <michael.miller@holdingnuts.net>
 */


#include <cstdio>
#include <cmath>
#include <numeric>

#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOption>
#include <QTime>
#include <QGraphicsPixmapItem>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QShortcut>
#include <QMenu>

#include "Config.h"
#include "Debug.h"
#include "Logger.h"
#include "ConfigParser.hpp"
#include "SysAccess.h"
#include "GameLogic.hpp"

#include "pclient.hpp"

#include "WTable.hpp"
#include "ChatBox.hpp"
#include "Seat.hpp"
#include "DealerButton.hpp"
#include "EditableSlider.hpp"
#include "TimeOut.hpp"

#ifndef NOAUDIO
# include "Audio.h"
#endif
#include "data.h"


using namespace std;

extern ConfigParser config;

#ifdef DEBUG
#	include <QDebug>

QDebug operator << (QDebug s, const Table::State& state)
{
	switch (state)
	{
		case Table::GameStart:
				s << "GameStart";
			break;
		case Table::ElectDealer:
				s << "ElectDealer";
			break;
		case Table::NewRound:
				s << "NewRound";
			break;
		case Table::Blinds:
				s << "Blinds";
			break;
		case Table::Betting:
				s << "Betting";
			break;
		case Table::AskShow:
				s << "AskShow";
			break;
		case Table::AllFolded:
				s << "AllFolded";
			break;
		case Table::Showdown:
				s << "Showdown";
			break;
		case Table::EndRound:
				s << "EndRound";
			break;
		default:
			s << static_cast<int>(state);
	}
	return s;
}

QDebug operator << (QDebug s, const Table::BettingRound& r)
{
	switch (r)
	{
		case Table::Preflop:
				s << "Preflop";
			break;
		case Table::Flop:
				s << "Flop";
			break;
		case Table::Turn:
				s << "Turn";
			break;
		case Table::River:
				s << "River";
			break;
		default:
			s << static_cast<int>(r);
	}
	return s;
}

QDebug operator << (QDebug s, const Player::PlayerAction& p)
{
	switch (p)
	{
		case Player::None:
				s << "None";
			break;
		case Player::ResetAction:
				s << "ResetAction";
			break;
		case Player::Check:
				s << "Check";
			break;
		case Player::Fold:
				s << "Fold";
			break;
		case Player::Call:
				s << "Call";
			break;
		case Player::Bet:
				s << "Bet";
			break;
		case Player::Raise:
				s << "Raise";
			break;
		case Player::Allin:
				s << "Allin";
			break;
		case Player::Show:
				s << "Show";
			break;
		case Player::Muck:
				s << "Muck";
			break;
		case Player::Sitout:
				s << "Sitout";
			break;
		case Player::Back:
				s << "Back";
			break;
		default:
			s << "unkown";
	}
	return s;
}

QDebug operator << (QDebug s, const seatinfo& si)	
{
	s << "seatinfo" << "\n";
	s << "\t client_id= " << si.client_id << "\n";
	s << "\t valid= " << (si.valid ? "true" : "false") << "\n";
	s << "\t in_round= " << (si.in_round ? "true" : "false") << "\n";
	s << "\t sitout= " << (si.sitout ? "true" : "false") << "\n";
	s << "\t PlayerAction= " << si.action << "\n";
	s << "\t bet= " << si.bet << "\n";
	s << "\t stake= " << si.stake << "\n";
	s << "end seatinfo" << "\n";
	
	return s;
}

QDebug operator << (QDebug s, const table_snapshot& t)
{
	s << "table_snapshot\n";
	s << "\t state= " << Table::State(t.state) << "\n";
	s << "\t betting_round= " << Table::BettingRound(t.betting_round) << "\n";
	s << "\t s_dealer= " << t.s_dealer << "\n";
	s << "\t small blind= " << t.s_sb << "\n";
	s << "\t big blind= " << t.s_bb << "\n";
	s << "\t s_cur= " << t.s_cur << "\n";
	s << "\t s_lastbet= " << t.s_lastbet << "\n";
	s << "\t minimum_bet= " << t.minimum_bet << "\n";

	s << "\t pots= ";
	for(unsigned i = 0; i < t.pots.size(); ++i)
		s << "[" << i << "]= " << t.pots.at(i);
	s << "\n";

	s << "\t my_seat= " << t.my_seat << "\n";
	s << "end table_snapshot" << "\n";
	
	return s;
}

#endif /* DEBUG */

const unsigned int WTable::nMaxSeats = 10;

WTable::WTable(int gid, int tid, QWidget *parent)
:	QWidget(parent),
	m_nGid(gid),
	m_nTid(tid),
	m_pImgTable(0)
{
	//setAttribute(Qt::WA_DeleteOnClose); // FIXME: implement correctly

	// scene
	m_pScene = new QGraphicsScene(this);

	//m_pScene->setBackgroundBrush(Qt::black);//QPixmap("gfx/table/background.png"));
	// don't use bsptree
	m_pScene->setItemIndexMethod(QGraphicsScene::NoIndex);

	m_pImgTable = new QGraphicsPixmapItem(QPixmap("gfx/table/table.png"));
	m_pImgTable->setTransformationMode(Qt::SmoothTransformation);

	m_pScene->addItem(m_pImgTable);

	m_pDealerButton = new DealerButton;
	m_pDealerButton->setPos(m_pImgTable->boundingRect().center());
	
	m_pScene->addItem(m_pDealerButton);

	m_pTimeout = new TimeOut;
	m_pTimeout->hide();

	connect(m_pTimeout, SIGNAL(timeup(int)), this, SLOT(slotTimeup(int)));

	m_pScene->addItem(m_pTimeout);

	for (unsigned int j = 0; j < 5; j++)
	{
		m_CommunityCards[j] = new QGraphicsPixmapItem(
			QPixmap(QString("gfx/deck/%1/blank.png")
				.arg(QString::fromStdString(config.get("ui_card_deck")))));

		m_CommunityCards[j]->setTransformationMode(Qt::SmoothTransformation);
		m_CommunityCards[j]->scale(1.1, 1.1);
		m_CommunityCards[j]->setZValue(5.0);
		m_CommunityCards[j]->setPos(calcCCardsPos(j));
		m_CommunityCards[j]->hide();

		m_pScene->addItem(m_CommunityCards[j]);
	}

	for (unsigned int i = 0; i < nMaxSeats; i++)
	{
		wseats[i] = new Seat(i, this);
		wseats[i]->setPos(calcSeatPos(i));

		m_pScene->addItem(wseats[i]);
		
		// calculate dealer button pos
		m_ptDealerBtn[i] = calcDealerBtnPos(i);
	}

	QFont font = QApplication::font();
	
	font.setPointSize(20); 
	font.setBold(true);
	
	const QFontMetrics fm(font);
	const QPointF ptCenter = m_pImgTable->boundingRect().center();
	
	m_pTxtPots = m_pScene->addSimpleText("Main pot: 0.00", font);
	m_pTxtPots->setPos(calcPotsPos());
	m_pTxtPots->setZValue(3);
	
	font.setBold(false);
	
	m_pTxtHandStrength = m_pScene->addSimpleText("HandStrength", font);
	m_pTxtHandStrength->setPos(calcHandStrengthPos());
	m_pTxtHandStrength->setZValue(3);
	m_pTxtHandStrength->setVisible(config.getBool("ui_show_handstrength"));

	// view
	m_pView = new QGraphicsView(m_pScene);
	m_pView->setRenderHint(QPainter::SmoothPixmapTransform);
	m_pView->setCacheMode(QGraphicsView::CacheNone);
	m_pView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_pView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_pView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	m_pView->setOptimizationFlag(QGraphicsView::DontClipPainter, true);
	m_pView->setFrameStyle(QFrame::Plain);
	m_pView->setStyleSheet("background: transparent");
	
	
	// fixed width for action buttons which have an amount in their caption
	const unsigned int actionbtn_width = 100;
	
	// ui - widgets
	QPushButton *btnFold = new QPushButton(tr("&Fold"), this);
	btnFold->setFixedWidth(actionbtn_width);
	connect(btnFold, SIGNAL(clicked()), this, SLOT(actionFold()));
	
	btnCheckCall = new QPushButton("Check/Call", this);
	btnCheckCall->setFixedWidth(actionbtn_width);
	connect(btnCheckCall, SIGNAL(clicked()), this, SLOT(actionCheckCall()));
	
	btnBetRaise = new QPushButton("Bet/Raise", this);
	btnBetRaise->setFixedWidth(actionbtn_width);
	connect(btnBetRaise, SIGNAL(clicked()), this, SLOT(actionBetRaise()));
	
	QPushButton *btnShow = new QPushButton(tr("&Show"), this);
	connect(btnShow, SIGNAL(clicked()), this, SLOT(actionShow()));
	
	QPushButton *btnMuck = new QPushButton(tr("&Muck"), this);
	connect(btnMuck, SIGNAL(clicked()), this, SLOT(actionMuck()));
	
	QPushButton *btnBack = new QPushButton(tr("I'm bac&k"), this);
	connect(btnBack, SIGNAL(clicked()), this, SLOT(actionBack()));
	
	QPushButton *btnSitout = new QPushButton(tr("Sit&out"), this);
	connect(btnSitout, SIGNAL(clicked()), this, SLOT(actionSitout()));

#ifdef RAISE_MNU
	actRaiseQuarterPot = new QAction(tr("1/4"), this);
	actRaiseHalfPot = new QAction(tr("1/2"), this);
	actRaiseThreeQuarterPot = new QAction(tr("3/4"), this);
	actRaisePotsize = new QAction(tr("Potsize"), this);
	actAllin = new QAction(tr("Allin"), this);

	connect(actRaiseQuarterPot, SIGNAL(triggered()), this, SLOT(actionRaiseQuarterPot()));
	connect(actRaiseHalfPot, SIGNAL(triggered()), this, SLOT(actionRaiseHalfPot()));
	connect(actRaiseThreeQuarterPot, SIGNAL(triggered()), this, SLOT(actionRaiseThreeQuarterPot()));
	connect(actRaisePotsize, SIGNAL(triggered()), this, SLOT(actionRaisePotsize()));
	connect(actAllin, SIGNAL(triggered()), this, SLOT(actionAllin()));

	QMenu *mnuRaise = new QMenu(this);

	mnuRaise->addAction(actRaiseQuarterPot);
	mnuRaise->addAction(actRaiseHalfPot);
	mnuRaise->addAction(actRaiseThreeQuarterPot);
	mnuRaise->addAction(actRaisePotsize);
	mnuRaise->addAction(actAllin);

	connect(mnuRaise, SIGNAL(aboutToShow()), this, SLOT(showMenuRaise()));

	btnRaisePot = new QPushButton(this);
	btnRaisePot->setMenu(mnuRaise);
#else
	const unsigned int raisebtn_width = 39;
	const unsigned int raisebtn_height = 18;
	
	btnQuarterPot = new QPushButton(tr("1/4"), this);
	btnQuarterPot->setFixedSize(raisebtn_width, raisebtn_height);
	connect(btnQuarterPot, SIGNAL(clicked()), this, SLOT(actionRaiseQuarterPot()));

	btnHalfPot = new QPushButton(tr("1/2"), this);
	btnHalfPot->setFixedSize(raisebtn_width, raisebtn_height);
	connect(btnHalfPot, SIGNAL(clicked()), this, SLOT(actionRaiseHalfPot()));

	btnThreeQuarterPot = new QPushButton(tr("3/4"), this);
	btnThreeQuarterPot->setFixedSize(raisebtn_width, raisebtn_height);
	connect(btnThreeQuarterPot, SIGNAL(clicked()), this, SLOT(actionRaiseThreeQuarterPot()));

	btnPotsize = new QPushButton(tr("Potsize"), this);
	btnPotsize->setFixedSize(raisebtn_width, raisebtn_height);
	connect(btnPotsize, SIGNAL(clicked()), this, SLOT(actionRaisePotsize()));

	btnAllin = new QPushButton(tr("Allin"), this);
	btnAllin->setFixedSize(raisebtn_width, raisebtn_height);
	connect(btnAllin, SIGNAL(clicked()), this, SLOT(actionAllin()));

	QHBoxLayout *lPots = new QHBoxLayout();
	lPots->addStretch(2); 
	lPots->setContentsMargins(0, 0, 0, 0);
	lPots->addWidget(btnQuarterPot, Qt::AlignRight);
	lPots->addWidget(btnHalfPot, Qt::AlignRight);
	lPots->addWidget(btnThreeQuarterPot, Qt::AlignRight);
	lPots->addWidget(btnPotsize, Qt::AlignRight);
	lPots->addWidget(btnAllin, Qt::AlignRight);
	
	wRaiseBtns = new QWidget(this);
	wRaiseBtns->setLayout(lPots);
#endif

	chkAutoFoldCheck = new QCheckBox("Fold/Check", this);
	chkAutoCheckCall = new QCheckBox("Check/Call", this);
	connect(chkAutoCheckCall, SIGNAL(stateChanged(int)), this, SLOT(actionAutoCheckCall(int)));
	
	m_pSliderAmount = new EditableSlider(this);
	connect(m_pSliderAmount, SIGNAL(dataChanged()), this, SLOT(slotBetRaiseAmountChanged()));
	connect(m_pSliderAmount, SIGNAL(returnPressed()), this, SLOT(actionBetRaise()));

	QHBoxLayout *lActionsBtns = new QHBoxLayout();
	lActionsBtns->addWidget(btnFold);
	lActionsBtns->addWidget(btnCheckCall);
	lActionsBtns->addWidget(btnBetRaise);
	lActionsBtns->addWidget(m_pSliderAmount);
#ifdef RAISE_MNU
	lActionsBtns->addWidget(btnRaisePot);
#endif

	QVBoxLayout *lActions = new QVBoxLayout();
#ifndef RAISE_MNU	
	lActions->addWidget(wRaiseBtns);
#endif	
	lActions->addLayout(lActionsBtns);
	
	QHBoxLayout *lPreActions = new QHBoxLayout();
	lPreActions->addWidget(chkAutoFoldCheck);
	lPreActions->addWidget(chkAutoCheckCall);
	lPreActions->addWidget(btnSitout);   // FIXME: display outside StackedLayout
	
	QHBoxLayout *lPostActions = new QHBoxLayout();
	lPostActions->addWidget(btnMuck);
	lPostActions->addWidget(btnShow);
	
	QHBoxLayout *lSitoutActions = new QHBoxLayout();
	lSitoutActions->addWidget(btnBack);
	
	stlayActions = new QStackedLayout();
	QWidget *pageActions = new QWidget(this);
	QWidget *pagePreActions = new QWidget(this);
	QWidget *pagePostActions = new QWidget(this);
	QWidget *pageNoActions = new QWidget(this);
	QWidget *pageSitoutActions = new QWidget(this);
	
	pageActions->setLayout(lActions);
	pagePreActions->setLayout(lPreActions);
	pagePostActions->setLayout(lPostActions);
	pageSitoutActions->setLayout(lSitoutActions);
	
	m_nActions = stlayActions->addWidget(pageActions);
	m_nPreActions = stlayActions->addWidget(pagePreActions);
	m_nPostActions = stlayActions->addWidget(pagePostActions);
	m_nNoAction = stlayActions->addWidget(pageNoActions);
	m_nSitoutActions = stlayActions->addWidget(pageSitoutActions);
	
	QLabel *lblActions = new QLabel(this);
	lblActions->setPixmap(QPixmap("gfx/table/actions.png"));
	lblActions->setScaledContents(true);
#ifdef RAISE_MNU
	lblActions->setFixedSize(450, 70);
#else
	lblActions->setFixedSize(450, 80);
#endif
	lblActions->setLayout(stlayActions);

	m_pChat	= new ChatBox(ChatBox::INPUTLINE_BOTTOM, 0, this);
	m_pChat->setFixedHeight(150);
	m_pChat->setFontPointSize(m_pChat->fontPointSize() - 1);
	connect(m_pChat, SIGNAL(dispatchedMessage(QString)), this, SLOT(actionChat(QString)));

	QGridLayout *mainLayout = new QGridLayout(this);

	mainLayout->addWidget(m_pView, 0, 0, 1, 5);
	mainLayout->addWidget(m_pChat, 1, 0);
	mainLayout->addWidget(lblActions, 1, 3);
	mainLayout->setColumnStretch(2, 2);
	mainLayout->setColumnStretch(4, 2);

	this->setLayout(mainLayout);
	
	// shortcuts
	shortcutFold = new QShortcut(tr("Ctrl+F"), btnFold);
	connect(shortcutFold, SIGNAL(activated()), this, SLOT(actionFold()));

	shortcutCallCheck = new QShortcut(tr("Ctrl+C"), btnCheckCall);
	connect(shortcutCallCheck, SIGNAL(activated()), this, SLOT(actionCheckCall()));

	shortcutBet = new QShortcut(tr("Ctrl+B"), btnBetRaise);
	connect(shortcutBet, SIGNAL(activated()), this, SLOT(actionBetRaise()));

	shortcutRaise = new QShortcut(tr("Ctrl+R"), btnBetRaise);
	connect(shortcutRaise, SIGNAL(activated()), this, SLOT(actionBetRaise()));

	shortcutAllin = new QShortcut(tr("Ctrl+A"), btnBetRaise);
	shortcutAllin->setEnabled(false);
	connect(shortcutAllin, SIGNAL(activated()), this, SLOT(actionBetRaise()));

	shortcutMuck = new QShortcut(tr("Ctrl+M"), btnMuck);
	connect(shortcutMuck, SIGNAL(activated()), this, SLOT(actionMuck()));

	shortcutShow = new QShortcut(tr("Ctrl+S"), btnShow);
	connect(shortcutShow, SIGNAL(activated()), this, SLOT(actionShow()));
	
	shortcutSitout = new QShortcut(tr("Ctrl+O"), this);
	connect(shortcutSitout, SIGNAL(activated()), this, SLOT(actionSitout()));
	
	shortcutBack = new QShortcut(tr("Ctrl+K"), this);
	connect(shortcutBack, SIGNAL(activated()), this, SLOT(actionBack()));
	
	// assign shortcut for making screenshot
	QShortcut *shortcutScreenshot = new QShortcut(Qt::Key_F10, this);
	connect(shortcutScreenshot, SIGNAL(activated()), this, SLOT(actionScreenshot()));
	
	// set background
	QPalette p(this->palette());
	p.setBrush(QPalette::Window, QBrush(QPixmap("gfx/table/background.png")));
	this->setPalette(p);	
	
	this->setMinimumSize(640, 480);
	this->setWindowTitle(tr("HoldingNuts table"));
	this->setWindowIcon(QIcon(":/res/hn_logo.png"));
	this->resize(800, 630);
}

WTable::~WTable()
{
	delete m_pImgTable;
	delete m_pDealerButton;
	delete m_pTimeout;
	delete m_pTxtPots;
	delete m_pTxtHandStrength;

	for (unsigned int j = 0; j < 5; j++)
		delete m_CommunityCards[j];

	for (unsigned int i = 0; i < nMaxSeats; i++)
		delete wseats[i];
		
	delete stlayActions;
}

QPointF WTable::calcSeatPos(unsigned int nSeatID) const
{
	Q_ASSERT_X(nSeatID < nMaxSeats, Q_FUNC_INFO, "invalid Seat Number");
	Q_ASSERT_X(wseats[nSeatID], Q_FUNC_INFO, "bad seat pointer");
	Q_ASSERT_X(m_pImgTable, Q_FUNC_INFO, "bad table image pointer");

	//		8	9	0
	//	 7			   1
	// 						
	//   6			   2
	//		5	4	3
	
	const QRectF& rcSeat = wseats[nSeatID]->boundingRectSeat();
	const QRectF& rcTable = this->m_pImgTable->boundingRect();

	switch (nSeatID)
	{
		case 0:
			return QPointF(rcTable.width() * 0.65, 0);
		case 1:
			return QPointF(rcTable.width() - rcSeat.width() * 0.75, rcTable.height() * 0.30);
		case 2:
			return QPointF(rcTable.width() - rcSeat.width() * 0.75, rcTable.height() * 0.65);
		case 3:
			return QPointF(rcTable.width() * 0.65, rcTable.height() - rcSeat.height());
		case 4:
			return QPointF(rcTable.width() * 0.5 - rcSeat.width() * 0.5, rcTable.height() - rcSeat.height());
		case 5:
			return QPointF(rcTable.width() * 0.35 - rcSeat.width(), rcTable.height() - rcSeat.height());
		case 6:
			return QPointF(-(rcSeat.width() * 0.25), rcTable.height() * 0.65);
		case 7:
			return QPointF(-(rcSeat.width() * 0.25), rcTable.height() * 0.30);
		case 8:
			return QPointF(rcTable.width() * 0.35 - rcSeat.width(), 0);
		case 9:
			return QPointF(rcTable.width() * 0.5 - rcSeat.width() * 0.5, 0);
	}
	return QPointF(0, 0);
}

QPointF WTable::calcCCardsPos(unsigned int nCard) const
{
	Q_ASSERT_X(nCard < 5, Q_FUNC_INFO, "invalid Card Number");
	Q_ASSERT_X(m_pScene, Q_FUNC_INFO, "bad scene pointer");
	Q_ASSERT_X(m_CommunityCards[nCard], Q_FUNC_INFO, "bad community card pointer");

	QRectF rc = m_CommunityCards[nCard]->boundingRect();
	const QTransform m = m_CommunityCards[nCard]->transform();
	
	rc = m.mapRect(rc);

	const qreal card_spacing = rc.width() * 0.1;
	const qreal card_width = rc.width() + card_spacing;

	return QPointF(
		((m_pScene->width() - (5 * card_width - card_spacing)) / 2) + nCard * card_width,
		m_pScene->height() * 0.4);
}

QPointF WTable::calcTimeoutPos(unsigned int nSeatID) const
{
	Q_ASSERT_X(nSeatID < nMaxSeats, Q_FUNC_INFO, "invalid Seat Number");
	Q_ASSERT_X(wseats[nSeatID], Q_FUNC_INFO, "bad seat pointer");
	Q_ASSERT_X(m_pTimeout, Q_FUNC_INFO, "bad timeout pointer");
	
	QPointF pt = wseats[nSeatID]->scenePos();

	pt.ry() += wseats[nSeatID]->boundingRectSeat().height();

	return pt;
}

QPointF WTable::calcHandStrengthPos() const
{
	Q_ASSERT_X(m_pTxtHandStrength, Q_FUNC_INFO, "bad hand strength pointer");
	Q_ASSERT_X(m_pImgTable, Q_FUNC_INFO, "bad table image pointer");

	static const QFontMetrics fm(m_pTxtHandStrength->font());
	static const QPointF ptCenter = m_pImgTable->boundingRect().center();
	
	return QPointF(
		ptCenter.x() - (fm.width(m_pTxtHandStrength->text()) / 2),
		230 + fm.height());
}

QPointF WTable::calcPotsPos() const
{
	Q_ASSERT_X(m_pTxtPots, Q_FUNC_INFO, "bad text pots pointer");
	Q_ASSERT_X(m_pImgTable, Q_FUNC_INFO, "bad table image pointer");

	static const QFontMetrics fm(m_pTxtPots->font());
	static const QPointF ptCenter = m_pImgTable->boundingRect().center();
	
	return QPointF(
		ptCenter.x() - (fm.width(m_pTxtPots->text()) / 2),
		230);
}

QPointF WTable::calcDealerBtnPos(
	unsigned int nSeatID, 
	int offset) const
{
	Q_ASSERT_X(nSeatID < nMaxSeats, Q_FUNC_INFO, "invalid Seat Number");
	Q_ASSERT_X(wseats[nSeatID], Q_FUNC_INFO, "bad seat pointer");
	Q_ASSERT_X(m_pDealerButton, Q_FUNC_INFO, "bad table image pointer");
	
	QPointF pt = wseats[nSeatID]->sceneBoundingRect().center();

	//		8	9	0
	//	 7			   1
	// 						
	//   6			   2
	//		5	4	3
	
	switch (nSeatID)
	{
		case 0: case 8: case 9:
				pt.ry() += (wseats[nSeatID]->sceneBoundingRect().height() * 0.5f + offset);
			break;
		case 1: case 2:
				pt.rx() -= (
					wseats[nSeatID]->sceneBoundingRect().width() * 0.5f + 
					m_pDealerButton->sceneBoundingRect().width() + 
					offset);
			break;
		case 3: case 4: case 5:
				pt.ry() -= (
					wseats[nSeatID]->sceneBoundingRect().height() * 0.5f + 
					m_pDealerButton->sceneBoundingRect().height() + 
					offset);
			break;
		case 6: case 7:
				pt.rx() += (wseats[nSeatID]->sceneBoundingRect().width() * 0.5f + offset);
			break;
	}

	return pt;
}

void WTable::evaluateActions(const table_snapshot *snap)
{
	// player does not sit at table
	if (snap->my_seat == -1)
	{
		stlayActions->setCurrentIndex(m_nNoAction);
		return;
	}
	
	
	const seatinfo *s = &(snap->seats[snap->my_seat]);
	
	// minimum bet
	if (snap->minimum_bet > s->stake)
	{
		m_pSliderAmount->setMinimum(s->stake + s->bet);
		m_pSliderAmount->setEnabled(false);
	}
	else
	{
		m_pSliderAmount->setMinimum(snap->minimum_bet);
		m_pSliderAmount->setEnabled(true);
	}
	
	// maximum bet is stake size
	m_pSliderAmount->setMaximum(s->stake + s->bet);
	
	
	// evaluate available actions
	if (s->sitout)
	{
		stlayActions->setCurrentIndex(m_nSitoutActions);
	}
	else if (!s->in_round ||
		!(snap->state == Table::Blinds ||
			snap->state == Table::Betting ||
			snap->state == Table::AskShow)
		/*|| isNoMoreActionPossible(snap)*/)  // FIXME: doesn't work that easy
	{
		stlayActions->setCurrentIndex(m_nNoAction);
	}
	else if (snap->state == Table::AskShow)
	{
		if (snap->s_cur == (unsigned int)snap->my_seat)
			stlayActions->setCurrentIndex(m_nPostActions);
		else
			stlayActions->setCurrentIndex(m_nNoAction);
	}
	else
	{
		// re-enable if actions were disabled by something else
		btnCheckCall->setEnabled(true);
		btnBetRaise->setEnabled(true);
		
		
		qreal greatest_bet = 0;
		bool bGreaterBet = greaterBet(snap, s->bet, &greatest_bet);
		
		if (snap->s_cur == (unsigned int)snap->my_seat)
		{
			if ((int)s->stake == 0)
				stlayActions->setCurrentIndex(m_nNoAction);
			else
			{
				if (bGreaterBet)
				{
					btnCheckCall->setText(tr("&Call %1").arg(greatest_bet - s->bet, 0, 'f', 2));
					btnBetRaise->setText(tr("&Raise %1").arg(m_pSliderAmount->value(), 0, 'f', 2));
					
					shortcutBet->setEnabled(false);
					shortcutRaise->setEnabled(true);
				}
				else
				{
					btnCheckCall->setText(tr("&Check"));
					
					if (greaterBet(snap, 0))
					{
						btnBetRaise->setText(tr("&Raise %1").arg(m_pSliderAmount->value(), 0, 'f', 2));
						
						shortcutBet->setEnabled(false);
						shortcutRaise->setEnabled(true);
					}
					else
					{
						btnBetRaise->setText(tr("&Bet %1").arg(m_pSliderAmount->value(), 0, 'f', 2));
						
						shortcutBet->setEnabled(true);
						shortcutRaise->setEnabled(false);
					}
				}
				
				// handle allin-case
				if (greatest_bet >= s->stake + s->bet)
				{
					btnCheckCall->setVisible(false);
					m_pSliderAmount->setVisible(false);
#ifdef RAISE_MNU
					btnRaisePot->setVisible(false);
#else
					wRaiseBtns->setVisible(false);
#endif					
					btnBetRaise->setText(tr("&Allin %1").arg(s->stake, 0, 'f', 2));
					shortcutAllin->setEnabled(true);
					shortcutBet->setEnabled(false);
					shortcutRaise->setEnabled(false);
				}
				else if (snap->minimum_bet >= s->stake + s->bet)
				{
					btnCheckCall->setVisible(true);
					m_pSliderAmount->setVisible(false);
#ifdef RAISE_MNU
					btnRaisePot->setVisible(false);
#else
					wRaiseBtns->setVisible(false);
#endif					
					btnBetRaise->setText(tr("&Allin %1").arg(s->stake, 0, 'f', 2));
					shortcutAllin->setEnabled(true);
					shortcutBet->setEnabled(false);
					shortcutRaise->setEnabled(false);
				}
				else
				{
					btnCheckCall->setVisible(true);
					m_pSliderAmount->setVisible(true);
					shortcutAllin->setEnabled(false);
					shortcutBet->setEnabled(true);
					shortcutRaise->setEnabled(true);
				}
				
				stlayActions->setCurrentIndex(m_nActions);
				
#ifndef RAISE_MNU
				const float cur_pot = currentPot();
	
				btnQuarterPot->setEnabled(cur_pot * 0.25f > snap->minimum_bet);
				btnHalfPot->setEnabled(cur_pot * 0.5f > snap->minimum_bet);
				btnThreeQuarterPot->setEnabled(cur_pot * 0.75f > snap->minimum_bet);
				btnPotsize->setEnabled(cur_pot >= snap->minimum_bet);
#endif				
			}
		}
		else
		{
			if ((int)s->stake == 0 || (!bGreaterBet && snap->s_lastbet == (unsigned int)snap->my_seat))  // FIXME: do not show actions if there is no action possible for this betting round
				stlayActions->setCurrentIndex(m_nNoAction);
			else
			{
				if (bGreaterBet)
				{
					chkAutoFoldCheck->setText(tr("Fold"));
					
					if (greatest_bet >= s->stake + s->bet)
						chkAutoCheckCall->setText(tr("Allin %1").arg(s->stake, 0, 'f', 2));
					else
						chkAutoCheckCall->setText(tr("Call %1").arg(greatest_bet - s->bet, 0, 'f', 2));
				}
				else
				{
					chkAutoFoldCheck->setText(tr("Check/Fold"));
					chkAutoCheckCall->setText(tr("Check"));
				}
				
				stlayActions->setCurrentIndex(m_nPreActions);
			}
		}
	}
}

unsigned int WTable::seatToCentralView(int my, unsigned int seat) const
{
	int mapped_seat;
	if (my != -1 && config.getBool("ui_centralized_view"))
	{
		mapped_seat = (seat - my) + 4;
		if (mapped_seat > 9)
			mapped_seat -= 10;
		else if (mapped_seat < 0)
			mapped_seat += 10;
	}
	else
		mapped_seat = seat;
	
	return mapped_seat;
}

void WTable::updateView()
{
	int my_cid = ((PClient*)qApp)->getMyCId();
	
	const gameinfo *ginfo = ((PClient*)qApp)->getGameInfo(m_nGid);
	if (!ginfo)
		return;
	
	const tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	if (!tinfo)
		return;

	const table_snapshot *snap = &(tinfo->snap);
	Q_ASSERT_X(snap, Q_FUNC_INFO, "invalid snapshot pointer");
	
	
	for (unsigned int i=0; i < nMaxSeats; i++)
	{
		const seatinfo *seat = &(snap->seats[i]);
		
		// centralized view
		const unsigned int mapped_seat = seatToCentralView(snap->my_seat, i);
		
		
		Seat *ui_seat = wseats[mapped_seat];
		
		if (seat->valid)
		{
			int cid = seat->client_id;
			playerinfo *pinfo = ((PClient*)qApp)->getPlayerInfo(cid);
			
			if (pinfo)
				ui_seat->setInfo(pinfo->name, pinfo->location);
			else
				ui_seat->setInfo(QString("??? (%1)").arg(cid), "");
			
			ui_seat->setStake(seat->stake);
			ui_seat->setValid(true);
			ui_seat->setSitout(seat->sitout);
			
			if (snap->state > Table::ElectDealer)
			{
				ui_seat->setCurrent(snap->s_cur == i);
				
				if ((snap->state == Table::Blinds || 
					snap->state == Table::Betting) &&
					snap->seats[snap->s_cur].stake > 0 &&
					snap->seats[snap->s_cur].sitout == false)
				{
					const unsigned int curseat_mapped = seatToCentralView(snap->my_seat, snap->s_cur);
					m_pTimeout->setPos(calcTimeoutPos(curseat_mapped));
					m_pTimeout->start(curseat_mapped, ginfo->player_timeout);
					m_pTimeout->show();
				}
				else
				{
					m_pTimeout->hide();
					m_pTimeout->stop();
				}
			}
			
			if (seat->in_round)
			{
				if (snap->state == Table::EndRound)
				{
					ui_seat->setWin(seat->bet);
				}
				else
				{
					if (seat->stake == 0)
						ui_seat->setAction(Player::Allin, seat->bet);
					else
						ui_seat->setAction(seat->action, seat->bet);
				}
				
				ui_seat->setMySeat(my_cid == cid);
				
				std::vector<Card> allcards;
				
				if (my_cid == cid)
					tinfo->holecards.copyCards(&allcards);
				else
					seat->holecards.copyCards(&allcards);
				
				if (allcards.size())
				{
					char card1[3], card2[3];
					strcpy(card1, allcards[0].getName());
					strcpy(card2, allcards[1].getName());
					ui_seat->setCards(card1, card2);
					
					ui_seat->showBigCards(true);
				}
				else
				{
					if (my_cid == cid)
						ui_seat->setCards("blank", "blank");
					else
						ui_seat->setCards("back", "back");
					
					ui_seat->showBigCards(false);
				}
				
				ui_seat->showSmallCards(true);
			}
			else   // player isn't anymore involved in current hand
			{
				ui_seat->setAction(seat->action);
				ui_seat->setCards("blank", "blank");
				ui_seat->showBigCards(false);
				ui_seat->showSmallCards(false);
			}

			// schedule scene update
			ui_seat->update(ui_seat->boundingRect());
		}
		else
		{
			// if seat was valid force schedule redraw
			if (ui_seat->isValid())
				ui_seat->update(ui_seat->boundingRect());

			ui_seat->setValid(false);
		}
	}
	
	
	// remember last state change
	static int last_state = -1;
	
	if (snap->state == Table::NewRound && last_state != snap->state)
	{
		// dealerbutton
		const unsigned int dealerseat_mapped = seatToCentralView(snap->my_seat, snap->s_dealer);
		m_pDealerButton->startAnimation(m_ptDealerBtn[dealerseat_mapped]);
		
		playSound(SOUND_DEAL_1);
	}
	
	last_state = snap->state;
	
	
	// Pots
	QString strPots;
	if (snap->pots.at(0) > .0f)
	{
		strPots = QString(tr("Main pot: %1").arg(snap->pots.at(0), 0, 'f', 2));
		for (unsigned int t = 1; t < snap->pots.size(); ++t)
		{
			strPots.append(
				QString("  " + tr("Side pot %1: %2")
					.arg(t).arg(snap->pots.at(t), 0, 'f', 2)));
		}
	}
		
	m_pTxtPots->setText(strPots);
	m_pTxtPots->setPos(calcPotsPos());
	
	// CommunityCards
	if (snap->state == Table::NewRound)
		for (unsigned int i = 0; i < 5; i++)
			m_CommunityCards[i]->hide();

	std::vector<Card> allcards;
	snap->communitycards.copyCards(&allcards);
	
	// load community cards
	for (unsigned int i = 0; i < allcards.size(); i++)
	{
		m_CommunityCards[i]->setPixmap(
			QPixmap(
				QString("gfx/deck/%1/%2.png")
					.arg(QString::fromStdString(config.get("ui_card_deck")))
					.arg(allcards[i].getName())));
		m_CommunityCards[i]->show();
	}

	evaluateActions(snap);
	
	
	if (snap->my_seat != -1)
	{
		const seatinfo *s = &(snap->seats[snap->my_seat]);
		
		
		// handle auto-actions
		if (snap->state == Table::Betting)
		{
			if ((int)snap->s_cur == snap->my_seat)
			{
				if (chkAutoFoldCheck->checkState() == Qt::Checked)
				{
					if (greaterBet(snap, s->bet))
						actionFold();
					else
						actionCheckCall();
					
					chkAutoFoldCheck->setCheckState(Qt::Unchecked);
					stlayActions->setCurrentIndex(m_nNoAction);
				}
				else if (chkAutoCheckCall->checkState() == Qt::Checked)
				{
					qreal greatest_bet;
					greaterBet(snap, 0, &greatest_bet);
					
					if (m_autocall_amount >= greatest_bet)
					{
						actionCheckCall();
						stlayActions->setCurrentIndex(m_nNoAction);
					}
					
					chkAutoCheckCall->setCheckState(Qt::Unchecked);
				}
			}
			else  // validate pre-actions
			{
				if (chkAutoCheckCall->checkState() == Qt::Checked)
				{
					qreal greatest_bet;
					greaterBet(snap, 0, &greatest_bet);
					
					if (m_autocall_amount < greatest_bet)
						chkAutoCheckCall->setCheckState(Qt::Unchecked);
				}
			}
		}
		
		
		// determine hand-strength
		if (s->in_round)
		{
			vector<Card> allcards;
			tinfo->holecards.copyCards(&allcards);
			
			if (allcards.size())
			{
				HandStrength strength;
				GameLogic::getStrength(&(tinfo->holecards), &(snap->communitycards), &strength);
				//const char *sstrength = HandStrength::getRankingName(strength.getRanking());
				
				// provide translation for hand strength
				QString sstrength = "unknown hand strength";
				switch (strength.getRanking())
				{
				case HandStrength::HighCard:
					sstrength = tr("High Card");
					break;
				case HandStrength::OnePair:
					sstrength = tr("One Pair");
					break;
				case HandStrength::TwoPair:
					sstrength = tr("Two Pair");
					break;
				case HandStrength::ThreeOfAKind:
					sstrength = tr("Three Of A Kind");
					break;
				case HandStrength::Straight:
					sstrength = tr("Straight");
					break;
				case HandStrength::Flush:
					sstrength = tr("Flush");
					break;
				case HandStrength::FullHouse:
					sstrength = tr("Full House");
					break;
				case HandStrength::FourOfAKind:
					sstrength = tr("Four Of A Kind");
					break;
				case HandStrength::StraightFlush:
					{
						vector<Card> rank;
						strength.copyRankCards(&rank);
						
						// handle RoyalFlush as special case
						if (strength.getRanking() == HandStrength::StraightFlush && rank.front().getFace() == Card::Ace)
							sstrength = tr("Royal Flush");
						else
							sstrength = tr("Straight Flush");
						break;
					}
				}
				
				m_pTxtHandStrength->setText(sstrength);
				m_pTxtHandStrength->setPos(calcHandStrengthPos());
			}
			else
				m_pTxtHandStrength->setText(QString());
		}
		else
			m_pTxtHandStrength->setText(QString());
	}
	else
		m_pTxtHandStrength->setText(QString());
	
	
	// set focus on EditableSlider if focus isn't on ChatBox
	if (focusWidget() != m_pChat->getInputWidget())
		m_pSliderAmount->setFocus();
}

void WTable::addChat(const QString& from, const QString& text)
{
	m_pChat->addMessage(text, from);
}

void WTable::addServerMessage(const QString& text)
{
	m_pChat->addMessage(text, Qt::blue);
}

void WTable::closeEvent(QCloseEvent *event)
{
	// FIXME: handle this case: send sitout if player (my_seat != -1 && sitout == false)
}

void WTable::actionFold()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Fold);
	
	stlayActions->setCurrentIndex(m_nNoAction);
}

void WTable::actionCheckCall()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Call);
	
	stlayActions->setCurrentIndex(m_nNoAction);
}

void WTable::actionBetRaise()
{
	tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	
	Q_ASSERT_X(tinfo, Q_FUNC_INFO, "getTableInfo failed");
	
	table_snapshot *snap = &(tinfo->snap);
	
	Q_ASSERT_X(snap, Q_FUNC_INFO, "invalid snapshot pointer");

	if (snap->my_seat == -1)
		return;
	
	qreal greatest_bet = 0;
	greaterBet(snap, 0, &greatest_bet);
	
	if (greatest_bet >= snap->seats[snap->my_seat].stake + snap->seats[snap->my_seat].bet ||
		snap->minimum_bet >= snap->seats[snap->my_seat].stake + snap->seats[snap->my_seat].bet)
	{
		((PClient*)qApp)->doSetAction(m_nGid, Player::Allin);
	}
	else
		((PClient*)qApp)->doSetAction(m_nGid, Player::Raise, m_pSliderAmount->value());
	
	stlayActions->setCurrentIndex(m_nNoAction);
}

void WTable::actionShow()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Show);
	
	stlayActions->setCurrentIndex(m_nNoAction);
}

void WTable::actionMuck()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Muck);
	
	stlayActions->setCurrentIndex(m_nNoAction);
}

void WTable::doSitout(bool bSitout)
{
	const tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	if (!tinfo)
		return;
	
	const table_snapshot *snap = &(tinfo->snap);
	if (!snap)
		return;
	
	if (snap->my_seat == -1)
		return;
	
	seatinfo *seat = (seatinfo*) &(snap->seats[snap->my_seat]);
	
	// set sitout and show available actions again
	seat->sitout = bSitout;
	evaluateActions(snap);
}

void WTable::actionSitout()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Sitout);
	
	doSitout(true);
}

void WTable::actionBack()
{
	((PClient*)qApp)->doSetAction(m_nGid, Player::Back);
	
	doSitout(false);
}

void WTable::actionAutoCheckCall(int state)
{
	if (state != Qt::Checked)
		return;
	
	const tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	if (!tinfo)
		return;
	
	const table_snapshot *snap = &(tinfo->snap);
	if (!snap)
		return;
	
	qreal greatest_bet;
	greaterBet(snap, 0, &greatest_bet);
	
	m_autocall_amount = greatest_bet;
}

void WTable::slotBetRaiseAmountChanged()
{
	const bool is_valid = m_pSliderAmount->validValue();
	btnBetRaise->setEnabled(is_valid);
	shortcutRaise->setEnabled(is_valid);
	shortcutBet->setEnabled(is_valid);
	
	const QString str = btnBetRaise->text();
	
	btnBetRaise->setText(QString("%1 %2")
				.arg(str.left(str.lastIndexOf(' ')))
				.arg(m_pSliderAmount->value(), 0, 'f', 2));
}

float WTable::currentPot() const
{
	const tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	
	Q_ASSERT_X(tinfo, Q_FUNC_INFO, "getTableInfo failed");
	
	const table_snapshot *snap = &(tinfo->snap);
	
	Q_ASSERT_X(snap, Q_FUNC_INFO, "invalid snapshot pointer");

	float cur_pot = snap->pots.at(snap->pots.size() - 1);
	
	for (unsigned int i=0; i < nMaxSeats; i++)
	{
		const seatinfo *seat = &(snap->seats[i]);

		if (seat->valid && seat->in_round)
			cur_pot += seat->bet;
	}

	return cur_pot;
}

void WTable::actionRaiseQuarterPot()
{
	m_pSliderAmount->setValue(currentPot() * 0.25f);
}

void WTable::actionRaiseHalfPot()
{
	m_pSliderAmount->setValue(currentPot() * 0.5f);
}

void WTable::actionRaiseThreeQuarterPot()
{
	m_pSliderAmount->setValue(currentPot() * 0.75f);
}

void WTable::actionRaisePotsize()
{
	m_pSliderAmount->setValue(currentPot());
}

void WTable::actionAllin()
{
	const tableinfo *tinfo = ((PClient*)qApp)->getTableInfo(m_nGid, m_nTid);
	
	Q_ASSERT_X(tinfo, Q_FUNC_INFO, "getTableInfo failed");
	
	const table_snapshot *snap = &(tinfo->snap);
	
	Q_ASSERT_X(snap, Q_FUNC_INFO, "invalid snapshot pointer");
	
	if (snap->my_seat == -1)
		return;
	
	m_pSliderAmount->setValue(snap->seats[snap->my_seat].stake + snap->seats[snap->my_seat].bet);
}

#ifdef RAISE_MNU
void WTable::showMenuRaise()
{
	const float cur_pot = currentPot();
	
	actRaiseQuarterPot->setEnabled(cur_pot * 0.25f > snap->minimum_bet);
	actRaiseHalfPot->setEnabled(cur_pot * 0.5f > snap->minimum_bet);
	actRaiseThreeQuarterPot->setEnabled(cur_pot * 0.75f > snap->minimum_bet);
	actRaisePotsize->setEnabled(cur_pot >= snap->minimum_bet);
}
#endif

void WTable::slotShow()
{
	updateView();
	
	show();
	
	// FIXME: better solution
	resizeEvent(NULL);
}

void WTable::slotTimeup(int seat)
{
	qDebug() << "timeup seat= " << seat;
}

void WTable::resizeEvent(QResizeEvent *event)
{
	// preserve aspect ratio of our view
	const float aspect_ratio = 0.6f;
	int new_width = (int)(m_pView->height() / aspect_ratio);
	
	// fit in window if preserving aspect-ratio is not possible
	if (new_width > this->width())
		new_width = this->width();
	
	m_pView->resize(new_width, m_pView->height());
	m_pView->move(width()/2 - m_pView->width() / 2, m_pView->y());
	
	m_pView->fitInView(m_pScene->itemsBoundingRect());
}

bool WTable::greaterBet(const table_snapshot *snap, const qreal& bet, qreal *pbet) const
{
	qreal cur_bet = bet;
	
	for (unsigned int i=0; i < nMaxSeats; i++)
	{
		const seatinfo *seat = &(snap->seats[i]);

		if (seat->valid && seat->in_round && seat->bet > cur_bet)
			cur_bet = seat->bet;
	}
	
	if (pbet)
		*pbet = cur_bet;
	
	return (cur_bet > bet);
}

#if 0
bool WTable::isNoMoreActionPossible(const table_snapshot *snap)
{
	unsigned int countPlayers = 0;
	unsigned int countAllin = 0;
	
	for (unsigned int i=0; i < nMaxSeats; i++)
	{
		const seatinfo *seat = &(snap->seats[i]);
		
		if (seat->valid && seat->in_round)
		{
			countPlayers++;
			
			if ((int)seat->stake == 0)
				countAllin++;
		}
	}
	
	return (countAllin >= countPlayers - 1);
}
#endif

void WTable::actionScreenshot()
{
	QString pathScrshot = QString(sys_config_path()) + "/screenshots";
	QDateTime datetime = QDateTime::currentDateTime();
	QString filename = QString("holdingnuts_%1.png")
		.arg(datetime.toString("yyyy-MM-dd_hh.mm.ss"));
	
	if (!sys_isdir(pathScrshot.toStdString().c_str()))
		sys_mkdir(pathScrshot.toStdString().c_str());
	
	// grab the content of this window
	QPixmap pixShot = QPixmap::grabWidget(this);
	
	if (pixShot.save(pathScrshot + "/" + filename, "PNG"))
		addServerMessage(tr("Saved screenshot: %1.").arg(filename));
	else
		addServerMessage(tr("Unable to save screenshot in %1.").arg(pathScrshot));
}

void WTable::actionChat(QString msg)
{
	((PClient*)qApp)->chat(msg, m_nGid, m_nTid);
}

void WTable::playSound(unsigned int id)
{
#ifndef NOAUDIO
	if (!config.getBool("sound") || (config.getBool("sound_focus") && !isActiveWindow()))
		return;
	
	audio_play(id);
#endif
}

