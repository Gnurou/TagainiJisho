/*
 *  Copyright (C) 2008  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GUI_UPDATECHECKER_H_
#define __GUI_UPDATECHECKER_H_

#include <QBuffer>
#include <QNetworkAccessManager>
#include <QObject>

class UpdateChecker : public QObject {
    Q_OBJECT
  private:
    QString _versionURL;
    QNetworkAccessManager *_http;

  public:
    UpdateChecker(const QString &versionURL, QObject *parent = 0);
    void checkForUpdates(bool beta = false);
    virtual ~UpdateChecker();

  private slots:
    void finished(QNetworkReply *);

  signals:
    void updateAvailable(const QString &version);
};

#endif
