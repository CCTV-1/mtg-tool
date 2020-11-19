#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTranslator>

#include "card.h"
#include "deck_parser.h"
#include "download_manager.h"
#include "enums.h"
#include "enum_model.h"
#include "sets_model.h"
#include "tool_settings.h"

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile log_file( "tool.log" );
    if ( log_file.size() > 1024*1024*4 )
    {
        log_file.remove();
    }

    if (!log_file.open( QIODevice::Append | QIODevice::Text ))
    {
        qFatal( "open log file failure" );
    }
    QTextStream out_stream( &log_file );

    QDateTime current_time = QDateTime::currentDateTime();
    QString time_str = current_time.toLocalTime().toString( "yyyy-MM-dd hh.mm.ss" );
    QByteArray localMsg = msg.toLocal8Bit();
    const char * file = context.file ? context.file : "";
    const char * function = context.function ? context.function : "";
    const char * type_str = "";
    switch( type )
    {
        case QtDebugMsg:
        {
            type_str = "Debug";
            break;
        }
        case QtInfoMsg:
        {
            type_str = "Info";
            break;
        }
        case QtWarningMsg:
        {
            type_str = "Warning";
            break;
        }
        case QtCriticalMsg:
        {
            type_str = "Critical";
            break;
        }
        case QtFatalMsg:
        {
            type_str = "Fatal";
            break;
        }
    }
    out_stream << QString( "%1 %2: %3 (%4:%5, %6)\n" ).arg(time_str).arg(type_str).arg(localMsg.constData()).arg(file).arg(context.line).arg(function);
    log_file.close();
}

int main( int argc , char * argv[] )
{
    qInstallMessageHandler( message_handler );

    const QUrl qml_url( "qrc:/main.qml" );

    QCoreApplication::setOrganizationName( "MTG Tools" );
    QCoreApplication::setApplicationName( "MTG Tools" );
    ToolSetting settings;

    QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QGuiApplication app( argc , argv );
    QTranslator translator_zh;
    translator_zh.load( "mtgtool_zh_CN.qm" , ":/" );
    app.installTranslator( &translator_zh );

    SetsModel sets;
    EnumModel<LanguageEnums> lang_model;
    EnumModel<ImageStylesEnums> image_styles_model;
    EnumModel<ImageNameFormatEnums> nameformat_model;
    EnumModel<SetsModel> role_model;
    SetsFilter filter;
    filter.setSourceModel( &sets );
    filter.setFilterRole( settings.get_filter_role() );

    QQmlApplicationEngine engine;
    qmlRegisterAnonymousType<Card>("Card", 1);
    qmlRegisterAnonymousType<Deck>("Deck", 1);
    qmlRegisterType<DownloadManager>( "com.mtgtool.utility" , 1 , 0 , "Downloader" );
    qmlRegisterType<DeckParser>( "com.mtgtool.utility" , 1 , 0 , "DeckParser" );
    engine.rootContext()->setContextProperty( "tool_settings", &settings );
    engine.rootContext()->setContextProperty( "lang_model", &lang_model );
    engine.rootContext()->setContextProperty( "image_styles_model", &image_styles_model );
    engine.rootContext()->setContextProperty( "nameformat_model", &nameformat_model );
    engine.rootContext()->setContextProperty( "role_model", &role_model );
    engine.rootContext()->setContextProperty( "sets_model", &filter );
    QObject::connect( &engine , &QQmlApplicationEngine::objectCreated , &app ,
    [ &qml_url ]( QObject * obj , const QUrl &objUrl )
    {
        if ( !obj && qml_url == objUrl )
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection
    );
    engine.load( qml_url );

    return app.exec();
}
