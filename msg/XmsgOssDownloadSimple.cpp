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

#include <libx-msg-oss-db.h>
#include "XmsgOssDownloadSimple.h"
#include "XmsgOssUploadSimple.h"

XmsgOssDownloadSimple::XmsgOssDownloadSimple()
{

}

void XmsgOssDownloadSimple::handle(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOss4download> download)
{
	if (req->oid().empty())
	{
		XmsgOssUploadSimple::endRet(channel, RET_FORMAT_ERROR, "oid can not be null");
		return;
	}
	auto coll = XmsgOssInfoMgr::instance()->find(req->oid());
	if (coll != nullptr) 
	{
		XmsgOssDownloadSimple::handleOnFileInfo(channel, req, coll);
		return;
	}
	XmsgOssDb::instance()->future([channel, req] 
	{
		auto coll = XmsgOssInfoCollOper::instance()->find(req->oid());
		if (coll != nullptr)
		{
			XmsgOssInfoMgr::instance()->add(coll); 
			XmsgOssDownloadSimple::handleOnFileInfo(channel, req, coll);
			return;
		}
		LOG_DEBUG("can not found object info for oid, req: %s", req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_NOT_FOUND, "can not found object info for oid");
	});
}

void XmsgOssDownloadSimple::handleOnFileInfo(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOssInfoColl> coll)
{
	if (req->offset() + req->len() >= coll->objSize) 
	{
		string str;
		SPRINTF_STRING(&str, "over the object size, offset + len = %llu, object-size: %llu", (ullong )(req->offset() + req->len()), coll->objSize)
		LOG_DEBUG("over the object size, channel: %s, req: %s", channel->toString().c_str(), req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_FORBIDDEN, str);
		return;
	}
	if (req->cgt().empty()) 
	{
		SptrCgt cgt = XmsgOssDownloadSimple::getDownload(channel)->cgt;
		if (!cgt->isSame(coll->cgt)) 
		{
			LOG_DEBUG("it`s not your object, channel: %s, req: %s", channel->toString().c_str(), req->ShortDebugString().c_str())
			XmsgOssUploadSimple::endRet(channel, RET_NO_PERMISSION, "it`s not your object");
			return;
		}
		XmsgOssTransmissionMgr::instance()->future([channel, req , coll]
		{
			XmsgOssDownloadSimple::download(channel, req, coll); 
		}, req->oid());
		return;
	}
	SptrCgt cgt = ChannelGlobalTitle::parse(req->cgt());
	if (cgt == nullptr)
	{
		LOG_DEBUG("channel global title format error, channel: %s, req: %s", channel->toString().c_str(), req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_FORMAT_ERROR, "channel global title format error");
		return;
	}
	if (cgt->isSame(coll->cgt)) 
	{
		XmsgOssTransmissionMgr::instance()->future([channel, req , coll]
		{
			XmsgOssDownloadSimple::download(channel, req, coll); 
		}, req->oid());
		return;
	}
	if (!cgt->isGroup()) 
	{
		LOG_DEBUG("must be group channel global title, channel: %s, req: %s", channel->toString().c_str(), req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_FORMAT_ERROR, "must be group channel global title");
		return;
	}
	auto group = XmsgNeMgr::instance()->getGroup();
	if (group == nullptr)
	{
		LOG_ERROR("can not allocate x-msg-im-group, channel: %s, req: %s", channel->toString().c_str(), req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
		return;
	}
	shared_ptr<XmsgImGroupObjInfoQueryReq> r(new XmsgImGroupObjInfoQueryReq());
	r->set_ucgt(XmsgOssDownloadSimple::getDownload(channel)->cgt->toString());
	r->set_gcgt(req->cgt());
	r->set_oid(req->oid());
	XmsgImChannel::cast(group->channel)->begin(r, [r, channel, req, coll](SptrXiti xiti)
	{
		if (xiti->ret != RET_SUCCESS) 
		{
			XmsgOssUploadSimple::endRet(channel, RET_FORBIDDEN, xiti->desc);
			return;
		}
		LOG_DEBUG("got a object info query response, channel: %s, req: %s, rsp: %s", channel->toString().c_str(), r->ShortDebugString().c_str(), xiti->endMsg->ShortDebugString().c_str())
		XmsgOssTransmissionMgr::instance()->future([channel, req , coll]
				{
					XmsgOssDownloadSimple::download(channel, req, coll); 
				}, req->oid());
	});
}

void XmsgOssDownloadSimple::download(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOssInfoColl> coll)
{
	string object = (coll->storePath + coll->oid);
	struct stat64 s64 = { 0 };
	::stat64(object.c_str(), &s64);
	if ((ullong) s64.st_size != coll->objSize) 
	{
		LOG_FAULT("it`s a bug, object size not match, object-size: %llu, s64.st_size: %lu", coll->objSize, s64.st_size)
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
		return;
	}
	int fd = ::open64(object.c_str(), O_RDONLY);
	if (fd < 1)
	{
		LOG_ERROR("open object failed, coll: %s, errno: %s, channel: %s, req: %s", coll->toString().c_str(), ::strerror(errno), channel->toString().c_str(), req->ShortDebugString().c_str())
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
		return;
	}
	off64_t offset = ::lseek64(fd, req->offset(), SEEK_SET);
	if ((ullong) offset != (ullong) req->offset()) 
	{
		LOG_ERROR("seek object failed, coll: %s, errno: %s, channel: %s, req: %s", coll->toString().c_str(), ::strerror(errno), channel->toString().c_str(), req->ShortDebugString().c_str())
		::close(fd);
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
		return;
	}
	ullong len = req->len() < 1 ? coll->objSize - req->offset() : req->len(); 
	int seg = (int) XmsgOssCfg::instance()->cfgPb->misc().objdownloadwritebufsize();
	if (len <= (ullong) seg)
	{
		uchar* buf = (uchar*) ::malloc(len);
		int rlen = Misc::readAll(fd, buf, (int) len);
		::close(fd);
		if (rlen != (int) len)
		{
			LOG_FAULT("it`s a bug, read object length not equals buffer length, rlen: %d, len: %d, errno: %s, channel: %s, req: %s", rlen, (int ) len, ::strerror(errno), channel->toString().c_str(), req->ShortDebugString().c_str())
			::free(buf);
			XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
			return;
		}
		XmsgOssDownloadSimple::sendStartAndNoMore(channel, buf, len, XmsgOssDownloadSimple::makeRspHeader(coll));
		return;
	}
	uchar* buf = (uchar*) ::malloc(seg);
	int rlen = Misc::readAll(fd, buf, seg);
	if (rlen != seg)
	{
		LOG_FAULT("it`s a bug, read object length not equals buffer length, rlen: %d, len: %d, errno: %s, channel: %s, req: %s", rlen, seg, ::strerror(errno), channel->toString().c_str(), req->ShortDebugString().c_str())
		::free(buf);
		XmsgOssUploadSimple::endRet(channel, RET_EXCEPTION, "system exception");
		return;
	}
	XmsgOssDownloadSimple::sendStartAndMore(channel, buf, seg, len, XmsgOssDownloadSimple::makeRspHeader(coll), fd); 
}

void XmsgOssDownloadSimple::sendStartAndNoMore(shared_ptr<XscChannel> channel, uchar* buf, int len, shared_ptr<map<string, string>> header)
{
	if (channel->proType == XscProtocolType::XSC_PROTOCOL_HTTP)
	{
		shared_ptr<XmsgOssHttpChannel> httpChannel = static_pointer_cast<XmsgOssHttpChannel>(channel);
		httpChannel->future([httpChannel, buf, len, header]
		{
			httpChannel->sendBin(buf, len, len, header);
			::free(buf);
		});
		return;
	}
	::free(buf);
	LOG_FAULT("it`s a bug, only support XSC_PROTOCOL_HTTP, channel: %02X", channel->proType)
}

void XmsgOssDownloadSimple::sendStartAndMore(shared_ptr<XscChannel> channel, uchar* buf, int len , ullong length , shared_ptr<map<string, string>> header, int fd)
{
	if (channel->proType == XscProtocolType::XSC_PROTOCOL_HTTP)
	{
		shared_ptr<XmsgOssHttpChannel> httpChannel = static_pointer_cast<XmsgOssHttpChannel>(channel);
		httpChannel->download->fd = fd;
		httpChannel->download->writeSize = len;
		httpChannel->download->len = length;
		httpChannel->future([httpChannel, buf, len, length, header]
		{
			httpChannel->sendBin(buf, len, length, header);
			::free(buf);
		});
		return;
	}
	::free(buf);
	LOG_FAULT("it`s a bug, only support XSC_PROTOCOL_HTTP, channel: %02X", channel->proType)
}

shared_ptr<map<string, string>> XmsgOssDownloadSimple::makeRspHeader(shared_ptr<XmsgOssInfoColl> coll)
{
	shared_ptr<XmsgOssDownloadSimpleRsp> rsp(new XmsgOssDownloadSimpleRsp());
	rsp->set_objname(coll->objName);
	rsp->set_objsize(coll->objSize);
	rsp->set_hashval(coll->hashVal);
	rsp->set_gts(coll->gts);
	shared_ptr<map<string, string>> header(new map<string, string>());
	(*header)["x-msg-name"] = XmsgOssDownloadSimpleRsp::descriptor()->name();
	(*header)["x-msg-dat"] = Crypto::base64enc(rsp->SerializeAsString());
	return header;
}

shared_ptr<XmsgOss4download> XmsgOssDownloadSimple::getDownload(shared_ptr<XscChannel> channel)
{
	if (channel->proType == XscProtocolType::XSC_PROTOCOL_HTTP)
	{
		shared_ptr<XmsgOssHttpChannel> httpChannel = static_pointer_cast<XmsgOssHttpChannel>(channel);
		return httpChannel->download;
	}
	LOG_FAULT("it`s a bug, only support XSC_PROTOCOL_HTTP, channel: %02X", channel->proType)
	return nullptr;
}

XmsgOssDownloadSimple::~XmsgOssDownloadSimple()
{

}
