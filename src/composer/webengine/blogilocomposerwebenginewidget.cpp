/*
  Copyright (C) 2016-2017 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "blogilocomposerwebenginewidget.h"
#include "blogilocomposerwebengineeditor.h"

#include <PimCommon/CustomToolsWidgetng>

#include <QVBoxLayout>

BlogiloComposerWebEngineWidget::BlogiloComposerWebEngineWidget(BlogiloComposerWebEngineView *view, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);
    mEditor = new BlogiloComposerWebEngineEditor(view, this);

    lay->addWidget(mEditor);
    mCustomToolsWidget = new PimCommon::CustomToolsWidgetNg(nullptr);
    connect(mCustomToolsWidget, &PimCommon::CustomToolsWidgetNg::insertText,
            this, &BlogiloComposerWebEngineWidget::slotInsertShortUrl);
    connect(mCustomToolsWidget, &PimCommon::CustomToolsWidgetNg::toolActivated,
            this, &BlogiloComposerWebEngineWidget::slotInsertText);
    lay->addWidget(mCustomToolsWidget);
    view->setCustomTools(mCustomToolsWidget);
}

BlogiloComposerWebEngineWidget::~BlogiloComposerWebEngineWidget()
{
}

BlogiloComposerWebEngineEditor *BlogiloComposerWebEngineWidget::editor() const
{
    return mEditor;
}

void BlogiloComposerWebEngineWidget::slotInsertText()
{
    const QString text = mEditor->view()->selectedText();
    if (!text.isEmpty()) {
        mCustomToolsWidget->setText(text);
    }
}

void BlogiloComposerWebEngineWidget::slotInsertShortUrl(const QString &shortUrl)
{
    mEditor->insertShortUrl(shortUrl);
}
