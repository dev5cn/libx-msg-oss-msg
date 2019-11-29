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
#include "XmsgOssUsrObjInfoQuery.h"

XmsgOssUsrObjInfoQuery::XmsgOssUsrObjInfoQuery()
{

}

void XmsgOssUsrObjInfoQuery::handle(shared_ptr<XmsgNeUsr> nu, shared_ptr<XmsgClient> client, SptrXitp trans, shared_ptr<XmsgOssUsrObjInfoQueryReq> req)
{
	XmsgOssDb::instance()->future([client, trans, req]
	{
		int pageSize = req->pagesize() < 1 ? 1 : (req->pagesize() > 0x100 ? 0x100 : req->pagesize());
		list<shared_ptr<XmsgOssInfoColl>> lis;
		if (!XmsgOssInfoCollOper::instance()->queryByGts(client->cgt, req->sts(), req->ets(), req->page(), pageSize, lis))
		{
			trans->endDesc(RET_EXCEPTION, "may be database exception");
			return;
		}
		if (lis.empty())
		{
			trans->end(RET_NO_RECORD);
			return;
		}
		shared_ptr<XmsgOssUsrObjInfoQueryRsp> rsp(new XmsgOssUsrObjInfoQueryRsp());
		for (auto& it: lis)
		{
			auto objnfo = rsp->add_objinfo();
			objnfo->set_oid(it->oid);
			objnfo->set_objname(it->objName);
			objnfo->set_objsize(it->objSize);
			objnfo->set_hashval(it->hashVal);
			objnfo->set_gts(it->gts);
		}
		trans->end(rsp);
	});
}

XmsgOssUsrObjInfoQuery::~XmsgOssUsrObjInfoQuery()
{

}

