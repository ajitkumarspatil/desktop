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

#include "shareemodel.h"
#include "accountstate.h"
#include "sharee.h"
#include "ocsshareejob.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace OCC {

Q_LOGGING_CATEGORY(lcShareeModel, "com.nextcloud.shareemodel")

ShareeModel::ShareeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _userStoppedTypingTimer.setSingleShot(true);
    _userStoppedTypingTimer.setInterval(500);
    connect(&_userStoppedTypingTimer, &QTimer::timeout, this, &ShareeModel::fetch);

    /*
    ShareeModel::ShareeSet blacklist;

    // Add the current user to _sharees since we can't share with ourself
    ShareePtr currentUser(new Sharee(_account->credentials()->user(), "", Sharee::Type::User));
    blacklist << currentUser;

    foreach (auto sw, _parentScrollArea->findChildren<ShareUserLine *>()) {
        blacklist << sw->share()->getShareWith();
    }
    */
}

/************************ QAbstractListModel methods ************************/

int ShareeModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid() || !_accountState) {
        return 0;
    }

    return _sharees.count();
}

QHash<int, QByteArray> ShareeModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles[ShareeRole] = "sharee";
    roles[AutoCompleterStringMatchRole] = "autoCompleterStringMatch";

    return roles;
}

QVariant ShareeModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > _sharees.size()) {
        return {};
    }

    const auto sharee = _sharees.at(index.row());

    if(sharee.isNull()) {
        return {};
    }

    switch(role) {
    case Qt::DisplayRole:
        return sharee->format();
    case AutoCompleterStringMatchRole:
        // Don't show this to the user
        return QString(sharee->displayName() + " (" + sharee->shareWith() + ")");
    case ShareeRole:
        return QVariant::fromValue(sharee);
    }

    return {};
}

/***************************** QPROPERTY methods *****************************/

AccountState *ShareeModel::accountState() const
{
    return _accountState.data();
}

void ShareeModel::setAccountState(AccountState *accountState)
{
    _accountState = accountState;
    Q_EMIT accountStateChanged();
}

bool ShareeModel::shareItemIsFolder() const
{
    return _shareItemIsFolder;
}

void ShareeModel::setShareItemIsFolder(const bool shareItemIsFolder)
{
    _shareItemIsFolder = shareItemIsFolder;
    Q_EMIT shareItemIsFolderChanged();
}

QString ShareeModel::searchString() const
{
    return _searchString;
}

void ShareeModel::setSearchString(const QString &searchString)
{
    _searchString = searchString;
    Q_EMIT searchStringChanged();

    _userStoppedTypingTimer.start();
}

bool ShareeModel::fetchOngoing() const
{
    return _fetchOngoing;
}

ShareeModel::LookupMode ShareeModel::lookupMode() const
{
    return _lookupMode;
}

void ShareeModel::setLookupMode(ShareeModel::LookupMode lookupMode)
{
    _lookupMode = lookupMode;
    Q_EMIT lookupModeChanged();
}

/*************************** Internal data methods ***************************/

void ShareeModel::fetch()
{
    if(!_accountState || !_accountState->account() || _searchString.isEmpty()) {
        qCInfo(lcShareeModel) << "Not fetching sharees for searchString: " << _searchString;
        return;
    }

    const auto shareItemTypeString = _shareItemIsFolder ? QStringLiteral("folder") : QStringLiteral("file");

    auto *job = new OcsShareeJob(_accountState->account());
    connect(job, &OcsShareeJob::shareeJobFinished, this, &ShareeModel::shareesFetched);
    connect(job, &OcsJob::ocsError, this, &ShareeModel::displayErrorMessage);
    job->getSharees(_searchString, shareItemTypeString, 1, 50, _lookupMode == GlobalSearch ? true : false);
}

void ShareeModel::shareesFetched(const QJsonDocument &reply)
{
    qCInfo(lcShareeModel) << "SearchString: " << _searchString << "resulted in reply: " << reply;

    QVector<ShareePtr> newSharees;

    {
        const QStringList shareeTypes {"users", "groups", "emails", "remotes", "circles", "rooms"};

        const auto appendSharees = [this, &shareeTypes](const QJsonObject &data, QVector<ShareePtr>& out) {
            for (const auto &shareeType : shareeTypes) {
                const auto category = data.value(shareeType).toArray();
                for (const auto &sharee : category) {
                    out.append(parseSharee(sharee.toObject()));
                }
            }
        };

        appendSharees(reply.object().value("ocs").toObject().value("data").toObject(), newSharees);
        appendSharees(reply.object().value("ocs").toObject().value("data").toObject().value("exact").toObject(), newSharees);
    }

    // Filter sharees that we have already shared with
    QVector<ShareePtr> filteredSharees;
    for (const auto &sharee : newSharees) {
        bool found = false;

        for (const auto &blacklistSharee : _shareeBlacklist) {
            if (sharee->type() == blacklistSharee->type() && sharee->shareWith() == blacklistSharee->shareWith()) {
                found = true;
                break;
            }
        }

        if (found == false) {
            filteredSharees.append(sharee);
        }
    }

    setNewSharees(filteredSharees);
    Q_EMIT shareesReady();
}

ShareePtr ShareeModel::parseSharee(const QJsonObject &data)
{
    QString displayName = data.value("label").toString();
    const QString shareWith = data.value("value").toObject().value("shareWith").toString();
    const Sharee::Type type = (Sharee::Type)data.value("value").toObject().value("shareType").toInt();
    const QString additionalInfo = data.value("value").toObject().value("shareWithAdditionalInfo").toString();
    if (!additionalInfo.isEmpty()) {
        displayName = tr("%1 (%2)", "sharee (shareWithAdditionalInfo)").arg(displayName, additionalInfo);
    }

    return ShareePtr(new Sharee(shareWith, displayName, type));
}

// Set the new sharee while preserving the model index so the selection stays
void ShareeModel::setNewSharees(const QVector<ShareePtr> &newSharees)
{
    Q_EMIT layoutAboutToBeChanged();

    const auto persistent = persistentIndexList();
    QVector<ShareePtr> oldPersistantSharees;
    oldPersistantSharees.reserve(persistent.size());

    std::transform(persistent.begin(), persistent.end(), std::back_inserter(oldPersistantSharees), [&](const QModelIndex &idx) {
        return idx.data(Qt::UserRole).value<ShareePtr>();
    });

    _sharees = newSharees;

    QModelIndexList newPersistant;

    for (const auto &oldPersistentSharee : oldPersistantSharees) {
        const auto it = std::find_if(_sharees.constBegin(), _sharees.constEnd(), [&](const auto newSharee) {
            return newSharee->format() == oldPersistentSharee->format() &&
                newSharee->displayName() == oldPersistentSharee->displayName();
        });

        it == _sharees.constEnd() ? newPersistant << QModelIndex() : newPersistant << index(std::distance(_sharees.constBegin(), it));
    }

    changePersistentIndexList(persistent, newPersistant);
    Q_EMIT layoutChanged();
}

ShareePtr ShareeModel::getSharee(int at)
{
    if (at < 0 || at > _sharees.size()) {
        return {};
    }

    return _sharees.at(at);
}

}
