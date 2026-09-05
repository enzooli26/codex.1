INSERT OR IGNORE INTO admins(username,password_hash,role,status) VALUES('admin','8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92','ADMIN','NORMAL');
INSERT OR IGNORE INTO stations(id,name,address,longitude,latitude,base_price,status) VALUES(1,'软件园充电站','大连市甘井子区软件园路',121.5300,38.8800,1.20,'ONLINE');
INSERT OR IGNORE INTO stations(id,name,address,longitude,latitude,base_price,status) VALUES(2,'高新园区充电站','大连市高新园区黄浦路',121.5200,38.8600,1.35,'ONLINE');
INSERT OR IGNORE INTO chargers(id,station_id,code,type,rated_power,status) VALUES(1,1,'DL-SW-001','FAST',120,'IDLE');
INSERT OR IGNORE INTO chargers(id,station_id,code,type,rated_power,status) VALUES(2,1,'DL-SW-002','SLOW',7,'IDLE');
INSERT OR IGNORE INTO chargers(id,station_id,code,type,rated_power,status) VALUES(3,2,'DL-GX-001','FAST',160,'IDLE');
INSERT OR IGNORE INTO tariffs(id,name,start_time,end_time,energy_price,service_price,occupancy_price,enabled) VALUES(1,'默认电价','00:00','24:00',1.00,0.20,0.10,1);
