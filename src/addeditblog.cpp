/*
    This file is part of Blogilo, A KDE Blogging Client

    Copyright (C) 2008-2010 Mehrdad Momeny <mehrdad.momeny@gmail.com>
    Copyright (C) 2008-2010 Golnaz Nilieh <g382nilieh@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.


    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see http://www.gnu.org/licenses/
*/

#include "addeditblog.h"
#include "waitwidget.h"
#include "bilboblog.h"
#include "dbman.h"

#include <kblog/blogger1.h>
#include <kblog/metaweblog.h>
#include <kblog/movabletype.h>
#include <kblog/wordpressbuggy.h>
#include "blogger.h"
#include <kurl.h>
#include <kmessagebox.h>
#include <qdebug.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KLocalizedString>

#include <QTableWidget>
#include <QTimer>
static const int TIMEOUT = 45000;

class AddEditBlog::Private
{
public:
    Private()
        : mainW(0),
          isNewBlog(false),
          bBlog(0),
          mBlog(0),
          mFetchProfileIdTimer(0),
          mFetchBlogIdTimer(0),
          mFetchAPITimer(0),
          wait(0)
    {}
    Ui::AddEditBlogBase ui;
    QTabWidget *mainW;
    bool isNewBlog;
    BilboBlog *bBlog;
    KBlog::Blog *mBlog;
    QTimer* mFetchProfileIdTimer;
    QTimer* mFetchBlogIdTimer;
    QTimer* mFetchAPITimer;
    WaitWidget *wait;
    QString tmpBlogUrl;
};

AddEditBlog::AddEditBlog( int blog_id, QWidget *parent, Qt::WindowFlags flags )
    : KDialog( parent, flags ), d(new Private)
{
    qDebug();
    d->mainW = new QTabWidget( this );
    d->ui.setupUi( d->mainW );
    setMainWidget( d->mainW );

    d->isNewBlog = true;
    d->mFetchAPITimer = d->mFetchBlogIdTimer = d->mFetchProfileIdTimer = 0;

    connect(d->ui.txtId, &KLineEdit::textChanged, this, &AddEditBlog::enableOkButton);
    connect(d->ui.txtUrl, &KLineEdit::textChanged, this, &AddEditBlog::enableAutoConfBtn);
    connect(d->ui.txtUser, &QLineEdit::textChanged, this, &AddEditBlog::enableAutoConfBtn);
    connect(d->ui.txtPass, &KLineEdit::textChanged, this, &AddEditBlog::enableAutoConfBtn);
    connect(d->ui.btnAutoConf, &QPushButton::clicked, this, &AddEditBlog::autoConfigure);
    connect(d->ui.btnFetch, &QPushButton::clicked, this, &AddEditBlog::fetchBlogId);
    connect(d->ui.comboApi, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &AddEditBlog::slotComboApiChanged);
    connect(d->ui.txtUrl, &KLineEdit::returnPressed, this, &AddEditBlog::slotReturnPressed);
    connect(d->ui.txtUser, &QLineEdit::returnPressed, this, &AddEditBlog::slotReturnPressed);
    connect(d->ui.txtPass, &KLineEdit::returnPressed, this, &AddEditBlog::slotReturnPressed);
    connect(d->ui.txtId, &KLineEdit::returnPressed, this, &AddEditBlog::slotReturnPressed);

    if ( blog_id > -1 ) {
        setWindowTitle( i18n( "Edit blog settings" ) );
        enableButtonOk( true );
        d->ui.btnFetch->setEnabled( true );
        d->ui.btnAutoConf->setEnabled( true );
        d->isNewBlog = false;
        d->bBlog =  DBMan::self()->blog( blog_id );
        d->ui.txtUrl->setText( d->bBlog->url().url() );
        d->ui.txtUser->setText( d->bBlog->username() );
        d->ui.txtPass->setText( d->bBlog->password() );
        d->ui.txtId->setText( d->bBlog->blogid() );
        QString title = d->bBlog->title();
        title = title.replace(QLatin1String("&amp;"), QLatin1String("&"));
        d->ui.txtTitle->setText( title );
        d->ui.comboApi->setCurrentIndex( d->bBlog->api() );
        d->ui.comboDir->setCurrentIndex( d->bBlog->direction() );
        d->ui.txtTitle->setEnabled(true);
    } else {
        setWindowTitle( i18n( "Add a new blog" ) );
        d->bBlog = new BilboBlog( this );
        d->bBlog->setBlogId( QString() );
        enableButtonOk( false );
        d->ui.txtTitle->setEnabled(false);
    }

    slotComboApiChanged( d->ui.comboApi->currentIndex() );
    d->ui.txtUrl->setFocus();
}

void AddEditBlog::enableAutoConfBtn()
{
    if ( d->ui.txtUrl->text().isEmpty() || d->ui.txtUser->text().isEmpty() || d->ui.txtPass->text().isEmpty() ) {
        d->ui.btnAutoConf->setEnabled( false );
        d->ui.btnFetch->setEnabled( false );
    } else {
        d->ui.btnAutoConf->setEnabled( true );
        d->ui.btnFetch->setEnabled( true );
    }
}

void AddEditBlog::autoConfigure()
{
    qDebug();
    if ( d->ui.txtUrl->text().isEmpty() || d->ui.txtUser->text().isEmpty() || d->ui.txtPass->text().isEmpty() ) {
        qDebug() << "Username, Password or Url doesn't set!";
        KMessageBox::sorry( this, i18n( "You have to set the username, password and URL of your blog or website." ),
                            i18n( "Incomplete fields" ) );
        return;
    }
    showWaitWidget( i18n("Trying to guess blog and API type...") );
    QString textUrl;
    ///Guess API with Url:
    if ( d->ui.txtUrl->text().contains( QLatin1String("xmlrpc.php"), Qt::CaseInsensitive ) ) {
        d->ui.comboApi->setCurrentIndex( 3 );
        fetchBlogId();
        return;
    }
    if ( d->ui.txtUrl->text().contains( QLatin1String("blogspot"), Qt::CaseInsensitive ) ) {
        d->ui.comboApi->setCurrentIndex( 4 );
        fetchBlogId();
        return;
    }
    if ( d->ui.txtUrl->text().contains( QLatin1String("wordpress"), Qt::CaseInsensitive ) ) {
        d->ui.comboApi->setCurrentIndex( 3 );

        textUrl = d->ui.txtUrl->text();
        while (textUrl.endsWith(QLatin1Char('/'))) {
            textUrl.remove(textUrl.length()-1, 1);
        }
        d->ui.txtUrl->setText( textUrl + QLatin1String("/xmlrpc.php") );
        fetchBlogId();
        return;
    }
    if ( d->ui.txtUrl->text().contains( QLatin1String("livejournal"), Qt::CaseInsensitive ) ) {
        d->ui.comboApi->setCurrentIndex( 0 );
        d->tmpBlogUrl = d->ui.txtUrl->text();
        d->ui.txtUrl->setText( QLatin1String("http://www.livejournal.com/interface/blogger/") );
        d->ui.txtId->setText( d->ui.txtUser->text() );
        d->ui.txtTitle->setText( d->ui.txtUser->text() );
        hideWaitWidget();
        return;
    }
    qDebug() << "Trying to guess API type by Homepage contents";
    KIO::StoredTransferJob *httpGetJob = KIO::storedGet( d->ui.txtUrl->text() , KIO::NoReload, KIO::HideProgressInfo );
    connect(httpGetJob, &KIO::StoredTransferJob::result, this, &AddEditBlog::gotHtml);
    d->mFetchAPITimer = new QTimer( this );
    d->mFetchAPITimer->setSingleShot( true );
    connect(d->mFetchAPITimer, &QTimer::timeout, this, &AddEditBlog::handleFetchAPITimeout);
    d->mFetchAPITimer->start( TIMEOUT );
}

void AddEditBlog::gotHtml( KJob *job )
{
    if ( !job )
        return;
    if ( job->error() ) {
        qDebug() << "Auto configuration failed! Error: " << job->errorString();
        hideWaitWidget();
        KMessageBox::sorry(this, i18n("Auto configuration failed. You have to set Blog API on Advanced tab manually."));
        return;
    }
    QString httpData( QString::fromUtf8(static_cast<KIO::StoredTransferJob*>( job )->data() ) );
    job->deleteLater();

    QRegExp rxGData( QString::fromLatin1( "content='blogger' name='generator'" ) );
    if ( rxGData.indexIn( httpData ) != -1 ) {
        qDebug() << "content='blogger' name='generator' matched";
        d->mFetchAPITimer->deleteLater();
        d->ui.comboApi->setCurrentIndex( 4 );
        QRegExp rxBlogId( QString::fromLatin1("BlogID=(\\d+)" ) );
        d->ui.txtId->setText( rxBlogId.cap( 1 ) );
        hideWaitWidget();
        return;
    }

    QRegExp rxLiveJournal( QString::fromLatin1( "rel=\"openid.server\" href=\"http://www.livejournal.com/openid/server.bml\"" ) );
    if ( rxLiveJournal.indexIn( httpData ) != -1 ) {
        qDebug() << " rel=\"openid.server\" href=\"http://www.livejournal.com/openid/server.bml\" matched";
        d->mFetchAPITimer->deleteLater();
        d->ui.comboApi->setCurrentIndex( 0 );
        d->ui.txtUrl->setText( QLatin1String("http://www.liverjournal.com/interface/blogger/") );
        d->ui.txtId->setText( d->ui.txtUser->text() );
        hideWaitWidget();
        return;
    }

    QString textUrl;
    QRegExp rxWordpress( QString::fromLatin1( "name=\"generator\" content=\"WordPress" ) );
    if ( rxWordpress.indexIn( httpData ) != -1 ) {
        qDebug() << "name=\"generator\" content=\"WordPress matched";
        d->mFetchAPITimer->deleteLater();
        d->ui.comboApi->setCurrentIndex( 3 );

        textUrl = d->ui.txtUrl->text();
        while (textUrl.endsWith(QLatin1Char('/'))) {
            textUrl.remove(textUrl.length()-1, 1);
        }
        d->ui.txtUrl->setText( textUrl + QLatin1String("/xmlrpc.php") );
        fetchBlogId();
        return;
    }

    // add MT for WordpressBuggy -> URL/xmlrpc.php exists
    textUrl = d->ui.txtUrl->text();
    while (textUrl.endsWith(QLatin1Char('/'))) {
        textUrl.remove(textUrl.length()-1, 1);
    }
    KIO::StoredTransferJob *testXmlRpcJob = KIO::storedGet( QString::fromLatin1("%1/xmlrpc.php").arg(textUrl),
                                                            KIO::NoReload, KIO::HideProgressInfo );

    connect(testXmlRpcJob, &KIO::StoredTransferJob::result, this, &AddEditBlog::gotXmlRpcTest);
}

void AddEditBlog::gotXmlRpcTest( KJob *job )
{
    qDebug();
    d->mFetchAPITimer->deleteLater();
    if ( !job )
        return;
    if ( job->error() ) {
        qDebug() << "Auto configuration failed! Error: " << job->errorString();
        hideWaitWidget();
        KMessageBox::sorry(this, i18n("Auto configuration failed. You have to set Blog API on Advanced tab manually."));
        return;
    }
    KMessageBox::information(this, i18n("The program could not guess the API of your blog, "
                                        "but has found an XMLRPC interface and is trying to use it.\n"
                                        "The MovableType API is assumed for now; choose another API if you know the server supports it."));
    d->ui.comboApi->setCurrentIndex( 2 );
    QString textUrl = d->ui.txtUrl->text();
    while (textUrl.endsWith(QLatin1Char('/'))) {
        textUrl.remove(textUrl.length()-1, 1);
    }
    d->ui.txtUrl->setText( textUrl + QLatin1String("/xmlrpc.php") );
    fetchBlogId();
}

void AddEditBlog::fetchBlogId()
{
    switch ( d->ui.comboApi->currentIndex() ) {
    case 0:
    case 1:
    case 2:
    case 3:
    {
        KBlog::Blogger1 *blog = new KBlog::Blogger1( KUrl( d->ui.txtUrl->text() ), this );
        d->mBlog = blog;
        blog->setUsername( d->ui.txtUser->text() );
        blog->setPassword( d->ui.txtPass->text() );
        connect(blog, &KBlog::Blogger1::listedBlogs, this, &AddEditBlog::fetchedBlogId);
        d->mFetchBlogIdTimer = new QTimer( this );
        d->mFetchBlogIdTimer->setSingleShot( true );
        connect(d->mFetchBlogIdTimer, &QTimer::timeout, this, &AddEditBlog::handleFetchIDTimeout);
        d->mFetchBlogIdTimer->start( TIMEOUT );
        blog->listBlogs();
        break;
    }
    case 4:
    {
        KBlog::Blogger *blog = new KBlog::Blogger( KUrl( d->ui.txtUrl->text() ), this );
        d->mBlog = blog;
        blog->setUsername( d->ui.txtUser->text() );
        blog->setPassword( d->ui.txtPass->text() );
        blog->setApiKey( QLatin1String("500754804903-g6n1rfjjcmhct64p3qgj6ma3oo8l8s6a.apps.googleusercontent.com") );
        blog->setSecretKey( QLatin1String("jzSzkrD7ert2z0v5VEq6CcSs") );
        connect(blog, &KBlog::Blogger::authenticated, this, &AddEditBlog::bloggerAuthenticated);
        blog->authenticate();
        break;
    }
    default:
        qDebug()<<"Unknown API";
        return;
    }

    connect(d->mBlog, &KBlog::Blog::error, this, &AddEditBlog::handleFetchError);
    d->ui.txtId->setText( i18n( "Please wait..." ) );
    d->ui.txtId->setEnabled( false );
    showWaitWidget( i18n( "Fetching Blog Id..." ) );
}

void AddEditBlog::handleFetchIDTimeout()
{
    qDebug();
    if ( d->mFetchBlogIdTimer ) {
        d->mFetchBlogIdTimer->stop();
    }
    if( d->mFetchProfileIdTimer ) {
        d->mFetchProfileIdTimer->stop();
    }
    d->ui.txtId->setText( QString() );
    d->ui.txtId->setEnabled( true );
    hideWaitWidget();
    KMessageBox::error( this, i18n( "Fetching the blog id timed out. Check your Internet connection,"
                                    "and your homepage URL, username or password.\nNote that the URL has to contain \"http://\"\n"
                                    "If you are using a self-hosted Wordpress blog, you have to enable Remote Publishing in its configuration." ) );
}

void AddEditBlog::handleFetchAPITimeout()
{
    qDebug();
    d->mFetchAPITimer->deleteLater();
    d->mFetchAPITimer = 0;
    hideWaitWidget();
    d->ui.txtId->setEnabled( true );
    d->ui.txtId->setText( QString() );
    KMessageBox::sorry( this, i18n( "The API guess function has failed, "
                                    "please check your Internet connection. Otherwise, you have to set the API type manually on the Advanced tab." ),
                                    i18n( "Auto Configuration Failed" ) );
}

void AddEditBlog::handleFetchError( KBlog::Blog::ErrorType type, const QString & errorMsg )
{
    qDebug() << " ErrorType: " << type;
    d->ui.txtId->setEnabled( true );
    d->ui.txtId->setText( QString() );
    hideWaitWidget();
    KMessageBox::detailedError( this, i18n( "Fetching BlogID Failed.\nPlease check your Internet connection." ), errorMsg );
}

void AddEditBlog::fetchedBlogId( const QList< QMap < QString , QString > > & list )
{
    if( d->mFetchBlogIdTimer ) {
        d->mFetchBlogIdTimer->deleteLater();
        d->mFetchBlogIdTimer = 0;
    }
    hideWaitWidget();
    QString blogId, blogName, blogUrl, apiUrl;
    const int listCount(list.count());
    if ( listCount > 1 ) {
        qDebug() << "User has more than ONE blog!";
        KDialog *blogsDialog = new KDialog(this);
        QTableWidget *blogsList = new QTableWidget(blogsDialog);
        blogsList->setSelectionBehavior(QAbstractItemView::SelectRows);
        QList< QMap<QString,QString> >::const_iterator it = list.constBegin();
        QList< QMap<QString,QString> >::const_iterator endIt = list.constEnd();
        int i=0;
        blogsList->setColumnCount(4);
        QStringList headers;
        headers<<i18n("Title")<<i18n("Url");

        blogsList->setHorizontalHeaderLabels(headers);
        blogsList->setColumnHidden(2, true);
        blogsList->setColumnHidden(3, true);
        for(;it != endIt; ++it){
            qDebug()<<it->value(QLatin1String("title"));
            blogsList->insertRow(i);
            blogsList->setCellWidget(i, 0, new QLabel( it->value(QLatin1String("title"))) );
            blogsList->setCellWidget(i, 1, new QLabel( it->value(QLatin1String("url"))) );
            blogsList->setCellWidget(i, 2, new QLabel( it->value(QLatin1String("id"))) );
            blogsList->setCellWidget(i, 3, new QLabel( it->value(QLatin1String("apiUrl"))) );
            ++i;
        }
        blogsDialog->setMainWidget(blogsList);
        blogsDialog->setWindowTitle( i18n("Which blog?") );
        if( blogsDialog->exec() ) {
            int row = blogsList->currentRow();
            if( row == -1 ) {
                delete blogsDialog;
                return;
            }
            blogId = qobject_cast<QLabel*>( blogsList->cellWidget(row, 2) )->text();
            blogName = qobject_cast<QLabel*>( blogsList->cellWidget(row, 0) )->text();
            blogUrl = qobject_cast<QLabel*>( blogsList->cellWidget(row, 1) )->text();
            apiUrl = qobject_cast<QLabel*>( blogsList->cellWidget(row, 3) )->text();
        } else {
            delete blogsDialog;
            return;
        }
        delete blogsDialog;
    } else if (listCount > 0) {
        blogId = list.constBegin()->value(QLatin1String("id"));
        blogName = list.constBegin()->value(QLatin1String("title"));
        blogUrl = list.constBegin()->value(QLatin1String("url"));
        apiUrl = list.constBegin()->value(QLatin1String("apiUrl"));
    } else {
        KMessageBox::sorry(this, i18n("Sorry, No blog found with the specified account info."));
        return;
    }
    d->ui.txtId->setText( blogId );
    d->ui.txtTitle->setText( blogName );
    d->ui.txtId->setEnabled( true );
    d->ui.btnFetch->setEnabled( true );
    d->ui.btnAutoConf->setEnabled( true );

    if( !apiUrl.isEmpty() ){
        d->ui.txtUrl->setText( apiUrl );
    } else {
        apiUrl = d->ui.txtUrl->text();
    }
    if( !blogUrl.isEmpty() ) {
        d->bBlog->setBlogUrl( blogUrl );
    } else {
        if(d->tmpBlogUrl.isEmpty())
            d->bBlog->setBlogUrl( apiUrl );
        else
            d->bBlog->setBlogUrl( d->tmpBlogUrl );
    }

    d->bBlog->setUrl( QUrl( apiUrl ) );
    d->bBlog->setUsername( d->ui.txtUser->text() );
    d->bBlog->setPassword( d->ui.txtPass->text() );
    d->bBlog->setBlogId( blogId );
    d->bBlog->setTitle( blogName );
}

void AddEditBlog::enableOkButton( const QString & txt )
{
    const bool check = !txt.isEmpty();
    enableButtonOk( check );
    d->ui.txtTitle->setEnabled( check );
}

void AddEditBlog::slotReturnPressed()
{
    ///FIXME This function commented temporarilly! check its functionality! and uncomment it!
    if(isButtonEnabled(KDialog::Ok)){
        setButtonFocus(KDialog::Ok);
    } else {
        if(d->mainW->currentIndex()==0){
            if(d->ui.btnAutoConf->isEnabled()){
                autoConfigure();
            }
        } else {
            fetchBlogId();
        }
    }
}

AddEditBlog::~AddEditBlog()
{
    qDebug();
    delete d;
}

void AddEditBlog::setSupportedFeatures( BilboBlog::ApiType api )
{
    const QString yesStyle = QLatin1String("QLabel{color: green;}");
    const QString yesText = i18nc( "Supported feature or Not", "Yes" );
    const QString noStyle = QLatin1String("QLabel{color: red;}");
    const QString noText = i18nc( "Supported feature or Not", "No, API does not support it" );
    const QString notYetText = i18nc( "Supported feature or Not", "No, Blogilo does not yet support it" );

    d->ui.featureCreatePost->setText( yesText );
    d->ui.featureCreatePost->setStyleSheet( yesStyle );
    d->ui.featureRemovePost->setText( yesText );
    d->ui.featureRemovePost->setStyleSheet( yesStyle );
    d->ui.featurRecentPosts->setText( yesText );
    d->ui.featurRecentPosts->setStyleSheet( yesStyle );

    d->ui.featureCreateCategory->setStyleSheet( noStyle );

    switch( api ) {
    case BilboBlog::BLOGGER1_API:
        d->ui.featureUploadMedia->setText( noText );
        d->ui.featureUploadMedia->setStyleSheet( noStyle );
        d->ui.featureCategories->setText( noText );
        d->ui.featureCategories->setStyleSheet( noStyle );
        d->ui.featureMultipagedPosts->setText( noText );
        d->ui.featureMultipagedPosts->setStyleSheet( noStyle );
        d->ui.featureCreateCategory->setText( noText );
        d->ui.featureTags->setText( noText );
        d->ui.featureTags->setStyleSheet( noStyle );
        break;
    case BilboBlog::METAWEBLOG_API:
        d->ui.featureUploadMedia->setText( yesText );
        d->ui.featureUploadMedia->setStyleSheet( yesStyle );
        d->ui.featureCategories->setText( noText );
        d->ui.featureCategories->setStyleSheet( noStyle );
        d->ui.featureMultipagedPosts->setText( noText );
        d->ui.featureMultipagedPosts->setStyleSheet( noStyle );
        d->ui.featureCreateCategory->setText( noText );
        d->ui.featureTags->setText( noText );
        d->ui.featureTags->setStyleSheet( noStyle );
        break;
    case BilboBlog::MOVABLETYPE_API:
        d->ui.featureUploadMedia->setText( yesText );
        d->ui.featureUploadMedia->setStyleSheet( yesStyle );
        d->ui.featureCategories->setText( yesText );
        d->ui.featureCategories->setStyleSheet( yesStyle );
        d->ui.featureMultipagedPosts->setText( yesText );
        d->ui.featureMultipagedPosts->setStyleSheet( yesStyle );
        d->ui.featureCreateCategory->setText( noText );
        d->ui.featureTags->setText( yesText );
        d->ui.featureTags->setStyleSheet( yesStyle );
        break;
    case BilboBlog::WORDPRESSBUGGY_API:
        d->ui.featureUploadMedia->setText( yesText );
        d->ui.featureUploadMedia->setStyleSheet( yesStyle );
        d->ui.featureCategories->setText( yesText );
        d->ui.featureCategories->setStyleSheet( yesStyle );
        d->ui.featureMultipagedPosts->setText( yesText );
        d->ui.featureMultipagedPosts->setStyleSheet( yesStyle );
        d->ui.featureCreateCategory->setText( notYetText );
        d->ui.featureTags->setText( yesText );
        d->ui.featureTags->setStyleSheet( yesStyle );
        break;
    case BilboBlog::BLOGGER_API:
        d->ui.featureUploadMedia->setText( noText );
        d->ui.featureUploadMedia->setStyleSheet( noStyle );
        d->ui.featureCategories->setText( noText );
        d->ui.featureCategories->setStyleSheet( noStyle );
        d->ui.featureMultipagedPosts->setText( noText );
        d->ui.featureMultipagedPosts->setStyleSheet( noStyle );
        d->ui.featureCreateCategory->setText( noText );
        d->ui.featureTags->setText( yesText );
        d->ui.featureTags->setStyleSheet( yesStyle );
        break;
    };
}

void AddEditBlog::slotComboApiChanged( int index )
{
    ///This wrapper is to change api if needed!
    setSupportedFeatures( (BilboBlog::ApiType) index );
}

void AddEditBlog::slotButtonClicked( int button )
{
    qDebug();
    if ( button == KDialog::Ok ) {
        if ( d->bBlog->blogid().isEmpty() && d->ui.txtId->text().isEmpty() ) {
            KMessageBox::sorry( this, i18n( "Blog ID has not yet been retrieved.\n"
                                            "You can fetch the blog ID by clicking on \"Auto Configure\" or the \"Fetch ID\" button; otherwise, you have "
                                            "to insert your blog ID manually." )
                                            );
                    return;
        }
        d->bBlog->setApi(( BilboBlog::ApiType )d->ui.comboApi->currentIndex() );
        d->bBlog->setDirection(( Qt::LayoutDirection )d->ui.comboDir->currentIndex() );
        d->bBlog->setTitle( d->ui.txtTitle->text() );
        d->bBlog->setPassword( d->ui.txtPass->text() );
        d->bBlog->setUsername( d->ui.txtUser->text() );
        d->bBlog->setBlogId( d->ui.txtId->text() );
        d->bBlog->setUrl( KUrl( d->ui.txtUrl->text() ) );
        if(d->bBlog->blogUrl().isEmpty())
            d->bBlog->setBlogUrl(d->ui.txtUrl->text());

        if ( d->isNewBlog ) {
            int blog_id = DBMan::self()->addBlog( *d->bBlog );
            d->bBlog->setId( blog_id );
            if ( blog_id != -1 ) {
                qDebug() << "Emitting sigBlogAdded() ...";
                Q_EMIT sigBlogAdded( *d->bBlog );
            } else {
                qDebug() << "Cannot add blog";
            }
        } else {
            if ( DBMan::self()->editBlog( *d->bBlog ) ) {
                qDebug() << "Emitting sigBlogEdited() ...";
                Q_EMIT sigBlogEdited( *d->bBlog );
            } else {
                qDebug() << "Cannot edit blog with id " << d->bBlog->id();
            }
        }
        accept();
    } else
        KDialog::slotButtonClicked( button );
}

void AddEditBlog::showWaitWidget( QString text )
{
    d->ui.btnAutoConf->setEnabled( false );
    d->ui.btnFetch->setEnabled( false );
    if( !d->wait ) {
        d->wait = new WaitWidget(this);
        d->wait->setWindowModality( Qt::WindowModal );
        d->wait->setBusyState();
    }
    d->wait->setText( text );
    d->wait->show();
}

void AddEditBlog::hideWaitWidget()
{
    d->ui.btnAutoConf->setEnabled( true );
    d->ui.btnFetch->setEnabled( true );
    if( d->wait )
        d->wait->deleteLater();
    d->wait = 0;
}

void AddEditBlog::bloggerAuthenticated(const QMap< QString, QString > &authData)
{
    d->bBlog->setAuthData(authData);
    KBlog::Blogger *blogger = qobject_cast<KBlog::Blogger*>(d->mBlog);
    connect(blogger, &KBlog::Blogger::listedBlogs, this, &AddEditBlog::fetchedBlogId);
    d->mFetchBlogIdTimer = new QTimer( this );
    d->mFetchBlogIdTimer->setSingleShot( true );
    connect(d->mFetchBlogIdTimer, &QTimer::timeout, this, &AddEditBlog::handleFetchIDTimeout);
    d->mFetchBlogIdTimer->start( TIMEOUT );
    blogger->listBlogs();
}
