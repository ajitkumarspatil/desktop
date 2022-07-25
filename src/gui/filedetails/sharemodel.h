/*
 * Copyright (C) 2022 by Claudio Cambra <claudio.cambra@nextcloud.com>
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

#ifndef OCC_SHAREDATA_H
#define OCC_SHAREDATA_H

#include "accountstate.h"
#include "folder.h"
#include "sharemanager.h"
#include "sharepermissions.h"
#include "syncfileitem.h"

#include <QAbstractListModel>
#include <QString>
#include <QDialog>

namespace OCC {

class ShareModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AccountState* accountState READ accountState WRITE setAccountState NOTIFY accountStateChanged)
    Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath NOTIFY localPathChanged)
    Q_PROPERTY(bool accountConnected READ accountConnected NOTIFY accountConnectedChanged)
    Q_PROPERTY(bool sharingEnabled READ sharingEnabled NOTIFY sharingEnabledChanged)
    Q_PROPERTY(bool publicLinkSharesEnabled READ publicLinkSharesEnabled NOTIFY publicLinkSharesEnabledChanged)
    Q_PROPERTY(bool userGroupSharingEnabled READ userGroupSharingEnabled CONSTANT)
    Q_PROPERTY(bool canShare READ canShare NOTIFY sharePermissionsChanged)
    Q_PROPERTY(bool canCreateShare READ canCreateShare NOTIFY sharePermissionsChanged)
    Q_PROPERTY(QString lockExpirationTimeDisplayString READ lockExpirationTimeDisplayString NOTIFY lockExpirationTimeDisplayStringChanged)

public:
    enum Roles {
        ShareRole = Qt::UserRole + 1,
        IconUrlRole,
        AvatarUrlRole,
        LinkRole,
    };
    Q_ENUM(Roles)

    explicit ShareModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    AccountState *accountState() const;
    QString localPath() const;

    bool accountConnected() const;
    bool sharingEnabled() const;
    bool publicLinkSharesEnabled() const;
    bool userGroupSharingEnabled() const;
    bool canShare() const;
    bool canCreateShare() const;

    QString lockExpirationTimeDisplayString() const;

signals:
    void localPathChanged();
    void accountStateChanged();
    void accountConnectedChanged();
    void sharingEnabledChanged();
    void publicLinkSharesEnabledChanged();
    void sharePermissionsChanged();
    void lockExpirationTimeDisplayStringChanged();

    void serverError();
    void requestPasswordForLinkShare();
    void requestPasswordForEmailSharee(const ShareePtr &sharee);

public slots:
    void setAccountState(AccountState *accountState);
    void setLocalPath(const QString &localPath);

    void createNewLinkShare() const;
    void createNewLinkShareWithPassword(const QString &password) const;
    void createNewUserGroupShare(const ShareePtr &sharee);
    void createNewUserGroupShareFromQml(const QVariant &sharee);
    void createNewUserGroupShareWithPassword(const ShareePtr &sharee, const QString &password) const;
    void createNewUserGroupShareWithPasswordFromQml(const QVariant &sharee, const QString &password) const;

    void deleteShare(const QSharedPointer<Share> &share);

private slots:
    void resetData();
    void updateData();
    void initShareManager();

    void slotPropfindReceived(const QVariantMap &result);
    void slotServerError(const int code, const QString &message);
    void slotAddShare(const QSharedPointer<Share> &share);
    void slotRemoveShareWithId(const QString &shareId);
    void slotRemoveShareWithIndex(const int shareIndex);
    void slotSharesFetched(const QList<QSharedPointer<Share>> &shares);

private:
    AccountStatePtr _accountState;
    QSharedPointer<Folder> _folder;
    QString _localPath;
    QString _sharePath;
    SharePermissions _maxSharingPermissions;
    QByteArray _numericFileId;
    SyncJournalFileLockInfo _filelockState;
    QString _privateLinkUrl;
    QString _lockExpirationTimeDisplayString;

    QSharedPointer<ShareManager> _manager;

    QVector<QSharedPointer<Share>> _shares;
};

} // namespace OCC

#endif // OCC_SHAREDATA_H
