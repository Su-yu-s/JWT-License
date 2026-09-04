#include "logdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>

LogDialog::LogDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("查看详细日志");
    setMinimumSize(600, 400);
    setMaximumSize(800, 600);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Log text area
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_logEdit->setFont(monoFont);
    m_logEdit->setStyleSheet(
        "QPlainTextEdit {"
        "    background-color: #1a1a1a;"
        "    color: #d4d4d4;"
        "    border: 1px solid #3d3d3d;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "}"
    );
    mainLayout->addWidget(m_logEdit, 1);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    m_copyBtn = new QPushButton("复制到剪贴板", this);
    m_closeBtn = new QPushButton("关闭", this);
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_copyBtn, &QPushButton::clicked, this, &LogDialog::copyToClipboard);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addStretch();
    btnLayout->addWidget(m_copyBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(btnLayout);
}

void LogDialog::setLogContent(const QString& content) {
    m_logEdit->setPlainText(content);
    m_logEdit->moveCursor(QTextCursor::Start);
}

void LogDialog::copyToClipboard() {
    QApplication::clipboard()->setText(m_logEdit->toPlainText());
}
