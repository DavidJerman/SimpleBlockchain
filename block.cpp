#include "block.h"

Block::Block(qint64 index, const QByteArray &data, const QByteArray &prevHash, qint8 difficulty) :
    nonce(-1)
  , index(index)
  , data(data)
  , prevHash(prevHash)
  , difficulty(difficulty)
{

}

Block::Block(qint64 index, qint64 timestamp, QByteArray data, QByteArray hash, QByteArray prevHash, qint64 nonce, qint8 difficulty)
    : index(index)
    , timestamp(timestamp)
    , data(data)
    , hash(hash)
    , prevHash(prevHash)
    , nonce(nonce)
    , difficulty(difficulty)

{

}

qint64 Block::getIndex() const
{
    return index;
}

void Block::setIndex(qint64 newIndex)
{
    index = newIndex;
}

QByteArray Block::getData() const
{
    return data;
}

void Block::setData(const QByteArray &newData)
{
    data = newData;
}

QByteArray Block::getHash() const
{
    return hash;
}

void Block::setHash(const QByteArray &newHash)
{
    hash = newHash;
}

QByteArray Block::getPrevHash() const
{
    return prevHash;
}

void Block::setPrevHash(const QByteArray &newPrevHash)
{
    prevHash = newPrevHash;
}


qint64 Block::getTimestamp() const
{
    return timestamp;
}

void Block::setTimestamp(qint64 newTimestamp)
{
    timestamp = newTimestamp;
}

qint64 Block::getNonce() const
{
    return nonce;
}

void Block::setNonce(qint64 newNonce)
{
    nonce = newNonce;
}

qint8 Block::getDifficulty() const
{
    return difficulty;
}

void Block::setDifficulty(qint8 newDifficulty)
{
    difficulty = newDifficulty;
}

QByteArray Block::calculateHash() const
{
    QByteArray temp;
    temp = QByteArray::number(index) + QByteArray::number(timestamp) + data + prevHash + QByteArray::number(difficulty) + QByteArray::number(nonce);
    return QCryptographicHash::hash(temp, QCryptographicHash::Sha256);
}

QString Block::getHashString(QByteArray hash)
{
    return hash.toHex();
}

qint8 Block::getHashDiff(const QByteArray &hash)
{
    int counter = 0;
    bool bit = false;
    for (const auto &c: hash) {
        if (bit)
            break;
        auto mask = 0b10000000;
        for (int i = 0; i < 8; i++) {
            bit = mask & c;
            if (bit)
                break;
            mask >>= 1;
            counter++;
        }
    }
    return counter;
}

QString Block::toQString() const
{
    std::stringstream stream;
    QDateTime datetime;
    datetime.setMSecsSinceEpoch(getTimestamp());
    stream << "Index: " << (int)getIndex() << "\n";
    stream << "Timestamp: " << datetime.toString().toStdString() << "\n";
    stream << "Data: " << getData().toStdString() << "\n";
    stream << "Prev hash: " << getHashString(getPrevHash()).toStdString() << "\n";
    stream << "Hash: " << getHashString(getHash()).toStdString() << "\n";
    stream << "Nonce: " << (int)getNonce() << "\n";
    stream << "Difficulty: " << (int)getDifficulty();
    stream.flush();
    return {stream.str().c_str()};
}
