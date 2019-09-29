#include "buyticket.h"
#include "ui_buyticket.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "clientmodel.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"
#include "guiconstants.h"
#include "styleSheet.h"

#include "base58.h"
#include "chainparams.h"
#include "wallet/coincontrol.h"
#include "validation.h" // mempool and minRelayTxFee
#include "ui_interface.h"
#include "txmempool.h"
#include "policy/fees.h"
#include "wallet/wallet.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>
#include "init.h"


//#include "chain.h"
#include "consensus/validation.h"
//#include "core_io.h"
//#include "init.h"
//#include "httpserver.h"
//#include "validation.h"
#include "net.h"
//#include "policy/feerate.h"
//#include "policy/fees.h"
//#include "policy/policy.h"
//#include "policy/rbf.h"
//#include "rpc/mining.h"
//#include "rpc/server.h"
//#include "script/sign.h"
//#include "timedata.h"
//#include "util.h"
//#include "utilmoneystr.h"
//#include "wallet/coincontrol.h"
//#include "wallet/feebumper.h"
#include "wallet/wallet.h"
//#include "wallet/walletdb.h"

#include <boost/optional.hpp>
#include "../util.h"
BuyTicket::BuyTicket(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BuyTicket),
      m_model(0),
      m_clientModel(0)
{
    m_platformStyle = platformStyle;

    // Setup ui components
    Q_UNUSED(platformStyle);
    ui->setupUi(this);
    this->setLayout(ui->verticalLayout_3);
}

BuyTicket::~BuyTicket()
{
    delete ui;
}


void BuyTicket::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void BuyTicket::setModel(WalletModel *_model)
{
    m_model = _model;
}


void BuyTicket::on_sendButton_clicked()
{
    /*if(!m_model || !m_model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    SendCoinsRecipient recipient;
    if (recipient.paymentRequest.IsInitialized())
        return ;

    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->messageTextLabel->text();
    recipient.fSubtractFeeFromAmount = (ui->checkboxSubtractFeeFromAmount->checkState() == Qt::Checked);*/

    {
        buyaticket();
    }



}
#define QT_BUYTICKET true

void BuyTicket::on_clearButton_clicked()
{
    LogPrint(BCLog::BUYTICKET,"Wallet Is Available \n");

}
void BuyTicket::buyaticket()
{

    CWallet * const pwallet = ::vpwallets[0];

    if (pwallet)
    {
        LogPrint(BCLog::BUYTICKET,"Wallet Is Available \n");
    }


    LOCK2(cs_main, pwallet->cs_wallet);


    CAmount MinerFee = ui->reqAmount->value();
    LogPrint(BCLog::BUYTICKET,"miner fee is : %d  %jd \n",MinerFee / 100000000,MinerFee);

    CAmount ticketprice = TICKET_PRICE * COIN;

    LogPrint(BCLog::BUYTICKET,"ticketprice is :%jd \n",ticketprice / 100000000);

    CCoinControl coin_control;

    bool fHasbuyTicketAddress = false;
    CBitcoinAddress buyTicketAddress;
    QString buyaddress = ui->payTo->text();
    if (!buyaddress.isEmpty())
    {
        buyTicketAddress.SetString(buyaddress.toStdString());
        LogPrint(BCLog::BUYTICKET," buy ticket from  :%s \n",buyTicketAddress.ToString());

        if (!buyTicketAddress.IsValid())
        {
            LogPrint(BCLog::BUYTICKET,"Invalid Tsc address(buyTicketAddress) to send from\n");
            ui->payTo->setValid(false);
            return ;
        }
        else
        {
            fHasbuyTicketAddress = true;
        }

    }
    else
    {
        LogPrint(BCLog::BUYTICKET," don't have buyTicketAddress\n");
    }
    //
        std::string fHasbuyTicketAddressStr;
        if(fHasbuyTicketAddress == true)
        {
            fHasbuyTicketAddressStr = "true";
        }
        else
        {
            fHasbuyTicketAddressStr = "false";
        }
        LogPrint(BCLog::BUYTICKET,"fHasbuyTicketAddress %s",fHasbuyTicketAddressStr);
    if (fHasbuyTicketAddress)
    {
        LogPrint(BCLog::BUYTICKET,"HasbuyTicketAddress \n");

        //find a UTXO with sender address
        //UniValue results(UniValue::VARR);
        std::vector<COutput> vecOutputs;

        coin_control.fAllowOtherInputs = false;

        assert(pwallet != NULL);
        pwallet->AvailableCoins(vecOutputs, false, NULL, true);
        LogPrint(BCLog::BUYTICKET,"wallet can be used :\n");
        /*for (const COutput& out : vecOutputs)
        {

            LogPrint(BCLog::BUYTICKET,"            " << out.ToString();

        }*/


        CAmount selectmoney = 0;
        for (const COutput& out : vecOutputs)
        {
            CTxDestination address;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;

            bool fValidAddress = ExtractDestination(scriptPubKey, address);

            CBitcoinAddress destAdress(address);
            //std::cout <<"####     :" <<destAdress.ToString() << std::endl;
            if (!fValidAddress || buyTicketAddress.Get() != destAdress.Get())
                continue;

            selectmoney += (out.tx->tx->vout[out.i].nValue);
            LogPrint(BCLog::BUYTICKET,"*** %s\n",out.ToString());
            coin_control.Select(COutPoint(out.tx->GetHash(), out.i));

        }
        if (ticketprice+MinerFee > selectmoney)
        {

            LogPrint(BCLog::BUYTICKET,"don't have enough selectmoney  %jd,  selectmoney  %jd\n",ticketprice + MinerFee,selectmoney);


        }
        LogPrint(BCLog::BUYTICKET,"have enough money\n");

        if (!coin_control.HasSelected())
        {
            LogPrint(BCLog::BUYTICKET,"Sender address does not have any unspent outputs\n");
        }
//"TXzZMoHXx9pSVKg2RC18qBgsCnirWhybij"

    }
    else
    {
        LogPrint(BCLog::BUYTICKET," don't have coin_control limit\n") ;
    }

    bool fChangeToSender = true;
    CBitcoinAddress ChangeToSender;


    if(ui->payTo_2->text().isEmpty())
    {
        fChangeToSender = true;

    }
    else
    {   ChangeToSender.SetString(ui->payTo_2->text().toStdString());
        if (!ChangeToSender.IsValid())
        {
            LogPrint(BCLog::BUYTICKET,"Invalid Tsc address(ChangeToSenderAddress) to send from\n");
            ui->payTo_2->setValid(false);
            return ;
        }
        else
        {
           fChangeToSender = false;
        }


    }

    if (!fChangeToSender) {
        LogPrint(BCLog::BUYTICKET," have change address \n");
        //ChangeToSender.SetString(ui->payTo_2->text().toStdString());
        coin_control.destChange = ChangeToSender.Get();
    }
    else
    {

        LogPrint(BCLog::BUYTICKET,"don't have change address \n");
        coin_control.destChange = buyTicketAddress.Get();
    }
    // Wallet comments

    if (pwallet->IsLocked())
    {
        LogPrint(BCLog::BUYTICKET,"wallet is locked \n");
    }

    CWalletTx wtx;

    toBuyTicketTx(pwallet, ticketprice, MinerFee, wtx, coin_control, fHasbuyTicketAddress);




}


void BuyTicket::toBuyTicketTx(CWallet * const pwallet, CAmount ticketprice, CAmount minerFee, CWalletTx& wtxNew, const CCoinControl& coin_control, bool fHasbuyTicketAddress)
{
    CAmount curBalance = pwallet->GetBalance();
    LogPrint(BCLog::BUYTICKET,"all Balance is : %jd how mach we need: %jd \n",curBalance / 100000000,(ticketprice + minerFee) / 100000000);

    // µÚÒ»´Î¼ì²é £ºÇ®°üÓÐ×ã¹»µÄÇ®

    if (ticketprice + minerFee > curBalance)
    {
        LogPrint(BCLog::BUYTICKET,"no enough money to buy ticket\n");
        return;
    }

    if (pwallet->GetBroadcastTransactions() && !g_connman)
    {
        LogPrint(BCLog::BUYTICKET,"Error: Peer-to-peer functionality missing or disabled\n");
        return;
        //throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    if (fWalletUnlockStakingOnly)
    {
        LogPrint(BCLog::BUYTICKET,"Error: Wallet unlocked for staking only, unable to create transaction.\n");
        //std::string strError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        //throw JSONRPCError(RPC_WALLET_ERROR, strError);
        return;
    }
    CReserveKey reservekey(pwallet);
    if (!pwallet->CreateTicketPurchaseTx(pwallet, ticketprice, minerFee, wtxNew, coin_control, fHasbuyTicketAddress))
    {
        LogPrint(BCLog::BUYTICKET,"CreateTicketPurchaseTx error\n");
        //throw JSONRPCError(RPC_WALLET_ERROR, "CreateTicketPurchaseTx error");
        return;

    }

    LogPrint(BCLog::BUYTICKET," wtxNew.tx->ToString :%s\n",wtxNew.tx->ToString());
    CValidationState state;
    if (!pwallet->CommitTicketTx(wtxNew, reservekey, g_connman.get(), state)) {
        //strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
    //	throw JSONRPCError(RPC_WALLET_ERROR, strError);
        return;
    }
    return ;

}

















