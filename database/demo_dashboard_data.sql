-- V0.7 dashboard demo dataset. Safe to re-run: only IDs in the reserved demo ranges are replaced.
PRAGMA foreign_keys=ON;
BEGIN IMMEDIATE;

DELETE FROM telemetry WHERE id BETWEEN 70001 AND 70999;
DELETE FROM wallet_transactions WHERE id BETWEEN 60001 AND 60999;
DELETE FROM alarms WHERE id BETWEEN 80001 AND 80999;
DELETE FROM reservations WHERE id BETWEEN 51001 AND 51999;
DELETE FROM charge_orders WHERE id BETWEEN 50001 AND 50999;
DELETE FROM chargers WHERE id BETWEEN 1001 AND 1099;
DELETE FROM stations WHERE id BETWEEN 101 AND 199;
DELETE FROM users WHERE id BETWEEN 9001 AND 9099;
DELETE FROM operation_logs WHERE id BETWEEN 90001 AND 90999;

INSERT INTO stations(id,name,address,longitude,latitude,base_price,status) VALUES
(101,'星海广场超级充电站','大连市沙河口区星海广场B3停车场',121.588700,38.881200,1.48,'ONLINE'),
(102,'东港商务区充电中心','大连市中山区港浦路20号',121.676200,38.921500,1.56,'ONLINE'),
(103,'高新万达智慧充电站','大连市高新区黄浦路500号',121.528400,38.858900,1.42,'ONLINE'),
(104,'软件园夜间充电站','大连市甘井子区软件园路18号',121.534800,38.884300,1.18,'ONLINE'),
(105,'机场快速补能站','大连市甘井子区迎客路100号',121.541600,38.965800,1.68,'ONLINE'),
(106,'北站综合能源站','大连市甘井子区华北路北站南广场',121.613300,39.013100,1.38,'ONLINE'),
(107,'金石滩游客中心站','大连市金州区金石路65号',122.007600,39.090200,1.32,'ONLINE'),
(108,'旅顺大学城充电站','大连市旅顺口区学城路8号',121.261900,38.812600,1.16,'MAINTENANCE');

INSERT INTO chargers(id,station_id,code,type,rated_power,status,last_seen,total_sessions,total_duration) VALUES
(1001,101,'DL-XH-001','FAST',180,'IDLE',datetime('now','localtime','-1 minute'),0,0),
(1002,101,'DL-XH-002','FAST',180,'CHARGING',datetime('now','localtime','-10 seconds'),0,0),
(1003,101,'DL-XH-003','FAST',120,'FAULT',datetime('now','localtime','-36 minutes'),0,0),
(1004,101,'DL-XH-004','SLOW',7,'OFFLINE',datetime('now','localtime','-2 days'),0,0),
(1005,102,'DL-DG-001','FAST',240,'IDLE',datetime('now','localtime','-20 seconds'),0,0),
(1006,102,'DL-DG-002','FAST',180,'IDLE',datetime('now','localtime','-30 seconds'),0,0),
(1007,102,'DL-DG-003','FAST',180,'CHARGING',datetime('now','localtime','-12 seconds'),0,0),
(1008,102,'DL-DG-004','SLOW',11,'IDLE',datetime('now','localtime','-1 minute'),0,0),
(1009,103,'DL-GX-101','FAST',160,'FAULT',datetime('now','localtime','-18 minutes'),0,0),
(1010,103,'DL-GX-102','FAST',160,'IDLE',datetime('now','localtime','-40 seconds'),0,0),
(1011,103,'DL-GX-103','SLOW',7,'IDLE',datetime('now','localtime','-2 minutes'),0,0),
(1012,103,'DL-GX-104','SLOW',7,'OFFLINE',datetime('now','localtime','-5 hours'),0,0),
(1013,104,'DL-RJ-201','FAST',120,'IDLE',datetime('now','localtime','-12 seconds'),0,0),
(1014,104,'DL-RJ-202','FAST',120,'CHARGING',datetime('now','localtime','-8 seconds'),0,0),
(1015,104,'DL-RJ-203','SLOW',7,'IDLE',datetime('now','localtime','-50 seconds'),0,0),
(1016,104,'DL-RJ-204','SLOW',7,'FAULT',datetime('now','localtime','-48 minutes'),0,0),
(1017,105,'DL-JC-301','FAST',300,'IDLE',datetime('now','localtime','-16 seconds'),0,0),
(1018,105,'DL-JC-302','FAST',240,'IDLE',datetime('now','localtime','-25 seconds'),0,0),
(1019,105,'DL-JC-303','FAST',180,'IDLE',datetime('now','localtime','-20 seconds'),0,0),
(1020,105,'DL-JC-304','FAST',180,'FAULT',datetime('now','localtime','-1 hour'),0,0),
(1021,106,'DL-BZ-401','FAST',180,'IDLE',datetime('now','localtime','-22 seconds'),0,0),
(1022,106,'DL-BZ-402','FAST',120,'OFFLINE',datetime('now','localtime','-8 hours'),0,0),
(1023,106,'DL-BZ-403','SLOW',11,'IDLE',datetime('now','localtime','-45 seconds'),0,0),
(1024,106,'DL-BZ-404','SLOW',7,'IDLE',datetime('now','localtime','-55 seconds'),0,0),
(1025,107,'DL-JST-501','FAST',160,'IDLE',datetime('now','localtime','-1 minute'),0,0),
(1026,107,'DL-JST-502','FAST',160,'IDLE',datetime('now','localtime','-1 minute'),0,0),
(1027,107,'DL-JST-503','SLOW',7,'FAULT',datetime('now','localtime','-3 hours'),0,0),
(1028,107,'DL-JST-504','SLOW',7,'IDLE',datetime('now','localtime','-2 minutes'),0,0),
(1029,108,'DL-LS-601','FAST',120,'IDLE',datetime('now','localtime','-5 minutes'),0,0),
(1030,108,'DL-LS-602','FAST',120,'IDLE',datetime('now','localtime','-6 minutes'),0,0),
(1031,108,'DL-LS-603','SLOW',7,'IDLE',datetime('now','localtime','-7 minutes'),0,0),
(1032,108,'DL-LS-604','SLOW',7,'OFFLINE',datetime('now','localtime','-1 day'),0,0);

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<20)
INSERT INTO users(id,phone,nickname,balance,status,failed_attempts,created_at)
SELECT 9000+i,printf('1398000%04d',i),printf('演示车主%02d',i),
       round(80+(i*37)%520,2),CASE WHEN i=19 THEN 'FROZEN' WHEN i=20 THEN 'LOCKED' ELSE 'NORMAL' END,
       CASE WHEN i=20 THEN 5 ELSE 0 END,datetime('now','localtime',printf('-%d days',20-i)) FROM n;

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<180),
facts AS (
 SELECT i,9001+((i-1)%20) user_id,1001+((i*7)%32) charger_id,
        datetime('now','localtime',printf('-%d days',(i-1)%30),printf('-%d hours',(i*5)%22),printf('-%d minutes',(i*11)%50)) started,
        6.0+(i%38)*0.85 energy,900+(i%12)*240 duration
 FROM n
)
INSERT INTO charge_orders(id,user_id,charger_id,status,mode,target,start_at,end_at,energy,duration,amount)
SELECT 50000+i,user_id,charger_id,'COMPLETED',CASE i%3 WHEN 0 THEN 'AMOUNT' WHEN 1 THEN 'ENERGY' ELSE 'TIME' END,
       CASE i%3 WHEN 0 THEN 50 WHEN 1 THEN 30 ELSE 60 END,started,datetime(started,printf('+%d seconds',duration)),
       round(energy,2),duration,round(energy*(1.08+(i%6)*0.09),2) FROM facts;

INSERT INTO charge_orders(id,user_id,charger_id,status,mode,target,start_at,end_at,energy,duration,amount) VALUES
(50201,9001,1002,'CHARGING','ENERGY',45,datetime('now','localtime','-24 minutes'),NULL,18.60,1440,0),
(50202,9006,1007,'CHARGING','AMOUNT',80,datetime('now','localtime','-17 minutes'),NULL,12.80,1020,0),
(50203,9011,1014,'CHARGING','TIME',60,datetime('now','localtime','-39 minutes'),NULL,25.40,2340,0);

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<10)
INSERT INTO charge_orders(id,user_id,charger_id,status,mode,target,start_at,end_at,energy,duration,amount)
SELECT 50210+i,9001+(i%20),1001+((i*3)%32),'CANCELLED','ENERGY',30,
       datetime('now','localtime',printf('-%d days',i%7)),datetime('now','localtime',printf('-%d days',i%7)),0,0,0 FROM n;

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<180)
INSERT INTO wallet_transactions(id,user_id,type,amount,balance_after,related_order_id,created_at)
SELECT 60000+i,9001+((i-1)%20),'CHARGE',-round((6.0+(i%38)*0.85)*(1.08+(i%6)*0.09),2),
       round(300-(i%20)*4.5,2),50000+i,datetime('now','localtime',printf('-%d days',(i-1)%30)) FROM n;

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<90)
INSERT INTO telemetry(id,charger_id,order_id,sampled_at,voltage,current,power,soc,temperature,energy_total)
SELECT 70000+i,CASE i%3 WHEN 0 THEN 1002 WHEN 1 THEN 1007 ELSE 1014 END,
       CASE i%3 WHEN 0 THEN 50201 WHEN 1 THEN 50202 ELSE 50203 END,
       datetime('now','localtime',printf('-%d seconds',(90-i)*20)),380+(i%8),110+(i%35),
       42+(i%20)*2.7,20+(i%70),31+(i%9)*0.7,round(i*0.28,2) FROM n;

INSERT INTO alarms(id,charger_id,level,type,message,status,created_at,resolved_at) VALUES
(80001,1003,'CRITICAL','INSULATION','绝缘检测异常，设备已停止输出','OPEN',datetime('now','localtime','-36 minutes'),NULL),
(80002,1009,'WARNING','TEMPERATURE_HIGH','枪线温度持续偏高','OPEN',datetime('now','localtime','-18 minutes'),NULL),
(80003,1016,'CRITICAL','COMMUNICATION_LOST','设备心跳中断','OPEN',datetime('now','localtime','-48 minutes'),NULL),
(80004,1020,'WARNING','VOLTAGE','输入电压波动超限','OPEN',datetime('now','localtime','-1 hour'),NULL),
(80005,1027,'CRITICAL','EMERGENCY_STOP','急停按钮被触发','OPEN',datetime('now','localtime','-3 hours'),NULL),
(80006,1005,'INFO','DOOR_OPEN','维护门开启记录','RESOLVED',datetime('now','localtime','-2 days'),datetime('now','localtime','-2 days','+8 minutes'));

WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<24)
INSERT INTO operation_logs(id,actor_type,actor_id,action,target_type,target_id,result,created_at)
SELECT 90000+i,CASE WHEN i%4=0 THEN 'SYSTEM' ELSE 'ADMIN' END,CASE WHEN i%4=0 THEN NULL ELSE 1 END,
       CASE i%4 WHEN 0 THEN 'DEVICE_HEALTH_CHECK' WHEN 1 THEN 'UPDATE_STATION' WHEN 2 THEN 'RESTART_CHARGER' ELSE 'EXPORT_REPORT' END,
       CASE WHEN i%4=1 THEN 'STATION' ELSE 'CHARGER' END,1001+(i%32),'SUCCESS',datetime('now','localtime',printf('-%d hours',i*3)) FROM n;

UPDATE chargers SET
 total_sessions=(SELECT COUNT(*) FROM charge_orders o WHERE o.charger_id=chargers.id AND o.status='COMPLETED'),
 total_duration=COALESCE((SELECT SUM(duration) FROM charge_orders o WHERE o.charger_id=chargers.id AND o.status='COMPLETED'),0)
WHERE id BETWEEN 1001 AND 1099;

COMMIT;

SELECT '演示电站',COUNT(*) FROM stations WHERE id BETWEEN 101 AND 199;
SELECT '演示电桩',COUNT(*) FROM chargers WHERE id BETWEEN 1001 AND 1099;
SELECT '演示用户',COUNT(*) FROM users WHERE id BETWEEN 9001 AND 9099;
SELECT '已完成订单',COUNT(*) FROM charge_orders WHERE id BETWEEN 50001 AND 50180;
SELECT '充电中订单',COUNT(*) FROM charge_orders WHERE id BETWEEN 50201 AND 50203;
