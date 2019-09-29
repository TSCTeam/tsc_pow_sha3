#ifndef BUYTICKET_H
#define BUYTICKET_H

#include <QWidget>
#include "amount.h"
class PlatformStyle;
class WalletModel;
class ClientModel;
class SendCoinsRecipient;
class CWallet;
//class CAmount;
class CWalletTx;
class CCoinControl;
namespace Ui {
class BuyTicket;
}

class BuyTicket : public QWidget
{
    Q_OBJECT

public:
    explicit BuyTicket(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~BuyTicket();


    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

private:
    Ui::BuyTicket *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    const PlatformStyle* m_platformStyle;
    //SendCoinsRecipient recipient;

private Q_SLOTS:
    void on_sendButton_clicked();
    void on_clearButton_clicked();
    void buyaticket();
    void toBuyTicketTx(CWallet * const pwallet, CAmount ticketprice, CAmount minerFee, CWalletTx& wtxNew, const CCoinControl& coin_control, bool fHasbuyTicketAddress);
};

#endif // BUYTICKET_H
