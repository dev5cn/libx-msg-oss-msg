/*
  Copyright 2019 www.dev5.cn, Inc. dev5@qq.com
 
  This file is part of X-MSG-IM.
 
  X-MSG-IM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  X-MSG-IM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU Affero General Public License
  along with X-MSG-IM.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libmisc-ipfs-http-c.h>
#include <libx-msg-oss-db.h>
#include "XmsgOssUploadSimple.h"

XmsgOssUploadSimple::XmsgOssUploadSimple()
{

}

void XmsgOssUploadSimple::handle(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more)
{
	if (upload->req == nullptr) 
		XmsgOssUploadSimple::handleFirst(channel, req, upload, dat, len, more);
	else
		XmsgOssUploadSimple::handleContinue(channel, req, upload, dat, len, more);
}

void XmsgOssUploadSimple::handleFirst(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more)
{
	upload->req = req;
	upload->storePath = XmsgOssCfg::instance()->cfgPb->misc().objpath();
	if (!Misc::isDirExisted(upload->storePath) && !Misc::mkdir(upload->storePath))
	{
		LOG_FAULT("can not found object store path, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
		channel->close();
		return;
	}
	time_t now = DateMisc::nowGmt0() / 1000L;
	int year = DateMisc::getYear(now);
	SPRINTF_STRING(&upload->storePath, "%s%d/", Misc::endWith(upload->storePath, "/") ? "" : "/", year)
	if (!Misc::isDirExisted(upload->storePath) && !Misc::mkdir(upload->storePath))
	{
		LOG_FAULT("can not found object store path, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
		channel->close();
		return;
	}
	int month = DateMisc::getMonth(now);
	SPRINTF_STRING(&upload->storePath, "%02d/", month)
	if (!Misc::isDirExisted(upload->storePath) && !Misc::mkdir(upload->storePath))
	{
		LOG_FAULT("can not found object store path, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
		channel->close();
		return;
	}
	int day = DateMisc::getDay(now);
	SPRINTF_STRING(&upload->storePath, "%02d/", day)
	if (!Misc::isDirExisted(upload->storePath) && !Misc::mkdir(upload->storePath))
	{
		LOG_FAULT("can not found object store path, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
		channel->close();
		return;
	}
	int hour = DateMisc::getHour(now);
	SPRINTF_STRING(&upload->storePath, "%02d/", hour)
	if (!Misc::isDirExisted(upload->storePath) && !Misc::mkdir(upload->storePath))
	{
		LOG_FAULT("can not found object store path, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
		channel->close();
		return;
	}
	upload->oid = XscMisc::uuid();
	upload->fd = ::open64((upload->storePath + upload->oid).c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (upload->fd < 0)
	{
		LOG_ERROR("open object failed, path, upload: %s, channel: %s, err: %s", upload->toString().c_str(), channel->toString().c_str(), ::strerror(errno))
		channel->close();
		return;
	}
	LOG_DEBUG("open object successful, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
	ssize_t wlen = ::write(upload->fd, dat, len);
	if (wlen != (ssize_t) len)
	{
		LOG_ERROR("write object failed, path, upload: %s, channel: %s, err: %s", upload->toString().c_str(), channel->toString().c_str(), ::strerror(errno))
		::remove((upload->storePath + upload->oid).c_str());
		channel->close();
		return;
	}
	upload->storeSize += wlen;
	LOG_TRACE("write object successful, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
	upload->calHash(dat, len, more);
	if (more) 
		return;
	upload->finish();
	if (upload->storageType == XmsgOssStorageType::X_MSG_OSS_STORAGE_TYPE_DISK)
	{
		XmsgOssDb::instance()->future([channel, upload] 
		{
			XmsgOssUploadSimple::finish(channel, upload);
		});
		return;
	}
	if (upload->storageType == XmsgOssStorageType::X_MSG_OSS_STORAGE_TYPE_IPFS)
	{
		XmsgOssIpfsOper::instance()->futureu([channel, upload]
		{
			XmsgOssUploadSimple::finish4ipfs(channel, upload);
		});
		return;
	}
	LOG_FAULT("it`s a bug, unexpected x-msg-oss storage type: %d", upload->storageType)
	XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "unexpected x-msg-oss storage type");
}

void XmsgOssUploadSimple::handleContinue(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more)
{
	ssize_t wlen = ::write(upload->fd, dat, len);
	if (wlen != (ssize_t) len)
	{
		LOG_ERROR("write object failed, path, upload: %s, channel: %s, err: %s", upload->toString().c_str(), channel->toString().c_str(), ::strerror(errno))
		::remove((upload->storePath + upload->oid).c_str());
		channel->close();
		return;
	}
	upload->storeSize += wlen;
	upload->calHash(dat, len, more);
	LOG_TRACE("write object successful, upload: %s, channel: %s", upload->toString().c_str(), channel->toString().c_str())
	if (more) 
		return;
	upload->finish();
	if (upload->storageType == XmsgOssStorageType::X_MSG_OSS_STORAGE_TYPE_DISK)
	{
		XmsgOssDb::instance()->future([channel, upload]
		{
			XmsgOssUploadSimple::finish(channel, upload);
		});
	}
	if (upload->storageType == XmsgOssStorageType::X_MSG_OSS_STORAGE_TYPE_IPFS)
	{
		XmsgOssIpfsOper::instance()->futureu([channel, upload]
		{
			XmsgOssUploadSimple::finish4ipfs(channel, upload);
		});
		return;
	}
	LOG_FAULT("it`s a bug, unexpected x-msg-oss storage type: %d", upload->storageType)
	XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "unexpected x-msg-oss storage type");
}

void XmsgOssUploadSimple::finish4ipfs(shared_ptr<XscChannel> channel, shared_ptr<XmsgOss4upload> upload)
{
	shared_ptr<IpfsHttpClient> ipfs(new IpfsHttpClient(XmsgOssCfg::instance()->cfgPb->misc().ipfsnode()));
	string::size_type pos = upload->storePath.find(XmsgOssCfg::instance()->cfgPb->misc().objpath());
	string subPath = upload->storePath.substr(pos + XmsgOssCfg::instance()->cfgPb->misc().objpath().length(), upload->storePath.length());
	int err;
	string desc;
	ullong sts = DateMisc::dida();
	string path = (Misc::endWith(subPath, "/") ? "" : "/") + subPath;
	if (!ipfs->filesWrite(upload->storePath, path, err, desc)) 
	{
		LOG_ERROR("upload file to ipfs node failed, elap: %dms, err: %d, desc: %s, obj: %s", DateMisc::elapDida(sts), err, desc.c_str(), path.c_str())
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "upload to ipfs node failed");
		return;
	}
	LOG_TRACE("upload file to ipfs node successful, elap: %dms, obj: %s", DateMisc::elapDida(sts), path.c_str())
	sts = DateMisc::dida();
	shared_ptr<IpfsFileStatRsp> stat = ipfs->filesStat(path, err, desc);
	if (stat == nullptr) 
	{
		LOG_ERROR("query ipfs file stat failed, elap: %dms, err: %d, desc: %s, obj: %s", DateMisc::elapDida(sts), err, desc.c_str(), path.c_str())
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "query ipfs file stat failed");
		return;
	}
	LOG_TRACE("query ipfs file stat successful, elap: %dms, obj: %s, stat: %s", DateMisc::elapDida(sts), path.c_str(), stat->toString().c_str())
	XmsgMisc::insertKv(upload->info->mutable_kv(), "ipfs-key", stat->hash);
	XmsgOssDb::instance()->future([channel, upload] 
	{
		XmsgOssUploadSimple::finish(channel, upload);
	});
}

void XmsgOssUploadSimple::finish(shared_ptr<XscChannel> channel, shared_ptr<XmsgOss4upload> upload)
{
	shared_ptr<XmsgOssInfoColl> coll(new XmsgOssInfoColl());
	coll->oid = upload->oid;
	coll->cgt = upload->cgt;
	coll->hashVal = upload->hashVal;
	coll->objName = upload->req->objname();
	coll->storePath = upload->storePath;
	coll->objSize = upload->objSize;
	coll->storeSize = coll->objSize;
	coll->info = upload->info;
	coll->gts = DateMisc::nowGmt0();
	if (!XmsgOssInfoCollOper::instance()->insert(coll))
	{
		LOG_DEBUG("insert object info into database failed, we will delete this object: %s, upload: %s, channel: %s", coll->toString().c_str(), channel->toString().c_str(), upload->toString().c_str())
		::remove((upload->storePath + upload->oid).c_str());
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "database exception");
		return;
	}
	shared_ptr<XmsgOssUploadSimpleRsp> rsp(new XmsgOssUploadSimpleRsp());
	rsp->set_oid(coll->oid);
	if (channel->proType == XscProtocolType::XSC_PROTOCOL_HTTP)
	{
		shared_ptr<XmsgOssHttpChannel> httpChannel = static_pointer_cast<XmsgOssHttpChannel>(channel);
		httpChannel->future([httpChannel, rsp]
		{
			httpChannel->sendXmsgOssUploadSimpleRsp(rsp);
		});
		return;
	}
	LOG_FAULT("it`s a bug, only support XSC_PROTOCOL_HTTP, channel: %02X", channel->proType)
}

void XmsgOssUploadSimple::endRet(shared_ptr<XscChannel> channel, int ret, const string& desc)
{
	if (channel->proType == XscProtocolType::XSC_PROTOCOL_HTTP)
	{
		shared_ptr<XmsgOssHttpChannel> httpChannel = static_pointer_cast<XmsgOssHttpChannel>(channel);
		httpChannel->future([httpChannel, ret, desc]
		{
			httpChannel->sendRet(ret, desc);
		});
		return;
	}
	LOG_FAULT("it`s a bug, only support XSC_PROTOCOL_HTTP, channel: %02X", channel->proType)
}

XmsgOssUploadSimple::~XmsgOssUploadSimple()
{

}
