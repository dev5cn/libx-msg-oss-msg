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

#ifndef MSG_XMSGOSSUPLOADSIMPLE_H_
#define MSG_XMSGOSSUPLOADSIMPLE_H_

#include <libx-msg-oss-core.h>

class XmsgOssUploadSimple
{
public:
	static void handle(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more); 
private:
	static void handleFirst(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more);
	static void handleContinue(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssUploadSimpleReq> req, shared_ptr<XmsgOss4upload> upload, uchar* dat, int len, bool more);
private:
	static void finish(shared_ptr<XscChannel> channel, shared_ptr<XmsgOss4upload> upload); 
	static void finish4ipfs(shared_ptr<XscChannel> channel, shared_ptr<XmsgOss4upload> upload); 
private:
	XmsgOssUploadSimple();
	virtual ~XmsgOssUploadSimple();
public:
	static void endRet(shared_ptr<XscChannel> channel, int ret, const string& desc); 
};

#endif 
