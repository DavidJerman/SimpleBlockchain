#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

#include "block.h"

// Temp
#include "QMessageBox"

#include <QObject>

class Blockchain : public QObject
{
    Q_OBJECT
public:
    Blockchain(QObject *parent = nullptr);
    virtual ~Blockchain();

    qint16 startServer();
    bool connectToPort(qint64 port);

public slots:
    // Server
    void onNewConnectionServer();
    void onDisconnectedServer();
    void onReadyReadServer();

    // Client
    void onReadyReadClient();
    void onDisconnectedClient();

    // Ledger update
    void onBroadcastLedger();

signals:
    void blockMined(const Block &block, QColor color) const;
    void messageSent(QString message, QColor color) const;
    void updateAverage(qint64);
    void update10Average(qint64);
    void difficultyChanged(qint64 diff);
    void broadcastLedger();

    // Miner
public:
    void startMining();

private:
    Block mine(const Block &prevBlock);
    void addBlock(Block block);
    bool isBlockValid(const Block &block) const;
    static bool isBlockValid(const Block &prev, const Block &block);
    bool validateLedger() const;
    bool validateLedgerIntegrity() const;
    bool validateLedger(const QVector<Block> &ledger);
    QByteArray getLedgerJson() const;
    void updateLedgerFromJson(QByteArray json);
    qint64 cumulativeDifficulty();
    qint64 cumulativeDifficulty(QVector<Block> &ledger);
    QByteArray nextHash(Block &block);

private:
    QTcpServer *_server;
    QTcpSocket *_socket;
    QList<QTcpSocket*> clients;
    QTimer *timer;
    QVector<Block> ledger;
    QDebug debug;
    bool minig {false};
    bool updated {false};
};

#endif // BLOCKCHAIN_H
