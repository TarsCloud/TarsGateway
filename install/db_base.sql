CREATE TABLE IF NOT EXISTS `t_station` (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_station_id` varchar(64) NOT NULL COMMENT '站点英文名，唯一',
  `f_name_cn` varchar(64) NOT NULL DEFAULT '' COMMENT '站点中文名称',	
  `f_monitor_url` varchar(255) NOT NULL DEFAULT '' COMMENT '监控url',	
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `f_station_id` (`f_station_id`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 COMMENT='站点';

CREATE TABLE IF NOT EXISTS `t_flow_control` (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_station_id` varchar(64) NOT NULL COMMENT '站点英文名, 对于wup接口调用的taf，就是服务的obj',
  `f_duration` int(10) NOT NULL DEFAULT 60 COMMENT '时间窗口，单位秒， 默认为60秒',	
  `f_max_flow` int(10) NOT NULL COMMENT '最大流量，即在f_duration时间内最多请求f_max_flow次',	
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `station_id` (`f_station_id`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 COMMENT='流量控制';

CREATE TABLE IF NOT EXISTS `t_blacklist` (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_station_id` varchar(64) NOT NULL DEFAULT '' COMMENT '站点英文名，为空时表示所有站点',
  `f_ip` varchar(20) NOT NULL COMMENT 'ip, 可以为表示所有',	
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `station_ip` (`f_station_id`, `f_ip`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 COMMENT='ip 黑名单';

CREATE TABLE IF NOT EXISTS `t_whitelist` (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_station_id` varchar(64) NOT NULL COMMENT '站点英文名,taf服务则为Obj',
  `f_ip` varchar(20) NOT NULL COMMENT 'ip, 可以为表示所有',	
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `station_ip` (`f_station_id`, `f_ip`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 COMMENT='ip 黑名单';

CREATE TABLE IF NOT EXISTS `t_http_router` (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_station_id` varchar(64) NOT NULL COMMENT '站点英文名',
  `f_server_name` varchar(64) NOT NULL DEFAULT '' COMMENT 'server_name',
  `f_path_rule` varchar(255) NOT NULL DEFAULT '' COMMENT 'url规则',	
  `f_proxy_pass` varchar(255) NOT NULL DEFAULT '' COMMENT 'proxy_pass',	
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `station_rule` (`f_server_name`, `f_path_rule`) 
) ENGINE=InnoDB AUTO_INCREMENT=1000 DEFAULT CHARSET=utf8 COMMENT='路由规则';

CREATE TABLE IF NOT EXISTS t_upstream (
  `f_id` int(11) NOT NULL AUTO_INCREMENT,
  `f_upstream` varchar(64) NOT NULL COMMENT '站点英文名',
  `f_addr` varchar(255) NOT NULL DEFAULT '' COMMENT 'ip:port', 
  `f_weight` int(10) NOT NULL DEFAULT 1 COMMENT '权重， 默认为1',	
  `f_fusing_onoff` int(2) NOT NULL DEFAULT 1 COMMENT '是否熔断处理，1是，0否',
  `f_valid` int(2) NOT NULL DEFAULT 1 COMMENT '1:valid, 0:invalid',
  `f_create_person` varchar(64) NOT NULL DEFAULT '' COMMENT '创建者',
  `f_create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `f_update_person` varchar(64) NOT NULL DEFAULT '' COMMENT '更新人',
  `f_update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`f_id`),
  UNIQUE KEY `station_addr` (`f_upstream`, `f_addr`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 COMMENT='后端服务地址';
