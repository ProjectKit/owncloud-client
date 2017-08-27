#ifndef PK_NETWORK_ACCESS_MANAGER_H
#define PK_NETWORK_ACCESS_MANAGER_H

#include <QNetworkAccessManager>
#include <QWebView>

class PKNetworkAccessManager : public QNetworkAccessManager
{
public:
	PKNetworkAccessManager(QObject *parent):QNetworkAccessManager(parent) {}

protected:
	QNetworkReply *createRequest(
	    QNetworkAccessManager::Operation operation, const QNetworkRequest &request,
	    QIODevice *device)
	{
	    if (request.url().scheme() == "pk")
	    	emit qobject_cast<QWebView *>(parent())->urlChanged(request.url());

	    return QNetworkAccessManager::createRequest(operation, request, device);
	}
};

class PKView : public QWebView
{
public:
	PKView(QWidget *parent = 0):
		QWebView(parent)
	{
	    PKNetworkAccessManager *newManager = new PKNetworkAccessManager(this);
	    page()->setNetworkAccessManager(newManager);
	}
};

#endif