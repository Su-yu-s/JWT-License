#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>

class LogDialog : public QDialog {
    Q_OBJECT

public:
    explicit LogDialog(QWidget* parent = nullptr);

    void setLogContent(const QString& content);
    void copyToClipboard();

private:
    QPlainTextEdit* m_logEdit;
    QPushButton* m_copyBtn;
    QPushButton* m_closeBtn;
};
