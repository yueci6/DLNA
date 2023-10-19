#ifndef _DLNA_H_
#define _DLNA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "list.h"
#include "queue.h"

#define MaxBuffLen (1024 * 10)
#define MaxHttpReplyLen (2000 * 10)

#define UDP_IP      "192.168.1.198"
#define UDP_PROT    8223


struct UDP_INFO {
    int sock_udp;
    struct sockaddr_in *Mulitcast_addr;
    socklen_t len;
} typedef UDP_INFO;



#define Debug(_fmt, _args...)  {printf("###%s   line:__%d__",__func__,__LINE__); printf(_fmt,##_args); printf("\n");}

#if(1)
#define Mulitcast_IP      "239.255.255.250"
#define Mulitcast_PORT    1900
#else
#define Mulitcast_IP      "224.0.0.251"
#define Mulitcast_PORT    5353
#endif




/*表示响应的 HTTP 版本和状态码，200 表示成功。*/
/*表示本机的IP和监听端口*/
/*表示缓存控制，指定响应可以在客户端缓存的最长时间（以秒为单位）。*/
/*表示服务器的标识和版本号。*/
/*表示扩展头字段的空行。*/
/*表示设备的启动 ID，用于标识设备的唯一实例。*/
/*表示设备的配置 ID，用于标识设备的不同配置。*/
/*表示设备的唯一服务名称，格式通常为 "uuid:UUID::urn:domain-name:device-type"。*/
/*表示请求中的服务类型，指示客户端正在发现根设备*/
/*表示响应消息的日期和时间。*/

#define http_get_request_reply_head \
"HTTP/1.1 200 OK\r\n"\
"Date: Thu, 01 Jan 1970 00:01:38 GMT\r\n"\
"Content-Length: 2272\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<root configId=\"8031848\" xmlns=\"urn:schemas-upnp-org:device-1-0\" xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">\r\n"\
"  <specVersion>\r\n"\
"    <major>1</major>\r\n"\
"    <minor>1</minor>\r\n"\
"  </specVersion>\r\n"\
"  <device>\r\n"\
"    <deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>\r\n"\
"    <friendlyName>QAQAQAQAQ-(dlna)</friendlyName>\r\n"\
"    <manufacturer>ZhiYing</manufacturer>\r\n"\
"    <manufacturerURL>http://www.qwqwqwqwqwq.com</manufacturerURL>\r\n"\
"    <modelDescription>MCast Media Render</modelDescription>\r\n"\
"    <modelName>qwqTest</modelName>\r\n"\
"    <modelURL>http://www.qwqwqwqwqwq.com/</modelURL>\r\n"\
"    <UDN>uuid:e6572b54-bc15-2d91-2fb5-b757f2537e21</UDN>\r\n"\
"    <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMR-1.50</dlna:X_DLNADOC>\r\n"\
"    <serviceList>\r\n"\
"      <service>\r\n"\
"        <serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>\r\n"\
"        <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>\r\n"\
"        <SCPDURL>/AVTransport/e6572b54-bc15-2d91-2fb5-b757f2537e21/scpd.xml</SCPDURL>\r\n"\
"        <controlURL>/AVTransport/e6572b54-bc15-2d91-2fb5-b757f2537e21/control.xml</controlURL>\r\n"\
"        <eventSubURL>/AVTransport/e6572b54-bc15-2d91-2fb5-b757f2537e21/event.xml</eventSubURL>\r\n"\
"      </service>\r\n"\
"      <service>\r\n"\
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>\r\n"\
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\r\n"\
"        <SCPDURL>/ConnectionManager/e6572b54-bc15-2d91-2fb5-b757f2537e21/scpd.xml</SCPDURL>\r\n"\
"        <controlURL>/ConnectionManager/e6572b54-bc15-2d91-2fb5-b757f2537e21/control.xml</controlURL>\r\n"\
"        <eventSubURL>/ConnectionManager/e6572b54-bc15-2d91-2fb5-b757f2537e21/event.xml</eventSubURL>\r\n"\
"      </service>\r\n"\
"      <service>\r\n"\
"        <serviceType>urn:schemas-upnp-org:service:RenderingControl:1</serviceType>\r\n"\
"        <serviceId>urn:upnp-org:serviceId:RenderingControl</serviceId>\r\n"\
"        <SCPDURL>/RenderingControl/e6572b54-bc15-2d91-2fb5-b757f2537e21/scpd.xml</SCPDURL>\r\n"\
"        <controlURL>/RenderingControl/e6572b54-bc15-2d91-2fb5-b757f2537e21/control.xml</controlURL>\r\n"\
"        <eventSubURL>/RenderingControl/e6572b54-bc15-2d91-2fb5-b757f2537e21/event.xml</eventSubURL>\r\n"\
"      </service>\r\n"\
"    </serviceList>\r\n"\
"  </device>\r\n"\
"</root>"

#define udp_reply_packet \
"HTTP/1.1 200 OK\r\n"\
"Location: http://%s:%d/\r\n"\
"Cache-Control: max-age=1800\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"EXT: \r\n"\
"BOOTID.UPNP.ORG: 67\r\n"\
"CONFIGID.UPNP.ORG: 8031848\r\n"\
"USN: uuid:e6572b54-bc15-2d91-2fb5-b757f2537e21::upnp:rootdevice\r\n"\
"ST: upnp:rootdevice\r\n"\
"Date: Thu, 01 Jan 1970 00:02:16 GMT\r\n"\
"\r\n"

#define HTTP_GetAVTransport \
"HTTP/1.1 200 OK\r\n"\
"Date: Thu, 01 Jan 1970 00:01:39 GMT\r\n"\
"Content-Length: 16944\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\r\n"\
"  <specVersion>\r\n"\
"    <major>1</major>\r\n"\
"    <minor>0</minor>\r\n"\
"  </specVersion>\r\n"\
"  <actionList>\r\n"\
"   <action>\r\n"\
"      <name>GetCurrentTransportActions</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Actions</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentTransportActions</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetDeviceCapabilities</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PlayMedia</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>PossiblePlaybackStorageMedia</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RecMedia</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>PossibleRecordStorageMedia</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RecQualityModes</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>PossibleRecordQualityModes</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetMediaInfo</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>NrTracks</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>NumberOfTracks</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>MediaDuration</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentMediaDuration</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentURI</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>AVTransportURI</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentURIMetaData</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>AVTransportURIMetaData</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>NextURI</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>NextAVTransportURI</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>NextURIMetaData</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>NextAVTransportURIMetaData</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PlayMedium</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>PlaybackStorageMedium</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RecordMedium</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>RecordStorageMedium</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>WriteStatus</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>RecordMediumWriteStatus</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetPositionInfo</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Track</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentTrack</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>TrackDuration</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentTrackDuration</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>TrackMetaData</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentTrackMetaData</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>TrackURI</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentTrackURI</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RelTime</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>RelativeTimePosition</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>AbsTime</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>AbsoluteTimePosition</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RelCount</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>RelativeCounterPosition</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>AbsCount</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>AbsoluteCounterPosition</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetTransportInfo</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentTransportState</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>TransportState</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentTransportStatus</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>TransportStatus</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentSpeed</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>TransportPlaySpeed</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetTransportSettings</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PlayMode</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentPlayMode</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RecQualityMode</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentRecordQualityMode</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Next</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Pause</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Play</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Speed</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>TransportPlaySpeed</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Previous</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Seek</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Unit</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_SeekMode</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Target</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_SeekTarget</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>SetAVTransportURI</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentURI</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>AVTransportURI</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentURIMetaData</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>AVTransportURIMetaData</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>SetPlayMode</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>NewPlayMode</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>CurrentPlayMode</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>Stop</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"  </actionList>\r\n"\
"  <serviceStateTable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentPlayMode</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <defaultValue>NORMAL</defaultValue>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NORMAL</allowedValue>\r\n"\
"        <allowedValue>REPEAT_ONE</allowedValue>\r\n"\
"        <allowedValue>REPEAT_ALL</allowedValue>\r\n"\
"        <allowedValue>SHUFFLE</allowedValue>\r\n"\
"        <allowedValue>SHUFFLE_NOREPEAT</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>RecordStorageMedium</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NOT_IMPLEMENTED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"yes\">\r\n"\
"      <name>LastChange</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>RelativeTimePosition</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentTrackURI</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentTrackDuration</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentRecordQualityMode</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NOT_IMPLEMENTED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentMediaDuration</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>AbsoluteCounterPosition</name>\r\n"\
"      <dataType>i4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>RelativeCounterPosition</name>\r\n"\
"      <dataType>i4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_InstanceID</name>\r\n"\
"      <dataType>ui4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>AVTransportURI</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>TransportState</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>STOPPED</allowedValue>\r\n"\
"        <allowedValue>PAUSED_PLAYBACK</allowedValue>\r\n"\
"        <allowedValue>PLAYING</allowedValue>\r\n"\
"        <allowedValue>TRANSITIONING</allowedValue>\r\n"\
"        <allowedValue>NO_MEDIA_PRESENT</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentTrackMetaData</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>NextAVTransportURI</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>PossibleRecordQualityModes</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NOT_IMPLEMENTED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentTrack</name>\r\n"\
"      <dataType>ui4</dataType>\r\n"\
"      <allowedValueRange>\r\n"\
"        <minimum>0</minimum>\r\n"\
"        <maximum>65535</maximum>\r\n"\
"        <step>1</step>\r\n"\
"      </allowedValueRange>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>AbsoluteTimePosition</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>NextAVTransportURIMetaData</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>PlaybackStorageMedium</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NONE</allowedValue>\r\n"\
"        <allowedValue>UNKNOWN</allowedValue>\r\n"\
"        <allowedValue>CD-DA</allowedValue>\r\n"\
"        <allowedValue>HDD</allowedValue>\r\n"\
"        <allowedValue>NETWORK</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>CurrentTransportActions</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>RecordMediumWriteStatus</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NOT_IMPLEMENTED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>PossiblePlaybackStorageMedia</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NONE</allowedValue>\r\n"\
"        <allowedValue>UNKNOWN</allowedValue>\r\n"\
"        <allowedValue>CD-DA</allowedValue>\r\n"\
"        <allowedValue>HDD</allowedValue>\r\n"\
"        <allowedValue>NETWORK</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>AVTransportURIMetaData</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>NumberOfTracks</name>\r\n"\
"      <dataType>ui4</dataType>\r\n"\
"      <allowedValueRange>\r\n"\
"        <minimum>0</minimum>\r\n"\
"        <maximum>65535</maximum>\r\n"\
"      </allowedValueRange>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_SeekMode</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>REL_TIME</allowedValue>\r\n"\
"        <allowedValue>TRACK_NR</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_SeekTarget</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>PossibleRecordStorageMedia</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>NOT_IMPLEMENTED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>TransportStatus</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>OK</allowedValue>\r\n"\
"        <allowedValue>ERROR_OCCURRED</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>TransportPlaySpeed</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>1</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"  </serviceStateTable>\r\n"\
"</scpd>"


#define HTTP_GetRenderingControl \
"HTTP/1.1 200 OK\r\n"\
"Date: Thu, 01 Jan 1970 00:02:16 GMT\r\n"\
"Content-Length: 6982\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\r\n"\
"  <specVersion>\r\n"\
"    <major>1</major>\r\n"\
"    <minor>0</minor>\r\n"\
"   </specVersion>\r\n"\
"   <actionList>\r\n"\
"     <action>\r\n"\
"       <name>GetMute</name>\r\n"\
"       <argumentList>\r\n"\
"         <argument>\r\n"\
"           <name>InstanceID</name>\r\n"\
"           <direction>in</direction>\r\n"\
"           <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"         </argument>\r\n"\
"         <argument>\r\n"\
"           <name>Channel</name>\r\n"\
"           <direction>in</direction>\r\n"\
"           <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"         </argument>\r\n"\
"         <argument>\r\n"\
"           <name>CurrentMute</name>\r\n"\
"           <direction>out</direction>\r\n"\
"           <relatedStateVariable>Mute</relatedStateVariable>\r\n"\
"         </argument>\r\n"\
"       </argumentList>\r\n"\
"     </action>\r\n"\
"     <action>\r\n"\
"      <name>GetVolume</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Channel</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentVolume</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>Volume</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetVolumeDB</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Channel</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentVolume</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>VolumeDB</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetVolumeDBRange</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Channel</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>MinValue</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>VolumeDB</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>MaxValue</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>VolumeDB</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>ListPresets</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>CurrentPresetNameList</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>PresetNameList</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>SelectPreset</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PresetName</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_PresetName</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>SetMute</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Channel</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>DesiredMute</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>Mute</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>SetVolume</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>InstanceID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Channel</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>DesiredVolume</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>Volume</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"  </actionList>\r\n"\
"  <serviceStateTable>\r\n"\
"    <stateVariable sendEvents=\"yes\">\r\n"\
"      <name>LastChange</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_Channel</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>Master</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_InstanceID</name>\r\n"\
"      <dataType>ui4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>Volume</name>\r\n"\
"      <dataType>ui2</dataType>\r\n"\
"      <allowedValueRange>\r\n"\
"        <minimum>0</minimum>\r\n"\
"        <maximum>100</maximum>\r\n"\
"        <step>1</step>\r\n"\
"      </allowedValueRange>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>Mute</name>\r\n"\
"      <dataType>boolean</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>PresetNameList</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>FactoryDefaults</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_PresetName</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>FactoryDefaults</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>VolumeDB</name>\r\n"\
"      <dataType>i2</dataType>\r\n"\
"      <allowedValueRange>\r\n"\
"        <minimum>-32767</minimum>\r\n"\
"        <maximum>32767</maximum>\r\n"\
"      </allowedValueRange>\r\n"\
"    </stateVariable>\r\n"\
"  </serviceStateTable>\r\n"\
"</scpd>"

#define HTTP_GetConnectionManager \
"HTTP/1.1 200 OK\r\n"\
"Date: Thu, 01 Jan 1970 00:01:02 GMT\r\n"\
"Content-Length: 4537\r\n"\
"Content-Type: text/xml; charset=\"utf-8\""\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"\
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"\
"  <specVersion>\r\n"\
"    <major>1</major>\r\n"\
"    <minor>0</minor>\r\n"\
"  </specVersion>\r\n"\
"  <actionList>\r\n"\
"    <action>\r\n"\
"      <name>GetCurrentConnectionInfo</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>ConnectionID</name>\r\n"\
"          <direction>in</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_ConnectionID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>RcsID</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_RcsID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>AVTransportID</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_AVTransportID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>ProtocolInfo</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_ProtocolInfo</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PeerConnectionManager</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_ConnectionManager</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>PeerConnectionID</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_ConnectionID</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Direction</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_Direction</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Status</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>A_ARG_TYPE_ConnectionStatus</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetProtocolInfo</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>Source</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>SourceProtocolInfo</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"        <argument>\r\n"\
"          <name>Sink</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>SinkProtocolInfo</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"    <action>\r\n"\
"      <name>GetCurrentConnectionIDs</name>\r\n"\
"      <argumentList>\r\n"\
"        <argument>\r\n"\
"          <name>ConnectionIDs</name>\r\n"\
"          <direction>out</direction>\r\n"\
"          <relatedStateVariable>CurrentConnectionIDs</relatedStateVariable>\r\n"\
"        </argument>\r\n"\
"      </argumentList>\r\n"\
"    </action>\r\n"\
"  </actionList>\r\n"\
"  <serviceStateTable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_ProtocolInfo</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_ConnectionStatus</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>OK</allowedValue>\r\n"\
"        <allowedValue>ContentFormatMismatch</allowedValue>\r\n"\
"        <allowedValue>InsufficientBandwidth</allowedValue>\r\n"\
"        <allowedValue>UnreliableChannel</allowedValue>\r\n"\
"        <allowedValue>Unknown</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_AVTransportID</name>\r\n"\
"      <dataType>i4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_RcsID</name>\r\n"\
"      <dataType>i4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_ConnectionID</name>\r\n"\
"      <dataType>i4</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_ConnectionManager</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"yes\">\r\n"\
"      <name>SourceProtocolInfo</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"yes\">\r\n"\
"      <name>SinkProtocolInfo</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"no\">\r\n"\
"      <name>A_ARG_TYPE_Direction</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"      <allowedValueList>\r\n"\
"        <allowedValue>Input</allowedValue>\r\n"\
"        <allowedValue>Output</allowedValue>\r\n"\
"      </allowedValueList>\r\n"\
"    </stateVariable>\r\n"\
"    <stateVariable sendEvents=\"yes\">\r\n"\
"      <name>CurrentConnectionIDs</name>\r\n"\
"      <dataType>string</dataType>\r\n"\
"    </stateVariable>\r\n"\
"  </serviceStateTable>\r\n"\
"</scpd>"



#define POST_GetTransportInfoResponse \
"HTTP/1.1 200 OK\r\n"\
"Ext: \r\n"\
"Date: Thu, 01 Jan 1970 00:02:11 GMT\r\n"\
"Content-Length: 439\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:GetTransportInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><CurrentTransportState>PLAYING</CurrentTransportState><CurrentTransportStatus>OK</CurrentTransportStatus><CurrentSpeed>1</CurrentSpeed></u:GetTransportInfoResponse></s:Body></s:Envelope>"

#define POST_SetAVTransportURI \
"HTTP/1.1 200 OK\r\n"\
"Ext: \r\n"\
"Date: Thu, 01 Jan 1970 00:02:11 GMT\r\n"\
"Content-Length: 277\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:SetAVTransportURIResponse xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"/></s:Body></s:Envelope>"

#define POST_AVTransportStop \
"HTTP/1.1 200 OK\r\n"\
"Ext: \r\n"\
"Date: Thu, 01 Jan 1970 00:02:11 GMT\r\n"\
"Content-Length: 264\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:StopResponse xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"/></s:Body></s:Envelope>"

#define POST_GetVolumeDBRange \
"HTTP/1.1 200 OK\r\n"\
"Ext: \r\n"\
"Date: Thu, 01 Jan 1970 00:02:11 GMT\r\n"\
"Content-Length: 281\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:GetVolumeDBRangeResponse xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\"/></s:Body></s:Envelope>"

#define POST_GetVolume \
"HTTP/1.1 200 OK\r\n"\
"Ext: \r\n"\
"Date: Thu, 01 Jan 1970 00:02:11 GMT\r\n"\
"Content-Length: 274\r\n"\
"Content-Type: text/xml; charset=\"utf-8\"\r\n"\
"Server: UPnP/1.0 DLNADOC/1.50 Platinum/1.0.5.13\r\n"\
"\r\n"\
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:GetVolumeResponse xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\"/></s:Body></s:Envelope>"
#endif
