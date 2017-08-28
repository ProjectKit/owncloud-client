/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include <QInputDialog>
#include <QLabel>
#include "creds/httpcredentialsgui.h"
#include "theme.h"
#include "account.h"
#include "jwt.h"
#include <iostream>

using namespace std;
using namespace QKeychain;

namespace OCC
{

void HttpCredentialsGui::askFromUser()
{
    // The rest of the code assumes that this will be done asynchronously
    QMetaObject::invokeMethod(this, "askFromUserAsync_OAuth", Qt::QueuedConnection);
}

void HttpCredentialsGui::askFromUserAsync_OAuth()
{
    _login_window = new PKWidget(this);
    _login_window->setWindowTitle(_user);
    _login_view = new PKView(_login_window);
    //set position and size
    _login_window->setFixedSize(600, 600);
    _login_view->setGeometry(0,0,600,600);
    _login_view->load(QUrl(Theme::instance()->oauthLoginURL()));
    QObject::connect(_login_view, SIGNAL(urlChanged(QUrl)),
                     this, SLOT(on_url_changed(QUrl)));
    _login_window->show();
}

bool HttpCredentialsGui::_parseAccessToken(QString url)
{
    QString accessToken = url;
    int begin = accessToken.indexOf("access_token=");
    accessToken.remove(0, begin + 13);

    int end = accessToken.indexOf("&scope");
    accessToken.remove(end, accessToken.length() - end);
    cout << "after parse:" << endl;
    cout << accessToken.toStdString() << endl;

    jwt_t * jwtobject;
    jwt_new(&jwtobject);
    jwt_decode(&jwtobject, accessToken.toStdString().c_str(), NULL, 0);
    cout << "after decode:" << endl;
    QString info(jwt_dump_str(jwtobject, 1));

    begin = info.indexOf("\"email\": \"");
    info.remove(0, begin + 10);
    end = info.indexOf("\"");
    info.remove(end, info.length() - end);

    cout << info.toStdString() << endl;

    if (_user == info) _password = accessToken;

    return _user == info;
}

void HttpCredentialsGui::on_url_changed(QUrl url)
{
    cout << "url has changed" << endl;
    cout << "new url: " << url.url().toStdString() << endl;
    if (url.scheme() == "pk")
    {
        if (_parseAccessToken(url.url()))
        {
            _ready = true;
            persist();
            _login_window->hide();
            cout << "hidden" << endl;
        }
        else
        {
        }
    }
}

void HttpCredentialsGui::askFromUserAsync()
{
    QString msg = tr("Please enter %1 password:<br>"
                     "<br>"
                     "User: %2<br>"
                     "Account: %3<br>")
                  .arg(Utility::escape(Theme::instance()->appNameGUI()),
                       Utility::escape(_user),
                       Utility::escape(_account->displayName()));

    QString reqTxt = requestAppPasswordText(_account);
    if (!reqTxt.isEmpty()) {
        msg += QLatin1String("<br>") + reqTxt + QLatin1String("<br>");
    }
    if (!_fetchErrorString.isEmpty()) {
        msg += QLatin1String("<br>") + tr("Reading from keychain failed with error: '%1'").arg(
                    Utility::escape(_fetchErrorString)) + QLatin1String("<br>");
    }

    QInputDialog dialog;
    dialog.setWindowTitle(tr("Enter Password"));
    dialog.setLabelText(msg);
    dialog.setTextValue(_previousPassword);
    dialog.setTextEchoMode(QLineEdit::Password);
    if (QLabel *dialogLabel = dialog.findChild<QLabel *>()) {
        dialogLabel->setOpenExternalLinks(true);
        dialogLabel->setTextFormat(Qt::RichText);
    }

    bool ok = dialog.exec();
    if (ok) {
        _password = dialog.textValue();
        _ready = true;
        persist();
    }
    emit asked();
}

QString HttpCredentialsGui::requestAppPasswordText(const Account* account)
{
    if (account->serverVersionInt() < 0x090100) {
        // Older server than 9.1 does not have trhe feature to request App Password
        return QString();
    }

    return tr("<a href=\"%1\">Click here</a> to request an app password from the web interface.")
        .arg(account->url().toString() + QLatin1String("/index.php/settings/personal?section=apppasswords"));
}


} // namespace OCC
