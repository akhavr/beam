#include "wallet.h"
#include "utility/logger.h"
#include "utility/io/asyncevent.h"

using namespace beam;
using namespace beam::io;
using namespace std;

struct WalletModelAsync : IWalletModelAsync
{
	WalletModelAsync(Reactor::Ptr reactor
		, shared_ptr<Wallet> wallet
		, std::shared_ptr<INetworkIO> wallet_io)
		: _reactor(reactor) 
		, _wallet(wallet)
		, _network_io(wallet_io)
	{}

	void sendMoney(beam::WalletID receiver, Amount&& amount, Amount&& fee) override
	{
        _sendMoneyEvent = AsyncEvent::create(_reactor, [this, receiver = move(receiver), amount = move(amount), fee = move(fee) ]() mutable
			{
				_wallet->transfer_money(receiver, move(amount), move(fee));
			}
		);

		_sendMoneyEvent->post();
	}

	void syncWithNode() override
	{
		_syncWithNodeEvent = AsyncEvent::create(_reactor, [this]() mutable
			{
				_network_io->connect_node();
			}
		);

		_syncWithNodeEvent->post();
	}
private:
	Reactor::Ptr _reactor;
	shared_ptr<Wallet> _wallet;
	std::shared_ptr<INetworkIO> _network_io;

	AsyncEvent::Ptr _sendMoneyEvent;
	AsyncEvent::Ptr _syncWithNodeEvent;
};

WalletModel::WalletModel(IKeyChain::Ptr keychain, uint16_t port, const string& nodeAddr)
	: _keychain(keychain)
	, _port(port)
	, _nodeAddrString(nodeAddr)
{
	qRegisterMetaType<WalletStatus>("WalletStatus");
	qRegisterMetaType<vector<TxDescription>>("std::vector<beam::TxDescription>");
	qRegisterMetaType<vector<TxPeer>>("std::vector<beam::TxPeer>");
}

WalletModel::~WalletModel()
{
	if (_wallet_io)
	{
		_wallet_io->stop();
		wait();
	}
}

WalletStatus WalletModel::getStatus() const
{
	WalletStatus status{ wallet::getAvailable(_keychain), 0, 0, 0};

	auto history = _keychain->getTxHistory();

	for (const auto& item : history)
	{
		switch (item.m_status)
		{
		case TxDescription::Completed:
			(item.m_sender ? status.sent : status.received) += item.m_amount;
			break;
		default: break;
		}
	}

	status.unconfirmed += wallet::getTotal(_keychain, Coin::Unconfirmed);

	status.update.lastTime = _keychain->getLastUpdateTime();

	return status;
}

void WalletModel::run()
{
	try
	{
		emit onStatus(getStatus());
		emit onTxStatus(_keychain->getTxHistory());
		emit onTxPeerUpdated(_keychain->getPeers());

		_reactor = Reactor::create();

		Address node_addr;

		if(node_addr.resolve(_nodeAddrString.c_str()))
		{
			_wallet_io = make_shared<WalletNetworkIO>( Address().ip(INADDR_ANY).port(_port)
				, node_addr
				, true
				, _keychain
				, _reactor);
            _wallet = make_shared<Wallet>(_keychain, _wallet_io);

			async = make_shared<WalletModelAsync>(_reactor, _wallet, _wallet_io);

			struct WalletSubscriber
			{
				WalletSubscriber(IWalletObserver* client, std::shared_ptr<beam::Wallet> wallet)
					: _client(client)
					, _wallet(wallet)
				{
					_wallet->subscribe(_client);
				}

				~WalletSubscriber()
				{
					_wallet->unsubscribe(_client);
				}
			private:
				IWalletObserver* _client;
				std::shared_ptr<beam::Wallet> _wallet;
			};

			WalletSubscriber subscriber(this, _wallet);

			_wallet_io->start();
		}
		else
		{
			LOG_ERROR() << "unable to resolve node address";
		}
	}
	catch (const runtime_error& e)
	{
		LOG_ERROR() << e.what();
	}
	catch (...)
	{
		LOG_ERROR() << "Unhandled exception";
	}
}

void WalletModel::onStatusChanged()
{
	emit onStatus(getStatus());
}

void WalletModel::onKeychainChanged()
{
	onStatusChanged();
}

void WalletModel::onTransactionChanged()
{
	emit onTxStatus(_keychain->getTxHistory());
	onStatusChanged();
}

void WalletModel::onSystemStateChanged()
{
	onStatusChanged();
}

void WalletModel::onTxPeerChanged()
{
	emit onTxPeerUpdated(_keychain->getPeers());
}

void WalletModel::onSyncProgress(int done, int total)
{
	emit onSyncProgressUpdated(done, total);
}