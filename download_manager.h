#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>

class DownloadManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int request_count READ request_count CONSTANT)
public:
    DownloadManager();

    Q_INVOKABLE void download_card( QUrl local_url , QUrl network_url );
    Q_INVOKABLE void download_set( const QString& set_code );
    Q_INVOKABLE void generator_rankingtable( QString set_code );

    int request_count( void );
public slots:
    void finished( QNetworkReply * reply );
signals:
    void download_end( void );
    void request_count_changed( int now_count );
private:
    QNetworkAccessManager manager;
    QMap<QUrl,QNetworkReply *> localurl_map;
};

#endif // DOWNLOAD_MANAGER_H
