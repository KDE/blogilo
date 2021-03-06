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

#include "composerwebengine.h"
#include "composereditorwebengine_debug.h"
#include "private/composerwebengine_p.h"

#include <WebEngineViewer/WebEnginePage>
#include <WebEngineViewer/WebHitTest>
#include <WebEngineViewer/WebHitTestResult>

#include <KActionCollection>
#include <KFontAction>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSelectAction>
#include <KToggleAction>
#include <KToolBar>

#include <QAction>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QMenu>
#include <QStandardPaths>
#include <QWebEngineSettings>

using namespace ComposerEditorWebEngine;

ComposerWebEngine::ComposerWebEngine(QWidget *parent)
    : WebEngineViewer::WebEngineView(parent),
      d(new ComposerEditorWebEngine::ComposerEditorWebEnginePrivate(this))
{
    settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
    settings()->setAttribute(QWebEngineSettings::XSSAuditingEnabled, false);
    settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    d->mPageEngine = new WebEngineViewer::WebEnginePage(this);
    setPage(d->mPageEngine);

    QFile file(initialHtml());
    qCDebug(COMPOSEREDITORWEBENGINE_LOG) << file.fileName();

    if (!file.open(QIODevice::ReadOnly)) {
        KMessageBox::error(this,
                           i18n("Cannot open template file %1.",
                                QFileInfo(file).absoluteFilePath()),
                           i18n("composer editor"));
    } else {
        setHtmlContent(QString::fromUtf8(file.readAll()));    //, "application/xhtml+xml" );
    }
    d->execCommand(QStringLiteral("contentReadOnly"), QStringLiteral("false"));
    page()->runJavaScript(QStringLiteral("document.documentElement.contentEditable = true"));
    setFocusPolicy(Qt::WheelFocus);
}

ComposerWebEngine::~ComposerWebEngine()
{
    delete d;
}

QString ComposerWebEngine::initialHtml()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                  QStringLiteral("composereditorwebengine/composereditorinitialhtml"));
}

void ComposerWebEngine::createActions(const QList<ComposerWebEngineAction> &lstActions)
{
    Q_FOREACH (const ComposerWebEngineAction &action, lstActions) {
        d->createAction(action);
    }

    d->connectActionGroup();
}
//TODO fixme add to kactioncollection
void ComposerWebEngine::createAllActions()
{
    for (uint i = 0; i < LastType; ++i) {
        d->createAction((ComposerWebEngineAction)i);
    }
    d->connectActionGroup();
}

void ComposerWebEngine::addCreatedActionsToActionCollection(KActionCollection *actionCollection)
{
    if (actionCollection) {
        if (d->action_text_bold) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_bold"), d->action_text_bold);
        }
        if (d->action_text_italic) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_italic"), d->action_text_italic);
        }
        if (d->action_text_underline) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_underline"), d->action_text_underline);
        }
        if (d->action_text_strikeout) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_strikeout"), d->action_text_strikeout);
        }
        if (d->action_align_left) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_align_left"), d->action_align_left);
        }
        if (d->action_align_center) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_align_center"), d->action_align_center);
        }
        if (d->action_align_right) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_align_right"), d->action_align_right);
        }
        if (d->action_align_justify) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_align_justify"), d->action_align_justify);
        }
        if (d->action_direction_ltr) {
            actionCollection->addAction(QStringLiteral("htmleditor_direction_ltr"), d->action_direction_ltr);
        }
        if (d->action_direction_rtl) {
            actionCollection->addAction(QStringLiteral("htmleditor_direction_rtl"), d->action_direction_rtl);
        }
        if (d->action_text_subscript) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_subscript"), d->action_text_subscript);
        }
        if (d->action_text_superscript) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_superscript"), d->action_text_superscript);
        }
        if (d->action_page_color) {
            actionCollection->addAction(QStringLiteral("htmleditor_page_color_and_background"), d->action_page_color);
        }
        if (d->action_insert_table) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_new_table"), d->action_insert_table);
        }
        if (d->action_insert_link) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_link"), d->action_insert_link);
        }
        if (d->action_insert_horizontal_rule) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_horizontal_rule"), d->action_insert_horizontal_rule);
        }
        if (d->action_text_foreground_color) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_foreground_color"), d->action_text_foreground_color);
        }
        if (d->action_text_background_color) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_text_background_color"), d->action_text_background_color);
        }
        if (d->action_add_emoticon) {
            actionCollection->addAction(QStringLiteral("htmleditor_add_emoticon"), d->action_add_emoticon);
        }
        if (d->action_insert_html) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_html"), d->action_insert_html);
        }
        if (d->action_insert_image) {
            actionCollection->addAction(QStringLiteral("htmleditor_add_image"), d->action_insert_image);
        }
        if (d->action_spell_check) {
            actionCollection->addAction(QStringLiteral("htmleditor_spell_check"), d->action_spell_check);
        }
        if (d->action_format_reset) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_reset"), d->action_format_reset);
        }
        if (d->action_font_family) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_font_family"), d->action_font_family);
        }
        if (d->action_font_size) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_font_size"), d->action_font_size);
        }
        if (d->action_format_type) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_type"), d->action_format_type);
        }
        if (d->action_block_quote) {
            actionCollection->addAction(QStringLiteral("htmleditor_block_quote"), d->action_block_quote);
        }
        if (d->action_ordered_list) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_list_ordered"), d->action_ordered_list);
        }
        if (d->action_unordered_list) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_list_unordered"), d->action_unordered_list);
        }
        if (d->action_list_indent) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_list_indent_more"), d->action_list_indent);
        }
        if (d->action_list_dedent) {
            actionCollection->addAction(QStringLiteral("htmleditor_format_list_indent_less"), d->action_list_dedent);
        }
        if (d->action_find) {
            actionCollection->addAction(QStringLiteral("htmleditor_find"), d->action_find);
        }
        if (d->action_replace) {
            actionCollection->addAction(QStringLiteral("htmleditor_replace"), d->action_replace);
        }
        if (d->action_save_as) {
            actionCollection->addAction(QStringLiteral("htmleditor_save_as"), d->action_save_as);
        }
        if (d->action_print) {
            actionCollection->addAction(QStringLiteral("htmleditor_print"), d->action_print);
        }
        if (d->action_print_preview) {
            actionCollection->addAction(QStringLiteral("htmleditor_print_preview"), d->action_print_preview);
        }
        if (d->action_paste_withoutformatting) {
            actionCollection->addAction(QStringLiteral("htmleditor_paste_without_formatting"), d->action_paste_withoutformatting);
        }
        if (d->action_insert_specialchar) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_specialchar"), d->action_insert_specialchar);
        }
        if (d->action_insert_anchor) {
            actionCollection->addAction(QStringLiteral("htmleditor_insert_anchor"), d->action_insert_anchor);
        }
    }
}

void ComposerWebEngine::contextMenuEvent(QContextMenuEvent *e)
{
    WebEngineViewer::WebHitTest *webHit = d->mPageEngine->hitTestContent(e->pos());
    connect(webHit, &WebEngineViewer::WebHitTest::finished, this, &ComposerWebEngine::slotWebHitFinished);
}

void ComposerWebEngine::slotWebHitFinished(const WebEngineViewer::WebHitTestResult &result)
{
    QMenu *menu = new QMenu;

    const bool linkSelected = result.linkUrl().isValid();
    const bool imageSelected = result.imageUrl().isValid();
    if (imageSelected) {
        QAction *editImageAction = menu->addAction(i18n("Edit Image..."));
        connect(editImageAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditImage()));
    } else if (linkSelected) {
        QAction *editLinkAction = menu->addAction(i18n("Edit Link..."));
        connect(editLinkAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditLink()));
        QAction *openLinkAction = menu->addAction(i18n("Open Link"));
        connect(openLinkAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotOpenLink()));
    }

#if 0
    d->hideImageResizeWidget();
    d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());

    const bool linkSelected = !d->contextMenuResult.linkElement().isNull();
    const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();

    const QWebElement elm = d->contextMenuResult.element();
    const bool tableCellSelected = (elm.tagName().toLower() == QLatin1String("td"));
    const bool tableSelected = (elm.tagName().toLower() == QLatin1String("table") ||
                                tableCellSelected);

    const bool listSelected = (elm.tagName().toLower() == QLatin1String("ol") ||
                               elm.tagName().toLower() == QLatin1String("ul") ||
                               elm.tagName().toLower() == QLatin1String("li"));

    const bool anchorSelected = (elm.tagName().toLower() == QLatin1String("a"));

    qCDebug(COMPOSEREDITORNG_LOG) << " elm.tagName().toLower() " << elm.tagName().toLower();

    QMenu *menu = new QMenu;
    const QString selectedText = page()->mainFrame()->toPlainText().simplified();
    const bool emptyDocument = selectedText.isEmpty();

    menu->addAction(page()->action(QWebPage::Undo));
    menu->addAction(page()->action(QWebPage::Redo));
    menu->addSeparator();
    menu->addAction(page()->action(QWebPage::Cut));
    menu->addAction(page()->action(QWebPage::Copy));
    menu->addAction(page()->action(QWebPage::Paste));
    if (d->action_paste_withoutformatting) {
        menu->addAction(d->action_paste_withoutformatting);
    }

    menu->addSeparator();
    if (!emptyDocument) {
        menu->addAction(page()->action(QWebPage::SelectAll));
        menu->addSeparator();
    }
    if (!emptyDocument && d->action_find) {
        menu->addAction(d->action_find);
        menu->addSeparator();
    }
    if (imageSelected) {
        QAction *editImageAction = menu->addAction(i18n("Edit Image..."));
        connect(editImageAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditImage()));
    } else if (linkSelected) {
        QAction *editLinkAction = menu->addAction(i18n("Edit Link..."));
        connect(editLinkAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditLink()));
        QAction *openLinkAction = menu->addAction(i18n("Open Link"));
        connect(openLinkAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotOpenLink()));
    } else if (tableSelected) {
        ComposerTableActionMenu *tableActionMenu = new ComposerTableActionMenu(elm, menu, this);
        connect(tableActionMenu, SIGNAL(insertNewTable()), this, SLOT(_k_slotInsertTable()));
        menu->addAction(tableActionMenu);
    } else if (listSelected) {
        QAction *editListAction = menu->addAction(i18n("Edit List..."));
        connect(editListAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditList()));
    }
    menu->addSeparator();
#if 0 //Laurent. Never implemented
    if (!emptyDocument && d->action_spell_check) {
        menu->addAction(d->action_spell_check);
        menu->addSeparator();
    }
    //Don't use "Auto Spell Check" it will confict with search menu entry in spellchecklineedit.
    QAction *autoSpellCheckingAction = menu->addAction(i18n("Enable Spell Checking"));
    autoSpellCheckingAction->setCheckable(true);
    autoSpellCheckingAction->setChecked(d->checkSpellingEnabled());
    connect(autoSpellCheckingAction, SIGNAL(triggered(bool)), this, SLOT(_k_changeAutoSpellChecking(bool)));
#endif
    if (KPIMTextEdit::TextToSpeech::self()->isReady()) {
        QAction *speakAction = menu->addAction(i18n("Speak Text"));
        speakAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-text-to-speech")));
        speakAction->setEnabled(!emptyDocument);
        connect(speakAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotSpeakText()));
    }
#endif
    addExtraAction(menu);
    menu->exec(mapToGlobal(result.pos()));
    delete menu;
}

void ComposerWebEngine::addExtraAction(QMenu *menu)
{
    Q_UNUSED(menu);
    //Redefine if necessary.
}

void ComposerWebEngine::setActionsEnabled(bool enabled)
{
    Q_FOREACH (QAction *action, d->htmlEditorActionList) {
        action->setEnabled(enabled);
    }
}

void ComposerWebEngine::forwardMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
#if 0
    if (event->button() == Qt::LeftButton) {
        d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());
        const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();
        if (imageSelected) {
            d->showImageResizeWidget();
        }
    } else {
        d->hideImageResizeWidget();
    }
    // KWebView::mousePressEvent(event);
#endif
}

void ComposerWebEngine::forwardKeyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    d->hideImageResizeWidget();
}

void ComposerWebEngine::forwardWheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);
    d->hideImageResizeWidget();
}

void ComposerWebEngine::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
#if 0
    if (event->button() == Qt::LeftButton) {
        d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());
        const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();
        if (imageSelected) {
            d->showImageResizeWidget();
            d->_k_slotEditImage();
        }
    } else {
        d->hideImageResizeWidget();
    }
    //KWebView::mouseDoubleClickEvent(event);
#endif
}

void ComposerWebEngine::setHtmlContent(const QString &html)
{
    setHtml(html);
}

void ComposerWebEngine::evaluateJavascript(const QString &javascript)
{
    d->evaluateJavascript(javascript);
}

void ComposerWebEngine::createToolBar(const QList<ComposerWebEngineAction> &lstAction, KToolBar *toolbar)
{
    const int numberOfAction(lstAction.count());
    for (int i = 0; i < numberOfAction; ++i) {
        switch (lstAction.at(i)) {
        case Bold:
            toolbar->addAction(d->action_text_bold);
            break;
        case Italic:
            toolbar->addAction(d->action_text_italic);
            break;
        case Underline:
            toolbar->addAction(d->action_text_underline);
            break;
        case StrikeOut:
            toolbar->addAction(d->action_text_strikeout);
            break;
        case AlignLeft:
            toolbar->addAction(d->action_align_left);
            break;
        case AlignCenter:
            toolbar->addAction(d->action_align_center);
            break;
        case AlignRight:
            toolbar->addAction(d->action_align_right);
            break;
        case AlignJustify:
            toolbar->addAction(d->action_align_justify);
            break;
        case DirectionLtr:
            toolbar->addAction(d->action_direction_ltr);
            break;
        case DirectionRtl:
            toolbar->addAction(d->action_direction_rtl);
            break;
        case SubScript:
            toolbar->addAction(d->action_text_subscript);
            break;
        case SuperScript:
            toolbar->addAction(d->action_text_superscript);
            break;
        case HorizontalRule:
            toolbar->addAction(d->action_insert_horizontal_rule);
            break;
        case ListIndent:
            toolbar->addAction(d->action_list_indent);
            break;
        case ListDedent:
            toolbar->addAction(d->action_list_dedent);
            break;
        case OrderedList:
            toolbar->addAction(d->action_ordered_list);
            break;
        case UnorderedList:
            toolbar->addAction(d->action_unordered_list);
            break;
        case FormatType:
            toolbar->addAction(d->action_format_type);
            break;
        case FontSize:
            toolbar->addAction(d->action_font_size);
            break;
        case FontFamily:
            toolbar->addAction(d->action_font_family);
            break;
        case Emoticon:
            toolbar->addAction(d->action_add_emoticon);
            break;
        case InsertHtml:
            toolbar->addAction(d->action_insert_html);
            break;
        case InsertImage:
            toolbar->addAction(d->action_insert_image);
            break;
        case InsertTable:
            toolbar->addAction(d->action_insert_table);
            break;
        case InsertLink:
            toolbar->addAction(d->action_insert_link);
            break;
        case InsertSpecialChar:
            toolbar->addAction(d->action_insert_specialchar);
            break;
        case TextForegroundColor:
            toolbar->addAction(d->action_text_foreground_color);
            break;
        case TextBackgroundColor:
            toolbar->addAction(d->action_text_background_color);
            break;
        case FormatReset:
            toolbar->addAction(d->action_format_reset);
            break;
        case SpellCheck:
            toolbar->addAction(d->action_spell_check);
            break;
        case Find:
            toolbar->addAction(d->action_find);
            break;
        case Replace:
            toolbar->addAction(d->action_replace);
            break;
        case PageColor:
            toolbar->addAction(d->action_page_color);
            break;
        case BlockQuote:
            toolbar->addAction(d->action_block_quote);
            break;
        case SaveAs:
            toolbar->addAction(d->action_save_as);
            break;
        case Print:
            toolbar->addAction(d->action_print);
            break;
        case PrintPreview:
            toolbar->addAction(d->action_print_preview);
            break;
        case Separator: {
            QAction *act = new QAction(this);
            act->setSeparator(true);
            toolbar->addAction(act);
            break;
        }
        case PasteWithoutFormatting: {
            toolbar->addAction(d->action_paste_withoutformatting);
            break;
        }
        case InsertAnchor: {
            toolbar->addAction(d->action_insert_anchor);
            break;
        }
        case LastType: {
            //nothing
            break;
        }
        }
    }
}

QAction *ComposerWebEngine::action(ComposerWebEngineAction actionType) const
{
    switch (actionType) {
    case Bold:
        return d->action_text_bold;
    case Italic:
        return d->action_text_italic;
    case Underline:
        return d->action_text_underline;
    case StrikeOut:
        return d->action_text_strikeout;
    case AlignLeft:
        return d->action_align_left;
    case AlignCenter:
        return d->action_align_center;
    case AlignRight:
        return d->action_align_right;
    case AlignJustify:
        return d->action_align_justify;
    case DirectionLtr:
        return d->action_direction_ltr;
    case DirectionRtl:
        return d->action_direction_rtl;
    case SubScript:
        return d->action_text_subscript;
    case SuperScript:
        return d->action_text_superscript;
    case HorizontalRule:
        return d->action_insert_horizontal_rule;
    case ListIndent:
        return d->action_list_indent;
    case ListDedent:
        return d->action_list_dedent;
    case OrderedList:
        return d->action_ordered_list;
    case UnorderedList:
        return d->action_unordered_list;
    case FormatType:
        return d->action_format_type;
    case FontSize:
        return d->action_font_size;
    case FontFamily:
        return d->action_font_family;
    case Emoticon:
        return d->action_add_emoticon;
    case InsertHtml:
        return d->action_insert_html;
    case InsertImage:
        return d->action_insert_image;
    case InsertTable:
        return d->action_insert_table;
    case InsertLink:
        return d->action_insert_link;
    case InsertSpecialChar:
        return d->action_insert_specialchar;
    case TextForegroundColor:
        return d->action_text_foreground_color;
    case TextBackgroundColor:
        return d->action_text_background_color;
    case FormatReset:
        return d->action_format_reset;
    case SpellCheck:
        return d->action_spell_check;
    case Find:
        return d->action_find;
    case Replace:
        return d->action_replace;
    case PageColor:
        return d->action_page_color;
    case BlockQuote:
        return d->action_block_quote;
    case SaveAs:
        return d->action_save_as;
    case Print:
        return d->action_print;
    case PrintPreview:
        return d->action_print_preview;
    case PasteWithoutFormatting:
        return d->action_paste_withoutformatting;
    case InsertAnchor:
        return d->action_insert_anchor;
    case Separator:
    case LastType:
        break;
    }
    return nullptr;
}

QMap<QString, QString> ComposerWebEngine::localImages() const
{
    return d->localImages();
}

#include "moc_composerwebengine.cpp"
