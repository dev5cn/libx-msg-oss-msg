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

#include <libx-msg-oss-core.h>
#include "XmsOssMsg.h"
#include "mgr/XmsgImMgrNeNetLoad.h"
#include "msg/XmsgOssDownloadSimple.h"
#include "msg/XmsgOssUploadSimple.h"
#include "msg/XmsgOssUsrObjInfoQuery.h"
#include "ne/XmsgNeAuth.h"
#include "ne/XmsgOssInfoQuery.h"

XmsOssMsg::XmsOssMsg()
{

}

void XmsOssMsg::init(shared_ptr<XmsgImN2HMsgMgr> priMsgMgr)
{
	XmsgOssTransmissionMgr::instance()->addMsgStub(XmsgOssUploadSimpleReq::descriptor()->name(), (void*) XmsgOssUploadSimple::handle);
	XmsgOssTransmissionMgr::instance()->addMsgStub(XmsgOssDownloadSimpleReq::descriptor()->name(), (void*) XmsgOssDownloadSimple::handle);
	X_MSG_H2N_PRPC_AFTER_AUTH(XmsgAp, XmsgOssUsrObjInfoQueryReq, XmsgOssUsrObjInfoQueryRsp, XmsgOssUsrObjInfoQuery::handle)
	X_MSG_N2H_PRPC_BEFOR_AUTH(priMsgMgr, XmsgNeAuthReq, XmsgNeAuthRsp, XmsgNeAuth::handle)
	X_MSG_N2H_PRPC_AFTER_AUTH(priMsgMgr, XmsgOssInfoQueryReq, XmsgOssInfoQueryRsp, XmsgOssInfoQuery::handle)
	X_MSG_N2H_PRPC_AFTER_AUTH(priMsgMgr, XmsgImMgrNeNetLoadReq, XmsgImMgrNeNetLoadRsp, XmsgImMgrNeNetLoad::handle)
}

XmsOssMsg::~XmsOssMsg()
{

}

