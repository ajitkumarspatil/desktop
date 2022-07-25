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

#include "filedetails.h"
#include <QDateTime>

namespace OCC {

FileDetails::FileDetails(QObject *parent)
    : QObject(parent)
{
}

void FileDetails::refreshFileDetails()
{
    _fileInfo.refresh();
    Q_EMIT fileChanged();
}

QString FileDetails::localPath() const
{
    return _localPath;
}

void FileDetails::setLocalPath(const QString &localPath)
{
    if(!_localPath.isEmpty()) {
        _fileWatcher.removePath(_localPath);
    }

    if(_fileInfo.exists()) {
        disconnect(&_fileWatcher, &QFileSystemWatcher::fileChanged, this, &FileDetails::refreshFileDetails);
    }

    _localPath = localPath;

    QFileInfo newFileInfo(localPath);
    _fileInfo = newFileInfo;

    _fileWatcher.addPath(localPath);
    connect(&_fileWatcher, &QFileSystemWatcher::fileChanged, this, &FileDetails::refreshFileDetails);

    Q_EMIT fileChanged();
}

QString FileDetails::name() const
{
    return _fileInfo.fileName();
}

QString FileDetails::sizeString() const
{
    return _locale.formattedDataSize(_fileInfo.size());
}

QString FileDetails::lastChangedString() const
{
    constexpr int secsInMinute = 60;
    constexpr int secsInHour = secsInMinute * 60;
    constexpr int secsInDay = secsInHour * 24;
    constexpr int secsInMonth = secsInDay * 30;
    constexpr int secsInYear = secsInMonth * 12;

    const auto elapsedSecs = _fileInfo.lastModified().secsTo(QDateTime::currentDateTime());

    if(elapsedSecs < 60) {
        return tr("%1 seconds ago", "a second ago", elapsedSecs).arg(elapsedSecs);
    } else if (elapsedSecs < secsInHour) {
        const int elapsedMinutes = elapsedSecs / secsInMinute;
        return tr("%1 minutes ago", "a minute ago", elapsedMinutes).arg(elapsedMinutes);
    } else if (elapsedSecs < secsInDay) {
        const int elapsedHours = elapsedSecs / secsInHour;
        return tr("%1 hours ago", "an hour ago", elapsedHours).arg(elapsedHours);
    } else if (elapsedSecs < secsInMonth) {
        const int elapsedDays = elapsedSecs / secsInDay;
        return tr("%1 days ago", "a day ago", elapsedDays).arg(elapsedDays);
    } else if (elapsedSecs < secsInYear) {
        const int elapsedMonths = elapsedSecs / secsInMonth;
        return tr("%1 months ago", "a month ago", elapsedMonths).arg(elapsedMonths);
    } else {
        const int elapsedYears = elapsedSecs / secsInYear;
        return tr("%1 years ago", "a year ago", elapsedYears).arg(elapsedYears);
    }
}

QString FileDetails::iconUrl() const
{
    return QStringLiteral("image://tray-image-provider/:/fileicon") + _localPath;
}

bool FileDetails::isFolder() const
{
    return _fileInfo.isDir();
}

} // namespace OCC
