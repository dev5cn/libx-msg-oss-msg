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
#include "XmsgOssInfoQuery.h"

XmsgOssInfoQuery::XmsgOssInfoQuery()
{

}

void XmsgOssInfoQuery::handle(shared_ptr<XmsgNeUsr> nu, SptrXitp trans, shared_ptr<XmsgOssInfoQueryReq> req)
{
	auto coll = XmsgOssInfoMgr::instance()->find(req->oid());
	if (coll != nullptr)
	{
		shared_ptr<XmsgOssInfoQueryRsp> rsp(new XmsgOssInfoQueryRsp());
		rsp->set_cgt(coll->cgt->toString());
		rsp->set_hashval(coll->hashVal);
		rsp->set_objname(coll->objName);
		rsp->set_storepath(coll->storePath);
		rsp->set_objsize(coll->objSize);
		rsp->set_gts(coll->gts);
		trans->end(rsp);
		return;
	}
	XmsgOssDb::instance()->future([trans, req] 
	{
		auto coll = XmsgOssInfoCollOper::instance()->find(req->oid());
		if (coll == nullptr)
		{
			trans->end(RET_NOT_FOUND);
			return;
		}
		shared_ptr<XmsgOssInfoQueryRsp> rsp(new XmsgOssInfoQueryRsp());
		rsp->set_cgt(coll->cgt->toString());
		rsp->set_hashval(coll->hashVal);
		rsp->set_objname(coll->objName);
		rsp->set_storepath(coll->storePath);
		rsp->set_objsize(coll->objSize);
		rsp->set_gts(coll->gts);
		trans->end(rsp);
		XmsgOssInfoMgr::instance()->add(coll);
	});
}

XmsgOssInfoQuery::~XmsgOssInfoQuery()
{

}
