#include <QtCore/QCoreApplication>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtCore/QDebug>
#include <QtCore/QByteArray>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

// ============================================================
// 1. فئة معالجة الطلب (تعمل في خيط منفصل)
// ============================================================
class ProxyTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    ProxyTask(qintptr socketDescriptor, QNetworkAccessManager *nam)
        : m_socketDescriptor(socketDescriptor), m_nam(nam) {}

    void run() override {
        QTcpSocket clientSocket;
        if (!clientSocket.setSocketDescriptor(m_socketDescriptor)) {
            qWarning() << "❌ فشل في قبول الاتصال";
            return;
        }

        if (!clientSocket.waitForReadyRead(5000)) {
            qWarning() << "⚠️ انتهت المهلة أثناء انتظار الطلب";
            return;
        }

        QByteArray requestData = clientSocket.readAll();
        if (requestData.isEmpty()) {
            qWarning() << "⚠️ طلب فارغ";
            return;
        }

        QString requestStr = QString::fromUtf8(requestData);
        QStringList lines = requestStr.split("\r\n");
        if (lines.isEmpty()) {
            qWarning() << "⚠️ طلب غير صحيح";
            return;
        }

        QString firstLine = lines.first();
        qDebug() << "📨 الطلب:" << firstLine;

        QRegularExpression regex("^(GET|POST|HEAD|CONNECT)\\s+(https?://[^\\s]+)");
        QRegularExpressionMatch match = regex.match(firstLine);

        if (!match.hasMatch()) {
            qWarning() << "⚠️ لم يتم العثور على URL في الطلب";
            return;
        }

        QString method = match.captured(1);
        QString fullUrl = match.captured(2);

        // معالجة CONNECT (ليس مطلوباً في هذا الوكيل البسيط)
        if (method == "CONNECT") {
            qWarning() << "⚠️ CONNECT غير مدعوم في هذا الوكيل";
            clientSocket.write("HTTP/1.1 501 Not Implemented\r\n\r\n");
            clientSocket.disconnectFromHost();
            return;
        }

        QUrl url(fullUrl);
        if (!url.isValid()) {
            qWarning() << "⚠️ URL غير صحيح:" << fullUrl;
            return;
        }

        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", "RSSProxy/1.0");

        // إضافة الـ headers من الطلب الأصلي
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines[i];
            if (!line.isEmpty() && !line.startsWith("Proxy-", Qt::CaseInsensitive)) {
                int colonIndex = line.indexOf(':');
                if (colonIndex != -1) {
                    QString headerName = line.left(colonIndex).trimmed();
                    QString headerValue = line.mid(colonIndex + 1).trimmed();
                    request.setRawHeader(headerName.toUtf8(), headerValue.toUtf8());
                }
            }
        }

        // إرسال الطلب عبر HTTPS
        QNetworkReply *reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, [this, &clientSocket, reply]() {
            QByteArray responseData = reply->readAll();

            QString response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/xml; charset=utf-8\r\n";
            response += "Content-Length: " + QString::number(responseData.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";

            clientSocket.write(response.toUtf8());
            clientSocket.write(responseData);
            clientSocket.flush();
            clientSocket.disconnectFromHost();

            qDebug() << "✅ تم إرجاع" << responseData.size() << "بايت";
            reply->deleteLater();
        });
    }

private:
    qintptr m_socketDescriptor;
    QNetworkAccessManager *m_nam;
};

// ============================================================
// 2. الدالة الرئيسية
// ============================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // إجبار Qt على استخدام Schannel بدلاً من OpenSSL
    qputenv("QT_SSL_USE_OPENSSL", "0");

    QNetworkAccessManager nam;
    QThreadPool threadPool;

    QTcpServer server;
    if (!server.listen(QHostAddress::Any, 8080)) {
        qCritical() << "❌ فشل في بدء الخادم على المنفذ 8080";
        return 1;
    }

    qDebug() << "✅ وكيل HTTP/HTTPS يعمل على المنفذ 8080";
    qDebug() << "📌 استخدم عناوين مثل: http://127.0.0.1:8080/https://example.com/rss.xml";

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        if (!clientSocket) return;

        qintptr socketDescriptor = clientSocket->socketDescriptor();
        delete clientSocket;

        ProxyTask *task = new ProxyTask(socketDescriptor, &nam);
        task->setAutoDelete(true);
        threadPool.start(task);
    });

    qDebug() << "🔄 جاهز لاستقبال الطلبات...";
    return app.exec();
}

#include "proxy.moc"
