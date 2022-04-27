- [中文文档](README.md)

# Introduction

TarsGateway is a general API gateway based on the TARS development framework, supporting the HTTP protocol for requests and tars-tup&tars-tars protocol, tars-json protocol, and HTTP protocol for the backend. In addition to protocol forwarding, it also supports flow control, black and white lists and other functions. For more information, please refer to [TarsDocs](http://doc.tarsyun.com)

# Versions Supported

- TarsCpp: >= v2.4.5
- TarsJava: >= v1.7.2
- TarsGo: >= v1.1.4
- TarsNode: rpc: >= v2.0.14, stream: >= v2.0.3, tars2node: >= v20200707
- TarsPHP: tars-server: >= v0.6.0

## Installation

Before installation, please note that the gateway service depends on MySQL, and the related database and tables[请参考 db_base.sql](./yaml/db_base.sql)

There are two modes to install gateway on tars:

- Install from service market

Please use the latest version of tars framework (>=tarscloud/framework:v3.0.7 or >=K8SFramework:v1.2.5) to install services directly through the service market!

From the top tab of tarsweb, click the service market to enter, then select `base/gatewayserver` in the directory tree on the left, select the corresponding version on the right, and then click Install!

Note that you need to use `db_base.sql` (available for download in the service market) manually create the database table, and then configure the configuration file `GatewayServer.conf`, edit the address of DB in conf

```
<main>
    .....

    # Pay attention to modify the configuration and point to the DB address you created
    <db>
        charset=utf8
        dbhost =db.tars.com
        dbname =db_base
        dbpass =taf2015
        dbport =3306
        dbuser =tars
    </db>
</main>
```

- Install from source

If you want to install the source code, [please refer to](doc/INSTALL_SOURCE_en.md)

Note that DB will be created with one click through the source code installation, and there is no need to create dB in advance

# Functions of TarsGateway

## 1. Recognize agency type

TarsGateway can recognize the type of requests based on the host+url requests that are configured. The configuration and its corresponding logics are as follows:

[comment]: <***Configuration Instruction***：>

- Configuration domain: /main/base.
- tup_host: the host that corresponds to the tup request. If the requested host is in the tup_host list, then the subsequent tup&&json request will be judged. If the list configuration is empty, it will also be judged. The wildcard character is supported here as well.
- tup_path: the basic path for a tup or tars request. The default is /tup.
- json_path: the basic path for json request. The default is /json.
- monitor_url: TarsGateway's monitoring address, used to remotely judge whether the service is alive.
- Configuration example:

```
 <main>
        <base>
            #If tup_host is not configured, all those request starting with host, and without a path or path is /, are also judged as tup requests
            tup_host=prx.tup.whup.com|prx2.tup.whup.com|*.prx.upchina.com
            tup_path=/tup
            json_path=/json
            monitor_url=/monitor/monitor.jsp
        </base>
    </main>
```

## 2. TARS-tup && TARS-tars protocol agent

TARS-tup protocol proxy must be the post request type, with the path as /tup and the body content as the serialized content of BasePacket package tars. After TarsGateway receives the packet, it deserializes the content of the body and parses the BasePacket packet, and then looks for the obj of the actual tars service in the configuration according to the sServantName in it. If auto_proxy=1 is configured, sServantName can be filled with the real obj address when the client calls. A suggestion: since TarsGateway is directly exposed to the C external network, it is recommended to configure auto_proxy=0 to avoid direct exposure of internal network services to the outside world. In addition, the proxy configuration can also support the configuration of sServerName:sFuncName, which will be prioritized. This type of configuration has priority over the configuration of the sServerName type solely. The proxy configuration is as follows:

```
  <proxy>
        hello = TestApp.HelloServer.HelloObj
        hello:sayhello = TestApp.Hello2Server.HelloObj
    </proxy>
```

After TarsGateway calls the back-end service, the http header requested by the client can be transparently transmitted through the tars context. By default, REMOTE_IP (client ip) will be transparently transmitted to the back-end. The configuration is filterheaders, which can also be multiple ones. For example:

```
filterheaders = X-GUID|X-XUA
```

When calling the back-end tars service, TarsGateway defaults to tars's load balancing strategy (robin rotation). You can also configure a custom hash strategy. When hash_type is 1, tarshash is called according to the client request id. When hash_type is 2, Tarshash is called according to the specified http header (httpheader in breeding), such as the X-GUID in the http header. Be careful abou the choice of httpheader here because you will want to avoid the excessive concentration on a certain value leading to uneven load balancing. When hash_type is 3, the tarshash call is made according to the client's ip. If hash_type is not configured after obj, tars is used to call in rotation. The configuration example is as follows:

```
<proxy>
        # servant = server_full_obj [| hash_type [| http header key] ]
        # hash_type: 0, rotation; 1: requestid, 2: httphead, 3: client ip
        # Hello=TestApp.HelloServer.HelloObj | 1
        # Hello=TestApp.HelloServer.HelloObj | 2 | X-GUID
        # Hello=TestApp.HelloServer.HelloObj | 3
        hello = TestApp.HelloServer.HelloObj | 3
        hello:sayhello = TestApp.Hello2Server.HelloObj
    </proxy>
```

## 3. TARS-JSON protocol agent

TARS-JSON protocol proxy supports two types of interfaces.

- **servantName and funcName are specified in the http url path**

The path is /json/servantName/funcName, where /json is fixed and followed by servantName and funcName respectively.

```
resquest：
    url： http://xx.xx.com/json/Test.GetSumServer.GetSumObj/getSumEx
    {"req":{"userKey":"upchina","userToken":"upchinatoken","x":1,"y":9900989}}

    response：
    { "rsp": { "otherMsg": [ "1 + 9900989 = 9900990" ], "msg": "succ.", "sum": 9900990, "ret": 0 }, "": 0 }
```

- **The relevant parameters are specified in the http body: **

A post request type is required, with a /json path and the body content a json structure. There must be four fields for reqid, obj, func, and data, which respectively represent request id, service servant, service interface, and interface parameters. These also correspond to reqid:iRequestId, obj:sServantName, func:sFuncName in BasePacket. The data content is the parameter in the interface, the key is the parameter name, and the value is the parameter content. In addition to the four required fields above, context is an optional field. The content of the returned package includes reqid and data. Data is the output parameter content of the interface, and "" ’s key corresponds to the function return value. Other than the packet format, everything is the same or identical to the TARS-tup type. Examples of request parameters are as follows:

```
    Request package：
    {
        "reqid": 99999,
        "obj": "getsum",
        "func": "getSumEx",
        "data": "{\"req\":{\"userKey\":\"upchina\",\"userToken\":\"upchinatoken\",\"x\":1,\"y\":9900989}}"
    }
    Response package：
    {
        "data": "{ \"rsp\": { \"otherMsg\": [ \"1 + 9900989 = 9900990\" ], \"msg\": \"succ.\", \"sum\": 9900990, \"ret\": 0 }, \"\": 0 }",
        "reqid": 99999
    }
```

## 4. General HTTP protocol proxy

General HTTP protocol proxy, similar to the reverse proxy function of nginx, has the following main functions: request forwarding according to domain and url, back-end load balancing, fault tolerance, blacklist shielding, flow control and others.

- **Routing strategy**
  First, match server_name and the location, and then forward according to the proxy_pass path. The specific rules are as follows:

```
server_name matching logic：
{
  1. Find all that match
  2. Wildcard matches first
  3. Wildcards are matched later
  4. Regular matching
  5. If server_name is empty, all will match by default
}

location matching logic：
{
  1. = Full match: /login
  2. ^~ uri starts with a regular string: ^~ /static/ (Once the match is successful, it will not be matched further)
  3. ~ Regular matching (case sensitive): ~ \.(gif|jpg|png|js|css)$ (In the case of multiple regular expression matches, the longest match is used)
  4. ~* Regular matching (not case sensitive): ~* \.png$
  5. !~ and !~* are the regularities for case-insensitive and case-insensitive mismatch: !~ \.xhtml$, !~* \.xhtml$
  6. /xxx matches the path from the beginning (the longer the matching length, the higher the priority)
  7. / Any request will match
}

proxy_pass:
{
  1. If there is no path in the proxy_pass configuration (http://host/ has a path /), then the full path matched by location will be directly transmitted to url
  2. The proxy_pass configuration contains the path (even if there is only one /, it is counted), the new path = proxypassPath + (access path-location path)
  3. When the location is regular, proxy_pass cannot carry a path
}
```

- **Load balancing**

Supports normal rotation training and weighted rotation training strategies. The default weight is 1, and the larger the data, the higher the weight. The weight represents the number of training rotations in one rotation training cycle.

- **Fault tolerance**

When the back-end node is more than one (≥ 2 nodes), the back-end supports the circuit breaker strategy. Whether to enable the circuit breaker can be configured, and the default is enabled .

**Failure shielding**: If a connection error occurs, the node will be temporarily shielded, corresponding to the inactive configuration, and the field value is RequestCallback::FAILED_CODE type. When joining the failed node, if the site is configured with monitor_url, then the url will be regularly rotated. If http 200 appears, the node will be restored, and the rotating detection interval will increase up to 2 minutes. If monitor_url is not configured, then directly connect to the ip:port of the node, and it can be connected normally, then restore the node.
Timeout switching: When the number of timeouts reaches a certain threshold in the specified time window, or exceeds a certain percentage, it will be temporarily blocked for a period of time, and it will try timeout recovery after a certain period of time.

```
        configuration：
        <http_retcode>
            # Define which return code is used for timeout, fault tolerance processing, and fault tolerance processing
            inactive=2|6
            timeout=1|3
        </http_retcode>

        RequestCallback::FAILED_CODE The type is defined as follows：
        enum FAILED_CODE
        {
            Failed_Net     = 0x01,      //Network error
            Failed_Connect = 0x02,      //Error connecting to the server
            Failed_Timeout = 0x03,      //Timeout
            Failed_Interrupt = 0x04,    //Interupted when receiving data
            Failed_Close    = 0x05,     //The server closed the connection
            Failed_ConnectTimeout = 0x06, //Connection timeout
        };
```

IP blacklist and flow control strategy support TarsGateway's three protocols at the same time, so they will be introduced together later.

## 5. Flow Control

TarsGateway supports the access to the backend for flow control, as well as single-machine control and multi-machine coordinated control. Flow control can be turned off.

**Switch control:** You can configure flow_control_onoff to switch flow control on or off. In addition, if the service servant is not configured with FlowControlObj, then the flow control strategy will not be enabled.

**Flow control strategy:** The number of times it can be accessed in a certain period of time, which is dynamically controlled by a timed sliding window. The size of the sliding window is 1s. If the number of times exceeds the designated times, it will directly return to http 403.

**Multi-machine coordination:** If tup_report_obj is configured, then multi-machine cooperative flow control will be performed through this obj; otherwise, it will perform single-machine control. Note that if it is a single-machine strategy, the maximum number of times that the flow control configuration can be accessed within a certain period of time is the maximum number of times a single machine can visit the site. If it is a multi-machine collaboration, then it is how many times the multiple machines are allowed to access the site at the same time.

**Configuration instructions:** If it is TARS-tup or TARS-JSON protocol, then the site ID of flow control is service Obj. If it is http protocol, then the site ID is the stationId in the configuration.

## 6. Blocklist strategy

The blocklist is an IP blocklist, which supports two levels: global blocklist and site blocklist.

**Blocklist format:** client IP address, which supports wildcards. Such as 192.168.2.130, 192.168.10.\*

**Global blocklist:** Controls all access to TarsGateway, including TARS-tup, TARS-JSON and general HTTP protocols.

**Site blocklist:** Controls only for designated sites, and other sites will not be affected.

**Site allowlist:** Once a site is configured with a allowlist, it can only be accessed by the designated IP, which is mainly used for internal system control of designated IP access, or for designated partners to call.

## 7. Configure Hot Update

Support hot update of common configurations, including:

1. loadProxy: Through the tars command, the servant proxy configuration update of TARS-tup&TARS-JSON protocol can be realized;
2. loadHttp: Through this configuration, common HTTP protocol routing strategy, back-end node configuration, monitoring url configuration, and others can be carried out;
3. loadComm: Some common configuration loading can be carried out through this command, mainly including black and white list loading;
4. The flow control strategy automatically loads the DB dynamically.

## 8. Environment Switch

When acting as a TARS-tup or TARS-JSON protocol proxy, you can specify it to the unconnected proxy sub-configuration domain through the value in the http header. The default is to use the configuration under proxy directly. If env_httpheader is configured, and the http header is in the current request, and the value of the http header is the content in the configuration, then the forwarding rule corresponding to the env subdomain under proxy is preferred. For example, the following configuration means that the user whose X-GUID=12345678123456781234567812345678 in the http request header is configured in the test environment is preferred, that is: when the user requests the servant as hello, then the real service obj selects TestApp.HelloServer.HelloObj@tcp -h 192.168. 2.101 -p 10029, but if the user requests the servant to be world, because the forwarding rules for world are not configured in the test environment, then the default rules under proxy are still used, and the real obj is Test.HelloworldServer.HelloworldObj, the configuration is as follows:

```
  <proxy>
        hello = TestApp.HelloServer.HelloObj
        hello:sayhello = TestApp.Hello2Server.HelloObj
        world= Test.HelloworldServer.HelloworldObj

        # test environment forwarding rules
        <test>
             hello = TestApp.HelloServer.HelloObj@tcp -h 192.168.2.101 -p 10029
        </test>
    </proxy>

    #http head:value, switch to a certain server of the proxy
    <env_httpheader>
       # httpheader-key:httpheader-value = env
        X-GUID:12345678123456781234567812345678 = test
    </env_httpheader>
```

## 9. Return code description

200: OK normal response
400: Bad Request 1. Solve the client request packet error.
403: Forbidden 1. The client IP hits the blacklist.
404: Not Found 1. Tup or json protocol can not find the corresponding servant agent; 2. Http can not find the back-end site;
429: Too Many Request 1. Flow control exceeds limit;
500: Server Interval Error 1. The http backend does not have a destination configured;
502: Bad Gateway 1. The back-end tars service is called or the http service is abnormal;
504: Gateway Timeout 1. Call the back-end tars service or http service is timed out;

##10. Log format description
TARS-tup & TARS-JSON protocol proxy request response log format description:

**Normal response log:**

Log time | Client ip | Client GUID | Client XUA | servantName | funcName | Request encryption type | Request compression type | Is the response encrypted | Is the response compressed | Time-consuming (ms) | Response packet size

**Abnormal request tupcall_exception log:**
Log time | Client ip | servantName | funcName | Client GUID | Client XUA | Request encryption type | Request compression type | Time-consuming (ms) | Back-end rpc return code

General HTTP protocol proxy log format description:

**Http access log:**

Client ip | access time | host | referer | request url | request packet size | http method | site ID | backend address | http return status code | response time | time (ms) | response packet size | UA | error message
