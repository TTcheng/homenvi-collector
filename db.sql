-- influxdb sql scripts
-- @date 04.12.2019
-- @author wangchuncheng
CREATE USER "homenvi-collector" WITH PASSWORD 'xxxx';
GRANT WRITE ON homenvi TO "homenvi-collector";

-- shell: influx -username homenvi-collector -password xxxx
USE homenvi;
INSERT collections,identifier=HomenviCollectorAlpha humidity=66.40,celsius=22.60,fahrenheit=72.68,heatIndexCelsius=22.65,heatIndexFahrenheit=72.77
-- 数据保存策略（Retention Policies）
CREATE RETENTION POLICY "homenvi_rp" ON "homenvi" DURATION 30d REPLICATION 1 DEFAULT
-- 连续查询（continuous queries）
CREATE CONTINUOUS QUERY collections_cq_1h ON homenvi BEGIN SELECT identifier,mean(humidity),mean(celsius),mean(fahrenheit),mean(heatIndexCelsius),mean(heatIndexFahrenheit) INTO collections_1h FROM collections GROUP BY identifier,time(1h) END