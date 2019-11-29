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

#ifndef MSG_XMSGOSSDOWNLOADSIMPLE_H_
#define MSG_XMSGOSSDOWNLOADSIMPLE_H_

#include <libx-msg-oss-core.h>

class XmsgOssDownloadSimple
{
public:
	static void handle(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOss4download> download); 
private:
	static void handleOnFileInfo(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOssInfoColl> coll); 
	static void download(shared_ptr<XscChannel> channel, shared_ptr<XmsgOssDownloadSimpleReq> req, shared_ptr<XmsgOssInfoColl> coll); 
	static void sendStartAndNoMore(shared_ptr<XscChannel> channel, uchar* buf, int len, shared_ptr<map<string, string>> header); 
	static void sendStartAndMore(shared_ptr<XscChannel> channel, uchar* buf, int len , ullong length , shared_ptr<map<string, string>> header, int fd); 
	static shared_ptr<map<string, string>> makeRspHeader(shared_ptr<XmsgOssInfoColl> coll); 
	static shared_ptr<XmsgOss4download> getDownload(shared_ptr<XscChannel> channel); 
	XmsgOssDownloadSimple();
	virtual ~XmsgOssDownloadSimple();
};

#endif 
