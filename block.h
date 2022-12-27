#ifndef BLOCK_H
#define BLOCK_H

#include "qglobal.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <QString>
#include <QDateTime>
#include <sstream>

class Block
{
public:
    Block() = default;
    Block(qint64 index, const QByteArray &data, const QByteArray &prevHash, qint8 difficulty);
    Block(qint64 index, qint64 timestamp, QByteArray data, QByteArray hash, QByteArray prevHash, qint64 nonce, qint8 difficulty);

    qint64 getIndex() const;
    void setIndex(qint64 newIndex);
    QByteArray getData() const;
    void setData(const QByteArray &newData);
    QByteArray getHash() const;
    void setHash(const QByteArray &newHash);
    QByteArray getPrevHash() const;
    void setPrevHash(const QByteArray &newPrevHash);
    qint64 getTimestamp() const;
    void setTimestamp(qint64 newTimestamp);
    qint64 getNonce() const;
    void setNonce(qint64 newNonce);
    qint8 getDifficulty() const;
    void setDifficulty(qint8 newDifficulty);

    QByteArray calculateHash() const;
    static QString getHashString(QByteArray hash);
    static qint8 getHashDiff(const QByteArray &hash);
    QString toQString() const;

private:
    qint64 index;
    qint64 timestamp;
    QByteArray data;
    QByteArray hash;
    QByteArray prevHash;
    qint64 nonce;
    qint8 difficulty;
};

#endif // BLOCK_H
