/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
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

#include "QProgressIndicator.h"

#include "creds/httpcredentialsgui.h"
#include "theme.h"
#include "account.h"
#include "configfile.h"
#include "wizard/owncloudhttpcredspage.h"
#include "wizard/owncloudwizardcommon.h"
#include "wizard/owncloudwizard.h"
#include "jwt.h"
#include <iostream>

using namespace std;
namespace OCC
{

OwncloudHttpCredsPage::OwncloudHttpCredsPage(QWidget* parent)
  : AbstractCredentialsWizardPage(),
    _ui(),
    _connected(false),
    _progressIndi(new QProgressIndicator (this))
{
    _ui.setupUi(this);

    if(parent){
        _ocWizard = qobject_cast<OwncloudWizard *>(parent);
    }

    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(Theme::instance()->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Enter user credentials")));

    _ui.resultLayout->addWidget( _progressIndi );
    stopSpinner();
    setupCustomization();
}

void OwncloudHttpCredsPage::setupCustomization()
{
    // set defaults for the customize labels.
    _ui.topLabel->hide();
    _ui.bottomLabel->hide();

    Theme *theme = Theme::instance();
    QVariant variant = theme->customMedia( Theme::oCSetupTop );
    if( !variant.isNull() ) {
        WizardCommon::setupCustomMedia( variant, _ui.topLabel );
    }

    variant = theme->customMedia( Theme::oCSetupBottom );
    WizardCommon::setupCustomMedia( variant, _ui.bottomLabel );
}

void OwncloudHttpCredsPage::initializePage()
{
    WizardCommon::initErrorLabel(_ui.errorLabel);

    OwncloudWizard* ocWizard = qobject_cast< OwncloudWizard* >(wizard());

    _ui.tokenLabel->setText(HttpCredentialsGui::requestAppPasswordText(ocWizard->account().data()));
    _ui.tokenLabel->setVisible(false);

    _login_window = new QWidget();
    _login_view = new PKView(_login_window);
    //set position and size
    _login_window->setFixedSize(600, 600);
    _login_view->setGeometry(0,0,600,600);
    _login_view->load(QUrl(Theme::instance()->oauthLoginURL()));
    QObject::connect(_login_view, SIGNAL(urlChanged(QUrl)),
                     this, SLOT(on_url_changed(QUrl)));
}

bool OwncloudHttpCredsPage::validatePage()
{
    if (_accessToken.isEmpty()) {
        return false;
    }

    if (!_connected) {
        _ui.errorLabel->setVisible(false);
        startSpinner();

        // Reset cookies to ensure the username / password is actually used
        OwncloudWizard* ocWizard = qobject_cast< OwncloudWizard* >(wizard());
        ocWizard->account()->clearCookieJar();

        emit completeChanged();
        emit connectToOCUrl(field("OCUrl").toString().simplified());

        return false;
    } else {
        // Reset, to require another connection attempt next time
        _connected = false;

        emit completeChanged();
        stopSpinner();
        return true;
    }
    return true;
}

int OwncloudHttpCredsPage::nextId() const
{
    return WizardCommon::Page_AdvancedSetup;
}

void OwncloudHttpCredsPage::setConnected()
{
    _connected = true;
    stopSpinner ();
}

void OwncloudHttpCredsPage::startSpinner()
{
    _ui.resultLayout->setEnabled(true);
    _progressIndi->setVisible(true);
    _progressIndi->startAnimation();
}

void OwncloudHttpCredsPage::stopSpinner()
{
    _ui.resultLayout->setEnabled(false);
    _progressIndi->setVisible(false);
    _progressIndi->stopAnimation();
}

void OwncloudHttpCredsPage::setErrorString(const QString& err)
{
    if( err.isEmpty()) {
        _ui.errorLabel->setVisible(false);
    } else {
        _ui.errorLabel->setVisible(true);
        _ui.errorLabel->setText(err);
    }
    emit completeChanged();
    stopSpinner();
}

AbstractCredentials* OwncloudHttpCredsPage::getCredentials() const
{
    return new HttpCredentialsGui(QString("user"), _accessToken, _ocWizard->ownCloudCertificatePath, _ocWizard->ownCloudCertificatePasswd);
}

void OwncloudHttpCredsPage::on_OAuth_clicked()
{   
    _login_window->show();
}

void OwncloudHttpCredsPage::on_url_changed(QUrl url)
{
    cout << "url has changed" << endl;
    cout << "new url: " << url.url().toStdString() << endl;
    if (url.scheme() == "pk")
    {
        _accessToken = _parseAccessToken(url.url());
        _login_window->hide();
        wizard()->next();
    }
}

QString OwncloudHttpCredsPage::_parseAccessToken(QString url)
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
    cout << jwt_dump_str(jwtobject, 1) << endl;

    return accessToken;
}


} // namespace OCC
