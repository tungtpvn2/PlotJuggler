#include "mainwindow.h"
#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>

#include "new_release_dialog.h"

static QString VERSION_STRING = QString("%1.%2.%3").
                                arg(PJ_MAJOR_VERSION).
                                arg(PJ_MINOR_VERSION).
                                arg(PJ_PATCH_VERSION);

inline int GetVersionNumber(QString str)
{
  QStringList online_version = str.split('.');
  int major = online_version[0].toInt();
  int minor = online_version[1].toInt();
  int patch = online_version[2].toInt();
  return major*10000 + minor*100 + patch;
}

void OpenNewReleaseDialog(QNetworkReply *reply)
{
  if (reply->error()) {
    return;
  }
  QString answer = reply->readAll();
  QJsonDocument document = QJsonDocument::fromJson(answer.toUtf8());
  QJsonObject data = document.object();
  QString url = data["html_url"].toString();
  QString name = data["name"].toString();
  QString tag_name = data["tag_name"].toString();
  QSettings settings;
  int online_number = GetVersionNumber( tag_name );
  QString dont_show = settings.value("NewRelease/dontShowThisVersion", VERSION_STRING).toString();
  int dontshow_number = GetVersionNumber( dont_show );
  int current_number = GetVersionNumber( VERSION_STRING );

  if( online_number > current_number &&
      online_number > dontshow_number )
  {
    NewReleaseDialog* dialog = new NewReleaseDialog(nullptr, tag_name, name, url );
    dialog->show();
  }
}

QPixmap getFunnySplashscreen(){

    QSettings settings;
    qsrand(time(nullptr));

    auto getNum = [](){
        const int last_image_num = 51;
        int n = qrand() % (last_image_num+2);
        if ( n > last_image_num ){ n = 0; }
        return n;
    };
    int n = getNum();
    int prev_n = settings.value("previousFunnySubtitle").toInt();
    while( n == prev_n ){
        n = getNum();
    }
    settings.setValue("previousFunnySubtitle", n);
    auto filename = QString("://resources/memes/meme_%1.jpg").arg(n, 2, 10, QChar('0'));
    return QPixmap(filename);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("IcarusTechnology");
    app.setApplicationName("PlotJuggler");

    app.setApplicationVersion(VERSION_STRING);

    QCommandLineParser parser;
    parser.setApplicationDescription("PlotJuggler: the time series visualization tool that you deserve ");
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption nosplash_option(QStringList() << "n" << "nosplash",
                                       "Don't display the splashscreen");
    parser.addOption(nosplash_option);

    QCommandLineOption test_option(QStringList() << "t" << "test",
                                   "Generate test curves at startup");
    parser.addOption(test_option);

    QCommandLineOption loadfile_option(QStringList() << "d" << "datafile",
                                       "Load a file containing data",
                                       "file" );
    parser.addOption(loadfile_option);

    QCommandLineOption layout_option(QStringList() << "l" << "layout",
                                     "Load a file containing the layout configuration",
                                     "file" );
    parser.addOption(layout_option);

    QCommandLineOption publish_option(QStringList() << "p" << "publish",
                                     "Automatically start publisher when loading the layout file" );
    parser.addOption(publish_option);

    QCommandLineOption buffersize_option(QStringList() << "buffer_size",
                                     QCoreApplication::translate("main", "Change the maximum size of the streaming buffer (minimum: 10 default: 60)"),
                                     QCoreApplication::translate("main", "seconds") );
    parser.addOption(buffersize_option);

    parser.process( *qApp );

    if( parser.isSet(publish_option) && !parser.isSet(layout_option) )
    {
        std::cerr << "Option [ -p / --publish ] is invalid unless [ -l / --layout ] is used too." << std::endl;
        return -1;
    }

    QIcon app_icon( ":/resources/office_chart_line_stacked.ico" );
    QApplication::setWindowIcon(app_icon);

    QNetworkAccessManager manager;
    QObject::connect( &manager, &QNetworkAccessManager::finished, OpenNewReleaseDialog);

    QNetworkRequest request;
    request.setUrl(QUrl("https://api.github.com/repos/facontidavide/plotjuggler/releases/latest"));
    manager.get(request);

    /*
     * You, fearless code reviewer, decided to start a journey into my source code.
     * For your bravery, you deserve to know the truth.
     * The splashscreen is useless; not only it is useless, it will make your start-up
     * time slower by few seconds for absolutely no reason.
     * But what are two seconds compared with the time that PlotJuggler will save you?
     * The splashscreen is the connection between me and my users, the glue that keeps
     * together our invisible relationship.
     * Now, it is up to you to decide: you can block the splashscreen forever or not,
     * reject a message that brings a little of happiness into your day, spent analyzing data.
     * Please don't do it.
     */

    if( !parser.isSet(nosplash_option) && !( parser.isSet(loadfile_option) || parser.isSet(layout_option) ) )
    // if(false) // if you uncomment this line, a kitten will die somewhere in the world.
    {
        QPixmap main_pixmap = getFunnySplashscreen();
        QSplashScreen splash(main_pixmap, Qt::WindowStaysOnTopHint);
        QDesktopWidget* desktop = QApplication::desktop();
        const int scrn = desktop->screenNumber(QCursor::pos());
        const QPoint currentDesktopsCenter = desktop->availableGeometry(scrn).center();
        splash.move(currentDesktopsCenter - splash.rect().center());

        splash.show();
        app.processEvents();

        auto deadline = QDateTime::currentDateTime().addMSecs( 500 );
        while( QDateTime::currentDateTime() < deadline)
        {
          app.processEvents();
        }

        MainWindow w(parser);

        deadline = QDateTime::currentDateTime().addMSecs( 3000 );
        while( QDateTime::currentDateTime() < deadline && !splash.isHidden() )
        {
          app.processEvents();
        }

        w.setWindowIcon(app_icon);
        w.show();
        splash.finish(&w);
        return app.exec();
    }
    MainWindow w(parser);
    w.setWindowIcon(app_icon);
    w.show();
    return app.exec();
}
