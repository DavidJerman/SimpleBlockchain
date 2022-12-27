#include "blockchain.h"
#include "Constants.h"
#include <QThread>


Blockchain::Blockchain(QObject *parent)
    : _server(nullptr)
    , _socket(nullptr)
    , QObject{parent}
    , debug(qDebug())
    , timer(nullptr)
{
    debug.noquote();
    debug.nospace();
    connect(this, SIGNAL(broadcastLedger()), this, SLOT(onBroadcastLedger()));
}


Blockchain::~Blockchain()
{
    if (_server) {
        _server->deleteLater();
        _server = nullptr;
    }
    if (_socket) {
        _socket->deleteLater();
        _socket = nullptr;
    }
}


qint16 Blockchain::startServer()
{
    if (_server) {
        emit messageSent("Server already running!", Qt::yellow);
        return -1;
    }
    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(onNewConnectionServer()));
    if (_server->listen(QHostAddress::Any)) {
        emit messageSent("Server started on port ...", Qt::gray);
    } else {
        emit messageSent("Server could not start!", Qt::red);
        _server->deleteLater();
        _server = nullptr;
        return 0;
    }
    // Start a timer to relay updates
    if (timer) {
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onBroadcastLedger()));
    timer->start(LEDGER_UPDATE_TIME);

    return _server->serverPort();
}


bool Blockchain::connectToPort(qint64 port)
{
    if (_socket) {
        emit messageSent("Already connected!", Qt::yellow);
    }
    _socket = new QTcpSocket(this);
    _socket->connectToHost(LOCALHOST, port);
    if (_socket->waitForConnected(WAIT_TIME)) {
        emit messageSent("Socket connected to: " + QString::number(port), Qt::green);
    } else {
        emit messageSent("Socket could not connect!", Qt::red);
        _socket->deleteLater();
        _socket = nullptr;
        return false;
    }
    connect(_socket, SIGNAL(readyRead()), this, SLOT(onReadyReadClient()));
    connect(_socket, SIGNAL(disconnected()), this, SLOT(onDisconnectedClient()));
    return true;
}


void Blockchain::onNewConnectionServer()
{
    auto *client = _server->nextPendingConnection();
    connect(client, SIGNAL(readyRead()), this, SLOT(onReadyReadServer()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnectedServer()));
    emit messageSent(QString("(S) Client connected: ") + QString("localhost:") + QString::number(client->peerPort()), Qt::green);
    clients.append(client);
}


void Blockchain::onDisconnectedServer()
{
    auto *client = reinterpret_cast<QTcpSocket *>(sender());
    emit messageSent("Cleint " + QString(LOCALHOST) + ":" + QString::number(client->peerPort()) +  + " has disconnected!", Qt::red);
    clients.removeAt(clients.indexOf(client));
    client->deleteLater();
}


void Blockchain::onReadyReadServer()
{
    auto *client = reinterpret_cast<QTcpSocket *>(sender());
    auto data = client->readAll();
    updateLedgerFromJson(data);
}


void Blockchain::onReadyReadClient()
{
    auto *server = reinterpret_cast<QTcpSocket *>(sender());
    auto data = server->readAll();
    updateLedgerFromJson(data);
    server->write(getLedgerJson());
}


void Blockchain::onDisconnectedClient()
{
    auto *server = reinterpret_cast<QTcpSocket *>(sender());
    emit messageSent(QString(LOCALHOST) + ":" + QString::number(server->peerPort()) + " has disconnected!", Qt::red);
    if (_socket == server) {
        _socket->deleteLater();
        _socket = nullptr;
    }
}

void Blockchain::onBroadcastLedger()
{
    if (_server && _server->isListening()) {
        auto json = getLedgerJson();
        for (const auto &client: clients) {
            client->write(json);
        }
    }
}


void Blockchain::startMining()
{
    Block block(0, 0, 0, 0);
    int maxCounter = 0;
    emit messageSent("Ledger started!", QColor(Qt::green));
    QColor color;
    while (true) {
        if (updated) {
            color = Qt::gray;
            updated = false;
        } else {
            color = Qt::black;
        }
        if (maxCounter == MAX_CHAIN_LENGTH)
            break;
        if (ledger.size() == 0) {
            emit difficultyChanged(DEFAULT_DIFF);
            block = mine(*(new Block(-1, "Block -1", QByteArray('\0', 32), DEFAULT_DIFF)));
            if (updated)
                continue;
            addBlock(block);
            emit blockMined(block, color);
        } else {
            block = mine(ledger.back());
            if (updated)
                continue;
            addBlock(block);
            emit blockMined(block, color);
        }
        maxCounter++;
    }
    emit messageSent("Stopped the ledger!", QColor(Qt::red));
    auto json = getLedgerJson();
    updateLedgerFromJson(json);
}


Block Blockchain::mine(const Block &prevBlock)
{
    Block *block;
    auto timeExpected = BLOCK_GENERATION_INTERVAL * DIFF_ADJUST_INTERVAL;
    if (prevBlock.getIndex() == -1) {
        block = new Block(0, "First block", QByteArray(), DEFAULT_DIFF);
    } else {
        block = new Block(prevBlock.getIndex() + 1, "Block " + QByteArray::number(prevBlock.getIndex() + 1), prevBlock.getHash(), DEFAULT_DIFF);
    }
    block->setDifficulty(prevBlock.getDifficulty());
    qint64 timeTaken = 0;
    Block *prevAdjBlock {nullptr};
    if (ledger.size() >= DIFF_ADJUST_INTERVAL) {
        prevAdjBlock = &ledger[ledger.size() - DIFF_ADJUST_INTERVAL];
    }
    block->setTimestamp(QDateTime::currentMSecsSinceEpoch());
    while (true) {
        if (updated)
            break;
        auto hash = nextHash(*block);
        // Adjust difficulty
        if (prevAdjBlock) {
            auto timeTaken = block->getTimestamp() - prevAdjBlock->getTimestamp();
            if (timeTaken < (timeExpected / 2)) {
                block->setDifficulty(block->getDifficulty() + 1);
                prevAdjBlock = nullptr;
                emit difficultyChanged(block->getDifficulty());
            } else if (timeTaken > (timeExpected * 2)) {
                block->setDifficulty(block->getDifficulty() - 1);
                prevAdjBlock = nullptr;
                emit difficultyChanged(block->getDifficulty());
            }
        }
        auto hashDiff = Block::getHashDiff(hash);
        if (hashDiff >= block->getDifficulty()) {
            // TODO: Change according to global value
            block->setHash(hash);
            return std::move(*block);
        }
    }
    if (prevAdjBlock)
        delete prevAdjBlock;
    return {};
}


void Blockchain::addBlock(Block block)
{
    if (isBlockValid(block)) {
        ledger.push_back(block);
        qint64 totalTime = (QDateTime::currentMSecsSinceEpoch() - ledger[0].getTimestamp()) / ledger.size();
        emit updateAverage(totalTime);
        qint64 past10Index = std::max(0, (int)ledger.size() - 10);
        qint64 time10 = (QDateTime::currentMSecsSinceEpoch() - ledger[past10Index].getTimestamp()) / (ledger.size() - past10Index);
        emit update10Average(time10);
    } else {
        emit messageSent("Invalid block:\n" + block.toQString(), QColor(Qt::red));
    }
}


bool Blockchain::isBlockValid(const Block &block) const
{
    if (block.getHash() != block.calculateHash())
        return false;
    if (block.getTimestamp() > QDateTime::currentMSecsSinceEpoch() + TIMESTAMP_LENGTH)
        return false;
    if (block.getIndex() != 0) {
        auto &prevBlock = ledger.back();
        if (prevBlock.getHash() != block.getPrevHash())
            return false;
        if (block.getTimestamp() < prevBlock.getTimestamp() - TIMESTAMP_LENGTH)
            return false;
    }
    return true;
}


bool Blockchain::isBlockValid(const Block &prevBlock, const Block &block)
{
    if (block.getPrevHash() != prevBlock.getHash())
        return false;
    if (block.calculateHash() != block.getHash())
        return false;
    if (block.getTimestamp() > QDateTime::currentMSecsSinceEpoch() + TIMESTAMP_LENGTH)
        return false;
    if (block.getTimestamp() < prevBlock.getTimestamp() - TIMESTAMP_LENGTH)
        return false;
    return true;
}


bool Blockchain::validateLedger() const
{
    if (ledger.size() == 0)
        return false;
    if (ledger.size() == 1)
        if (!isBlockValid(ledger.back()))
            return false;
    for (int i = 0; i < ledger.size() - 1; i++) {
        if (!isBlockValid(ledger[i], ledger[i + 1]))
            return false;
    }
    return true;
}


bool Blockchain::validateLedgerIntegrity() const
{
    auto valid = validateLedger();
    return valid;
}


bool Blockchain::validateLedger(const QVector<Block> &ledger)
{
    if (ledger.size() == 0)
        return false;
    if (ledger.size() == 1)
        if (!isBlockValid(ledger.back()))
            return false;
    for (int i = 0; i < ledger.size() - 1; i++) {
        if (!isBlockValid(ledger[i], ledger[i + 1]))
            return false;
    }
    return true;
}


QByteArray Blockchain::getLedgerJson() const
{
    QJsonObject jsonObject;
    for (const auto &block: ledger) {
        QJsonObject _block;
        _block.insert("index", block.getIndex());
        _block.insert("timestamp", block.getTimestamp());
        _block.insert("data", QJsonValue::fromVariant(QVariant::fromValue(block.getData().toBase64())));
        _block.insert("hash", QJsonValue::fromVariant(QVariant::fromValue(block.getHash().toBase64())));
        _block.insert("prevHash", QJsonValue::fromVariant(QVariant::fromValue(block.getPrevHash().toBase64())));
        _block.insert("nonce", block.getNonce());
        _block.insert("difficulty", block.getDifficulty());
        jsonObject.insert(QStringLiteral("%1").arg(block.getIndex(), 10, 10, QLatin1Char('0')), _block);
    }
    QJsonDocument doc;
    doc.setObject(jsonObject);
    return doc.toJson();
}


void Blockchain::updateLedgerFromJson(QByteArray json)
{
    auto doc = QJsonDocument::fromJson(json);
    if (doc.isNull())
        return;
    if (doc.isEmpty())
        return;
    auto jsonObject = doc.object();
    qint64 i = 0;
    QVector<Block> newLedger;
    for (auto item : jsonObject) {
        auto _block = item.toObject();
        auto index = _block.value("index").toInteger();
        if (index != i)
            return;
        auto timestamp = _block.value("timestamp").toInteger();
        auto data = QByteArray::fromBase64(_block.value("data").toVariant().value<QByteArray>());
        auto hash = QByteArray::fromBase64(_block.value("hash").toVariant().value<QByteArray>());
        auto prevHash = QByteArray::fromBase64(_block.value("prevHash").toVariant().value<QByteArray>());
        auto nonce = _block.value("nonce").toInteger();
        auto difficulty = _block.value("difficulty").toInteger();
        newLedger.push_back({index, timestamp, data, hash, prevHash, nonce, (qint8)difficulty});
        i++;
    }
    auto valid = validateLedger(newLedger);
    if (!valid)
        return;
    auto cumulativeDiff = cumulativeDifficulty();
    auto newCumulativeDiff = cumulativeDifficulty(newLedger);
    if (newCumulativeDiff > cumulativeDiff) {
        ledger = newLedger;
        updated = true;
        // emit messageSent("Starting with the new ledger!", QColor(Qt::black));
    }
}


/**
 * @brief Calculates the cumulative ledger difficulty. Assumes the ledger is valid.
 * @return Cumulative difficulty
 */
qint64 Blockchain::cumulativeDifficulty()
{
    qint64 sum = 0;
    for (const auto &block : ledger) {
        sum += std::pow(2, block.getDifficulty());
    }
    return sum;
}


/**
 * @brief Calculates the cumulative ledger difficulty. Assumes the ledger is valid.
 * @return Cumulative difficulty
 */
qint64 Blockchain::cumulativeDifficulty(QVector<Block> &ledger)
{
    qint64 sum = 0;
    for (const auto &block : ledger) {
        sum += std::pow(2, block.getDifficulty());
    }
    return sum;
}


QByteArray Blockchain::nextHash(Block &block)
{
    block.setNonce(block.getNonce() + 1);
    auto hash = block.calculateHash();
    return std::move(hash);
}
