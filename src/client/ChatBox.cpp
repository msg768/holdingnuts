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
 *     Michael Miller <michael.miller@holdingnuts.net>
 */


#include "ChatBox.hpp"
#include "pclient.hpp"

#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

ChatBox::ChatBox(const QString& title, int gid, int tid, QWidget *parent)
:	QGroupBox(title, parent),
	m_nGameID(gid),
	m_nTableID(tid)
{
	m_pEditChat = new QLineEdit(this);
	
	connect(m_pEditChat, SIGNAL(returnPressed()), this, SLOT(actionChat()));
	
	m_pEditChatLog = new QTextEdit(this);
	m_pEditChatLog->setReadOnly(true);
	
	QVBoxLayout *lchat = new QVBoxLayout();

	lchat->addWidget(m_pEditChat);
	lchat->addWidget(m_pEditChatLog);
	
	this->setLayout(lchat);
}

void ChatBox::addMessage(const QString& msg, const QColor& color)
{
	m_pEditChatLog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
	m_pEditChatLog->setTextColor(color);
	m_pEditChatLog->setFontWeight(QFont::Normal);
	m_pEditChatLog->insertPlainText(msg + "\r\n");
	m_pEditChatLog->setTextColor(QColor(0, 0, 0));	
}

void ChatBox::addMessage(const QString& from, const QString& msg)
{
	m_pEditChatLog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
	m_pEditChatLog->setFontWeight(QFont::Bold);
	m_pEditChatLog->insertPlainText("[" + from + "]");
	m_pEditChatLog->setFontWeight(QFont::Normal);
	m_pEditChatLog->insertPlainText(": " + msg + "\r\n");
}

void ChatBox::addMessage(const QString& from, const QString& msg, const QColor& color)
{
	m_pEditChatLog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
	m_pEditChatLog->setTextColor(color);
	m_pEditChatLog->setFontWeight(QFont::Bold);
	m_pEditChatLog->insertPlainText("[" + from + "]");
	m_pEditChatLog->setFontWeight(QFont::Normal);
	m_pEditChatLog->insertPlainText(": " + msg + "\r\n");
	m_pEditChatLog->setTextColor(QColor(0, 0, 0));
}

void ChatBox::actionChat()
{
	if (m_pEditChat->text().length())
	{
		if (m_nTableID == -1) // foyer chat
			((PClient*)qApp)->chatAll(m_pEditChat->text());
		else
			((PClient*)qApp)->chat(m_pEditChat->text(), m_nGameID, m_nTableID);

		m_pEditChat->clear();
		m_pEditChat->setFocus();
	}
}
