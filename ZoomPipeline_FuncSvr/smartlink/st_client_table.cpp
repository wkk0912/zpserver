#include "st_client_table.h"
#include "st_clientnode_applayer.h"
#include <assert.h>
#include <functional>
namespace SmartLink{
	st_client_table::st_client_table(
			ZPNetwork::zp_net_Engine * pool,
			ZPTaskEngine::zp_pipeline * taskeng,
			ZPDatabase::DatabaseResource * pDb,
			QObject *parent) :
		QObject(parent)
	  ,m_pThreadPool(pool)
	  ,m_pTaskEngine(taskeng)
	  ,m_pDatabaseRes(pDb)
	{
		m_nHeartBeatingDeadThrd = 180;
		connect (m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_NewClientConnected,this,&st_client_table::on_evt_NewClientConnected,Qt::QueuedConnection);
		connect (m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_ClientEncrypted,this,&st_client_table::on_evt_ClientEncrypted,Qt::QueuedConnection);
		connect (m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_ClientDisconnected,this,&st_client_table::on_evt_ClientDisconnected,Qt::QueuedConnection);
		connect (m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_Data_recieved,this,&st_client_table::on_evt_Data_recieved,Qt::QueuedConnection);
		connect (m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_Data_transferred,this,&st_client_table::on_evt_Data_transferred,Qt::QueuedConnection);
	}

	int st_client_table::heartBeatingThrd()
	{
		return m_nHeartBeatingDeadThrd;
	}
	void st_client_table::setHeartBeatingThrd(int h)
	{
		m_nHeartBeatingDeadThrd = h;
	}

	//Database and disk resources
	QString st_client_table::Database_UserAcct()
	{
		return m_strDBName_useraccount;
	}
	void st_client_table::setDatabase_UserAcct(const QString & s)
	{
		m_strDBName_useraccount = s;
	}
	QString st_client_table::Database_Event()
	{
		return m_strDBName_event;
	}
	void st_client_table::setDatabase_Event(const QString & s)
	{
		m_strDBName_event = s;
	}
	QString st_client_table::largeFileFolder()
	{
		return m_largeFileFolder;
	}
	void st_client_table::setLargeFileFolder(const QString & s)
	{
		m_largeFileFolder = s;
	}

	ZPDatabase::DatabaseResource * st_client_table::dbRes()
	{
		return m_pDatabaseRes;
	}

	st_client_table::~st_client_table()
	{
	}
	void  st_client_table::KickDeadClients()
	{
		m_hash_mutex.lock();
		for (QMap<QObject *,st_clientNode_baseTrans *>::iterator p =m_hash_sock2node.begin();
			 p!=m_hash_sock2node.end();p++)
		{
			p.value()->CheckHeartBeating();
		}
		m_hash_mutex.unlock();
	}
	bool st_client_table::regisitClientUUID(st_clientNode_baseTrans * c)
	{
		if (c->uuidValid()==false)
			return false;
		m_hash_mutex.lock();
		m_hash_uuid2node[c->uuid()] = c;
		m_hash_mutex.unlock();
		return true;
	}

	st_clientNode_baseTrans *  st_client_table::clientNodeFromUUID(quint32 uuid)
	{
		m_hash_mutex.lock();
		if (m_hash_uuid2node.contains(uuid))
		{
			m_hash_mutex.unlock();
			return m_hash_uuid2node[uuid];
		}
		m_hash_mutex.unlock();

		return NULL;
	}

	st_clientNode_baseTrans *  st_client_table::clientNodeFromSocket(QObject * sock)
	{
		m_hash_mutex.lock();
		if (m_hash_sock2node.contains(sock))
		{
			m_hash_mutex.unlock();
			return m_hash_sock2node[sock];
		}
		m_hash_mutex.unlock();
		return NULL;

	}
	//this event indicates new client encrypted.
	void st_client_table::on_evt_ClientEncrypted(QObject * clientHandle)
	{
		bool nHashContains = false;
		st_clientNode_baseTrans * pClientNode = 0;
		m_hash_mutex.lock();
		nHashContains = m_hash_sock2node.contains(clientHandle);
		if (false==nHashContains)
		{
			st_clientNode_baseTrans * pnode = new st_clientNodeAppLayer(this,clientHandle,0);
			//using queued connection of send and revieve;
			connect (pnode,&st_clientNode_baseTrans::evt_SendDataToClient,m_pThreadPool,&ZPNetwork::zp_net_Engine::SendDataToClient,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_BroadcastData,m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_BroadcastData,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_close_client,m_pThreadPool,&ZPNetwork::zp_net_Engine::KickClients,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_Message,this,&st_client_table::evt_Message,Qt::QueuedConnection);
			m_hash_sock2node[clientHandle] = pnode;
			nHashContains = true;
			pClientNode = pnode;
		}
		else
		{
			pClientNode =  m_hash_sock2node[clientHandle];
		}
		m_hash_mutex.unlock();
		assert(nHashContains!=0 && pClientNode !=0);
	}

	//this event indicates new client connected.
	void  st_client_table::on_evt_NewClientConnected(QObject * clientHandle)
	{
		bool nHashContains = false;
		st_clientNode_baseTrans * pClientNode = 0;
		m_hash_mutex.lock();
		nHashContains = m_hash_sock2node.contains(clientHandle);
		if (false==nHashContains)
		{
			st_clientNode_baseTrans * pnode = new st_clientNodeAppLayer(this,clientHandle,0);
			//using queued connection of send and revieve;
			connect (pnode,&st_clientNode_baseTrans::evt_SendDataToClient,m_pThreadPool,&ZPNetwork::zp_net_Engine::SendDataToClient,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_BroadcastData,m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_BroadcastData,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_close_client,m_pThreadPool,&ZPNetwork::zp_net_Engine::KickClients,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_Message,this,&st_client_table::evt_Message,Qt::QueuedConnection);
			m_hash_sock2node[clientHandle] = pnode;
			nHashContains = true;
			pClientNode = pnode;
		}
		else
		{
			pClientNode =  m_hash_sock2node[clientHandle];
		}
		m_hash_mutex.unlock();
		assert(nHashContains!=0 && pClientNode !=0);
	}

	//this event indicates a client disconnected.
	void  st_client_table::on_evt_ClientDisconnected(QObject * clientHandle)
	{
		bool nHashContains  = false;
		st_clientNode_baseTrans * pClientNode = 0;
		m_hash_mutex.lock();
		nHashContains = m_hash_sock2node.contains(clientHandle);
		if (nHashContains)
			pClientNode =  m_hash_sock2node[clientHandle];
		if (pClientNode)
		{
			m_hash_sock2node.remove(clientHandle);
			if (pClientNode->uuidValid())
				m_hash_uuid2node.remove(pClientNode->uuid());

			pClientNode->bTermSet = true;
			disconnect (pClientNode,&st_clientNode_baseTrans::evt_SendDataToClient,m_pThreadPool,&ZPNetwork::zp_net_Engine::SendDataToClient);
			disconnect (pClientNode,&st_clientNode_baseTrans::evt_BroadcastData,m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_BroadcastData);
			disconnect (pClientNode,&st_clientNode_baseTrans::evt_close_client,m_pThreadPool,&ZPNetwork::zp_net_Engine::KickClients);
			disconnect (pClientNode,&st_clientNode_baseTrans::evt_Message,this,&st_client_table::evt_Message);

			m_nodeToBeDel.push_back(pClientNode);
			//qDebug()<<QString("%1(ref %2) Node Push in queue.\n").arg((unsigned int)pClientNode).arg(pClientNode->ref());
		}
		m_hash_mutex.unlock();

		//Try to delete objects
		QList <st_clientNode_baseTrans *> toBedel;
		foreach(st_clientNode_baseTrans * pdelobj,m_nodeToBeDel)
		{
			if (pdelobj->ref() ==0)
				toBedel.push_back(pdelobj);
			else
			{
				//qDebug()<<QString("%1(ref %2) Waiting in del queue.\n").arg((unsigned int)pdelobj).arg(pdelobj->ref());
			}
		}
		foreach(st_clientNode_baseTrans * pdelobj,toBedel)
		{
			m_nodeToBeDel.removeAll(pdelobj);

			//qDebug()<<QString("%1(ref %2) deleting.\n").arg((unsigned int)pdelobj).arg(pdelobj->ref());
			pdelobj->deleteLater();

		}

	}

	//some data arrival
	void  st_client_table::on_evt_Data_recieved(QObject *  clientHandle,const QByteArray & datablock )
	{
		//Push Clients to nodes if it is not exist
		bool nHashContains = false;
		st_clientNode_baseTrans * pClientNode = 0;
		m_hash_mutex.lock();
		nHashContains = m_hash_sock2node.contains(clientHandle);
		if (false==nHashContains)
		{
			st_clientNode_baseTrans * pnode = new st_clientNodeAppLayer(this,clientHandle,0);
			//using queued connection of send and revieve;
			connect (pnode,&st_clientNode_baseTrans::evt_SendDataToClient,m_pThreadPool,&ZPNetwork::zp_net_Engine::SendDataToClient,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_BroadcastData,m_pThreadPool,&ZPNetwork::zp_net_Engine::evt_BroadcastData,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_close_client,m_pThreadPool,&ZPNetwork::zp_net_Engine::KickClients,Qt::QueuedConnection);
			connect (pnode,&st_clientNode_baseTrans::evt_Message,this,&st_client_table::evt_Message,Qt::QueuedConnection);
			m_hash_sock2node[clientHandle] = pnode;
			nHashContains = true;
			pClientNode = pnode;
		}
		else
		{
			pClientNode =  m_hash_sock2node[clientHandle];
		}

		assert(nHashContains!=0 && pClientNode !=0);


		int nblocks =  pClientNode->push_new_data(datablock);
		if (nblocks<=1)
			m_pTaskEngine->pushTask(pClientNode);
		m_hash_mutex.unlock();

	}

	//a block of data has been successfuly sent
	void  st_client_table::on_evt_Data_transferred(QObject *   /*clientHandle*/,qint64 /*bytes sent*/)
	{

	}

}

