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

#include "sharemodel.h"

#include "account.h"
#include "folderman.h"
#include "theme.h"
#include "wordlist.h"

#include <QFileInfo>

namespace {
QString createRandomPassword()
{
    const auto words = OCC::WordList::getRandomWords(10);

    const auto addFirstLetter = [](const QString &current, const QString &next) -> QString {
        return current + next.at(0);
    };

    return std::accumulate(std::cbegin(words), std::cend(words), QString(), addFirstLetter);
}
}

namespace OCC {

Q_LOGGING_CATEGORY(lcShareModel, "com.nextcloud.sharemodel")

ShareModel::ShareModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/************************ QAbstractListModel methods ************************/

int ShareModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid() || !_accountState || _localPath.isEmpty()) {
        return 0;
    }

    return _shares.count();
}

QHash<int, QByteArray> ShareModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles[ShareRole] = "share";
    roles[IconUrlRole] = "iconUrl";
    roles[AvatarUrlRole] = "avatarUrl";
    roles[LinkRole] = "link";

    return roles;
}

QVariant ShareModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) {
        return {};
    }

    const auto share = _shares.at(index.row());

    if(!share) {
        return {};
    }

    switch(role) {
    case Qt::DisplayRole:
        if (const auto linkShare = share.objectCast<LinkShare>()) {
            if(!linkShare->getLabel().isEmpty()) {
                return linkShare->getLabel();
            } else {
                return tr("Link share");
            }
        } else if (share->getShareWith() && !share->getShareWith()->displayName().isEmpty()) {
            return share->getShareWith()->format();
        }

        qCWarning(lcShareModel) << "Unable to provide good display string for share";
        return QStringLiteral("Share");
    case ShareRole:
        return QVariant::fromValue(share);
    case IconUrlRole:
    {
        const auto iconsPath = QStringLiteral("image://svgimage-custom-color/");

        switch(share->getShareType()) {
        case Share::TypeLink:
            return QString(iconsPath + QStringLiteral("public.svg"));
        case Share::TypeEmail:
            return QString(iconsPath + QStringLiteral("email.svg"));
        case Share::TypeRoom:
            return QString(iconsPath + QStringLiteral("wizard-talk.svg"));
        case Share::TypeUser:
            return QString(iconsPath + QStringLiteral("user.svg"));
        default:
            break;
        }
    }
    case AvatarUrlRole:
    {
        if(share->getShareWith() && share->getShareWith()->type() == Sharee::User && _accountState && _accountState->account()) {
            const QString provider = QStringLiteral("image://tray-image-provider/");
            const QString userId = share->getShareWith()->shareWith();
            const QString avatarUrl = Utility::concatUrlPath(_accountState->account()->url(), QString("remote.php/dav/avatars/%1/%2.png").arg(userId, QString::number(64))).toString();
            return QString(provider + avatarUrl);
        }

        return {};
    }
    case LinkRole:
        if(const auto linkShare = share.objectCast<LinkShare>()) {
            return linkShare->getLink();
        }

        return {};
    default:
        return {};
    }
}

/*************************** Internal data methods ***************************/

void ShareModel::resetData()
{
    beginResetModel();

    _folder.clear();
    _sharePath.clear();
    _maxSharingPermissions = SharePermissions({});
    _numericFileId.clear();
    _filelockState = SyncJournalFileLockInfo();
    _lockExpirationTimeDisplayString.clear();
    Q_EMIT lockExpirationTimeDisplayStringChanged();
    _manager.clear();
    _shares.clear();

    endResetModel();
}

void ShareModel::updateData()
{
    if (_localPath.isEmpty() || !_accountState || _accountState->account().isNull()) {
        qCWarning(lcShareModel) << "Not updating share model data. Local path is:"  << _localPath
                                << "Is account state null:" << !_accountState;
        resetData();
        return;
    }

    if (!sharingEnabled()) {
        qCWarning(lcShareModel) << "Server does not support sharing";
        resetData();
        return;
    }

    _folder.reset(FolderMan::instance()->folderForPath(_localPath));

    if(_folder.isNull()) {
        qCWarning(lcShareModel) << "Could not update share model data for" << _localPath << "no responsible folder found";
        resetData();
        return;
    }

    const QString fileName = QFileInfo(_localPath).fileName();
    _sharePath = _folder->remotePath() + fileName;

    const QString file = _localPath.mid(_folder->cleanPath().length() + 1);
    SyncJournalFileRecord fileRecord;
    bool resharingAllowed = true; // lets assume the good

    if (_folder->journalDb()->getFileRecord(file, &fileRecord) && fileRecord.isValid()) {
        // check the permission: Is resharing allowed?
        if (!fileRecord._remotePerm.isNull() && !fileRecord._remotePerm.hasPermission(RemotePermissions::CanReshare)) {
            resharingAllowed = false;
        }
    }

    _maxSharingPermissions = resharingAllowed ? SharePermissions(_accountState->account()->capabilities().shareDefaultPermissions()) : SharePermissions({});
    Q_EMIT sharePermissionsChanged();

    _numericFileId = fileRecord.numericFileId();
    _filelockState = fileRecord._lockstate;

    if (_filelockState._locked) {
        static constexpr auto SECONDS_PER_MINUTE = 60;
        const auto lockExpirationTime = _filelockState._lockTime + _filelockState._lockTimeout;
        const auto remainingTime = QDateTime::currentDateTime().secsTo(QDateTime::fromSecsSinceEpoch(lockExpirationTime));
        const auto remainingTimeInMinutes = static_cast<int>(remainingTime > 0 ? remainingTime / SECONDS_PER_MINUTE : 0);
        _lockExpirationTimeDisplayString = tr("Locked by %1 - Expires in %2 minutes", "remaining time before lock expires", remainingTimeInMinutes).arg(_filelockState._lockOwnerDisplayName).arg(remainingTimeInMinutes);
    } else {
        _lockExpirationTimeDisplayString.clear();
    }
    Q_EMIT lockExpirationTimeDisplayStringChanged();

    auto job = new PropfindJob(_accountState->account(), _sharePath);
    job->setProperties(
        QList<QByteArray>()
        << "http://open-collaboration-services.org/ns:share-permissions"
        << "http://owncloud.org/ns:fileid" // numeric file id for fallback private link generation
        << "http://owncloud.org/ns:privatelink");
    job->setTimeout(10 * 1000);
    connect(job, &PropfindJob::result, this, &ShareModel::slotPropfindReceived);
    connect(job, &PropfindJob::finishedWithError, this, [&]{ qCWarning(lcShareModel) << "Propfind for" << _sharePath << "failed"; });
    job->start();

    initShareManager();
}

void ShareModel::initShareManager()
{
    if(!_accountState || _accountState->account().isNull()) {
        return;
    }

    bool sharingPossible = true;
    if (!publicLinkSharesEnabled()) {
        qCWarning(lcSharing) << "Link shares have been disabled";
        sharingPossible = false;
    } else if (!canShare()) {
        qCWarning(lcSharing) << "The file cannot be shared because it does not have sharing permission.";
        sharingPossible = false;
    }

    if (_manager.isNull() && sharingPossible) {
        _manager.reset(new ShareManager(_accountState->account(), this));
        connect(_manager.data(), &ShareManager::sharesFetched, this, &ShareModel::slotSharesFetched);
        connect(_manager.data(), &ShareManager::shareCreated, this, [&]{ _manager->fetchShares(_sharePath); });
        connect(_manager.data(), &ShareManager::linkShareCreated, this, &ShareModel::slotAddShare);
        connect(_manager.data(), &ShareManager::linkShareRequiresPassword, this, &ShareModel::requestPasswordForLinkShare);

        _manager->fetchShares(_sharePath);
    }
}

void ShareModel::slotPropfindReceived(const QVariantMap &result)
{
    const QVariant receivedPermissions = result["share-permissions"];
    if (!receivedPermissions.toString().isEmpty()) {
        _maxSharingPermissions = static_cast<SharePermissions>(receivedPermissions.toInt());
        Q_EMIT sharePermissionsChanged();
        qCInfo(lcShareModel) << "Received sharing permissions for" << _sharePath << _maxSharingPermissions;
    }
    auto privateLinkUrl = result["privatelink"].toString();
    auto numericFileId = result["fileid"].toByteArray();
    if (!privateLinkUrl.isEmpty()) {
        qCInfo(lcShareModel) << "Received private link url for" << _sharePath << privateLinkUrl;
        _privateLinkUrl = privateLinkUrl;
    } else if (!numericFileId.isEmpty()) {
        qCInfo(lcShareModel) << "Received numeric file id for" << _sharePath << numericFileId;
        _privateLinkUrl = _accountState->account()->deprecatedPrivateLinkUrl(numericFileId).toString(QUrl::FullyEncoded);
    }
}

void ShareModel::slotSharesFetched(const QList<QSharedPointer<Share>> &shares)
{
    qCInfo(lcSharing) << "Fetched" << shares.count() << "shares";

    for (const auto &share : shares) {
        if (share.isNull() ||
            share->account().isNull() ||
            share->getUidOwner() != share->account()->davUser()) {

            continue;
        }

        slotAddShare(share);
    }
}

void ShareModel::slotAddShare(const QSharedPointer<Share> &share)
{
    if(share.isNull()) {
        return;
    }

    const auto shareId = share->getId();

    // Remove share with the same ID if we have it
    slotRemoveShareWithId(shareId);

    beginInsertRows({}, _shares.count(), _shares.count());
    _shares.append(share);
    endInsertRows();

    connect(share.data(), &Share::serverError, this, &ShareModel::slotServerError);
    // Passing shareId by reference here will cause crashing, so we pass by value
    connect(share.data(), &Share::shareDeleted, this, [this, shareId]{ slotRemoveShareWithId(shareId); });

    if(_manager) {
        connect(_manager.data(), &ShareManager::serverError, this, &ShareModel::slotServerError);
    }
}

void ShareModel::slotRemoveShareWithId(const QString &shareId)
{
    if(_shares.count() == 0 || shareId.isEmpty()) {
        return;
    }

    const auto existingShareIterator = std::find_if(_shares.begin(), _shares.end(), [&shareId](const QSharedPointer<Share> &findShare) {
        return findShare->getId() == shareId;
    });
    const auto shareIndex = existingShareIterator - _shares.begin();

    slotRemoveShareWithIndex(shareIndex);
}

void ShareModel::slotRemoveShareWithIndex(const int shareIndex)
{
    if(shareIndex < 0 || shareIndex >= _shares.count()) {
        return;
    }

    beginRemoveRows({}, shareIndex, shareIndex);
    _shares.removeAt(shareIndex);
    endRemoveRows();
}

void ShareModel::slotServerError(const int code, const QString &message)
{
    qCWarning(lcShareModel) << "Error from server" << code << message;
    Q_EMIT serverError();
}

void ShareModel::createNewLinkShare() const
{
    if(_manager) {
        const auto askOptionalPassword = _accountState->account()->capabilities().sharePublicLinkAskOptionalPassword();
        const auto password = askOptionalPassword ? createRandomPassword() : QString();
        _manager->createLinkShare(_sharePath, QString(), password);
    }
}

void ShareModel::createNewLinkShareWithPassword(const QString &password) const
{
    if(_manager) {
        _manager->createLinkShare(_sharePath, QString(), password);
    }
}

void ShareModel::createNewUserGroupShare(const ShareePtr &sharee)
{
    if (sharee.isNull()) {
        return;
    }

    qCInfo(lcShareModel) << "Creating new user/group share for sharee: " << sharee->format();

    if (sharee->type() == Sharee::Email &&
        _accountState &&
        !_accountState->account().isNull() &&
        _accountState->account()->capabilities().isValid() &&
        _accountState->account()->capabilities().shareEmailPasswordEnforced()) {

        Q_EMIT requestPasswordForEmailSharee(sharee);
        return;
    }

    _manager->createShare(_sharePath, Share::ShareType(sharee->type()),
                          sharee->shareWith(), _maxSharingPermissions, {});
}

void ShareModel::createNewUserGroupShareWithPassword(const ShareePtr &sharee, const QString &password) const
{
    if (sharee.isNull()) {
        return;
    }

    _manager->createShare(_sharePath, Share::ShareType(sharee->type()),
                          sharee->shareWith(), _maxSharingPermissions, password);
}

void ShareModel::createNewUserGroupShareFromQml(const QVariant &sharee)
{
    const auto ptr = sharee.value<ShareePtr>();
    createNewUserGroupShare(ptr);
}

void ShareModel::createNewUserGroupShareWithPasswordFromQml(const QVariant &sharee, const QString &password) const
{
    const auto ptr = sharee.value<ShareePtr>();
    createNewUserGroupShareWithPassword(ptr, password);
}

void ShareModel::deleteShare(const QSharedPointer<Share> &share)
{
    share->deleteShare();
}

/***************************** QPROPERTY methods *****************************/

QString ShareModel::localPath() const
{
    return _localPath;
}

void ShareModel::setLocalPath(const QString &localPath)
{
    _localPath = localPath;
    Q_EMIT localPathChanged();
    updateData();
}

AccountState *ShareModel::accountState() const
{
    return _accountState.data();
}

void ShareModel::setAccountState(AccountState *accountState)
{
    if(_accountState) {
        disconnect(_accountState.data(), &AccountState::stateChanged, this, &ShareModel::accountConnectedChanged);
    }

    AccountStatePtr newAccountState(accountState);
    _accountState.swap(newAccountState);
    connect(_accountState.data(), &AccountState::stateChanged, this, &ShareModel::accountConnectedChanged);
    Q_EMIT accountStateChanged();

    // Change the server and account-related properties
    Q_EMIT accountConnectedChanged();
    Q_EMIT sharingEnabledChanged();
    Q_EMIT publicLinkSharesEnabledChanged();
    updateData();
}

bool ShareModel::accountConnected() const
{
    return _accountState &&
            _accountState->isConnected();
}

bool ShareModel::sharingEnabled() const
{
    return _accountState &&
            _accountState->account() &&
            _accountState->account()->capabilities().isValid() &&
            _accountState->account()->capabilities().shareAPI();
}

bool ShareModel::publicLinkSharesEnabled() const
{
    return _accountState &&
            _accountState->account() &&
            _accountState->account()->capabilities().isValid() &&
            _accountState->account()->capabilities().sharePublicLink() &&
            Theme::instance()->linkSharing();
}

bool ShareModel::userGroupSharingEnabled() const
{
    return Theme::instance()->userGroupSharing();
}

QString ShareModel::lockExpirationTimeDisplayString() const
{
    return _lockExpirationTimeDisplayString;
}

bool ShareModel::canShare() const
{
    return _maxSharingPermissions & SharePermissionShare;
}

bool ShareModel::canCreateShare() const
{
    return _maxSharingPermissions & SharePermissionCreate;
}

} // namespace OCC
