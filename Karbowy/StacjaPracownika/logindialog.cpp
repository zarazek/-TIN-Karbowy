#include "logindialog.h"
#include "ui_logindialog.h"
#include "protocol.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

class UuidValidator : public QValidator
{
public:
    UuidValidator(QObject* parent = 0) :
        QValidator(parent) { }

    State validate(QString& value, int& cursorPosition) const override;
};

static bool isHexDigit(QChar c)
{
    return ('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'f') ||
           ('A' <= c && c <= 'F');
}

QValidator::State UuidValidator::validate(QString& value, int&) const
{
    static const char* pattern = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";

    const char *pc = pattern;
    auto vc = value.constBegin(), end = value.constEnd();
    for (; *pc != '\0' && vc != end; ++pc, ++vc)
    {
        if (*pc == 'x')
        {
            if (! isHexDigit(*vc))
            {
                break;
            }
        }
        else
        {
            if (*vc != *pc)
            {
                break;
            }
        }
    }
    if (*pc == '\0' && vc == end)
    {
        // preliminary OK
        try
        {
            boost::lexical_cast<boost::uuids::uuid>(value.toStdString());
        }
        catch (boost::bad_lexical_cast&)
        {
            return Invalid;
        }
        return Acceptable;

    }
    else if (*pc == '\0')
    {
        // value too long
        return Invalid;
    }
    else if (vc == end)
    {
        // value too short;
        return Intermediate;
    }
    else
    {
        // value has some invalid characters
        return Invalid;
    }
}

LoginDialog::LoginDialog(const std::string& myUuid, const ClientConfig* previousConfig, QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::LoginDialog),
    _myUuid(myUuid)
{
    _ui->setupUi(this);
    _ui->serverUuidEdit->setValidator(new UuidValidator(this));
    _ui->portEdit->setValidator(new QIntValidator(1, std::numeric_limits<uint16_t>::max(), this));
    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &LoginDialog::accept);
    connect(_ui->buttonBox, &QDialogButtonBox::rejected, this, &LoginDialog::reject);
    if (previousConfig)
    {
        _ui->serverUuidEdit->setText(previousConfig->_serverUuid.c_str());
        _ui->serverAddressEdit->setText(previousConfig->_serverAddress.c_str());
        _ui->portEdit->setText(QString::number(previousConfig->_serverPort));
        _ui->userNameEdit->setText(previousConfig->_userId.c_str());
        _ui->passwordEdit->setText(previousConfig->_password.c_str());
        if (previousConfig->_useIpv6)
        {
            _ui->ipv4Button->setChecked(false);
            _ui->ipv6Button->setChecked(true);
        }
        else
        {
            _ui->ipv4Button->setChecked(true);
            _ui->ipv6Button->setChecked(false);
        }
    }
}

LoginDialog::~LoginDialog()
{
    delete _ui;
}

std::unique_ptr<ClientConfig> LoginDialog::retrieveResults()
{
    if (exec())
    {
        auto res = std::make_unique<ClientConfig>();
        res->_myUuid = _myUuid;
        res->_serverUuid = _ui->serverUuidEdit->text().toStdString();
        res->_serverAddress = _ui->serverAddressEdit->text().toStdString();
        res->_serverPort = _ui->portEdit->text().toUShort();
        res->_userId = _ui->userNameEdit->text().toStdString();
        res->_password = _ui->passwordEdit->text().toStdString();
        res->_useIpv6 = _ui->ipv6Button->isChecked();
        return res;
    }
    else
    {
        return std::unique_ptr<ClientConfig>();
    }
}
