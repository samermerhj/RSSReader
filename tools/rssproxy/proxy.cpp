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
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QSslSocket>
#include <QSslConfiguration>

// ============================================================
// 1. دالة تسجيل الأحداث (Logging)
// ============================================================
void logMessage(const QString &msg) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logLine = QString("[%1] %2").arg(timestamp).arg(msg);

    // طباعة في CMD
    qDebug().noquote() << logLine;

    // كتابة في ملف السجل
    QFile logFile("proxy.log");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << logLine << "\n";
        logFile.close();
    }
}

// ============================================================
// 2. فئة معالجة الطلب (محسّنة)
// ============================================================
class ProxyTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    ProxyTask(qintptr socketDescriptor, QNetworkAccessManager *nam)
        : m_socketDescriptor(socketDescriptor), m_nam(nam) {}

    void run() override {
        QTcpSocket clientSocket;

        // قبول الاتصال
        if (!clientSocket.setSocketDescriptor(m_socketDescriptor)) {
            logMessage("❌ فشل في قبول الاتصال من العميل");
            return;
        }

        logMessage("📥 اتصال جديد من " + clientSocket.peerAddress().toString());

        // انتظار الطلب
        if (!clientSocket.waitForReadyRead(5000)) {
            logMessage("⚠️ انتهت المهلة أثناء انتظار الطلب (5 ثوانٍ)");
            return;
        }

        QByteArray requestData = clientSocket.readAll();
        if (requestData.isEmpty()) {
            logMessage("⚠️ طلب فارغ من العميل");
            return;
        }

        // تحليل الطلب
        QString requestStr = QString::fromUtf8(requestData);
        QStringList lines = requestStr.split("\r\n");
        if (lines.isEmpty()) {
            logMessage("⚠️ طلب غير صحيح (بدون سطور)");
            return;
        }

        QString firstLine = lines.first();
        logMessage("📨 الطلب: " + firstLine);

        // استخراج الـ URL
        QRegularExpression regex("^(GET|POST|HEAD|CONNECT)\\s+(https?://[^\\s]+)");
        QRegularExpressionMatch match = regex.match(firstLine);

        if (!match.hasMatch()) {
            logMessage("⚠️ لم يتم العثور على URL في الطلب");
            return;
        }

        QString method = match.captured(1);
        QString fullUrl = match.captured(2);

        // معالجة CONNECT (HTTPS عبر البروكسي)
        if (method == "CONNECT") {
            logMessage("🔒 طلب CONNECT لـ " + fullUrl + " (غير مدعوم)");
            clientSocket.write("HTTP/1.1 501 Not Implemented\r\n\r\n");
            clientSocket.disconnectFromHost();
            return;
        }

        // تحليل URL
        QUrl url(fullUrl);
        if (!url.isValid()) {
            logMessage("⚠️ URL غير صحيح: " + fullUrl);
            return;
        }

        logMessage("🌐 جاري جلب: " + url.toString());

        // إعداد الطلب
        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", "RSSProxy/2.0 (Windows 7)");

        // إضافة الـ Headers من الطلب الأصلي
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

        // إعدادات SSL: إجبار استخدام TLS 1.2
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setProtocol(QSsl::TlsV1_2);
        request.setSslConfiguration(sslConfig);

        // إرسال الطلب
        QNetworkReply *reply = m_nam->get(request);

        // معالجة الاستجابة
        connect(reply, &QNetworkReply::finished, [this, &clientSocket, reply]() {
            if (reply->error() != QNetworkReply::NoError) {
                logMessage("❌ خطأ في جلب البيانات: " + reply->errorString());
                clientSocket.write("HTTP/1.1 502 Bad Gateway\r\n\r\n");
                clientSocket.disconnectFromHost();
                reply->deleteLater();
                return;
            }

            QByteArray responseData = reply->readAll();
            logMessage("✅ تم جلب " + QString::number(responseData.size()) + " بايت بنجاح");

            // بناء استجابة HTTP
            QString response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/xml; charset=utf-8\r\n";
            response += "Content-Length: " + QString::number(responseData.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";

            clientSocket.write(response.toUtf8());
            clientSocket.write(responseData);
            clientSocket.flush();
            clientSocket.disconnectFromHost();

            reply->deleteLater();
        });
    }

private:
    qintptr m_socketDescriptor;
    QNetworkAccessManager *m_nam;
};

// ============================================================
// 3. الدالة الرئيسية
// ============================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // إعدادات التطبيق
    QCoreApplication::setApplicationName("RSSProxy");
    QCoreApplication::setOrganizationName("RSSReader");

    logMessage("🚀 بدء تشغيل وكيل RSSProxy v2.0");

    // إعداد SSL: استخدام Schannel (يمكن تغييره إلى OpenSSL)
    qputenv("QT_SSL_USE_OPENSSL", "0");
    logMessage("🔐 استخدام Schannel (SSL مدمج في ويندوز)");

    // إجبار TLS 1.2
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2);
    QSslConfiguration::setDefaultConfiguration(sslConfig);
    logMessage("🔐 تم إجبار TLS 1.2");

    QNetworkAccessManager nam;
    QThreadPool threadPool;

    // بدء الخادم
    QTcpServer server;
    int port = 8080;
    if (!server.listen(QHostAddress::Any, port)) {
        logMessage("❌ فشل في بدء الخادم على المنفذ " + QString::number(port));
        return 1;
    }

    logMessage("✅ وكيل HTTP/HTTPS يعمل على المنفذ " + QString::number(port));
    logMessage("📌 استخدم عناوين مثل: http://127.0.0.1:8080/https://example.com/rss.xml");
    logMessage("📄 سجل الأحداث في: proxy.log");

    // ربط إشارة الاتصال الجديد
    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        if (!clientSocket) return;

        qintptr socketDescriptor = clientSocket->socketDescriptor();
        delete clientSocket;

        ProxyTask *task = new ProxyTask(socketDescriptor, &nam);
        task->setAutoDelete(true);
        threadPool.start(task);
    });

    logMessage("🔄 جاهز لاستقبال الطلبات...");

    return app.exec();
}

#include "proxy.moc"
