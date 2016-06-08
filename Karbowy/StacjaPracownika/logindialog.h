#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <memory>

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class ClientConfig;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(const std::string& myUuid, const ClientConfig* previousConfig, QWidget *parent = 0);
    ~LoginDialog();

    std::unique_ptr<ClientConfig> retrieveResults();

private:
    Ui::LoginDialog* _ui;
    const std::string& _myUuid;
};

#endif // LOGINDIALOG_H
